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
            root / "pc-port/container/orig/RMGK01/files/KrKorean",
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

[[nodiscard]] std::size_t glyph_advance(
    const smgpc::layout::BrfntGlyph& glyph) {
    return glyph.widths.char_width == 0
               ? glyph.width
               : static_cast<std::size_t>(glyph.widths.char_width);
}

[[nodiscard]] std::uint16_t texture_extent(float value) {
    return static_cast<std::uint16_t>(
        std::clamp(std::ceil(value), 1.0F, 4096.0F));
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

struct ExpectedPictureRaster {
    std::uint16_t width = 0U;
    std::uint16_t height = 0U;
    std::size_t nontransparent_pixels = 0U;
    std::size_t rgb_colors = 0U;
    std::size_t alpha_values = 0U;
    std::uint64_t hash = 0U;
};

[[nodiscard]] float text_factor(std::uint8_t alignment) {
    return alignment == 1U ? 0.5F : alignment == 2U ? 1.0F : 0.0F;
}

[[nodiscard]] ExpectedPictureRaster expected_picture_only_raster(
    const smgpc::layout::BrlytTextBox& text_box, float pane_width,
    const smgpc::layout::BrfntFont& regular_font,
    const smgpc::layout::BrfntFont& picture_font,
    const smgpc::layout::BrfntGlyph& glyph,
    const std::array<std::uint8_t, 4U>& text_color) {
    const auto scale_x =
        text_box.font_width / static_cast<float>(regular_font.width);
    const auto scale_y =
        text_box.font_height / static_cast<float>(regular_font.height);
    require(scale_x > 0.0F && scale_y > 0.0F,
            "expected PictureFont raster requires positive writer scale");
    const auto native_char_space = text_box.char_space / scale_x;
    const auto line_width =
        static_cast<float>(glyph_advance(glyph)) + native_char_space;
    const auto native_wrap_width = pane_width / scale_x;
    const auto horizontal_position =
        static_cast<std::uint8_t>(text_box.text_position % 3U);
    const auto line_alignment = static_cast<std::uint8_t>(
        text_box.text_alignment == 0U || text_box.text_alignment > 2U
            ? horizontal_position
            : text_box.text_alignment);
    const auto native_width =
        line_alignment == 0U ? line_width
                             : std::max(native_wrap_width, line_width);
    const auto picture_y =
        static_cast<float>(regular_font.ascent) -
        static_cast<float>(picture_font.ascent) - (2.0F / scale_y);
    const auto top = std::min(0.0F, picture_y);
    const auto bottom = std::max(
        static_cast<float>(regular_font.height),
        picture_y + static_cast<float>(glyph.height));

    auto proof = ExpectedPictureRaster{
        .width = texture_extent(native_width),
        .height = texture_extent(bottom - top),
    };
    auto rgba = std::vector<std::uint8_t>(
        static_cast<std::size_t>(proof.width) * proof.height * 4U, 0U);
    const auto cursor_x =
        (static_cast<float>(proof.width) - line_width) *
        text_factor(line_alignment);
    const auto glyph_x = static_cast<int>(std::round(cursor_x)) +
                         static_cast<int>(glyph.widths.left);
    const auto glyph_y = static_cast<int>(std::round(picture_y - top));
    const auto draw_width =
        glyph.widths.glyph_width == 0U
            ? glyph.width
            : std::min<std::uint8_t>(glyph.width,
                                     glyph.widths.glyph_width);
    require(glyph.sheet_index < picture_font.sheets.size(),
            "expected PictureFont raster sheet must exist");
    const auto& sheet = picture_font.sheets[glyph.sheet_index];
    for (auto y = 0U; y < glyph.height; ++y) {
        for (auto x = 0U; x < draw_width; ++x) {
            const auto source_x = static_cast<std::uint16_t>(glyph.x + x);
            const auto source_y = static_cast<std::uint16_t>(glyph.y + y);
            if (source_x >= sheet.width || source_y >= sheet.height) {
                continue;
            }
            const auto dest_x = glyph_x + static_cast<int>(x);
            const auto dest_y = glyph_y + static_cast<int>(y);
            if (dest_x < 0 || dest_y < 0 || dest_x >= proof.width ||
                dest_y >= proof.height) {
                continue;
            }
            const auto source_offset =
                (static_cast<std::size_t>(source_y) * sheet.width + source_x) *
                4U;
            const auto source_alpha = static_cast<std::uint8_t>(
                (static_cast<std::uint16_t>(
                     sheet.rgba[source_offset + 3U]) *
                 text_color[3U]) /
                255U);
            if (source_alpha == 0U) {
                continue;
            }
            const auto dest_offset =
                (static_cast<std::size_t>(dest_y) * proof.width +
                 static_cast<std::size_t>(dest_x)) *
                4U;
            if (source_alpha < rgba[dest_offset + 3U]) {
                continue;
            }
            for (auto component = std::size_t{}; component < 3U;
                 ++component) {
                rgba[dest_offset + component] = static_cast<std::uint8_t>(
                    (static_cast<std::uint16_t>(
                         sheet.rgba[source_offset + component]) *
                     text_color[component]) /
                    255U);
            }
            rgba[dest_offset + 3U] = source_alpha;
        }
    }

    auto colors = std::set<std::uint32_t>{};
    auto alphas = std::set<std::uint8_t>{};
    proof.hash = 14695981039346656037ULL;
    for (auto offset = std::size_t{}; offset < rgba.size(); offset += 4U) {
        if (rgba[offset + 3U] != 0U) {
            ++proof.nontransparent_pixels;
            colors.insert(
                (static_cast<std::uint32_t>(rgba[offset]) << 16U) |
                (static_cast<std::uint32_t>(rgba[offset + 1U]) << 8U) |
                static_cast<std::uint32_t>(rgba[offset + 2U]));
            alphas.insert(rgba[offset + 3U]);
        }
        for (auto component = std::size_t{}; component < 4U; ++component) {
            proof.hash ^= rgba[offset + component];
            proof.hash *= 1099511628211ULL;
        }
    }
    proof.rgb_colors = colors.size();
    proof.alpha_values = alphas.size();
    return proof;
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

#ifndef NDEBUG
[[nodiscard]] smgpc::layout::LayoutRuntime::DebugTextRasterState
require_exact_raster(const smgpc::layout::LayoutRuntime& layout,
                     std::string_view text_box_name) {
    const auto rasters = layout.debugTextRasters(text_box_name);
    const auto found = std::ranges::find(
        rasters, text_box_name,
        &smgpc::layout::LayoutRuntime::DebugTextRasterState::text_box_name);
    require(found != rasters.end(),
            "the selected retail text box must produce a debug raster");
    return *found;
}
#endif

struct RasterProof {
    std::uint16_t width = 0U;
    std::uint16_t height = 0U;
    std::size_t colors = 0U;
    std::uint64_t hash = 0U;
};

[[nodiscard]] RasterProof test_retail_mixed_raster(
    const RetailFixture& fixture,
    const smgpc::resource::RarcArchive& font_archive,
    const smgpc::layout::BrfntFont& picture_font) {
#ifdef NDEBUG
    throw std::runtime_error("PictureFont mixed-raster proof requires a debug build");
#else
    const auto press_archive =
        smgpc::resource::RarcArchive::from_file(fixture.press_start_archive);
    const auto* brlyt_entry = smgpc::layout::find_layout_brlyt(
        press_archive.entries(), "PressStart");
    require(brlyt_entry != nullptr,
            "PressStart.arc must contain its exact retail BRLYT");
    const auto parsed = smgpc::layout::parse_brlyt_layout(
        press_archive.file_data(*brlyt_entry));

    const auto* text_box =
        static_cast<const smgpc::layout::BrlytTextBox*>(nullptr);
    auto regular_font = smgpc::layout::BrfntFont{};
    for (const auto& candidate : parsed.text_boxes) {
        try {
            auto font = load_font(font_archive, candidate.font_name);
            if (font.glyph_for_exact('X').has_value() &&
                font.glyph_for_exact('Y').has_value() &&
                candidate.pane_index < parsed.panes.size()) {
                text_box = &candidate;
                regular_font = std::move(font);
                break;
            }
        } catch (const std::exception&) {
        }
    }
    require(text_box != nullptr,
            "PressStart must expose a real text box backed by X/Y glyphs");

    auto layout = smgpc::layout::LayoutRuntime(
        "picture-font-tag-test", "PressStart", 1U, 0,
        fixture.press_start_archive);
    require(layout.hasPane(text_box->name),
            "PressStart runtime must parse the selected retail text box");
    const auto font_count_before = layout.debugFontCount();
    require(font_count_before > 0U,
            "PressStart must load its ordinary font before tagged text is installed");

    auto raw = std::u16string(u"X");
    append_picture_tag(raw, 0U);
    raw.push_back(u'Y');
    append_picture_tag(raw, 1U);
    layout.setTextBoxTaggedStringRecursive(text_box->name.c_str(), raw, u"");
    require(layout.debugFontCount() == font_count_before + 1U,
            "PictureFont must load lazily and exactly once when a group-3 tag is installed");

    const auto first = require_exact_raster(layout, text_box->name);
    const auto second = require_exact_raster(layout, text_box->name);
    require(first.rgba_hash == second.rgba_hash &&
                first.nontransparent_pixel_count ==
                    second.nontransparent_pixel_count,
            "mixed-font rasterization must be deterministic");

    const auto regular_x = *regular_font.glyph_for_exact('X');
    const auto regular_y = *regular_font.glyph_for_exact('Y');
    const auto picture_zero = *picture_font.glyph_for_exact('0');
    const auto picture_one = *picture_font.glyph_for_exact('1');
    const auto scale_x =
        text_box->font_width / static_cast<float>(regular_font.width);
    const auto scale_y =
        text_box->font_height / static_cast<float>(regular_font.height);
    require(scale_x > 0.0F && scale_y > 0.0F,
            "PressStart must retain positive writer scale");
    const auto native_char_space = text_box->char_space / scale_x;
    const auto expected_line_width =
        static_cast<float>(glyph_advance(regular_x) +
                           glyph_advance(picture_zero) +
                           glyph_advance(regular_y) +
                           glyph_advance(picture_one)) +
        native_char_space * 4.0F;
    const auto native_wrap_width =
        parsed.panes[text_box->pane_index].width / scale_x;
    require(expected_line_width <= native_wrap_width,
            "focused mixed string must remain on one authored line");
    const auto horizontal_position =
        static_cast<std::uint8_t>(text_box->text_position % 3U);
    const auto line_alignment = static_cast<std::uint8_t>(
        text_box->text_alignment == 0U || text_box->text_alignment > 2U
            ? horizontal_position
            : text_box->text_alignment);
    const auto expected_native_width =
        line_alignment == 0U
            ? expected_line_width
            : std::max(native_wrap_width, expected_line_width);
    const auto picture_y =
        static_cast<float>(regular_font.ascent) -
        static_cast<float>(picture_font.ascent) - (2.0F / scale_y);
    const auto expected_top = std::min(0.0F, picture_y);
    const auto expected_bottom = std::max(
        static_cast<float>(regular_font.height),
        picture_y + static_cast<float>(
                        std::max(picture_zero.height, picture_one.height)));

    require(first.width == texture_extent(expected_native_width) &&
                first.height ==
                    texture_extent(expected_bottom - expected_top) &&
                first.font_width == regular_font.width &&
                first.font_height == regular_font.height &&
                first.ordinary_glyph_count == 2U &&
                first.picture_glyph_count == 2U &&
                first.nontransparent_pixel_count > 0U &&
                first.nontransparent_rgb_color_count > 1U,
            "mixed raster must preserve exact writer metrics, roles, and nonzero full-color pixels");
    require(picture_glyph_rgb_count(picture_font, picture_zero) > 1U,
            "retail PictureFont U+0030 must contain full-color RGB data");

    constexpr auto custom_text_color =
        std::array<std::uint8_t, 4U>{127U, 193U, 71U, 173U};
    constexpr auto hostile_mapping_max =
        std::array<std::uint8_t, 4U>{3U, 5U, 7U, 1U};
    layout.debugSetTextBoxRasterColors(text_box->name, custom_text_color,
                                       hostile_mapping_max);
    auto picture_only = std::u16string{};
    append_picture_tag(picture_only, 0U);
    layout.setTextBoxTaggedStringRecursive(text_box->name.c_str(),
                                           picture_only, u"");
    const auto colored = require_exact_raster(layout, text_box->name);
    const auto expected_colored = expected_picture_only_raster(
        *text_box, parsed.panes[text_box->pane_index].width, regular_font,
        picture_font, picture_zero, custom_text_color);
    require(picture_font.sheet_format ==
                    smgpc::resource::TplTextureFormat::RGB5A3 &&
                colored.width == expected_colored.width &&
                colored.height == expected_colored.height &&
                colored.nontransparent_pixel_count ==
                    expected_colored.nontransparent_pixels &&
                colored.nontransparent_rgb_color_count ==
                    expected_colored.rgb_colors &&
                colored.nontransparent_alpha_value_count ==
                    expected_colored.alpha_values &&
                colored.rgba_hash == expected_colored.hash &&
                colored.nontransparent_alpha_value_count > 1U,
            "PictureFont RGB5A3 must preserve source alpha, reset color mapping, and modulate exact text RGBA");

    layout.setTextBoxStringRecursive(text_box->name.c_str(), picture_only);
    const auto direct_tag = require_exact_raster(layout, text_box->name);
    require(direct_tag.picture_glyph_count == 1U &&
                direct_tag.ordinary_glyph_count == 0U &&
                direct_tag.rgba_hash == colored.rgba_hash,
            "programmatic strings built with a valid type-3 tag must use the same PictureFont path as BMG text");
    const auto malformed_direct = std::u16string{
        static_cast<char16_t>(0x001aU),
        static_cast<char16_t>(0x0603U),
    };
    require_throws<std::invalid_argument>(
        [&] {
            layout.setTextBoxStringRecursive(text_box->name.c_str(),
                                             malformed_direct);
        },
        "programmatic strings must reject a malformed inline control before mutating the text box");
    require(require_exact_raster(layout, text_box->name).rgba_hash ==
                direct_tag.rgba_hash,
            "a rejected programmatic control sequence must preserve the prior raster state");

    auto player_tag = std::u16string{};
    append_picture_tag(player_tag, 0x002bU);
    layout.setTextBoxTaggedStringRecursive(text_box->name.c_str(), player_tag,
                                           u"");
    layout.setPictureTagPlayerCharacter(
        smgpc::resource::BmgPlayerCharacter::Mario);
    const auto mario = require_exact_raster(layout, text_box->name);
    layout.setPictureTagPlayerCharacter(
        smgpc::resource::BmgPlayerCharacter::Luigi);
    const auto luigi = require_exact_raster(layout, text_box->name);
    require(mario.picture_glyph_count == 1U &&
                luigi.picture_glyph_count == 1U &&
                mario.rgba_hash != luigi.rgba_hash,
            "changing the explicit player policy must invalidate the prior raster and select a different PictureFont glyph");
    layout.setPictureTagPlayerCharacter(std::nullopt);
    require_throws<std::logic_error>(
        [&] { (void)layout.debugTextRasters(text_box->name); },
        "an actual dynamic tag without game-data identity must fail explicitly instead of defaulting to Mario");

    layout.setTextBoxStringRecursive(text_box->name.c_str(), u"\ud55cAB");
    const auto literal = require_exact_raster(layout, text_box->name);
    require(literal.ordinary_glyph_count == 3U &&
                literal.picture_glyph_count == 0U,
            "localized literal A/B text must remain ordinary text without an icon alias heuristic");

    return RasterProof{
        .width = first.width,
        .height = first.height,
        .colors = first.nontransparent_rgb_color_count,
        .hash = first.rgba_hash,
    };
#endif
}

void test_missing_picture_font_fails(
    const RetailFixture& fixture,
    const smgpc::resource::RarcArchive& font_archive) {
#ifdef NDEBUG
    throw std::runtime_error("missing PictureFont proof requires a debug build");
#else
    const auto press_archive =
        smgpc::resource::RarcArchive::from_file(fixture.press_start_archive);
    const auto* brlyt_entry = smgpc::layout::find_layout_brlyt(
        press_archive.entries(), "PressStart");
    require(brlyt_entry != nullptr,
            "missing-font proof requires retail PressStart BRLYT");
    const auto parsed = smgpc::layout::parse_brlyt_layout(
        press_archive.file_data(*brlyt_entry));
    require(!parsed.text_boxes.empty(),
            "missing-font proof requires a retail PressStart text box");
    const auto& text_box = parsed.text_boxes.front();
    const auto& font_entry =
        require_font_entry(font_archive, text_box.font_name);
    const auto font_data = font_archive.file_data(font_entry);
    auto external_font = nw4r::ut::ResFont{};
    require(external_font.SetResource(
                const_cast<std::uint8_t*>(font_data.data()),
                font_data.size()) &&
                external_font.SetAlternateChar('?'),
            "missing-font proof requires a live ordinary retail ResFont");

    const auto unique = std::chrono::steady_clock::now()
                            .time_since_epoch()
                            .count();
    const auto directory = std::filesystem::temp_directory_path() /
                           ("smgpc-picture-font-missing-" +
                            std::to_string(unique));
    std::filesystem::create_directories(directory);
    struct TempDirectoryGuard {
        std::filesystem::path path;
        ~TempDirectoryGuard() {
            auto error = std::error_code{};
            std::filesystem::remove_all(path, error);
        }
    } guard{directory};
    const auto isolated_layout = directory / "PressStart.arc";
    std::filesystem::copy_file(fixture.press_start_archive, isolated_layout);

    auto layout = smgpc::layout::LayoutRuntime(
        "missing-picture-font-test", "PressStart", 1U, 0,
        isolated_layout);
    layout.setTextBoxFontRecursive(text_box.name.c_str(), external_font);
    auto tagged = std::u16string{};
    append_picture_tag(tagged, 0U);
    layout.setTextBoxTaggedStringRecursive(text_box.name.c_str(), tagged, u"");
    require_throws<std::logic_error>(
        [&] { (void)layout.debugTextRasters(text_box.name); },
        "a tagged layout with an ordinary font but no resolved PictureFont must fail explicitly");
#endif
}

}  // namespace

int main() {
    test_ordered_token_formatting();

    const auto fixture = find_retail_fixture();
    if (!fixture.has_value()) {
        std::cout << "[skip] extracted RMGK01 PictureFont/message/layout proof\n";
        std::cout << "PictureFont tag tests passed: 1/4\n";
        return 0;
    }

    const auto font_archive =
        smgpc::resource::RarcArchive::from_file(fixture->font_archive);
    const auto picture_font = load_font(font_archive, "PictureFont");
    const auto corpus = test_retail_corpus(*fixture, picture_font);
    const auto raster =
        test_retail_mixed_raster(*fixture, font_archive, picture_font);
    test_missing_picture_font_fails(*fixture, font_archive);
    std::cout << "picture_tags=" << corpus.tag_count
              << ";payloads=" << corpus.payload_count
              << ";picture_font=" << static_cast<unsigned>(picture_font.width)
              << 'x' << static_cast<unsigned>(picture_font.height)
              << ";ascent=" << static_cast<unsigned>(picture_font.ascent)
              << ";mixed=" << raster.width << 'x' << raster.height
              << ";colors=" << raster.colors << ";hash=" << raster.hash
              << '\n';
    std::cout << "PictureFont tag tests passed: 4/4\n";
    return 0;
}
