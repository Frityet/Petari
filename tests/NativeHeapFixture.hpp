#pragma once

#include "JSystem/JKernel/JKRExpHeap.hpp"
#include "JSystem/JKernel/JKRSolidHeap.hpp"
#include <aurora/allocation.hpp>
#include <aurora/guest_thread.hpp>
#include <aurora/mem2_arena.hpp>
#include <dolphin/ar.h>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>

namespace smgpc::test {
inline JKRHeap::Handle create_native_root_heap(std::size_t bytes) {
    const aurora::allocation::HostAllocationScope host;
    if (bytes > std::numeric_limits<s32>::max())
        throw std::invalid_argument("Fixture root exceeds the original heap size range");
    bytes &= ~std::size_t(31);
    void* memory = nullptr;
    if (posix_memalign(&memory, 32, bytes) != 0) throw std::bad_alloc();
    std::shared_ptr<void> storage(memory, std::free);
    return JKRExpHeap::createRoot(memory, static_cast<u32>(bytes), false)->adoptNativeOwnership(std::move(storage));
}

inline JKRHeap::Handle create_native_solid_heap(const JKRHeap::Handle& parent, std::size_t bytes) {
    const aurora::allocation::HostAllocationScope host;
    if (!parent || bytes > std::numeric_limits<s32>::max())
        throw std::invalid_argument("Fixture solid heap requires its actual parent and a bounded size");
    auto* heap = JKRSolidHeap::create(static_cast<u32>(bytes), parent.get(), false);
    if (!heap) throw std::bad_alloc();
    return heap->adoptNativeOwnership();
}

inline std::shared_ptr<void> create_native_mem2_storage(std::size_t bytes) {
    const aurora::allocation::HostAllocationScope host;
    void* memory = nullptr;
    if (posix_memalign(&memory, 32, bytes) != 0) throw std::bad_alloc();
    try { aurora::bind_mem2_arena(memory, bytes); }
    catch (...) { std::free(memory); throw; }
    return std::shared_ptr<void>(memory, [](void* allocation) {
        const aurora::os::GuestThreadExecutionScope execution;
        const aurora::allocation::HostAllocationScope host;
        ARReset();
        aurora::unbind_mem2_arena(allocation);
        std::free(allocation);
    });
}
}
