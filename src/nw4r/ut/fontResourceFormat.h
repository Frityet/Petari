#pragma once

#include <revolution.h>
#include <aurora/endian.hpp>
#include <aurora/relocated_ptr.hpp>

namespace nw4r {
    namespace ut {
        struct CharWidths {
            s8 left;
            u8 glyphWidth;
            s8 charWidth;
        };

        struct FontTextureGlyph {
            u8 cellWidth;
            u8 cellHeight;
            s8 baselinePos;
            u8 maxCharWidth;
            aurora::endian::BigEndian<u32> sheetSize;
            aurora::endian::BigEndian<u16> sheetNum;
            aurora::endian::BigEndian<u16> sheetFormat;
            aurora::endian::BigEndian<u16> sheetRow;
            aurora::endian::BigEndian<u16> sheetLine;
            aurora::endian::BigEndian<u16> sheetWidth;
            aurora::endian::BigEndian<u16> sheetHeight;
            aurora::resource::RelocatedPtr32<u8> sheetImage;
        };

        struct FontWidth {
            aurora::endian::BigEndian<u16> indexBegin;
            aurora::endian::BigEndian<u16> indexEnd;
            aurora::resource::RelocatedPtr32<FontWidth> pNext;
            CharWidths widthTable[];
        };

        struct FontCodeMap {
            aurora::endian::BigEndian<u16> ccodeBegin;
            aurora::endian::BigEndian<u16> ccodeEnd;
            aurora::endian::BigEndian<u16> mappingMethod;
            aurora::endian::BigEndian<u16> reserved;
            aurora::resource::RelocatedPtr32<FontCodeMap> pNext;
            aurora::endian::BigEndian<u16> mapInfo[];
        };

        struct FontInformation {
            u8 fontType;
            s8 linefeed;
            aurora::endian::BigEndian<u16> alterCharIndex;
            CharWidths defaultWidth;
            u8 encoding;
            aurora::resource::RelocatedPtr32<FontTextureGlyph> pGlyph;
            aurora::resource::RelocatedPtr32<FontWidth> pWidth;
            aurora::resource::RelocatedPtr32<FontCodeMap> pMap;
            u8 height;
            u8 width;
            u8 ascent;
            u8 padding_[1];
        };
    };  // namespace ut
};  // namespace nw4r

static_assert(sizeof(nw4r::ut::FontInformation) == 24);
static_assert(sizeof(nw4r::ut::FontTextureGlyph) == 24);
static_assert(sizeof(nw4r::ut::FontWidth) == 8);
static_assert(sizeof(nw4r::ut::FontCodeMap) == 12);
