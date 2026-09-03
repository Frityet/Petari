#include "runtime/SceneScheduler.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <array>
#include <algorithm>

int main() {
    using namespace smgpc::compat;
    auto heaps = JkrHeapRuntime::create(1U << 20);
    auto domain = JkrAllocationDomain::create(heaps, 64U << 10);
    std::vector<NameObj*> snapshot;
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
            scheduler.connect_name_obj(*object, 34, -1, -1, -1);
        if (domain->heap().getTotalFreeSize() != before)
            throw std::runtime_error("native scheduler metadata consumed the original Game heap");
    }
    domain.reset();
    snapshot.clear();
    snapshot.shrink_to_fit();
    scheduler.execute_movement();
    for (auto& object : objects) {
        if (!scheduler.is_draw_connected(*object))
            throw std::runtime_error("scheduler lost host-owned registration after Game heap retirement");
        scheduler.disconnect_name_obj(*object);
    }
    scheduler.clear();
    std::cout << "NameObj registry and scene scheduler metadata survive original heap retirement\n";
}
