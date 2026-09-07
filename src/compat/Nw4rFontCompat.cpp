#include "nw4r/ut/ResFont.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

#include "layout/BrfntFont.hpp"

namespace {

    constexpr auto kMinimumBrfntHeaderSize = std::size_t{16U};
    constexpr auto kMaximumBrfntFileSize = std::size_t{64U * 1024U * 1024U};

    [[nodiscard]] std::uint16_t read_be16(const std::uint8_t* data) {
        return static_cast< std::uint16_t >((static_cast< std::uint16_t >(data[0U]) << 8U) | data[1U]);
    }

    [[nodiscard]] std::uint32_t read_be32(const std::uint8_t* data) {
        return (static_cast< std::uint32_t >(data[0U]) << 24U) | (static_cast< std::uint32_t >(data[1U]) << 16U) |
               (static_cast< std::uint32_t >(data[2U]) << 8U) | static_cast< std::uint32_t >(data[3U]);
    }

    [[nodiscard]] std::size_t declared_brfnt_size(const void* buffer) {
        if (buffer == nullptr) {
            return 0U;
        }

        const auto* bytes = static_cast< const std::uint8_t* >(buffer);
        const auto version = read_be16(bytes + 6U);
        if (bytes[0U] != 'R' || bytes[1U] != 'F' || bytes[2U] != 'N' || bytes[3U] != 'T' ||
            read_be16(bytes + 4U) != 0xfeffU || (version != 0x0102U && version != 0x0104U) ||
            read_be16(bytes + 12U) != kMinimumBrfntHeaderSize) {
            return 0U;
        }

        const auto size = static_cast< std::size_t >(read_be32(bytes + 8U));
        return size >= kMinimumBrfntHeaderSize && size <= kMaximumBrfntFileSize ? size : 0U;
    }

    [[nodiscard]] smgpc::layout::BrfntFont& require_font(nw4r::ut::HostFontResourceState& state,
                                                          std::string_view operation) {
        if (state.font == nullptr) {
            throw std::logic_error(std::string(operation) + " requires an installed BRFNT resource");
        }
        return *state.font;
    }

    [[nodiscard]] const smgpc::layout::BrfntFont& require_font(const nw4r::ut::HostFontResourceState& state,
                                                                std::string_view operation) {
        if (state.font == nullptr) {
            throw std::logic_error(std::string(operation) + " requires an installed BRFNT resource");
        }
        return *state.font;
    }

    [[nodiscard]] nw4r::ut::CharWidths to_nw4r_widths(const smgpc::layout::BrfntCharWidths& widths) {
        return nw4r::ut::CharWidths{
            .left = widths.left,
            .glyphWidth = widths.glyph_width,
            .charWidth = widths.char_width,
        };
    }

    [[nodiscard]] smgpc::layout::BrfntCharWidths to_host_widths(const nw4r::ut::CharWidths& widths) {
        return smgpc::layout::BrfntCharWidths{
            .left = widths.left,
            .glyph_width = widths.glyphWidth,
            .char_width = widths.charWidth,
        };
    }

    [[nodiscard]] bool has_only_supported_blocks(const smgpc::layout::BrfntFont& font) {
        constexpr auto supported = std::array{std::string_view("FINF"), std::string_view("TGLP"),
                                               std::string_view("CWDH"), std::string_view("CMAP"),
                                               std::string_view("GLGR")};
        return std::ranges::all_of(font.blocks, [&supported](const auto& block) {
            return std::ranges::find(supported, block.magic) != supported.end();
        });
    }

}  // namespace

namespace nw4r::ut {

    Font::Font() : mHostResourceState(std::make_shared< HostFontResourceState >()) {
    }

    Font::~Font() = default;

    std::weak_ptr< const HostFontResourceState > Font::GetHostResourceState() const noexcept {
        return mHostResourceState;
    }

    ResFont::ResFont() = default;

    ResFont::~ResFont() = default;

    bool ResFont::SetResource(void* pBuffer) {
        const auto declared_size = declared_brfnt_size(pBuffer);
        return declared_size != 0U && SetResource(pBuffer, declared_size);
    }

    bool ResFont::SetResource(void* pBuffer, std::size_t bufferSize) {
        if (!IsManaging(nullptr) || pBuffer == nullptr || bufferSize < kMinimumBrfntHeaderSize) {
            return false;
        }

        const auto declared_size = declared_brfnt_size(pBuffer);
        if (declared_size == 0U || declared_size > bufferSize) {
            return false;
        }

        try {
            const auto bytes = std::span< const std::uint8_t >(static_cast< const std::uint8_t* >(pBuffer), declared_size);
            auto parsed = std::make_shared< smgpc::layout::BrfntFont >(smgpc::layout::parse_brfnt_font(bytes));
            if (!has_only_supported_blocks(*parsed)) {
                return false;
            }
            mHostResourceState->font = std::move(parsed);
            mHostResourceState->source = pBuffer;
            mHostResourceState->source_size = declared_size;
            ++mHostResourceState->generation;
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    void ResFont::RemoveResource() {
        mHostResourceState->font.reset();
        mHostResourceState->source = nullptr;
        mHostResourceState->source_size = 0U;
        ++mHostResourceState->generation;
    }

    int ResFont::GetWidth() const {
        return require_font(*mHostResourceState, "Reading font width").width;
    }

    int ResFont::GetHeight() const {
        return require_font(*mHostResourceState, "Reading font height").height;
    }

    int ResFont::GetAscent() const {
        return require_font(*mHostResourceState, "Reading font ascent").ascent;
    }

    int ResFont::GetDescent() const {
        const auto& font = require_font(*mHostResourceState, "Reading font descent");
        return static_cast< int >(font.height) - static_cast< int >(font.ascent);
    }

    int ResFont::GetBaselinePos() const {
        return require_font(*mHostResourceState, "Reading font baseline").baseline_position;
    }

    int ResFont::GetCellHeight() const {
        return require_font(*mHostResourceState, "Reading font cell height").cell_height;
    }

    int ResFont::GetCellWidth() const {
        return require_font(*mHostResourceState, "Reading font cell width").cell_width;
    }

    int ResFont::GetMaxCharWidth() const {
        return require_font(*mHostResourceState, "Reading maximum character width").max_char_width;
    }

    Font::Type ResFont::GetType() const {
        return TYPE_RESOURCE;
    }

    GXTexFmt ResFont::GetTextureFormat() const {
        return static_cast< GXTexFmt >(require_font(*mHostResourceState, "Reading font texture format").sheet_format);
    }

    int ResFont::GetLineFeed() const {
        return require_font(*mHostResourceState, "Reading font line feed").line_feed;
    }

    const CharWidths ResFont::GetDefaultCharWidths() const {
        return to_nw4r_widths(require_font(*mHostResourceState, "Reading default character widths").default_width);
    }

    void ResFont::SetDefaultCharWidths(const CharWidths& rWidths) {
        require_font(*mHostResourceState, "Setting default character widths").default_width = to_host_widths(rWidths);
        ++mHostResourceState->generation;
    }

    bool ResFont::SetAlternateChar(u16 ch) {
        if (mHostResourceState->font == nullptr) {
            return false;
        }
        const auto glyph_index = mHostResourceState->font->find_glyph_index_exact(ch);
        if (!glyph_index.has_value()) {
            return false;
        }
        mHostResourceState->font->alternate_char_index = *glyph_index;
        ++mHostResourceState->generation;
        return true;
    }

    void ResFont::SetLineFeed(int lineFeed) {
        require_font(*mHostResourceState, "Setting font line feed").line_feed = static_cast< s8 >(lineFeed);
        ++mHostResourceState->generation;
    }

    int ResFont::GetCharWidth(u16 ch) const {
        return require_font(*mHostResourceState, "Reading character width").char_width(ch);
    }

    const CharWidths ResFont::GetCharWidths(u16 ch) const {
        return to_nw4r_widths(require_font(*mHostResourceState, "Reading character widths").char_widths(ch));
    }

    void ResFont::GetGlyph(Glyph* pGlyph, u16 ch) const {
        if (pGlyph == nullptr) {
            throw std::invalid_argument("Reading a font glyph requires output storage");
        }

        const auto& font = require_font(*mHostResourceState, "Reading a font glyph");
        const auto glyph = font.glyph_for_resfont(ch);
        if (!glyph.has_value() || mHostResourceState->source == nullptr || glyph->sheet_index >= font.sheet_count) {
            throw std::runtime_error("The installed BRFNT does not contain usable glyph sheet data");
        }

        const auto sheet_offset = static_cast< std::size_t >(font.sheet_image_offset) +
                                  static_cast< std::size_t >(glyph->sheet_index) * font.sheet_size;
        if (sheet_offset >= mHostResourceState->source_size ||
            font.sheet_size > mHostResourceState->source_size - sheet_offset) {
            throw std::runtime_error("The installed BRFNT glyph sheet is outside its resource buffer");
        }

        auto* sheet_data = static_cast< const std::uint8_t* >(mHostResourceState->source) + sheet_offset;

        *pGlyph = Glyph{
            .pTexture = const_cast< std::uint8_t* >(sheet_data),
            .widths = to_nw4r_widths(glyph->widths),
            .height = glyph->height,
            .texFormat = static_cast< GXTexFmt >(font.sheet_format),
            .texWidth = font.sheet_width,
            .texHeight = font.sheet_height,
            .cellX = glyph->x,
            .cellY = glyph->y,
        };
    }

    bool ResFont::HasGlyph(CharCode ch) const {
        return mHostResourceState->font != nullptr && mHostResourceState->font->has_glyph(ch);
    }

    FontEncoding ResFont::GetEncoding() const {
        return static_cast< FontEncoding >(require_font(*mHostResourceState, "Reading font encoding").encoding);
    }

    bool ResFont::IsManaging(const void* pBuffer) const noexcept {
        return mHostResourceState->source == pBuffer;
    }

}  // namespace nw4r::ut
