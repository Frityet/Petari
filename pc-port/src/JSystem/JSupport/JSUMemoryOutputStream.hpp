#pragma once

#include <algorithm>
#include <cstring>

#include <revolution/types.h>

// See JSUMemoryInputStream.hpp. This stream deliberately copies bytes only;
// PPC scalar byte order is handled at the serialization compatibility boundary.
class JSUMemoryOutputStream {
public:
    JSUMemoryOutputStream(void* pBuffer, s32 size)
        : mBuffer(static_cast<u8*>(pBuffer)), mLength(std::max(size, 0)), mPosition(0) {
    }

    s32 write(const void* pSource, s32 size) {
        if (pSource == nullptr || size <= 0 || mBuffer == nullptr) {
            return 0;
        }

        const auto available = std::max(mLength - mPosition, 0);
        const auto copied = std::min(size, available);
        if (copied > 0) {
            std::memcpy(mBuffer + mPosition, pSource, static_cast<std::size_t>(copied));
            mPosition += copied;
        }
        return copied;
    }

    u8* mBuffer;
    s32 mLength;
    s32 mPosition;
};
