#include "Game/NameObj/NameObjGroup.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/LayoutPaneCtrl.hpp"
#include "Game/Screen/SceneWipeHolder.hpp"
#include "Game/Screen/WipeFade.hpp"
#include "Game/Screen/WipeGameOver.hpp"
#include "Game/Screen/WipeKoopa.hpp"
#include "Game/Screen/WipeRing.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
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
void owner(smgpc::runtime::RuntimeContext& runtime) {
    SceneObjHolder scene;
    const auto baseline = smgpc::compat::name_obj_runtime_state_count();
    std::weak_ptr<smgpc::compat::JkrAllocationDomain> domain;
    {
        smgpc::scene::SceneObjHolderBinding binding(scene);
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
        for (int cycle = 0; cycle < 2; ++cycle) owner(runtime);
        renderer.end_frame();
        std::cout << "Original scene wipe resources, animation predicates, transitions and repeated ownership passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
