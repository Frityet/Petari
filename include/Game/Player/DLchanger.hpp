#pragma once

#include "Game/Util/MemoryUtil.hpp"
#include <revolution/os.h>
#include <JSystem/JKernel/JKRHeap.hpp>

class J3DModelX;

struct DLholder {
    u8* mDL;
    u16 mSize;
    u16 _6;
};

class DLchanger {
public:
    DLchanger(u32 size, u8 count) {
        mBuffers = new DLholder[count];
        mNumBuffers = count;
        mCurrentBuffer = 0;
        for (u32 i = 0; i < mNumBuffers; i++) {
            mBuffers[i].mDL = new (32) u8[size];
        }
    }

    void setDL(const void* data, u32 size) {
        DLholder* buffer = &mBuffers[mCurrentBuffer];
        buffer->mSize = size;
        u32 alignedSize = (size + 31) & ~31;
        MR::copyMemory(buffer->mDL, data, alignedSize);
        DCStoreRange(buffer->mDL, alignedSize);
    }

    void addDL(J3DModelX*);
    void swap();

    /* 0x0 */ DLholder* mBuffers;
    /* 0x4 */ u8 mNumBuffers;
    /* 0x5 */ u8 mCurrentBuffer;
};
