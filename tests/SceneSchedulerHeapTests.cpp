#include "runtime/SceneScheduler.hpp"
#include "SceneExecutionFixture.hpp"
#include "Game/Scene/SceneNameObjMovementController.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/ActorPhysicsRuntime.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <array>
#include <algorithm>
#include <cstring>
#include <functional>
#include <aurora/allocation.hpp>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct CallbackAllocations {
    explicit CallbackAllocations(smgpc::runtime::SceneScheduler& value) : scheduler(value) {}
    ~CallbackAllocations() { release(); }
    void record(unsigned slot) const {
        delete[] values[slot];
        values[slot] = new unsigned[8];
        ++calls[slot];
        require(scheduler.allocation_domain() &&
                    JKRHeap::findFromRoot(values[slot]) == &scheduler.allocation_domain()->heap(),
                "original callback allocation must belong to its explicit scene Game heap");
    }
    void release() const {
        for (auto*& pointer : values) { delete[] pointer; pointer = nullptr; }
    }
    smgpc::runtime::SceneScheduler& scheduler;
    mutable std::array<unsigned*, 8> values{};
    mutable std::array<unsigned, 8> calls{};
};

struct CallbackObject final : NameObj {
    explicit CallbackObject(smgpc::runtime::SceneScheduler& scheduler) : NameObj("scene callback ownership"), allocation(scheduler) {}
    void movement() override { allocation.record(0); if (movement_hook) movement_hook(); }
    void calcAnim() override { allocation.record(1); if (animation_hook) animation_hook(); }
    void draw() const override { allocation.record(2); }
    void pre_draw() const { allocation.record(3); }
    CallbackAllocations allocation;
    std::function<void()> movement_hook;
    std::function<void()> animation_hook;
};

struct CallbackActor final : LiveActor {
    explicit CallbackActor(smgpc::runtime::SceneScheduler& scheduler) : LiveActor("scene sensor callback ownership"), allocation(scheduler) {}
    void movement() override { allocation.record(0); LiveActor::movement(); }
    void calcAnim() override { allocation.record(1); }
    void attackSensor(HitSensor*, HitSensor*) override { allocation.record(4); if (sensor_hook) sensor_hook(); }
    bool receiveMessage(u32, HitSensor*, HitSensor*) override {
        allocation.record(5);
        if (message_hook) message_hook();
        return true;
    }
    void startClipped() override { allocation.record(6); mFlag.mIsClipped = true; }
    void endClipped() override { allocation.record(7); mFlag.mIsClipped = false; }
    CallbackAllocations allocation;
    std::function<void()> sensor_hook;
    std::function<void()> message_hook;
};

void verify_backend_allocation_routing(const std::shared_ptr<smgpc::compat::JkrHeapRuntime>& heaps) {
    using namespace smgpc::compat;
    const auto before = heaps->root_heap().getFreeSize();
    {
        auto game = JkrAllocationDomain::create(heaps, 32U << 10);
        auto nested = JkrAllocationDomain::create(heaps, 32U << 10);
        const auto allocate_in = [](JKRHeap* expected) {
            auto* allocation = new unsigned[8];
            require(JKRHeap::findFromRoot(allocation) == expected, "native backend/client scope selects the wrong allocation owner");
            delete[] allocation;
        };
        {
            JkrAllocationScope outer(game);
            allocate_in(&game->heap());
            {
                aurora::allocation::HostAllocationScope backend;
                allocate_in(nullptr);
                {
                    aurora::allocation::HostAllocationScope nested_backend;
                    allocate_in(nullptr);
                    {
                        aurora::allocation::ClientAllocationScope callback;
                        allocate_in(&game->heap());
                    }
                    allocate_in(nullptr);
                }
                {
                    JkrAllocationScope explicit_nested(nested);
                    allocate_in(&nested->heap());
                    aurora::allocation::HostAllocationScope inner_backend;
                    allocate_in(nullptr);
                    aurora::allocation::ClientAllocationScope inner_callback;
                    allocate_in(&nested->heap());
                }
                allocate_in(nullptr);
                require(current_jkr_allocation_domain() == game, "nested native calls restore the selected original heap");
                try {
                    aurora::allocation::ClientAllocationScope callback;
                    allocate_in(&game->heap());
                    throw 91;
                } catch (int code) {
                    require(code == 91, "native callback exception fixture failed");
                }
                allocate_in(nullptr);
            }
            allocate_in(&game->heap());
        }
        allocate_in(nullptr);
    }
    require(heaps->root_heap().getFreeSize() == before, "nested backend/client routing retains no retired Game domain");
    std::cout << "backend_host_escape=pass nested_client_reentry=pass explicit_nested_domain=pass routing_exception_restore=pass\n";
}

void verify_explicit_scene_callbacks(const std::shared_ptr<smgpc::compat::JkrHeapRuntime>& heaps) {
    using namespace smgpc::compat;
    using namespace smgpc::runtime;
    const auto free_before = heaps->root_heap().getFreeSize();
    {
        auto game = JkrAllocationDomain::create(heaps, 2U << 20);
        auto caller = JkrAllocationDomain::create(heaps, 64U << 10);
        SceneScheduler scheduler;
        SceneSchedulerBinding active(scheduler);
        SceneSchedulerAllocationBinding scene(scheduler, game);
        smgpc::test::SceneExecutionFixture execution(scheduler, game);
        CallbackObject object(scheduler);
        scheduler.connect_name_obj(object, 34, 0, -1, 72);
        scheduler.register_pre_draw_function(MR::Functor(static_cast<const CallbackObject*>(&object), &CallbackObject::pre_draw), 72);
        execution.complete_initialization();
        {
            JkrAllocationScope outer(caller);
            scheduler.execute_movement();
            scheduler.execute_calc_anim();
            scheduler.execute_draw_type(72);
            require(current_jkr_allocation_domain() == caller, "scene callbacks restore their caller's selected heap");
            auto* after = new unsigned[8];
            require(JKRHeap::findFromRoot(after) == &caller->heap(), "scene callbacks restore the caller's allocation routing");
            delete[] after;
        }
        require(object.allocation.calls[0] == 1 && object.allocation.calls[1] == 1 &&
                    object.allocation.calls[2] == 1 && object.allocation.calls[3] == 1,
                "movement, animation, draw and pre-draw all invoke real allocating callbacks");
        {
            SceneSchedulerAllocationBinding nested(scheduler, caller);
            scheduler.execute_calc_anim();
            require(JKRHeap::findFromRoot(object.allocation.values[1]) == &caller->heap(), "nested scene bindings select their own domain");
        }
        require(scheduler.allocation_domain() == game, "leaving a nested binding restores the previous scene domain");
        object.movement_hook = [&] { scheduler.execute_calc_anim(); throw 73; };
        {
            JkrAllocationScope outer(caller);
            bool caught = false;
            try { scheduler.execute_movement(); } catch (int code) { caught = code == 73; }
            require(caught && current_jkr_allocation_domain() == caller,
                    "nested callbacks and exceptions restore the caller's original heap");
        }
        object.movement_hook = {};
        auto* host = new unsigned[8];
        require(JKRHeap::findFromRoot(host) == nullptr, "callback exceptions restore host allocation after the outer scope ends");
        delete[] host;
        require(JKRHeap::findFromRoot(const_cast<SceneSchedulerEntryState*>(scheduler.last_execution_trace().data())) == nullptr,
                "callback traces retain host backing while the scene is bound");
        scheduler.clear();

        CallbackObject mutator(scheduler), survivor(scheduler);
        auto victim = std::make_unique<CallbackObject>(scheduler);
        std::vector<std::unique_ptr<NameObj>> added;
        bool mutated = false;
        mutator.movement_hook = [&] {
            if (mutated) return;
            mutated = true;
            for (unsigned i = 0; i < 512; ++i) {
                auto* created = new NameObj("created from an original movement callback");
                scheduler.connect_name_obj(*created, 34, 0, -1, -1);
                JkrHostAllocationScope native;
                added.emplace_back(created);
            }
            victim.reset();
            scheduler.disconnect_name_obj(mutator);
        };
        scheduler.connect_name_obj(mutator, 34, 0, -1, -1);
        scheduler.connect_name_obj(*victim, 34, 0, -1, -1);
        scheduler.connect_name_obj(survivor, 34, 0, -1, -1);
        scheduler.execute_movement();
        require(victim == nullptr && added.size() == 512 && survivor.allocation.calls[0] == 1,
                "movement survives host registration reallocation and removal of current and future entries");
        require(JKRHeap::findFromRoot(added.back().get()) == &game->heap() && JKRHeap::findFromRoot(added.data()) == nullptr,
                "new original objects use the scene heap while the fixture's registry remains host owned");
        scheduler.clear();
        added.clear();

        auto animation_victim = std::make_unique<CallbackObject>(scheduler);
        mutator.animation_hook = [&] { animation_victim.reset(); scheduler.disconnect_name_obj(mutator); };
        scheduler.connect_name_obj(mutator, -1, 0, -1, -1);
        scheduler.connect_name_obj(*animation_victim, -1, 0, -1, -1);
        scheduler.connect_name_obj(survivor, -1, 0, -1, -1);
        execution.apply_connections();
        scheduler.execute_calc_anim();
        require(animation_victim == nullptr && survivor.allocation.calls[1] == 1,
                "animation revalidates registrations after callback removal");
        scheduler.clear();

        CallbackObject replacement(scheduler);
        mutator.movement_hook = [&] { scheduler.clear(); scheduler.connect_name_obj(replacement, 34, -1, -1, -1); };
        scheduler.connect_name_obj(mutator, 34, -1, -1, -1);
        scheduler.connect_name_obj(survivor, 34, -1, -1, -1);
        const auto marker = scheduler.registration_marker();
        scheduler.execute_movement();
        require(replacement.allocation.calls[0] == 0 && scheduler.registration_marker() > marker,
                "clear and reconnect do not reuse a snapshot's registration identity");
        scheduler.execute_movement();
        require(replacement.allocation.calls[0] == 1, "new registrations run in the next movement snapshot");
        scheduler.clear();

        (void)MR::createSceneObj(SceneObj_MessageSensorHolder);
        CallbackActor first(scheduler);
        auto second = std::make_unique<CallbackActor>(scheduler);
        for (auto* actor : {&first, second.get()}) {
            JkrAllocationScope initialization_heap(game);
            actor->initHitSensor(1);
            (void)add_actor_hit_sensor(actor, "body", 1U, 4U, 10.0F, {});
            actor->makeActorAppeared();
            scheduler.connect_name_obj(*actor, 34, 0, -1, -1);
        }
        first.sensor_hook = [&] { second.reset(); };
        scheduler.execute_movement();
        require(second == nullptr && first.allocation.calls[4] == 1,
                "sensor callbacks may retire the other actor without stale sensor dereferences");
        const auto initial_animation_calls = first.allocation.calls[1];
        scheduler.execute_calc_anim();
        require(first.allocation.calls[1] == initial_animation_calls + 1, "original actor animation receives the scene domain");
        auto message_victim = std::make_unique<CallbackActor>(scheduler);
        {
            JkrAllocationScope initialization_heap(game);
            message_victim->makeActorAppeared();
        }
        scheduler.connect_name_obj(*message_victim, 34, -1, -1, -1);
        first.message_hook = [&] { message_victim.reset(); };
        {
            JkrAllocationScope callback_heap(game);
            MR::sendMsgToAllLiveActor(0, nullptr);
        }
        require(!message_victim && first.allocation.calls[5] == 1,
                "original group broadcasts may remove future recipients without invalidating live traversal");

        configure_actor_clipping_sphere(&first, 1.0F, nullptr);
        CallbackObject clipping_driver(scheduler);
        clipping_driver.movement_hook = [&] {
            smgpc::camera::CameraPose camera{.eye = {0, 0, 1000}, .watch = {0, 0, 0}};
            first.mPosition.x = 1000000;
            update_actor_clipping(first, camera);
            first.mPosition.x = 0;
            update_actor_clipping(first, camera);
        };
        scheduler.connect_name_obj(clipping_driver, 34, -1, -1, -1);
        scheduler.execute_movement();
        require(first.allocation.calls[6] == 1 && first.allocation.calls[7] == 1,
                "native clipping transitions preserve the Game callback allocation scope");
        scheduler.clear();
    }
    require(heaps->root_heap().getFreeSize() == free_before, "all callback and scene domain allocations retire at scene teardown");
    {
        SceneScheduler scheduler;
        SceneSchedulerBinding active(scheduler);
        auto execution_domain = JkrAllocationDomain::create(heaps, 512U << 10);
        smgpc::test::SceneExecutionFixture execution(scheduler, execution_domain);
        auto domain = JkrAllocationDomain::create(heaps, 64U << 10);
        std::weak_ptr<JkrAllocationDomain> weak = domain;
        auto scene = std::make_unique<SceneSchedulerAllocationBinding>(scheduler, domain);
        domain.reset();
        CallbackObject object(scheduler);
        object.movement_hook = [&] {
            object.allocation.release();
            scene.reset();
            require(!weak.expired(), "an executing callback retains the scene heap after its binding is removed");
            auto* value = new unsigned[8];
            require(JKRHeap::findFromRoot(value) == &weak.lock()->heap(), "remaining callback work still uses its retained domain");
            delete[] value;
        };
        scheduler.connect_name_obj(object, 34, -1, -1, -1);
        execution.complete_initialization();
        scheduler.execute_movement();
        require(weak.expired() && scheduler.allocation_domain() == execution_domain, "the callback's last temporary domain lease releases on return to the execution owner");
        scheduler.clear();
    }
    require(heaps->root_heap().getFreeSize() == free_before, "removal of an active scene binding leaves no retained heap");
    std::cout << "explicit_scene_heap=pass nested_exception_restoration=pass stable_registration_mutation=pass "
                 "sensor_message_retirement=pass clipping_transitions=pass predraw_draw=pass callback_lease=pass scene_heap_retirement=pass\n";
}

void verify_category_execution(const std::shared_ptr<smgpc::compat::JkrHeapRuntime>& heaps) {
    using namespace smgpc::compat;
    using namespace smgpc::runtime;
    const auto free_before = heaps->root_heap().getFreeSize();
    {
        auto domain = JkrAllocationDomain::create(heaps, 1U << 20);
        auto caller = JkrAllocationDomain::create(heaps, 32U << 10);
        SceneScheduler scheduler;
        SceneSchedulerBinding active(scheduler);
        SceneSchedulerAllocationBinding game(scheduler, domain);
        smgpc::test::SceneExecutionFixture execution(scheduler, domain);
        CallbackObject camera(scheduler), clipping(scheduler), platform(scheduler), collision(scheduler), player(scheduler);
        std::vector<unsigned> order;
        const auto record = [&](unsigned event) {
            JkrHostAllocationScope host;
            order.push_back(event);
        };
        camera.movement_hook = [&] { record(1); };
        clipping.movement_hook = [&] { require(order == std::vector<unsigned>{1}, "camera category precedes clipping category"); record(2); };
        platform.movement_hook = [&] { record(3); };
        platform.animation_hook = [&] { record(4); };
        collision.movement_hook = [&] { require(platform.allocation.calls[1] == 1, "collision director sees this frame's collision animation"); record(5); };
        player.movement_hook = [&] { record(6); };
        // Deliberately register in an order different from the caller's phases.
        scheduler.connect_name_obj(player, MR::MovementType_Player, MR::CalcAnimType_Player, -1, -1);
        scheduler.connect_name_obj(collision, MR::MovementType_CollisionDirector, -1, -1, -1);
        scheduler.connect_name_obj(platform, MR::MovementType_CollisionMapObj, MR::CalcAnimType_CollisionMapObj, -1, -1);
        scheduler.connect_name_obj(clipping, MR::MovementType_ClippingDirector, -1, -1, -1);
        scheduler.connect_name_obj(camera, MR::MovementType_Camera, -1, -1, -1);
        execution.complete_initialization();
        {
            JkrAllocationScope external(caller);
            scheduler.begin_frame();
            CategoryList::execute(MR::MovementType_Camera);
            CategoryList::execute(MR::MovementType_ClippingDirector);
            CategoryList::execute(MR::MovementType_SensorHitChecker);
            CategoryList::execute(MR::MovementType_CollisionMapObj);
            CategoryList::execute(MR::CalcAnimType_CollisionMapObj);
            CategoryList::execute(MR::MovementType_CollisionDirector);
            CategoryList::execute(MR::MovementType_Player);
            require(current_jkr_allocation_domain() == caller, "category callbacks restore the caller's selected heap");
        }
        require(order == std::vector<unsigned>({1, 2, 3, 4, 5, 6}), "categories execute exactly the caller's movement/animation interleave");
        require(player.allocation.calls[1] == 0 && scheduler.last_execution_trace().size() == 6,
                "category dispatch never executes an aggregate animation list or clears earlier category traces");
        scheduler.request_movement_off(MR::MovementType_Player);
        MR::getSceneNameObjMovementController()->movement();
        CategoryList::execute(MR::MovementType_Player);
        CategoryList::execute(MR::CalcAnimType_Player);
        require(player.allocation.calls[0] == 1 && player.allocation.calls[1] == 1,
                "movement-off does not suppress original animation category calls");
        scheduler.clear();

        CallbackObject mutator(scheduler), survivor(scheduler), replacement(scheduler);
        auto victim = std::make_unique<CallbackObject>(scheduler);
        bool changed = false;
        mutator.movement_hook = [&] {
            if (changed) return;
            changed = true;
            victim.reset();
            scheduler.disconnect_name_obj(survivor);
            scheduler.connect_name_obj(survivor, MR::MovementType_Player, -1, -1, -1);
            scheduler.connect_name_obj(replacement, MR::MovementType_Player, -1, -1, -1);
        };
        for (auto* object : {&mutator, victim.get(), &survivor})
            scheduler.connect_name_obj(*object, MR::MovementType_Player, -1, -1, -1);
        execution.apply_connections();
        CategoryList::execute(MR::MovementType_Player);
        require(!victim && survivor.allocation.calls[0] == 0 && replacement.allocation.calls[0] == 0,
                "a category batch never uses deleted or re-registered snapshot identities");
        execution.apply_connections();
        CategoryList::execute(MR::MovementType_Player);
        require(survivor.allocation.calls[0] == 1 && replacement.allocation.calls[0] == 1,
                "new category registrations become visible to the next category call");
        scheduler.clear();

        const auto original_mode = j3dSys.mDrawMode;
        mutator.movement_hook = [&] {
            j3dSys.mDrawMode = 0x1234;
            CategoryList::execute(MR::CalcAnimType_Player);
            require(j3dSys.mDrawMode == 0x1234, "nested category restores its calling J3D context");
            throw 83;
        };
        replacement.animation_hook = [&] { j3dSys.mDrawMode = 0x5678; };
        scheduler.connect_name_obj(mutator, MR::MovementType_Player, -1, -1, -1);
        scheduler.connect_name_obj(replacement, -1, MR::CalcAnimType_Player, -1, -1);
        execution.apply_connections();
        {
            JkrAllocationScope external(caller);
            bool caught = false;
            try { CategoryList::execute(MR::MovementType_Player); } catch (int code) { caught = code == 83; }
            require(caught && current_jkr_allocation_domain() == caller && j3dSys.mDrawMode == original_mode,
                    "nested category exception restores both heap routing and J3D caller state");
        }
        scheduler.clear();

        CallbackActor first(scheduler), second(scheduler);
        for (auto* actor : {&first, &second}) {
            JkrAllocationScope initialization_heap(domain);
            actor->initHitSensor(1);
            (void)add_actor_hit_sensor(actor, "body", 1U, 4U, 10.0F, {});
            actor->makeActorAppeared();
            scheduler.connect_name_obj(*actor, MR::MovementType_Player, MR::CalcAnimType_Player, -1, -1);
        }
        execution.apply_connections();
        CategoryList::execute(MR::MovementType_Player);
        require(first.allocation.calls[4] == 0, "player category does not inject a sensor checker");
        CategoryList::execute(MR::MovementType_SensorHitChecker);
        auto* sensor = actor_hit_sensor(&first, "body");
        require(sensor && sensor->mSensorCount == 1, "sensor category computes one contact pass");
        second.mPosition.x = 1000.0F;
        update_actor_hit_sensors(&second);
        CategoryList::execute(MR::CalcAnimType_Player);
        CategoryList::execute(MR::MovementType_CollisionDirector);
        execution.apply_connections();
        CategoryList::execute(MR::MovementType_Player);
        require(first.allocation.calls[4] == 1 && second.allocation.calls[4] == 1,
                "later categories deliver the existing contacts instead of silently recomputing them");
        CategoryList::execute(MR::MovementType_SensorHitChecker);
        require(sensor->mSensorCount == 0, "the next explicit sensor category publishes the new positions");
        scheduler.clear();
    }
    require(heaps->root_heap().getFreeSize() == free_before, "category callbacks retain no retired scene arena");
    std::cout << "category_interleave=pass category_identity=pass category_exception_restore=pass single_sensor_phase=pass\n";
}
}

class AllocatingNameObj final : public NameObj {
public:
    AllocatingNameObj() : NameObj("original callback allocation routing") {}
    void movement() override { movement_allocation = new unsigned[8]; }
    void calcAnim() override { animation_allocation = new unsigned[8]; }
    unsigned* movement_allocation = nullptr;
    unsigned* animation_allocation = nullptr;
};

class MetadataActor final : public LiveActor {
public:
    MetadataActor() : LiveActor("live actor metadata heap lifetime") {}
    void movement() override { ++movements; }
    bool receiveMessage(u32, HitSensor*, HitSensor*) override { ++messages; return true; }
    unsigned movements = 0;
    unsigned messages = 0;
};

int main() {
    using namespace smgpc::compat;
    auto heaps = JkrHeapRuntime::create(16U << 20);
    auto domain = JkrAllocationDomain::create(heaps, 64U << 10);
    std::vector<NameObj*> snapshot;
    std::vector<smgpc::runtime::SceneSchedulerEntryState> scheduler_snapshot;
    u32 after_object = 0;
    {
        // First-ever registration must also allocate its global hash buckets
        // on the host. Do not prewarm the registry before this scope.
        JkrAllocationScope scope(domain);
        auto object = std::make_unique<NameObj>("original heap object");
        if (JKRHeap::findFromRoot(object.get()) != &domain->heap())
            throw std::runtime_error("Game NameObj did not use the original heap");
        if (JKRHeap::findFromRoot(const_cast<char*>(object->mName)) != nullptr)
            throw std::runtime_error("first NameObj registration used the original heap for its name");
        after_object = domain->heap().getTotalFreeSize();
        std::array<char, 512> long_name;
        std::fill(long_name.begin(), long_name.end() - 1, 'n');
        long_name.back() = 0;
        object->setName(long_name.data());
        snapshot = snapshot_name_obj_runtime_objects();
        if (snapshot.size() != 1 || snapshot[0] != object.get() ||
            JKRHeap::findFromRoot(snapshot.data()) != nullptr ||
            JKRHeap::findFromRoot(const_cast<char*>(object->mName)) != nullptr)
            throw std::runtime_error("NameObj registry metadata used the original heap");
    }
    // JKRSolidHeap keeps the original object's arena allocation until reset.
    // Renaming, snapshots and registry retirement must consume no more space.
    if (domain->heap().getTotalFreeSize() != after_object)
        throw std::runtime_error("NameObj metadata consumed additional original heap storage");
    smgpc::runtime::SceneScheduler scheduler;
    smgpc::runtime::SceneSchedulerBinding active(scheduler);
    auto execution_domain = JkrAllocationDomain::create(heaps, 1U << 20);
    auto execution = std::make_unique<smgpc::test::SceneExecutionFixture>(scheduler, execution_domain);
    std::vector<std::unique_ptr<NameObj>> objects;
    for (int i = 0; i < 128; ++i)
        objects.push_back(std::make_unique<NameObj>("heap registration fixture"));
    const auto before = domain->heap().getTotalFreeSize();
    {
        JkrAllocationScope scope(domain);
        for (auto& object : objects)
            scheduler.connect_name_obj(*object, 34, 0, -1, -1);
        execution->complete_initialization();
        scheduler.execute_movement_category(34);
        scheduler.execute_calc_anim_category(0);
        scheduler_snapshot = scheduler.snapshot();
        const auto trace = scheduler.last_execution_trace();
        if (trace.size() != objects.size() * 2 || std::count_if(scheduler_snapshot.begin(), scheduler_snapshot.end(), [](const auto& item) { return item.name == "heap registration fixture"; }) != objects.size() ||
            JKRHeap::findFromRoot(const_cast<smgpc::runtime::SceneSchedulerEntryState*>(trace.data())) != nullptr ||
            JKRHeap::findFromRoot(scheduler_snapshot.data()) != nullptr)
            throw std::runtime_error("scheduler history or snapshots use the original Game heap");
        for (const auto& state : trace)
            if (JKRHeap::findFromRoot(const_cast<char*>(state.name.data())) != nullptr)
                throw std::runtime_error("scheduler trace name uses the original Game heap");
        if (domain->heap().getTotalFreeSize() != before)
            throw std::runtime_error("scheduler registration, sorting, tracing or snapshots consumed the Game heap");
    }
    domain.reset();
    // Overwrite a reused arena before reading retained trace strings, so stale
    // arena contents cannot make the ownership check accidentally pass.
    {
        auto churn = JkrAllocationDomain::create(heaps, 64U << 10);
        JkrAllocationScope scope(churn);
        auto* bytes = new unsigned char[32U << 10];
        std::memset(bytes, 0xa5, 32U << 10);
        delete[] bytes;
    }
    for (const auto& state : scheduler.last_execution_trace())
        if (state.name != "heap registration fixture")
            throw std::runtime_error("scheduler history changed after scene heap retirement");
    for (const auto& state : scheduler_snapshot)
        if (state.movement_type == 34 && state.name != "heap registration fixture")
            throw std::runtime_error("scheduler snapshot changed after scene heap retirement");
    scheduler_snapshot.clear();
    snapshot.clear();
    snapshot.shrink_to_fit();
    scheduler.execute_movement();
    for (auto& object : objects) {
        if (!scheduler.is_draw_connected(*object))
            throw std::runtime_error("scheduler lost host-owned registration after Game heap retirement");
        scheduler.disconnect_name_obj(*object);
    }
    scheduler.clear();
    {
        MetadataActor first, second;
        second.mPosition.x = 1000;
        for (auto* actor : {&first, &second}) {
            actor->initHitSensor(1);
            (void)add_actor_hit_sensor(actor, "body", 1U, 1U, 10.0F, {});
            actor->makeActorAppeared();
            scheduler.connect_name_obj(*actor, 34, -1, -1, -1);
        }
        (void)MR::createSceneObj(SceneObj_MessageSensorHolder);
        auto invocation = JkrAllocationDomain::create(heaps, 64U << 10);
        {
            smgpc::runtime::SceneSchedulerAllocationBinding callback_heap(scheduler, invocation);
            JkrAllocationScope scope(invocation);
            const auto free_before = invocation->heap().getTotalFreeSize();
            for (unsigned i = 0; i < 32; ++i) {
                scheduler.execute_movement();
                MR::sendMsgToAllLiveActor(0, nullptr);
            }
            if (invocation->heap().getTotalFreeSize() != free_before)
                throw std::runtime_error("actor/sensor/message scratch metadata consumed the Game arena");
        }
        if (first.movements != 32 || second.movements != 32 || first.messages != 32 || second.messages != 32)
            throw std::runtime_error("actor callbacks were skipped by metadata scopes");
        scheduler.disconnect_name_obj(first);
        scheduler.disconnect_name_obj(second);
        invocation.reset();
        scheduler.clear();
    }
    {
        auto invocation = JkrAllocationDomain::create(heaps, 64U << 10);
        AllocatingNameObj object;
        scheduler.connect_name_obj(object, 34, 0, -1, -1);
        {
            smgpc::runtime::SceneSchedulerAllocationBinding callback_heap(scheduler, invocation);
            JkrAllocationScope scope(invocation);
            scheduler.execute_movement();
            scheduler.execute_calc_anim();
            if (JKRHeap::findFromRoot(object.movement_allocation) != &invocation->heap() ||
                JKRHeap::findFromRoot(object.animation_allocation) != &invocation->heap())
                throw std::runtime_error("scheduler metadata scope changed original callback allocation routing");
            delete[] object.movement_allocation;
            delete[] object.animation_allocation;
        }
        scheduler.disconnect_name_obj(object);
        invocation.reset();
        scheduler.clear();
    }
    std::cout << "NameObj registry and scheduler sorting/history/snapshots survive original heap retirement; "
                 "movement and animation callbacks retain original heap routing\n";
    execution.reset();
    execution_domain.reset();
    verify_explicit_scene_callbacks(heaps);
    verify_backend_allocation_routing(heaps);
    verify_category_execution(heaps);
}
