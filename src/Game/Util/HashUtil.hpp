#pragma once

#include <revolution/types.h>
#include <stdint.h>

class HashSortTable {
public:
    typedef uintptr_t Value;

    HashSortTable(u32);

    bool add(const char*, Value, bool);
    bool add(u32, Value);
    bool addOrSkip(u32, Value);
    void sort();
    bool search(u32, Value*);
    bool search(const char*, Value*);
    bool search(const char*, const char*, Value*);
    void swap(const char*, const char*);

    /* 0x00 */ bool mHasBeenSorted;
    /* 0x04 */ u32* mHashCodes;
    /* 0x08 */ Value* _8;
    /* 0x0C */ u16* _C;
    /* 0x10 */ u16* _10;
    /* 0x14 */ u32 mCurrentLength;
    /* 0x18 */ u32 mMaxLength;
};

namespace MR {
    u32 getHashCode(const char*);
    u32 getHashCodeLower(const char*);
};  // namespace MR
