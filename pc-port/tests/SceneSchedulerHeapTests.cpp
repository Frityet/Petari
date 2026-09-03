#include "runtime/SceneScheduler.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
#include <iostream>

int main() {
    using namespace smgpc::compat;
    auto heaps = JkrHeapRuntime::create(1U << 20);
    auto domain = JkrAllocationDomain::create(heaps, 64U << 10);
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
    scheduler.execute_movement();
    for (auto& object : objects) {
        if (!scheduler.is_draw_connected(*object))
            throw std::runtime_error("scheduler lost host-owned registration after Game heap retirement");
        scheduler.disconnect_name_obj(*object);
    }
    scheduler.clear();
    std::cout << "Scene scheduler metadata survives original heap retirement\n";
}
