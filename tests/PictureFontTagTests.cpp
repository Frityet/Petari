#include "layout/BrfntFont.hpp"
#include "layout/LayoutResourceResolver.hpp"
#include "layout/LayoutRuntime.hpp"
#include "resource/BmgMessageArchive.hpp"
#include "resource/RarcArchive.hpp"

#include <nw4r/ut/ResFont.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct RetailFixture {
    std::filesystem::path font_archive;
    std::filesystem::path message_archive;
    std::filesystem::path press_start_archive;
};

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

[[nodiscard]] std::optional<RetailFixture> find_retail_fixture() {
    for (auto root = std::filesystem::current_path(); !root.empty();
         root = root.parent_path()) {
        const std::filesystem::path candidates[]{
            root / "orig/RMGK01/files/KrKorean",
            root / "container/orig/RMGK01/files/KrKorean",
        };
        for (const auto& directory : candidates) {
            const auto fixture = RetailFixture{
                .font_archive = directory / "LayoutData/Font.arc",
                .message_archive = directory / "MessageData/Message.arc",
                .press_start_archive = directory / "LayoutData/PressStart.arc",
            };
            auto error = std::error_code{};
            if (std::filesystem::is_regular_file(fixture.font_archive, error) &&
                !error &&
                std::filesystem::is_regular_file(fixture.message_archive,
                                                 error) &&
                !error &&
                std::filesystem::is_regular_file(fixture.press_start_archive,
                                                 error) &&
                !error) {
                return fixture;
            }
        }
        if (root == root.root_path()) {
            break;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::string lower_basename(std::string_view path) {
    auto name = std::filesystem::path(path).filename().string();
    std::ranges::transform(name, name.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    if (!name.ends_with(".brfnt")) {
        name.append(".brfnt");
    }
    return name;
}

[[nodiscard]] const smgpc::resource::RarcEntry& require_font_entry(
    const smgpc::resource::RarcArchive& archive, std::string_view name) {
    auto requested_names = std::vector<std::string>{lower_basename(name)};
    const auto dot = requested_names.front().rfind(".brfnt");
    const auto stem = requested_names.front().substr(0U, dot);
    constexpr std::string_view locale_suffixes[]{
        "jpn", "eng", "fra", "ger", "ita", "spa", "kor"};
    for (const auto suffix : locale_suffixes) {
        if (stem.ends_with(suffix) && stem.size() > suffix.size()) {
            requested_names.push_back(
                stem.substr(0U, stem.size() - suffix.size()) + ".brfnt");
            break;
        }
    }
    const auto found = std::ranges::find_if(
        archive.entries(), [&requested_names](const auto& entry) {
            const auto loaded = lower_basename(entry.path);
            return std::ranges::find(requested_names, loaded) !=
                   requested_names.end();
        });
    if (found == archive.entries().end()) {
        throw std::runtime_error("retail Font.arc is missing " +
                                 std::string(name));
    }
    return *found;
}

[[nodiscard]] smgpc::layout::BrfntFont
load_font(const smgpc::resource::RarcArchive& archive,
          std::string_view name) {
    return smgpc::layout::parse_brfnt_font(
        archive.file_data(require_font_entry(archive, name)));
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

[[nodiscard]] std::size_t picture_glyph_rgb_count(
    const smgpc::layout::BrfntFont& font,
    const smgpc::layout::BrfntGlyph& glyph) {
    require(glyph.sheet_index < font.sheets.size(),
            "PictureFont glyph sheet index must be in range");
    const auto& sheet = font.sheets[glyph.sheet_index];
    const auto draw_width = glyph.widths.glyph_width == 0U
                                ? glyph.width
                                : std::min<std::uint8_t>(
                                      glyph.width,
                                      glyph.widths.glyph_width);
    auto colors = std::set<std::uint32_t>{};
    for (auto y = 0U; y < glyph.height; ++y) {
        for (auto x = 0U; x < draw_width; ++x) {
            const auto source_x = static_cast<std::uint16_t>(glyph.x + x);
            const auto source_y = static_cast<std::uint16_t>(glyph.y + y);
            require(source_x < sheet.width && source_y < sheet.height,
                    "PictureFont glyph cell must remain within its sheet");
            const auto offset =
                (static_cast<std::size_t>(source_y) * sheet.width + source_x) *
                4U;
            if (sheet.rgba[offset + 3U] == 0U) {
                continue;
            }
            colors.insert(
                (static_cast<std::uint32_t>(sheet.rgba[offset]) << 16U) |
                (static_cast<std::uint32_t>(sheet.rgba[offset + 1U]) << 8U) |
                static_cast<std::uint32_t>(sheet.rgba[offset + 2U]));
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
    const RetailFixture& fixture,
    const smgpc::layout::BrfntFont& picture_font) {
    const auto archive =
        smgpc::resource::RarcArchive::from_file(fixture.message_archive);
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
                require(picture_font.glyph_for_exact(
                            static_cast<std::uint16_t>(code))
                            .has_value(),
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

void test_actual_picture_font(const smgpc::resource::RarcArchive& archive,
                              const smgpc::layout::BrfntFont& decoded) {
    const auto bytes = archive.file_data(require_font_entry(archive, "PictureFont"));
    nw4r::ut::ResFont font;
    require(font.SetResource(const_cast<std::uint8_t*>(bytes.data()), bytes.size()),
            "actual SDK PictureFont installs the retained authored BRFNT bytes");
    size_t checked = 0;
    for (std::uint32_t code = 0; code <= 0xffff; ++code) {
        const auto expected = decoded.glyph_for_exact(static_cast<std::uint16_t>(code));
        if (!expected) continue;
        nw4r::ut::Glyph glyph;
        font.GetGlyph(&glyph, static_cast<std::uint16_t>(code));
        require(glyph.pTexture == bytes.data() + decoded.sheet_image_offset + expected->sheet_index * decoded.sheet_size &&
                glyph.cellX == expected->x && glyph.cellY == expected->y && glyph.height == expected->height &&
                glyph.widths.left == expected->widths.left && glyph.widths.glyphWidth == expected->widths.glyph_width &&
                glyph.widths.charWidth == expected->widths.char_width && glyph.texFormat == GX_TF_RGB5A3,
                "every actual PictureFont glyph retains exact encoded sheet identity, cell, width and color format");
        ++checked;
    }
    require(checked > 32 && picture_glyph_rgb_count(decoded, *decoded.glyph_for_exact('0')) > 1,
            "retail PictureFont contains its full glyph map and genuinely colored icon pixels");
}

}  // namespace

int main() {
    test_ordered_token_formatting();

    const auto fixture = find_retail_fixture();
    if (!fixture.has_value()) {
        std::cout << "[skip] extracted RMGK01 PictureFont/message/layout proof\n";
        std::cout << "[pending] Original picture-tag rendering and retail corpus require the actual resource fixture.\n";
        return 77;
    }

    const auto font_archive =
        smgpc::resource::RarcArchive::from_file(fixture->font_archive);
    const auto picture_font = load_font(font_archive, "PictureFont");
    const auto corpus = test_retail_corpus(*fixture, picture_font);
    test_actual_picture_font(font_archive, picture_font);
    std::cout << "picture_tags=" << corpus.tag_count << ";payloads=" << corpus.payload_count
              << ";actual_font_glyphs=verified\n";
    std::cout << "PictureFont resource checks passed: 3/3\n";
    std::cout << "[pending] Original mixed picture-tag rendering requires the actual GameSystem font owner; "
                 "the former CPU raster path has been removed.\n";
    return 77;
}
