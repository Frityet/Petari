#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include <revolution.h>

namespace smgpc::layout {
    struct BrfntFont;
}

namespace nw4r::ut {

    using CharCode = u16;

    enum FontEncoding {
        FONT_ENCODING_UTF8,
        FONT_ENCODING_UTF16,
        FONT_ENCODING_SJIS,
        FONT_ENCODING_CP1252,

        FONT_ENCODING_MAX,
    };

    struct CharWidths {
        s8 left = 0;
        u8 glyphWidth = 0U;
        s8 charWidth = 0;
    };

    struct Glyph {
        void* pTexture = nullptr;
        CharWidths widths{};
        u8 height = 0U;
        GXTexFmt texFormat = GX_TF_I4;
        u16 texWidth = 0U;
        u16 texHeight = 0U;
        u16 cellX = 0U;
        u16 cellY = 0U;
    };

    // Host-owned state lets layout bindings follow SetResource/RemoveResource while
    // retaining the retail Font pointer lifetime semantics through a weak reference.
    struct HostFontResourceState {
        std::shared_ptr< smgpc::layout::BrfntFont > font{};
        const void* source = nullptr;
        std::size_t source_size = 0U;
        std::uint64_t generation = 0U;
    };

    class Font {
    public:
        enum Type {
            TYPE_NULL,
            TYPE_ROM,
            TYPE_RESOURCE,
            TYPE_PAIR,
        };

        Font();
        virtual ~Font();

        virtual int GetWidth() const = 0;
        virtual int GetHeight() const = 0;
        virtual int GetAscent() const = 0;
        virtual int GetDescent() const = 0;
        virtual int GetBaselinePos() const = 0;
        virtual int GetCellHeight() const = 0;
        virtual int GetCellWidth() const = 0;
        virtual int GetMaxCharWidth() const = 0;
        virtual Type GetType() const = 0;
        virtual GXTexFmt GetTextureFormat() const = 0;
        virtual int GetLineFeed() const = 0;
        virtual const CharWidths GetDefaultCharWidths() const = 0;
        virtual void SetDefaultCharWidths(const CharWidths& rWidths) = 0;
        virtual bool SetAlternateChar(u16 ch) = 0;
        virtual void SetLineFeed(int lineFeed) = 0;
        virtual int GetCharWidth(u16 ch) const = 0;
        virtual const CharWidths GetCharWidths(u16 ch) const = 0;
        virtual void GetGlyph(Glyph* pGlyph, u16 ch) const = 0;
        virtual bool HasGlyph(CharCode ch) const = 0;
        virtual FontEncoding GetEncoding() const = 0;

        [[nodiscard]] std::weak_ptr< const HostFontResourceState > GetHostResourceState() const noexcept;

    protected:
        std::shared_ptr< HostFontResourceState > mHostResourceState;
    };

}  // namespace nw4r::ut
