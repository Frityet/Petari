#include "runtime/SceneScheduler.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <array>
#include <algorithm>
#include <cstring>

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
    auto heaps = JkrHeapRuntime::create(1U << 20);
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
    std::vector<std::unique_ptr<NameObj>> objects;
    for (int i = 0; i < 128; ++i)
        objects.push_back(std::make_unique<NameObj>("heap registration fixture"));
    const auto before = domain->heap().getTotalFreeSize();
    {
        JkrAllocationScope scope(domain);
        SceneObjHolder holder;
        smgpc::scene::SceneObjHolderBinding binding(holder);
        for (auto& object : objects)
            scheduler.connect_name_obj(*object, 34, 0, -1, -1);
        scheduler.execute_movement();
        scheduler.execute_calc_anim();
        scheduler_snapshot = scheduler.snapshot();
        const auto trace = scheduler.last_execution_trace();
        if (trace.size() != objects.size() * 2 || scheduler_snapshot.size() != objects.size() ||
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
        if (state.name != "heap registration fixture")
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
        SceneObjHolder holder;
        smgpc::scene::SceneObjHolderBinding scene(holder);
        (void)MR::createSceneObj(SceneObj_MessageSensorHolder);
        auto invocation = JkrAllocationDomain::create(heaps, 64U << 10);
        {
            JkrAllocationScope scope(invocation);
            const auto free_before = invocation->heap().getTotalFreeSize();
            for (unsigned i = 0; i < 32; ++i) {
                scheduler.execute_movement();
                if (scheduler.send_message_to_live_actors(0, nullptr) != 2)
                    throw std::runtime_error("metadata fixture message did not reach both real actors");
            }
            if (invocation->heap().getTotalFreeSize() != free_before)
                throw std::runtime_error("actor/sensor/message scratch metadata consumed the Game arena");
        }
        if (first.movements != 32 || second.movements != 32 || first.messages != 32 || second.messages != 32)
            throw std::runtime_error("actor callbacks were skipped by metadata scopes");
        scheduler.disconnect_name_obj(first);
        scheduler.disconnect_name_obj(second);
        invocation.reset();
        if (scheduler.message_trace().size() != 64)
            throw std::runtime_error("message trace did not survive scene arena retirement");
        for (const auto& state : scheduler.message_trace())
            if (state.target_name != "live actor metadata heap lifetime")
                throw std::runtime_error("message target name changed after scene arena retirement");
        scheduler.clear();
    }
    {
        auto invocation = JkrAllocationDomain::create(heaps, 64U << 10);
        AllocatingNameObj object;
        scheduler.connect_name_obj(object, 34, 0, -1, -1);
        {
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
}
