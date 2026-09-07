#include "compat/JkrAllocationRouting.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>

namespace {
    constexpr std::size_t default_new_alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
    void* allocate(std::size_t size, std::size_t alignment, bool from_tail = false, JKRHeap* heap = nullptr) {
        for (;;) {
            if (void* result = smgpc::compat::detail::allocate_jkr_or_host(size, alignment, from_tail, heap)) return result;
            auto handler = std::get_new_handler();
            if (handler == nullptr) throw std::bad_alloc();
            handler();
        }
    }

    void* allocate_jkr(std::size_t size, JKRHeap* heap, int alignment) {
        if (alignment == std::numeric_limits<int>::min()) throw std::bad_alloc();
        auto magnitude = static_cast<std::size_t>(alignment < 0 ? -alignment : alignment);
        if (magnitude == 0) magnitude = default_new_alignment;
        return allocate(size, magnitude, alignment < 0, heap);
    }

    void release(void* memory) noexcept { smgpc::compat::detail::deallocate_jkr_or_host(memory); }
}

void* operator new(std::size_t size) { return allocate(size, default_new_alignment); }
void* operator new[](std::size_t size) { return allocate(size, default_new_alignment); }
void* operator new(std::size_t size, std::align_val_t alignment) { return allocate(size, static_cast<std::size_t>(alignment)); }
void* operator new[](std::size_t size, std::align_val_t alignment) { return allocate(size, static_cast<std::size_t>(alignment)); }
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    try { return ::operator new(size); } catch (...) { return nullptr; }
}
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    try { return ::operator new[](size); } catch (...) { return nullptr; }
}
void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    try { return ::operator new(size, alignment); } catch (...) { return nullptr; }
}
void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    try { return ::operator new[](size, alignment); } catch (...) { return nullptr; }
}

void operator delete(void* memory) noexcept { release(memory); }
void operator delete[](void* memory) noexcept { release(memory); }
void operator delete(void* memory, std::size_t) noexcept { release(memory); }
void operator delete[](void* memory, std::size_t) noexcept { release(memory); }
void operator delete(void* memory, std::align_val_t) noexcept { release(memory); }
void operator delete[](void* memory, std::align_val_t) noexcept { release(memory); }
void operator delete(void* memory, std::size_t, std::align_val_t) noexcept { release(memory); }
void operator delete[](void* memory, std::size_t, std::align_val_t) noexcept { release(memory); }
void operator delete(void* memory, const std::nothrow_t&) noexcept { release(memory); }
void operator delete[](void* memory, const std::nothrow_t&) noexcept { release(memory); }
void operator delete(void* memory, std::align_val_t, const std::nothrow_t&) noexcept { release(memory); }
void operator delete[](void* memory, std::align_val_t, const std::nothrow_t&) noexcept { release(memory); }

void* operator new(std::size_t size, int alignment) { return allocate_jkr(size, nullptr, alignment); }
void* operator new[](std::size_t size, int alignment) { return allocate_jkr(size, nullptr, alignment); }
void* operator new(std::size_t size, JKRHeap* heap) { return allocate_jkr(size, heap, 4); }
void* operator new[](std::size_t size, JKRHeap* heap) { return allocate_jkr(size, heap, 4); }
void* operator new(std::size_t size, JKRHeap* heap, int alignment) { return allocate_jkr(size, heap, alignment); }
void* operator new[](std::size_t size, JKRHeap* heap, int alignment) { return allocate_jkr(size, heap, alignment); }
void operator delete(void* memory, int) noexcept { release(memory); }
void operator delete[](void* memory, int) noexcept { release(memory); }
void operator delete(void* memory, JKRHeap*) noexcept { release(memory); }
void operator delete[](void* memory, JKRHeap*) noexcept { release(memory); }
void operator delete(void* memory, JKRHeap*, int) noexcept { release(memory); }
void operator delete[](void* memory, JKRHeap*, int) noexcept { release(memory); }
