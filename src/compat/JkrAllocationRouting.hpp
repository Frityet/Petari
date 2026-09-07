#pragma once
#include <cstddef>
class JKRHeap;

namespace smgpc::compat::detail {
    // Internal bridge used by the complete ordinary/aligned/JKR new overloads.
    // Returns nullptr on exhaustion; the operator supplies new-handler policy.
    void* allocate_jkr_or_host(std::size_t size, std::size_t alignment,
                               bool from_tail, JKRHeap* explicit_heap);
    void deallocate_jkr_or_host(void* memory) noexcept;
}
