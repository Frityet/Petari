#pragma once

#include "compat/Types.hpp"

class BinaryDataContentHeaderSerializer {
public:
    BinaryDataContentHeaderSerializer(u8 *, u32);

    void addAttribute(const char *, u32);
    void flush();
    u32 getHeaderSize() const;
    u32 getDataSize() const;
    void *getBuffer() const;

private:
    /* 0x00 */ u8 *mBuffer;
    /* 0x04 */ u32 mCapacity;
    /* 0x08 */ u32 mPosition;
    /* 0x0C */ u32 mAttributeNum;
    /* 0x10 */ u32 mDataSize;
};

class BinaryDataContentAccessor {
public:
    explicit BinaryDataContentAccessor(u8 *);

    s32 getHeaderSize() const;
    s32 getDataSize() const;
    s32 getAttributeNum() const;
    void *getPointer(const char *, u8 *) const;

    /* 0x00 */ u8 *mData;
};
