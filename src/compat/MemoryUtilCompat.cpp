#include <aurora/exception.hpp>
#include "Game/Util/MemoryUtil.hpp"

#include <cstring>
#include <stdexcept>

namespace MR {
    void* NewDeleteAllocator::alloc(MEMAllocator*, u32 size) {
        return new u8[size];
    }

    void NewDeleteAllocator::free(MEMAllocator*, void* pointer) {
        delete[] static_cast<u8*>(pointer);
    }

    MEMAllocatorFunc NewDeleteAllocator::sAllocatorFunc = {
        NewDeleteAllocator::alloc,
        NewDeleteAllocator::free,
    };
    MEMAllocator NewDeleteAllocator::sAllocator = {&sAllocatorFunc};

    u32 calcCheckSum(const void* pointer, u32 size) {
        const auto* bytes = static_cast<const u8*>(pointer);
        u16 sum = 0;
        u16 inverse_sum = 0;
        for (u32 offset = 0; offset < size / sizeof(u16); ++offset) {
            u16 value;
            std::memcpy(&value, bytes + offset * sizeof(u16), sizeof(value));
            sum += value;
            inverse_sum += static_cast<u16>(~value);
        }
        return (static_cast<u32>(sum) << 16) | inverse_sum;
    }

    void copyMemory(void* destination, const void* source, u32 size) {
        if (size == 0U) {
            return;
        }
        if (destination == nullptr || source == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Cannot copy unavailable memory.");
        }
        std::memcpy(destination, source, size);
    }

    void fillMemory(void* destination, u8 value, u32 size) {
        if (size == 0U) {
            return;
        }
        if (destination == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Cannot fill unavailable memory.");
        }
        std::memset(destination, value, size);
    }

    void zeroMemory(void* destination, u32 size) {
        fillMemory(destination, 0U, size);
    }
}  // namespace MR
