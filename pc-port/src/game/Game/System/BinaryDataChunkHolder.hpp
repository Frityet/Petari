#pragma once

#include "compat/Types.hpp"

class BinaryDataChunkBase {
public:
    virtual ~BinaryDataChunkBase() = default;
    virtual u32 makeHeaderHashCode() const = 0;
    virtual u32 getSignature() const = 0;
    virtual s32 serialize(u8 *, u32) const = 0;
    virtual s32 deserialize(const u8 *, u32) = 0;
    virtual void initializeData() = 0;
};

struct BinaryDataChunkHolderChunkData {
    u32 getDataOffset() const {
        return 0x0CU;
    }

    u8 *getData() const {
        return reinterpret_cast<u8 *>(const_cast<BinaryDataChunkHolderChunkData *>(this)) + getDataOffset();
    }

    /* 0x00 */ u32 mSignature;
    /* 0x04 */ u32 mHash;
    /* 0x08 */ u32 mChunkSize;
    /* 0x0C */ void *mData;
};

class BinaryDataChunkHolder {
public:
    BinaryDataChunkHolder(u32, int);
    ~BinaryDataChunkHolder();

    void addChunk(BinaryDataChunkBase *);
    u32 makeFileBinary(u8 *, u32);
    bool loadFromFileBinary(const u8 *, u32);
    static void makeChunkData(BinaryDataChunkHolderChunkData *, u32, const BinaryDataChunkBase *);
    BinaryDataChunkBase *findFromSignature(u32) const;
    static u32 calcBinarySize(const u8 *);

    /* 0x00 */ BinaryDataChunkBase **mChunks;
    /* 0x04 */ s32 mMaxChunks;
    /* 0x08 */ s32 mNumChunks;
    /* 0x0C */ void *mData;
    /* 0x10 */ u32 mBufferSize;
};
