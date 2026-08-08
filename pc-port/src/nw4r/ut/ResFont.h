#pragma once

#include <cstddef>

#include "nw4r/ut/Font.h"

namespace nw4r::ut {

    class ResFont final : public Font {
    public:
        ResFont();
        ~ResFont() override;

        bool SetResource(void* pBuffer);
        bool SetResource(void* pBuffer, std::size_t bufferSize);
        void RemoveResource();

        int GetWidth() const override;
        int GetHeight() const override;
        int GetAscent() const override;
        int GetDescent() const override;
        int GetBaselinePos() const override;
        int GetCellHeight() const override;
        int GetCellWidth() const override;
        int GetMaxCharWidth() const override;
        Type GetType() const override;
        GXTexFmt GetTextureFormat() const override;
        int GetLineFeed() const override;
        const CharWidths GetDefaultCharWidths() const override;
        void SetDefaultCharWidths(const CharWidths& rWidths) override;
        bool SetAlternateChar(u16 ch) override;
        void SetLineFeed(int lineFeed) override;
        int GetCharWidth(u16 ch) const override;
        const CharWidths GetCharWidths(u16 ch) const override;
        void GetGlyph(Glyph* pGlyph, u16 ch) const override;
        bool HasGlyph(CharCode ch) const override;
        FontEncoding GetEncoding() const override;

        [[nodiscard]] bool IsManaging(const void* pBuffer) const noexcept;
    };

}  // namespace nw4r::ut
