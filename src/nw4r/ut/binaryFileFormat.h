#pragma once

#include <revolution.h>
#include <aurora/endian.hpp>

namespace nw4r {
    namespace ut {
        typedef u32 SigWord;

        struct BinaryFileHeader {
            aurora::endian::BigEndian<SigWord> signature;
            aurora::endian::BigEndian<u16> byteOrder;
            aurora::endian::BigEndian<u16> version;
            aurora::endian::BigEndian<u32> fileSize;
            aurora::endian::BigEndian<u16> headerSize;
            aurora::endian::BigEndian<u16> dataBlocks;
        };

        struct BinaryBlockHeader {
            aurora::endian::BigEndian<SigWord> kind;
            aurora::endian::BigEndian<u32> size;
        };

        bool IsValidBinaryFile(const BinaryFileHeader *, u32, u16, u16);
    };
};

static_assert(sizeof(nw4r::ut::BinaryFileHeader) == 16);
static_assert(sizeof(nw4r::ut::BinaryBlockHeader) == 8);
