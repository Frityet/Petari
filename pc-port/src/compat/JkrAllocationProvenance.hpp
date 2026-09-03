#pragma once
class JKRHeap;

namespace smgpc::compat::detail {
    // Native Base-heap boundary hooks. Records use malloc, never global new,
    // and do not own the heap: typed native resource owners retain its domain.
    void record_jkr_allocation(void* memory, JKRHeap* heap, int alignment);
    JKRHeap* forget_jkr_allocation(void* memory) noexcept;
    void retire_jkr_heap_allocations(JKRHeap* heap, bool tail_only = false) noexcept;
}
