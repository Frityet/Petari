#include "JSystem/JSupport/JSURandomInputStream.hpp"

JSUInputStream::~JSUInputStream() = default;

s32 JSUInputStream::read(void *destination, s32 length) {
    const s32 count = readData(destination, length);
    if (count != length) {
        setState(IO_ERROR);
    }
    return count;
}

s32 JSUInputStream::skip(s32 amount) {
    s32 count = 0;
    for (; count < amount; ++count) {
        u8 value;
        if (readData(&value, sizeof(value)) != sizeof(value)) {
            setState(IO_ERROR);
            break;
        }
    }
    return count;
}

s32 JSURandomInputStream::skip(s32 amount) {
    const s32 count = seekPos(amount, SEEK_FROM_POSITION);
    if (count != amount) {
        setState(IO_ERROR);
    }
    return count;
}

s32 JSURandomInputStream::seek(s32 offset, JSUStreamSeekFrom whence) {
    const s32 count = seekPos(offset, whence);
    clearState(IO_ERROR);
    return count;
}
