#include "Game/System/BinaryDataChunkHolder.hpp"

#include <cstring>
#include <limits>
#include <stdexcept>

namespace {
constexpr auto cFileHeaderSize = u32{4U};
constexpr auto cChunkHeaderSize = u32{12U};

[[nodiscard]] u32 read_be32(const u8* pData) {
    return (static_cast<u32>(pData[0]) << 24U) | (static_cast<u32>(pData[1]) << 16U) |
           (static_cast<u32>(pData[2]) << 8U) | static_cast<u32>(pData[3]);
}

void write_be32(u8* pData, u32 value) {
    pData[0] = static_cast<u8>(value >> 24U);
    pData[1] = static_cast<u8>(value >> 16U);
    pData[2] = static_cast<u8>(value >> 8U);
    pData[3] = static_cast<u8>(value);
}
}  // namespace

BinaryDataChunkHolder::BinaryDataChunkHolder(u32 bufferSize, int maxChunks)
    : mChunks(nullptr), mMaxChunks(0), mNumChunks(0), mData(nullptr), mBufferSize(bufferSize) {
    if (bufferSize < cChunkHeaderSize || maxChunks < 0 || maxChunks > 0xff) {
        throw std::invalid_argument("Invalid retail binary chunk holder dimensions");
    }

    mData = new u8[mBufferSize]{};
    mChunks = new BinaryDataChunkBase*[static_cast<std::size_t>(maxChunks)]{};
    mMaxChunks = maxChunks;
}

void BinaryDataChunkHolder::addChunk(BinaryDataChunkBase* pChunk) {
    if (pChunk == nullptr || mNumChunks >= mMaxChunks) {
        throw std::length_error("Retail binary chunk table capacity exceeded");
    }
    mChunks[mNumChunks++] = pChunk;
}

u32 BinaryDataChunkHolder::makeFileBinary(u8* pData, u32 dataSize) {
    if (pData == nullptr || dataSize < cFileHeaderSize) {
        throw std::length_error("Retail binary chunk destination is unavailable or too small");
    }

    std::memset(pData, 0, cFileHeaderSize);
    pData[0] = 1U;
    pData[1] = static_cast<u8>(mNumChunks);

    auto offset = cFileHeaderSize;
    for (auto index = s32{}; index < mNumChunks; ++index) {
        std::memset(mData, 0, mBufferSize);
        makeChunkData(static_cast<BinaryDataChunkHolderChunkData*>(mData), mBufferSize, mChunks[index]);

        const auto chunkSize = read_be32(static_cast<const u8*>(mData) + 8U);
        if (chunkSize < cChunkHeaderSize || chunkSize > mBufferSize || chunkSize > dataSize - offset) {
            throw std::length_error("Retail binary chunk does not fit its destination");
        }
        std::memcpy(pData + offset, mData, chunkSize);
        offset += chunkSize;
    }
    return offset;
}

bool BinaryDataChunkHolder::loadFromFileBinary(const u8* pData, u32 dataSize) {
    if (pData == nullptr || dataSize < cFileHeaderSize || pData[0] != 1U) {
        return false;
    }

    auto offset = cFileHeaderSize;
    for (auto index = u32{}; index < pData[1]; ++index) {
        if (offset > dataSize || dataSize - offset < cChunkHeaderSize) {
            return false;
        }

        const auto signature = read_be32(pData + offset);
        const auto hash = read_be32(pData + offset + 4U);
        const auto chunkSize = read_be32(pData + offset + 8U);
        if (chunkSize < cChunkHeaderSize || chunkSize > dataSize - offset) {
            return false;
        }

        if (const auto* chunk = findFromSignature(signature); chunk != nullptr) {
            if (hash != chunk->makeHeaderHashCode() || chunkSize == cChunkHeaderSize) {
                return false;
            }
        }
        offset += chunkSize;
    }

    offset = cFileHeaderSize;
    for (auto index = u32{}; index < pData[1]; ++index) {
        const auto signature = read_be32(pData + offset);
        const auto chunkSize = read_be32(pData + offset + 8U);
        if (auto* chunk = findFromSignature(signature); chunk != nullptr) {
            const auto status = chunk->deserialize(pData + offset + cChunkHeaderSize, chunkSize - cChunkHeaderSize);
            if (status != 0 && status != 1) {
                return false;
            }
        }
        offset += chunkSize;
    }
    return true;
}

void BinaryDataChunkHolder::makeChunkData(BinaryDataChunkHolderChunkData* pData, u32 bufferSize,
                                          const BinaryDataChunkBase* pChunk) {
    if (pData == nullptr || pChunk == nullptr || bufferSize < cChunkHeaderSize) {
        throw std::length_error("Retail binary chunk buffer is unavailable or too small");
    }

    auto* bytes = reinterpret_cast<u8*>(pData);
    const auto serialized = pChunk->serialize(bytes + cChunkHeaderSize, bufferSize - cChunkHeaderSize);
    if (serialized < 0 || static_cast<u32>(serialized) > bufferSize - cChunkHeaderSize) {
        throw std::length_error("Retail binary chunk serializer exceeded its buffer");
    }

    write_be32(bytes, pChunk->getSignature());
    write_be32(bytes + 4U, pChunk->makeHeaderHashCode());
    write_be32(bytes + 8U, cChunkHeaderSize + static_cast<u32>(serialized));
}

BinaryDataChunkBase* BinaryDataChunkHolder::findFromSignature(u32 signature) const {
    for (auto index = s32{}; index < mNumChunks; ++index) {
        if (mChunks[index] != nullptr && mChunks[index]->getSignature() == signature) {
            return mChunks[index];
        }
    }
    return nullptr;
}

u32 BinaryDataChunkHolder::calcBinarySize(const u8* pData) {
    if (pData == nullptr || pData[0] != 1U) {
        return 0U;
    }

    auto size = cFileHeaderSize;
    for (auto index = u32{}; index < pData[1]; ++index) {
        const auto chunkSize = read_be32(pData + size + 8U);
        if (chunkSize < cChunkHeaderSize || chunkSize > std::numeric_limits<u32>::max() - size) {
            return 0U;
        }
        size += chunkSize;
    }
    return size;
}
