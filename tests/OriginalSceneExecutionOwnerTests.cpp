#include "NativeHeapFixture.hpp"
#include "SceneExecutionFixture.hpp"
#include "OriginalSceneControllerFixture.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObjCategoryList.hpp"
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/NameObj/NameObjFinder.hpp"
#include "Game/Scene/SceneNameObjMovementController.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "layout/LayoutRuntime.hpp"
#include <aurora/exception.hpp>
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) aurora::throw_host_exception<std::runtime_error>(message);
}
struct Object final : NameObj {
    Object(int id, std::vector<int>& log) : NameObj("original execution queue fixture"), id(id), log(log) {}
    void movement() override { record(id); if (hook) hook(); }
    void calcAnim() override { record(id + 10); }
    void draw() const override { record(id + 20); }
    void record(int value) const {
        auto* allocation = new unsigned[8];
        require(JKRHeap::findFromRoot(allocation) == expected, "original callback must retain its scene Game heap");
        delete[] allocation;
        aurora::allocation::HostAllocationScope host;
        log.push_back(value);
    }
    int id;
    std::vector<int>& log;
    JKRHeap* expected = nullptr;
    std::function<void()> hook;
};

struct VisibilityActor final : LiveActor {
    VisibilityActor() : LiveActor("original model visibility fixture") {}
    void draw() const override { ++draws; }
    mutable unsigned draws = 0;
};

void verify_model_visibility(smgpc::runtime::SceneScheduler& scheduler,
                             const JKRHeap::Handle& heaps,
                             smgpc::test::OriginalSceneControllerFixture& original) {
    const auto domain = smgpc::test::create_native_solid_heap(heaps, 1U << 20);
    smgpc::test::SceneExecutionFixture scene(scheduler, domain, &original.scene);
    require(MR::createSceneObj(SceneObj_ClippingDirector) != nullptr,
            "the original LiveActor constructor requires its scene clipping director");
    VisibilityActor actor;
    actor.mFlag.mIsDead = false;
    MR::connectToScene(&actor, -1, -1, -1, 72);
    MR::connectToDrawTemporarily(&actor);
    MR::hideModel(&actor);
    scene.complete_initialization();
    const auto member_count = [&] { return scene.executor().mDrawList->mCategoryInfo[72].mNameObjArr.size(); };
    require(member_count() == 0 && actor.mFlag.mIsNoCalcAnim && actor.mFlag.mIsNoCalcView,
            "hiding before scene initialization cancels pending draw membership and stops model calculation");
    MR::showModel(&actor);
    require(member_count() == 0 && !actor.mFlag.mIsNoCalcAnim && !actor.mFlag.mIsNoCalcView,
            "showing restores model calculation while draw membership waits for its original requirement phase");
    scene.apply_connections();
    scheduler.execute_draw_type(72);
    require(member_count() == 1 && actor.draws == 1, "showing reconnects the original draw category exactly once");
    MR::hideModel(&actor);
    MR::hideModel(&actor);
    require(member_count() == 1, "hiding a live actor queues removal without mutating an active draw batch");
    scene.apply_connections();
    scheduler.execute_draw_type(72);
    require(member_count() == 0 && actor.draws == 1, "hidden actors stop receiving actual original draw callbacks");
    MR::showModel(&actor);
    MR::showModel(&actor);
    scene.apply_connections();
    scheduler.execute_draw_type(72);
    require(member_count() == 1 && actor.draws == 2, "repeated visibility requests never duplicate original category entries");
    MR::hideModelAndOnCalcAnim(&actor);
    scene.apply_connections();
    scheduler.execute_draw_type(72);
    require(member_count() == 0 && actor.draws == 2 && !actor.mFlag.mIsNoCalcAnim && actor.mFlag.mIsNoCalcView,
            "hidden animated models retain animation but leave the original draw category and stop view calculation");
    MR::showModel(&actor);
    scene.apply_connections();
    MR::offEntryDrawBuffer(&actor);
    scene.apply_connections();
    require(member_count() == 0 && !actor.mFlag.mIsNoCalcAnim && !actor.mFlag.mIsNoCalcView,
            "entry-only hiding removes draw membership while preserving animation and view calculation");
    MR::onEntryDrawBuffer(&actor);
    scene.apply_connections();
    require(member_count() == 1, "entry-only showing restores original draw membership");
    MR::hideModel(&actor);
    scene.apply_connections();
    for (bool dead : {true, false}) {
        actor.mFlag.mIsDead = dead;
        actor.mFlag.mIsClipped = !dead;
        MR::showModel(&actor);
        scene.apply_connections();
        require(member_count() == 0 && !actor.mFlag.mIsHiddenModel,
                "showing a dead or clipped actor changes visibility without reconnecting draw execution");
        MR::hideModel(&actor);
        scene.apply_connections();
        require(member_count() == 0, "hiding a dead or clipped actor leaves draw execution disconnected");
    }
    actor.mFlag.mIsDead = false;
    actor.mFlag.mIsClipped = false;
    MR::showModel(&actor);
    scene.apply_connections();
    MR::hideModel(&actor);
    MR::showModel(&actor);
    scene.apply_connections();
    scheduler.execute_draw_type(72);
    require(member_count() == 1 && actor.draws == 3,
            "same-phase hide and show cancel removal without duplicating the original draw category");
}
}

int main() {
    try {
        using namespace smgpc;
        const auto heaps = smgpc::test::create_native_root_heap(16U << 20);
        test::OriginalSceneControllerFixture original(heaps);
        const auto free = (*heaps).getFreeSize();
        NameObj outside("outside the scene NameObjHolder");
        const auto identities = NameObj::snapshotNativeObjects().size();
        runtime::SceneScheduler scheduler;
        runtime::SceneSchedulerBinding active(scheduler);
        verify_model_visibility(scheduler, heaps, original);
        for (int generation = 0; generation < 16; ++generation) {
            auto domain = smgpc::test::create_native_solid_heap(heaps, 1U << 20);
            std::weak_ptr<JKRHeap> weak = domain;
            {
                test::SceneExecutionFixture scene(scheduler, domain, &original.scene);
                auto* original_holder = &scene.executor().nativeRequirements();
                require(JKRHeap::findFromRoot(original_holder) == &(*domain), "actual requirement holder must use the scene heap");
                require(scene.executor().mBufferHolder != nullptr, "the real executor owns one draw holder");
                std::vector<int> log;
                Object a(1, log), b(2, log), c(3, log);
                a.setName("scene member a");
                require(NameObjFinder::find("scene member a") == &a && !NameObjFinder::find(outside.getName()),
                        "original lookup searches the active scene holder, excluding process-wide identities");
                for (auto* object : {&a, &b, &c}) {
                    object->expected = &(*domain);
                    MR::connectToScene(static_cast<NameObj*>(object), 34, 0, -1, 72);
                }
                require(scene.executor().mMovementList->mCategoryInfo[34].mNameObjArr.size() == 0,
                        "pre-initialization requests count objects without publishing list membership");
                scene.complete_initialization();
                scheduler.execute_movement_category(34);
                require(log == std::vector<int>({1,2,3}), "original category preserves initial registration order");
                log.clear();
                MR::disconnectToSceneTemporarily(&a);
                MR::disconnectToDrawTemporarily(&a);
                scheduler.execute_movement_category(34);
                require(log == std::vector<int>({1,2,3}), "queued disconnection must wait for its requirement phase");
                log.clear();
                scene.apply_connections();
                scheduler.execute_movement_category(34);
                scheduler.execute_calc_anim_category(0);
                scheduler.execute_draw_type(72);
                require(log == std::vector<int>({3,2,13,12,23,22}), "original swap-last removal drives every execution category");
                require(!scheduler.is_draw_connected(a), "queue draw-state query observes applied disconnect");
                log.clear();
                MR::connectToSceneTemporarily(&a);
                MR::connectToDrawTemporarily(&a);
                scene.apply_connections();
                scheduler.request_movement_off(34);
                scheduler.execute_movement_category(34);
                require(log == std::vector<int>({3,2,1}), "pending movement flags remain deferred before holder synchronization");
                log.clear();
                MR::getSceneNameObjMovementController()->movement();
                scheduler.execute_movement_category(34);
                scheduler.execute_calc_anim_category(0);
                require(log == std::vector<int>({13,12,11}), "movement suspension leaves original animation connected");
                scheduler.request_movement_on(34);
                MR::getSceneNameObjMovementController()->movement();
                log.clear();
                // A pending disconnect for another object must not be flushed
                // early when one registration is permanently retired.
                MR::disconnectToSceneTemporarily(&c);
                scheduler.disconnect_name_obj(b);
                require(b.mExecutorIdx == -1, "permanent retirement tombstones the original slot");
                scheduler.execute_movement_category(34);
                require(log == std::vector<int>({3,1}), "individual teardown preserves another object's queued phase");
                log.clear();
                scene.apply_connections();
                auto victim = std::make_unique<Object>(4, log);
                victim->expected = &(*domain);
                scheduler.connect_name_obj(*victim, 34, -1, -1, -1);
                scene.apply_connections();
                a.hook = [&] { victim.reset(); };
                scheduler.execute_movement_category(34);
                require(!victim && log == std::vector<int>({1}), "a callback cannot execute a deleted later list member");
                a.hook = {};
                scene.executor().prepareNativeRetirement();
                require(a.mExecutorIdx == -1 && c.mExecutorIdx == -1, "scene retirement applies each remaining original disconnection");
                MR::disconnectToSceneTemporarily(&a);
                MR::disconnectToDrawTemporarily(&a);
            }
            domain.reset();
            require(weak.expired() && !scheduler.allocation_heap(), "scene ownership releases every retained callback domain");
            require((*heaps).getFreeSize() == free, "all original executor and requirement allocations reclaim together");
            require(NameObj::snapshotNativeObjects().size() == identities, "scene controllers and late NameObjs retire their identities");
        }
        {
            auto domain = smgpc::test::create_native_solid_heap(heaps, 1U << 20);
            test::SceneExecutionFixture scene(scheduler, domain, &original.scene);
            layout::LayoutRuntime native_layout("scheduler-owned layout adaptor", "ownership fixture", 1, 72);
            const auto before = NameObj::snapshotNativeObjects().size();
            scheduler.register_layout(native_layout, 34, -1, 72);
            auto* adaptor = NameObjFinder::find(native_layout.getName().c_str());
            require(adaptor && NameObj::isNativeOwnershipClaimed(adaptor),
                    "the actual layout adaptor is claimed by its retaining scheduler");
            scene.complete_initialization();
            require(NameObjFinder::find(native_layout.getName().c_str()) == adaptor &&
                        NameObj::snapshotNativeObjects().size() == before + 1,
                    "the scheduler retains its own layout adaptor after execution initialization");
            scheduler.unregister_layout(native_layout);
            require(!NameObjFinder::find(native_layout.getName().c_str()) &&
                        NameObj::snapshotNativeObjects().size() == before,
                    "explicit layout unregistration destroys its adaptor and ownership record exactly once");
            scheduler.register_layout(native_layout, 34, -1, 72);
            scheduler.clear();
            require(!NameObjFinder::find(native_layout.getName().c_str()) &&
                        NameObj::snapshotNativeObjects().size() == before,
                    "scheduler clear also retires a protected layout adaptor without stale ownership");
        }
        require((*heaps).getFreeSize() == free && NameObj::snapshotNativeObjects().size() == identities,
                "layout adaptor teardown retains no scene domain or runtime identities");
        require(NameObjFinder::find(outside.getName()) == nullptr,
                "the original process holder remains valid after a scene and contains no unregistered object");
        std::cout << "original_queue=pass deferred_connections=pass category_swap_order=pass callback_retirement=pass sixteen_scene_domains=pass layout_adaptor_ownership=pass model_visibility=pass\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
