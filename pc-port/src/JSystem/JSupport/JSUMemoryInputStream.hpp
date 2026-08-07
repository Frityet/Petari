#pragma once

#include <algorithm>
#include <cstring>

#include <revolution/types.h>

// Host implementation of the byte-oriented JSU memory stream used by the
// decompiled save chunks. Multi-byte file endianness remains the responsibility
// of the platform compatibility implementation for each serialized field.
class JSUMemoryInputStream {
public:
    JSUMemoryInputStream(const void* pBuffer, s32 size)
        : mBuffer(static_cast<const u8*>(pBuffer)), mLength(std::max(size, 0)), mPosition(0) {
    }

    s32 read(void* pDestination, s32 size) {
        if (pDestination == nullptr || size <= 0 || mBuffer == nullptr) {
            return 0;
        }

        const auto available = getAvailable();
        const auto copied = std::min(size, available);
        if (copied > 0) {
            std::memcpy(pDestination, mBuffer + mPosition, static_cast<std::size_t>(copied));
            mPosition += copied;
        }
        return copied;
    }

    [[nodiscard]] s32 getAvailable() const {
        return std::max(mLength - mPosition, 0);
    }

    const u8* mBuffer;
    s32 mLength;
    s32 mPosition;
};
