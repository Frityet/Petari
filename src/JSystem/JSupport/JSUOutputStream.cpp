#include "JSystem/JSupport/JSURandomOutputStream.hpp"

JSUOutputStream::~JSUOutputStream() = default;

s32 JSUOutputStream::write(const void *source, s32 length) {
    const s32 count = writeData(source, length);
    if (count != length) {
        setState(IO_ERROR);
    }
    return count;
}

s32 JSUOutputStream::skip(s32 amount, s8 fill) {
    s32 count = 0;
    for (; count < amount; ++count) {
        if (writeData(&fill, sizeof(fill)) != sizeof(fill)) {
            setState(IO_ERROR);
            break;
        }
    }
    return count;
}

s32 JSURandomOutputStream::seek(s32 offset, JSUStreamSeekFrom whence) {
    const s32 count = seekPos(offset, whence);
    clearState(IO_ERROR);
    return count;
}

s32 JSURandomOutputStream::getAvailable() const {
    return getLength() - getPosition();
}
