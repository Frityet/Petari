#include <cstddef>
#include <cstdlib>
#include <new>

class JKRHeap;

namespace {
    [[nodiscard]] void* allocate_aligned(std::size_t size, int alignment) {
        if (alignment == 0) {
            return ::operator new(size);
        }
        if (alignment < 0) {
            throw std::bad_alloc{};
        }

        const auto host_alignment = static_cast<std::size_t>(alignment);
        if ((host_alignment & (host_alignment - 1U)) != 0U ||
            host_alignment < alignof(void*)) {
            throw std::bad_alloc{};
        }

        void* memory = nullptr;
        if (posix_memalign(&memory, host_alignment, size == 0U ? 1U : size) != 0 ||
            memory == nullptr) {
            throw std::bad_alloc{};
        }
        return memory;
    }
}  // namespace

void* operator new(std::size_t size, int alignment) {
    return allocate_aligned(size, alignment);
}

void* operator new[](std::size_t size, int alignment) {
    return alignment == 0 ? ::operator new[](size) : allocate_aligned(size, alignment);
}

void* operator new(std::size_t size, JKRHeap*, int alignment) {
    return allocate_aligned(size, alignment);
}

void* operator new[](std::size_t size, JKRHeap*, int alignment) {
    return alignment == 0 ? ::operator new[](size) : allocate_aligned(size, alignment);
}

void operator delete(void* memory, int alignment) noexcept {
    if (alignment == 0) {
        ::operator delete(memory);
    } else {
        std::free(memory);
    }
}

void operator delete[](void* memory, int alignment) noexcept {
    if (alignment == 0) {
        ::operator delete[](memory);
    } else {
        std::free(memory);
    }
}

void operator delete(void* memory, JKRHeap*, int alignment) noexcept {
    if (alignment == 0) {
        ::operator delete(memory);
    } else {
        std::free(memory);
    }
}

void operator delete[](void* memory, JKRHeap*, int alignment) noexcept {
    if (alignment == 0) {
        ::operator delete[](memory);
    } else {
        std::free(memory);
    }
}
