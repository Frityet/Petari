#include "Game/Util/MemoryUtil.hpp"

#include <cstring>
#include <stdexcept>

namespace MR {
    void copyMemory(void* destination, const void* source, u32 size) {
        if (size == 0U) {
            return;
        }
        if (destination == nullptr || source == nullptr) {
            throw std::logic_error("Cannot copy unavailable memory.");
        }
        std::memcpy(destination, source, size);
    }

    void fillMemory(void* destination, u8 value, u32 size) {
        if (size == 0U) {
            return;
        }
        if (destination == nullptr) {
            throw std::logic_error("Cannot fill unavailable memory.");
        }
        std::memset(destination, value, size);
    }

    void zeroMemory(void* destination, u32 size) {
        fillMemory(destination, 0U, size);
    }
}  // namespace MR
