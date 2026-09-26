#include "OriginalStageResourceProcessFixture.hpp"
#include "runtime/RuntimeServices.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "resource/TplTexture.hpp"
#include <nw4r/ut/binaryFileFormat.h>
#include "resource/BmgMessageArchive.hpp"
#include "resource/RarcArchive.hpp"

#include <nw4r/ut/ResFont.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

template <typename Exception, typename Operation>
void require_throws(Operation&& operation, std::string_view message) {
    try {
        operation();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error(std::string(message));
}

void append_picture_tag(std::u16string& text, std::uint16_t payload) {
    text.push_back(static_cast<char16_t>(0x001aU));
    text.push_back(static_cast<char16_t>(0x0603U));
    text.push_back(static_cast<char16_t>(payload));
}

[[nodiscard]] std::uint16_t only_picture_code(
    const std::vector<smgpc::resource::BmgTextToken>& tokens) {
    require(tokens.size() == 1U &&
                tokens.front().role ==
                    smgpc::resource::BmgTextToken::Role::Picture &&
                tokens.front().text.size() == 1U,
            "a single type-3 tag must decode to one picture token");
    return static_cast<std::uint16_t>(tokens.front().text.front());
}

[[nodiscard]] std::size_t picture_glyph_rgb_count(const nw4r::ut::ResFont& font, const nw4r::ut::Glyph& glyph) {
    const auto sheet = smgpc::resource::decode_raw_gx_texture(
        {static_cast<const u8*>(glyph.pTexture), u32(font.mFontInfo->pGlyph->sheetSize)},
        glyph.texWidth, glyph.texHeight, static_cast<smgpc::resource::TplTextureFormat>(glyph.texFormat));
    const auto draw_width = glyph.widths.glyphWidth == 0 ? font.GetCellWidth() :
        std::min(font.GetCellWidth(), int(glyph.widths.glyphWidth));
    std::set<u32> colors;
    for (int y = 0; y < glyph.height; ++y) {
        for (int x = 0; x < draw_width; ++x) {
            const auto source_x = glyph.cellX + x, source_y = glyph.cellY + y;
            require(source_x < sheet.width && source_y < sheet.height, "PictureFont glyph remains inside its sheet");
            const size_t offset = (size_t(source_y) * sheet.width + source_x) * 4;
            if (sheet.rgba[offset + 3]) {
                colors.insert((u32(sheet.rgba[offset]) << 16) | (u32(sheet.rgba[offset + 1]) << 8) | sheet.rgba[offset + 2]);
            }
        }
    }
    return colors.size();
}

void test_ordered_token_formatting() {
    auto zero = std::u16string{};
    append_picture_tag(zero, 0U);
    auto one = std::u16string{};
    append_picture_tag(one, 1U);
    require(only_picture_code(smgpc::resource::format_bmg_tokens(zero, {})) ==
                    0x0030U &&
                only_picture_code(
                    smgpc::resource::format_bmg_tokens(one, {})) == 0x0031U,
            "raw type-3 payloads 0/1 must select PictureFont U+0030/U+0031");

    auto player = std::u16string{};
    append_picture_tag(player, 0x002bU);
    require_throws<std::logic_error>(
        [&] { (void)smgpc::resource::format_bmg_tokens(player, {}); },
        "a dynamic player picture must reject an unavailable identity");
    require(only_picture_code(smgpc::resource::format_bmg_tokens(
                player, {},
                smgpc::resource::BmgPlayerCharacter::Mario)) == 0x0042U &&
                only_picture_code(smgpc::resource::format_bmg_tokens(
                    player, {},
                    smgpc::resource::BmgPlayerCharacter::Luigi)) == 0x004cU,
            "payload 0x2B must rewrite to Mario 0x12 or Luigi 0x1C before +0x30");

    auto mixed = std::u16string(u"N=");
    mixed.append({static_cast<char16_t>(0x001aU),
                  static_cast<char16_t>(0x0e06U),
                  static_cast<char16_t>(0x0005U), 0U, 0U, 0U, 0U});
    append_picture_tag(mixed, 0U);
    mixed.append(u"S=");
    mixed.append({static_cast<char16_t>(0x001aU),
                  static_cast<char16_t>(0x0605U), 1U});
    const auto args = std::vector<smgpc::resource::BmgFormatArg>{
        smgpc::resource::BmgFormatArg::number(7),
        smgpc::resource::BmgFormatArg::string(u"OK"),
    };
    const auto tokens = smgpc::resource::format_bmg_tokens(mixed, args);
    require(tokens.size() == 3U &&
                tokens[0U].role ==
                    smgpc::resource::BmgTextToken::Role::Ordinary &&
                tokens[0U].text == u"N=07" &&
                tokens[1U].role ==
                    smgpc::resource::BmgTextToken::Role::Picture &&
                tokens[1U].text == u"0" &&
                tokens[2U].role ==
                    smgpc::resource::BmgTextToken::Role::Ordinary &&
                tokens[2U].text == u"S=OK",
            "number/string substitution must preserve order around picture tokens");

    const auto malformed = std::u16string{
        static_cast<char16_t>(0x001aU),
        static_cast<char16_t>(0x0603U),
    };
    require_throws<std::invalid_argument>(
        [&] { (void)smgpc::resource::format_bmg_tokens(malformed, {}); },
        "a truncated control sequence must fail instead of becoming ordinary text");
}

struct CorpusProof {
    std::size_t tag_count = 0U;
    std::size_t payload_count = 0U;
};

[[nodiscard]] CorpusProof test_retail_corpus(
    const smgpc::resource::RarcArchive& archive,
    const nw4r::ut::ResFont& picture_font) {
    const auto messages =
        smgpc::resource::BmgMessageArchive::from_message_archive(archive);
    auto tag_count = std::size_t{};
    auto payloads = std::set<std::uint16_t>{};
    for (const auto& message : messages.messages()) {
        auto expected_picture_count = std::size_t{};
        for (const auto& tag : message.control_tags) {
            if (tag.type != 3U) {
                continue;
            }
            ++tag_count;
            ++expected_picture_count;
            require(tag.size_bytes == 6U && tag.payload_words.size() == 1U,
                    "every authored RMGK01 type-3 tag must have the retail six-byte/one-payload shape");
            payloads.insert(tag.payload_words.front());
        }
        if (expected_picture_count == 0U) {
            continue;
        }

        const auto tokens = smgpc::resource::format_bmg_tokens(
            message.raw_text, {},
            smgpc::resource::BmgPlayerCharacter::Mario);
        auto actual_picture_count = std::size_t{};
        for (const auto& token : tokens) {
            if (token.role !=
                smgpc::resource::BmgTextToken::Role::Picture) {
                continue;
            }
            actual_picture_count += token.text.size();
            for (const auto code : token.text) {
                require(picture_font.HasGlyph(static_cast<std::uint16_t>(code)),
                        "every authored RMGK01 group-3 tag must resolve to an exact PictureFont glyph");
            }
        }
        require(actual_picture_count == expected_picture_count,
                "token formatting must preserve every authored group-3 tag");
    }

    require(tag_count == 409U && payloads.size() == 32U,
            "RMGK01 must retain its exact 409 group-3 tags and 32 authored payloads");
    return CorpusProof{
        .tag_count = tag_count,
        .payload_count = payloads.size(),
    };
}

void test_actual_picture_font(const nw4r::ut::ResFont& font) {
    const auto* begin = static_cast<const u8*>(font.mResource);
    const auto* header = reinterpret_cast<const nw4r::ut::BinaryFileHeader*>(begin);
    require(header->signature == 'RFNU', "GameSystemFontHolder uses the original relocated font resource");
    const auto* end = begin + header->fileSize;
    size_t checked = 0;
    for (u32 code = 0; code <= 0xffff; ++code) {
        if (!font.HasGlyph(static_cast<u16>(code))) continue;
        nw4r::ut::Glyph glyph{};
        font.GetGlyph(&glyph, static_cast<u16>(code));
        const auto* sheet = static_cast<const u8*>(glyph.pTexture);
        require(sheet >= begin && sheet < end && u32(font.mFontInfo->pGlyph->sheetSize) <= size_t(end - sheet) &&
                    glyph.cellX + font.GetCellWidth() <= glyph.texWidth && glyph.cellY + glyph.height <= glyph.texHeight &&
                    glyph.texFormat == GX_TF_RGB5A3,
                "every authored PictureFont glyph borrows a complete encoded sheet and a valid colored cell");
        ++checked;
    }
    nw4r::ut::Glyph icon{};
    font.GetGlyph(&icon, '0');
    require(checked > 32 && picture_glyph_rgb_count(font, icon) > 1,
            "the original PictureFont exposes its full glyph map and colored icon pixels");
    std::cout << "actual_picture_font_glyphs=" << checked << '\n';
}

}  // namespace

int main() {
    return smgpc::test::run_stage_resource_process("original-picture-font", [] {
        test_ordered_token_formatting();
        smgpc::runtime::DvdFileSystemService dvd{"/"};
        const auto messages = smgpc::resource::RarcArchive::from_bytes(dvd.read_file("/KrKorean/MessageData/Message.arc"));
        const auto* font = static_cast<nw4r::ut::ResFont*>(MR::getPictureFontNW4R());
        require(font && font->mResource, "the original GameSystem font owner published PictureFont");
        const auto corpus = test_retail_corpus(messages, *font);
        test_actual_picture_font(*font);
        std::cout << "picture_tags=" << corpus.tag_count << ";payloads=" << corpus.payload_count << '\n';
        std::cout << "Original PictureFont resources and message corpus passed\n";
    });
}
