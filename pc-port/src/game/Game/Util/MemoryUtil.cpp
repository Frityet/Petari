#include "Game/Util/MemoryUtil.hpp"

#include <cstring>

namespace MR {

void copyMemory(void *pDst, const void *pSrc, u32 size) {
    if (size == 0U) {
        return;
    }
    std::memcpy(pDst, pSrc, size);
}

void fillMemory(void *pDst, u8 ch, u32 size) {
    if (size == 0U) {
        return;
    }
    std::memset(pDst, ch, size);
}

void zeroMemory(void *pDst, u32 size) {
    fillMemory(pDst, 0U, size);
}

u32 calcCheckSum(const void *pPtr, u32 size) {
    const auto *bytes = static_cast<const u8 *>(pPtr);
    u16 sum = 0;
    u16 invSum = 0;

    for (u32 offset = 0; offset + sizeof(u16) <= size; offset += sizeof(u16)) {
        const u16 value = static_cast<u16>((static_cast<u16>(bytes[offset]) << 8U) | bytes[offset + 1U]);
        sum = static_cast<u16>(sum + value);
        invSum = static_cast<u16>(invSum + static_cast<u16>(~value));
    }

    return (static_cast<u32>(sum) << 16U) | invSum;
}

}  // namespace MR
