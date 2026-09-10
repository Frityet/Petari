#include "SceneExecutionFixture.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/NameObj/NameObjGroup.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/LayoutPaneCtrl.hpp"
#include "Game/Screen/SceneWipeHolder.hpp"
#include "Game/Screen/CinemaFrame.hpp"
#include "Game/Screen/WipeFade.hpp"
#include "Game/Screen/WipeGameOver.hpp"
#include "Game/Screen/WipeKoopa.hpp"
#include "Game/Screen/WipeRing.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "layout/LayoutHost.hpp"
#include "layout/LayoutRuntime.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include <aurora/dvd.h>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
class Logger final : public smgpc::logging::ILogger {
    void write(std::FILE*, std::source_location, smgpc::logging::Level,
               smgpc::logging::Category, std::string_view) override {}
};
void tick(smgpc::runtime::RuntimeContext& runtime) {
    runtime.scheduler().execute_movement();
    runtime.scheduler().execute_calc_anim();
}
#ifndef NDEBUG
class LayoutLifetimeNerve final : public Nerve {
public:
    void execute(Spine* spine) const override {
        require(spine->mExecutor == actor, "the original Spine executes its real LayoutActor");
        ++calls;
    }
    LayoutActor* actor = nullptr;
    mutable unsigned calls = 0;
};

void layout_actor_lifetime(smgpc::runtime::RuntimeContext& runtime) {
    auto& root_heap = runtime.host_heaps()->root_heap();
    const auto free_before = root_heap.getTotalFreeSize();
    const auto identities_before = smgpc::compat::name_obj_runtime_state_count();
    const auto layouts_before = smgpc::layout::debug_layout_lifetime_state();
    const auto scheduled_before = runtime.scheduler().snapshot().size();
    for (int cycle = 0; cycle < 2; ++cycle) {
        for (const bool scheduled : {false, true}) {
            {
                smgpc::test::SceneExecutionFixture execution(
                    runtime.scheduler(), smgpc::compat::JkrAllocationDomain::create(runtime.host_heaps(), 8U << 20));
                const auto domain = smgpc::scene::current_scene_allocation_domain();
                const auto identities = smgpc::compat::name_obj_runtime_state_count();
                const auto layouts = smgpc::layout::debug_layout_lifetime_state();
                const auto scheduled_count = runtime.scheduler().snapshot().size();
                std::unique_ptr<LayoutActor> actor;
                {
                    const smgpc::compat::JkrAllocationScope game(domain);
                    actor = std::make_unique<LayoutActor>("Layout lifetime regression", false);
                    actor->initLayoutManager("SysInfoWindowMini", 1);
                    actor->getLayoutManager()->createAndAddPaneCtrl("SaveIconPosition", 1);
                }
                auto* resource = smgpc::layout::layout_runtime(actor.get());
                require(resource && resource->getArchivePath() &&
                        actor->getLayoutManager()->getPane("SaveIconPosition"),
                        "the lifetime regression retains the real disc layout and authored pane");
                const auto populated = smgpc::layout::debug_layout_lifetime_state();
                require(populated.actors == layouts.actors + 1 && populated.managers == layouts.managers + 1 &&
                        populated.pane_controls >= layouts.pane_controls + 2 &&
                        smgpc::compat::name_obj_runtime_state_count() == identities + 1,
                        "one actual LayoutActor creates one manager and real root and named pane controls");

                LayoutLifetimeNerve nerve;
                nerve.actor = actor.get();
                const auto free_before_spine = root_heap.getTotalFreeSize();
                {
                    const smgpc::compat::JkrAllocationScope game(domain);
                    // Use the original reclaiming heap to observe Spine deletion before scene-arena disposal.
                    const MR::CurrentHeapRestorer root(&root_heap);
                    actor->initNerve(&nerve);
                }
                require(JKRHeap::findFromRoot(actor->mSpine) == &root_heap &&
                        root_heap.getTotalFreeSize() < free_before_spine,
                        "the original Spine has a real individually reclaimable Game allocation");
                if (scheduled)
                    runtime.register_layout_actor(*actor, MR::MovementType_Layout, MR::CalcAnimType_Layout, MR::DrawType_Layout);
                execution.complete_initialization();
                actor->appear();
                require(runtime.scheduler().snapshot().size() == scheduled_count + (scheduled ? 1 : 0),
                        "scheduled and unscheduled layout lifetimes have distinct executor membership");
                if (scheduled) runtime.scheduler().execute_movement_category(MR::MovementType_Layout);
                else actor->movement();
                require(nerve.calls == 1 && actor->getNerveStep() == 1,
                        "the original Spine advances through the selected real execution path");
                actor.reset();
                require(root_heap.getTotalFreeSize() == free_before_spine,
                        "LayoutActor destruction individually frees its original Spine");
                require(smgpc::layout::debug_layout_lifetime_state() == layouts &&
                        smgpc::compat::name_obj_runtime_state_count() == identities &&
                        runtime.scheduler().snapshot().size() == scheduled_count,
                        "destruction retires the layout, manager, panes, identity and scheduling without recursive unregister");
                runtime.scheduler().execute_movement_category(MR::MovementType_Layout);
                require(nerve.calls == 1, "retired layout nerves cannot receive another scheduled movement");
            }
            require(root_heap.getTotalFreeSize() == free_before &&
                    smgpc::compat::name_obj_runtime_state_count() == identities_before &&
                    smgpc::layout::debug_layout_lifetime_state() == layouts_before &&
                    runtime.scheduler().snapshot().size() == scheduled_before,
                    "repeated layout teardown releases the whole scene domain and retains no native identities");
        }
    }
}
#endif

void owner(smgpc::runtime::RuntimeContext& runtime) {
    const auto baseline = smgpc::compat::name_obj_runtime_state_count();
    std::weak_ptr<smgpc::compat::JkrAllocationDomain> domain;
    {
        smgpc::test::SceneExecutionFixture execution(
            runtime.scheduler(), smgpc::compat::JkrAllocationDomain::create(runtime.host_heaps(), 8U << 20));
        auto& binding = execution.objects();
        domain = smgpc::scene::current_scene_allocation_domain();
        auto* group = dynamic_cast<NameObjGroup*>(MR::createSceneObj(SceneObj_NameObjGroup));
        auto* wipes = dynamic_cast<SceneWipeHolder*>(MR::createSceneObj(SceneObj_SceneWipeHolder));
        require(group && wipes && SceneWipeHolderFunction::getSceneWipeHolder() == wipes &&
                MR::createSceneObj(SceneObj_SceneWipeHolder) == wipes,
                "the real scene creates and retains exactly one original wipe holder");
        const std::array names{"円ワイプ", "フェードワイプ", "白フェードワイプ", "ゲームオーバー", "クッパ"};
        require(group->mObjectCount == 5 && group->mObjectNumMax == 16, "all five original wipe descendants join the actual pause group");
        for (std::size_t i = 0; i < names.size(); ++i)
            require(group->getObj(i) == wipes->findWipe(names[i]), "wipe catalog and pause membership preserve original identity and order");
        auto* ring = dynamic_cast<WipeRing*>(wipes->findWipe(names[0]));
        auto* black = dynamic_cast<WipeFade*>(wipes->findWipe(names[1]));
        auto* white = dynamic_cast<WipeFade*>(wipes->findWipe(names[2]));
        auto* game_over = dynamic_cast<WipeGameOver*>(wipes->findWipe(names[3]));
        auto* koopa = dynamic_cast<WipeKoopa*>(wipes->findWipe(names[4]));
        require(ring && black && white && black != white && game_over && koopa && !wipes->findWipe("AbsentWipe"),
                "all original wipe kinds are actual distinct typed owners and missing names remain absent");
        require(std::abs(ring->calcMaxRadius() - std::sqrt(900160.0F)) < 0.001F,
                "original ring maximum radius uses square root rather than reciprocal square root");
        for (auto* layout : std::array<LayoutActor*, 3>{ring, game_over, koopa}) {
            auto* resource = smgpc::layout::layout_runtime(layout);
            require(resource && resource->getArchivePath() && layout->getLayoutManager()->getPane(nullptr),
                    "each layout wipe owns its actual retained disc BRLYT and real root Pane");
            const LayoutActor* constant_layout = layout;
            require(MR::isAnimStopped(constant_layout, 0) && layout->getLayoutManager()->getPaneCtrl(nullptr)->isAnimStopped(0),
                    "initialized original root players with no animation transform are stopped through const public and pane APIs");
        }
        require(ring->getLayoutManager()->getPaneCtrl("Ring")->isAnimStopped(0),
                "a separately initialized named pane player has the same real unstarted state");
        bool invalid_layer = false, missing_owner = false;
        try { (void)MR::isAnimStopped(game_over, 1); } catch (const std::out_of_range&) { invalid_layer = true; }
        try { (void)MR::isAnimStopped(black, 0); } catch (const std::invalid_argument&) { missing_owner = true; }
        require(invalid_layer && missing_owner, "default stopped state never substitutes for an absent owner or unavailable layer");

        auto* cinema = dynamic_cast<CinemaFrame*>(MR::createSceneObj(SceneObj_CinemaFrame));
        execution.complete_initialization();

        MR::closeWipeFade(3);
        require(wipes->getCurrent() == black && MR::isWipeActive() && !MR::isWipeBlank(), "black fade begins its actual original close transition");
        tick(runtime); tick(runtime);
        require(MR::isWipeActive(), "fade remains active before its authored frame limit");
        tick(runtime);
        require(MR::isWipeBlank() && !MR::isWipeActive(), "actual original fade control closes at the requested third frame");
        MR::openWipeFade(3);
        tick(runtime); tick(runtime); tick(runtime);
        require(MR::isWipeOpen() && MR::isDead(black), "opening reaches the actual original kill boundary");
        MR::forceCloseWipeWhiteFade();
        require(wipes->getCurrent() == white && MR::isWipeBlank(), "white fade selects its own real owner");
        MR::forceOpenWipeWhiteFade();
        require(MR::isWipeOpen() && MR::isDead(white), "white force-open retires its actual active state");

        MR::startDownWipe();
        require(wipes->getCurrent() == koopa && koopa->isWipeOut(), "original Down utility selects actual Koopa wipe and nerve");
        tick(runtime);
        require(!MR::isAnimStopped(koopa, 0) && MR::getAnimFrame(koopa, 0) > 0,
                "the actual Koopa nerve starts and advances its real out BRLAN");
        for (int frame = 0; frame < 92 && !koopa->isClose(); ++frame) tick(runtime);
        require(koopa->isClose() && MR::isAnimStopped(koopa, 0), "original Koopa nerve completes from the real animation state");
        MR::startGameOverWipe();
        require(wipes->getCurrent() == game_over, "original GameOver utility selects its actual owner");
        require(game_over->isClose(), "before the first active step the original unstarted player predicate is stopped");
        tick(runtime);
        require(!game_over->isClose() && game_over->isWipeOut() && MR::getAnimFrame(game_over, 0) > 0,
                "the first original GameOver step starts its actual BRLAN and changes the stop predicate");
        const auto end = MR::getAnimCtrl(game_over, 0)->getEnd();
        for (int frame = 0; frame <= end + 1 && !game_over->isClose(); ++frame) tick(runtime);
        require(game_over->isClose() && !game_over->isWipeOut(), "actual GameOver animation completion drives the original public state");
        MR::forceOpenWipeCircle();
        require(wipes->getCurrent() == ring && MR::isWipeOpen() && MR::isDead(ring), "circle force-open restores the exact initial owner");

        require(cinema && MR::createSceneObj(SceneObj_CinemaFrame) == cinema && MR::isDead(cinema) && MR::isStopCinemaFrame(),
                "the actual CinemaFrame factory retains one original initialized Screen owner");
        require(smgpc::layout::layout_runtime(cinema)->getArchivePath() && cinema->getLayoutManager()->getPane(nullptr),
                "CinemaFrame owns its genuine disc BRLYT and root pane");
        auto complete_transition = [&] {
            require(!MR::isStopCinemaFrame(), "each original CinemaFrame transition enters an active nerve");
            tick(runtime);
            const auto duration = MR::getAnimCtrl(cinema, 0)->getEnd();
            require(duration > 0, "CinemaFrame starts its actual authored animation");
            for (int frame = 0; frame <= duration + 2 && !MR::isStopCinemaFrame(); ++frame) tick(runtime);
            require(MR::isStopCinemaFrame(), "real BRLAN completion drives the original stable CinemaFrame nerve");
            tick(runtime);
        };
        MR::tryScreenToFrameCinemaFrame(); complete_transition();
        require(!MR::isDead(cinema), "Screen-to-Frame leaves the actual cinema layout visible");
        MR::tryFrameToBlankCinemaFrame(); complete_transition();
        MR::tryBlankToFrameCinemaFrame(); complete_transition();
        MR::tryFrameToScreenCinemaFrame(); complete_transition();
        require(MR::isDead(cinema), "Frame-to-Screen reaches the original kill boundary");
        MR::forceToFrameCinemaFrame(); tick(runtime);
        require(MR::isStopCinemaFrame() && !MR::isDead(cinema), "forced Frame executes its original stable owner");
        MR::forceToBlankCinemaFrame(); tick(runtime);
        require(MR::isStopCinemaFrame() && MR::getAnimFrame(cinema, 0) == 0 && MR::isAnimStopped(cinema, 0),
                "forced Blank holds the original Open animation at frame zero");
        MR::forceToScreenCinemaFrame(); tick(runtime);
        require(MR::isStopCinemaFrame() && MR::isDead(cinema), "forced Screen executes original final-frame kill");
    }
    require(domain.expired() && !MR::getSceneObjHolder() && smgpc::compat::name_obj_runtime_state_count() == baseline,
            "scene teardown retires all original wipe descendants, memberships, resource owners and Game allocation domain");
}
}
int main() {
    try {
        const auto* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc, "SMGPC_REAL_DISC must name the real disc");
        smgpc::render::AuroraWindow window({.width = 640, .height = 456, .title = "Original scene wipe ownership"});
        smgpc::render::AuroraRenderer renderer(window);
        require(aurora_dvd_open(disc), "Cannot open the supplied disc");
        struct Disc { ~Disc() { aurora_dvd_close(); } } close_disc;
        DVDInit();
        smgpc::resource::GameResourceRuntime resources({96U << 20, 32U << 20, 4U << 20});
        Logger logger;
        smgpc::runtime::RuntimeContext runtime(logger, window, resources);
        smgpc::runtime::SceneSchedulerBinding scheduler_binding(runtime.scheduler());
        (void)renderer.begin_frame();
#ifndef NDEBUG
        layout_actor_lifetime(runtime);
#endif
        for (int cycle = 0; cycle < 2; ++cycle) owner(runtime);
        renderer.end_frame();
        std::cout << "Original scene wipe resources, animation predicates, transitions and repeated ownership passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
