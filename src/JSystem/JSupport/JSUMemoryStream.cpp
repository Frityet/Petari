#include "JSystem/JSupport/JSUMemoryInputStream.hpp"
#include "JSystem/JSupport/JSUMemoryOutputStream.hpp"
#include <algorithm>
#include <cstring>

namespace {
    s32 clamped_seek_position(s32 position, s32 length, s32 offset, JSUStreamSeekFrom whence) {
        // Widen positions before arithmetic; stream sizes remain the SDK's s32.
        s64 target = position;
        switch (whence) {
        case SEEK_FROM_START:
            target = offset;
            break;
        case SEEK_FROM_POSITION:
            target += offset;
            break;
        case SEEK_FROM_END:
            target = static_cast<s64>(length) - offset;
            break;
        }
        return static_cast<s32>(std::clamp<s64>(target, 0, length));
    }
}  // namespace

void JSUMemoryInputStream::setBuffer(const void *buffer, s32 length) {
    mBuffer = buffer;
    mLength = std::max(length, 0);
    mPosition = 0;
}

u32 JSUMemoryInputStream::readData(void *destination, s32 length) {
    // Invalid native spans transfer nothing. The public read() sets IO_ERROR
    // on a short transfer, while direct readData() retains the SDK state policy.
    if (length <= 0 || destination == nullptr || mBuffer == nullptr) {
        return 0;
    }
    const s32 count = std::min(length, mLength - mPosition);
    if (count > 0) {
        std::memcpy(destination, static_cast<const u8 *>(mBuffer) + mPosition, static_cast<std::size_t>(count));
        mPosition += count;
    }
    return count;
}

s32 JSUMemoryInputStream::getLength() const {
    return mLength;
}

s32 JSUMemoryInputStream::getPosition() const {
    return mPosition;
}

s32 JSUMemoryInputStream::seekPos(s32 offset, JSUStreamSeekFrom whence) {
    const s32 oldPosition = mPosition;
    mPosition = clamped_seek_position(mPosition, mLength, offset, whence);
    return mPosition - oldPosition;
}

void JSUMemoryOutputStream::setBuffer(void *buffer, s32 length) {
    mBuffer = buffer;
    mLength = std::max(length, 0);
    mPosition = 0;
}

s32 JSUMemoryOutputStream::writeData(const void *source, s32 length) {
    if (length <= 0 || source == nullptr || mBuffer == nullptr) {
        return 0;
    }
    const s32 count = std::min(length, mLength - mPosition);
    if (count > 0) {
        std::memcpy(static_cast<u8 *>(mBuffer) + mPosition, source, static_cast<std::size_t>(count));
        mPosition += count;
    }
    return count;
}

s32 JSUMemoryOutputStream::getLength() const {
    return mLength;
}

s32 JSUMemoryOutputStream::seekPos(s32 offset, JSUStreamSeekFrom whence) {
    const s32 oldPosition = mPosition;
    mPosition = clamped_seek_position(mPosition, mLength, offset, whence);
    return mPosition - oldPosition;
}
