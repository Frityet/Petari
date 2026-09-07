#include "SceneExecutionFixture.hpp"
#include "Game/NameObj/NameObjCategoryList.hpp"
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/NameObj/NameObjFinder.hpp"
#include "Game/Scene/SceneNameObjMovementController.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
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
        smgpc::compat::JkrHostAllocationScope host;
        log.push_back(value);
    }
    int id;
    std::vector<int>& log;
    JKRHeap* expected = nullptr;
    std::function<void()> hook;
};
}

int main() {
    try {
        using namespace smgpc;
        const auto heaps = compat::JkrHeapRuntime::create(16U << 20);
        const auto free = heaps->root_heap().getFreeSize();
        NameObj outside("outside the scene NameObjHolder");
        const auto identities = compat::name_obj_runtime_state_count();
        runtime::SceneScheduler scheduler;
        runtime::SceneSchedulerBinding active(scheduler);
        for (int generation = 0; generation < 16; ++generation) {
            auto domain = compat::JkrAllocationDomain::create(heaps, 1U << 20);
            std::weak_ptr<compat::JkrAllocationDomain> weak = domain;
            {
                test::SceneExecutionFixture scene(scheduler, domain);
                auto* original_holder = &scene.execution().requirements();
                require(JKRHeap::findFromRoot(original_holder) == &domain->heap(), "actual requirement holder must use the scene heap");
                require(scene.executor().mBufferHolder != nullptr, "the real executor owns one draw holder");
                std::vector<int> log;
                Object a(1, log), b(2, log), c(3, log);
                a.setName("scene member a");
                require(NameObjFinder::find("scene member a") == &a && !NameObjFinder::find(outside.getName()),
                        "original lookup searches the active scene holder, excluding process-wide identities");
                for (auto* object : {&a, &b, &c}) {
                    object->expected = &domain->heap();
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
                victim->expected = &domain->heap();
                scheduler.connect_name_obj(*victim, 34, -1, -1, -1);
                scene.apply_connections();
                a.hook = [&] { victim.reset(); };
                scheduler.execute_movement_category(34);
                require(!victim && log == std::vector<int>({1}), "a callback cannot execute a deleted later list member");
                a.hook = {};
                scene.execution().prepare_retirement();
                require(a.mExecutorIdx == -1 && c.mExecutorIdx == -1, "scene retirement applies each remaining original disconnection");
                MR::disconnectToSceneTemporarily(&a);
                MR::disconnectToDrawTemporarily(&a);
            }
            domain.reset();
            require(weak.expired() && !scheduler.allocation_domain(), "scene ownership releases every retained callback domain");
            require(heaps->root_heap().getFreeSize() == free, "all original executor and requirement allocations reclaim together");
            require(compat::name_obj_runtime_state_count() == identities, "scene controllers and late NameObjs retire their identities");
        }
        bool rejected = false;
        try { (void)NameObjFinder::find(outside.getName()); }
        catch (const std::logic_error&) { rejected = true; }
        require(rejected, "original name lookup requires the owning scene holder");
        std::cout << "original_queue=pass deferred_connections=pass category_swap_order=pass callback_retirement=pass sixteen_scene_domains=pass\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
