#include "Game/Screen/SimpleLayout.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "core/RenderTypes.hpp"
#include "layout/LytTexMap.hpp"
#include "render/GXState.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeContext.hpp"

namespace {

    [[nodiscard]] bool ends_with(std::string_view text, std::string_view suffix) {
        return text.size() >= suffix.size() && text.substr(text.size() - suffix.size()) == suffix;
    }

    [[nodiscard]] std::string texture_archive_path(std::string_view texture_name) {
        auto path = std::string("timg/");
        path.append(texture_name);
        if (!ends_with(path, ".tpl")) {
            path.append(".tpl");
        }
        std::ranges::transform(path, path.begin(), [](unsigned char character) { return static_cast< char >(std::tolower(character)); });

        return path;
    }

    [[nodiscard]] std::string lower_copy(std::string_view value) {
        auto lower = std::string(value);
        std::ranges::transform(lower, lower.begin(), [](unsigned char character) { return static_cast< char >(std::tolower(character)); });
        return lower;
    }

    [[nodiscard]] std::string base_name(std::string_view path) {
        const auto slash = path.find_last_of('/');
        if (slash == std::string_view::npos) {
            return std::string(path);
        }

        return std::string(path.substr(slash + 1U));
    }

    [[nodiscard]] std::string animation_name_from_path(std::string_view path) {
        auto name = base_name(path);
        if (ends_with(name, ".brlan")) {
            name.resize(name.size() - std::string_view(".brlan").size());
        }
        return lower_copy(name);
    }

    [[nodiscard]] std::string font_resource_name(std::string_view font_name) {
        auto name = lower_copy(base_name(font_name));
        if (!ends_with(name, ".brfnt")) {
            name.append(".brfnt");
        }

        return name;
    }

    [[nodiscard]] std::vector< std::string > font_resource_candidates(std::string_view font_name) {
        auto candidates = std::vector< std::string >{font_resource_name(font_name)};
        const auto dot = candidates.front().rfind(".brfnt");
        const auto stem = candidates.front().substr(0U, dot);
        constexpr std::array< std::string_view, 7U > locale_suffixes{"jpn", "eng", "fra", "ger", "ita", "spa", "kor"};
        for (const auto suffix : locale_suffixes) {
            if (ends_with(stem, suffix) && stem.size() > suffix.size()) {
                candidates.push_back(stem.substr(0U, stem.size() - suffix.size()) + candidates.front().substr(dot));
                break;
            }
        }

        return candidates;
    }

    [[nodiscard]] std::string_view layout_locale_suffix() {
        static const auto locale_suffix = [] {
            if (const auto* value = std::getenv("SMGPC_LAYOUT_LOCALE_SUFFIX"); value != nullptr && value[0] != '\0') {
                return std::string(value);
            }

            return std::string("KrKo");
        }();
        return locale_suffix;
    }

    [[nodiscard]] bool pane_name_has_inactive_locale_suffix(std::string_view name) {
        constexpr std::array< std::string_view, 10U > suffixes{"JpJa", "UsEn", "EuEn", "EuFr", "EuGe",
                                                               "EuSp", "EuIt", "KrKo", "CnSi", "CnTr"};
        const auto active = layout_locale_suffix();
        for (const auto suffix : suffixes) {
            if (suffix == active) {
                continue;
            }
            if (ends_with(name, suffix) && name.size() > suffix.size()) {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] bool pane_name_has_locale_suffix(std::string_view name) {
        constexpr std::array< std::string_view, 10U > suffixes{"JpJa", "UsEn", "EuEn", "EuFr", "EuGe",
                                                               "EuSp", "EuIt", "KrKo", "CnSi", "CnTr"};
        return std::ranges::any_of(suffixes, [name](const auto suffix) { return ends_with(name, suffix) && name.size() > suffix.size(); });
    }

    [[nodiscard]] bool pane_name_has_active_locale_variant(const smgpc::layout::BrlytLayout& layout, std::string_view name) {
        if (name.empty() || pane_name_has_locale_suffix(name)) {
            return false;
        }

        auto active_name = std::string(name);
        active_name.append(layout_locale_suffix());
        return std::ranges::any_of(layout.panes, [&active_name](const auto& pane) { return pane.name == active_name; });
    }

    [[nodiscard]] std::string active_locale_variant_name(std::string_view name) {
        auto active_name = std::string(name);
        active_name.append(layout_locale_suffix());
        return active_name;
    }

    [[nodiscard]] std::string layout_message_id(std::string_view layout_name, std::string_view user_data) {
        auto message_id = std::string("Layout_");
        message_id.append(layout_name);
        message_id.append(user_data);
        return message_id;
    }

    [[nodiscard]] std::vector< std::uint16_t > encode_utf16_words(std::u16string_view text) {
        auto words = std::vector< std::uint16_t >{};
        words.reserve(text.size());
        for (const auto code : text) {
            words.push_back(static_cast< std::uint16_t >(code));
        }
        return words;
    }

    [[nodiscard]] bool pane_name_matches_request(std::string_view pane_name, std::string_view requested_name) {
        if (pane_name == requested_name) {
            return true;
        }
        return !requested_name.empty() && !pane_name_has_locale_suffix(requested_name) && pane_name == active_locale_variant_name(requested_name);
    }

    [[nodiscard]] std::optional< std::size_t > find_preferred_pane_index(const smgpc::layout::BrlytLayout& layout, std::string_view pane_name) {
        if (pane_name.empty()) {
            return std::nullopt;
        }

        if (!pane_name_has_locale_suffix(pane_name)) {
            const auto active_name = active_locale_variant_name(pane_name);
            const auto active = std::ranges::find_if(layout.panes, [&active_name](const auto& pane) { return pane.name == active_name; });
            if (active != layout.panes.end()) {
                return static_cast< std::size_t >(std::distance(layout.panes.begin(), active));
            }
        }

        const auto exact = std::ranges::find_if(layout.panes, [pane_name](const auto& pane) { return pane.name == pane_name; });
        if (exact == layout.panes.end()) {
            return std::nullopt;
        }

        return static_cast< std::size_t >(std::distance(layout.panes.begin(), exact));
    }

    [[nodiscard]] bool font_name_matches(std::string_view loaded_name, std::string_view requested_name) {
        const auto loaded_candidates = font_resource_candidates(loaded_name);
        const auto requested_candidates = font_resource_candidates(requested_name);
        return std::ranges::any_of(loaded_candidates, [&requested_candidates](const auto& loaded) {
            return std::ranges::find(requested_candidates, loaded) != requested_candidates.end();
        });
    }

    [[nodiscard]] bool contains_texture(const std::vector< SimpleLayout::RenderTexture >& textures, std::string_view texture_name) {
        return std::ranges::any_of(textures, [texture_name](const auto& texture) { return texture.name == texture_name; });
    }

    [[nodiscard]] SimpleLayout::RenderTexture* find_texture(std::vector< SimpleLayout::RenderTexture >& textures, std::string_view texture_name) {
        const auto it = std::ranges::find_if(textures, [texture_name](const auto& texture) { return texture.name == texture_name; });

        return it == textures.end() ? nullptr : &(*it);
    }

    [[nodiscard]] std::string texture_format_name(smgpc::resource::TplTextureFormat format) {
        switch (format) {
        case smgpc::resource::TplTextureFormat::I4:
            return "I4";
        case smgpc::resource::TplTextureFormat::I8:
            return "I8";
        case smgpc::resource::TplTextureFormat::IA4:
            return "IA4";
        case smgpc::resource::TplTextureFormat::IA8:
            return "IA8";
        case smgpc::resource::TplTextureFormat::RGB565:
            return "RGB565";
        case smgpc::resource::TplTextureFormat::RGB5A3:
            return "RGB5A3";
        case smgpc::resource::TplTextureFormat::RGBA8:
            return "RGBA8";
        case smgpc::resource::TplTextureFormat::C4:
            return "C4";
        case smgpc::resource::TplTextureFormat::C8:
            return "C8";
        case smgpc::resource::TplTextureFormat::C14X2:
            return "C14X2";
        case smgpc::resource::TplTextureFormat::CMPR:
            return "CMPR";
        }

        return "Unknown";
    }

    [[nodiscard]] bool contains_font(const std::vector< SimpleLayout::RenderFont >& fonts, std::string_view font_name) {
        return std::ranges::any_of(fonts, [font_name](const auto& font) { return font_name_matches(font.name, font_name); });
    }

    [[nodiscard]] SimpleLayout::RenderFont* find_font(std::vector< SimpleLayout::RenderFont >& fonts, std::string_view font_name) {
        const auto it = std::ranges::find_if(fonts, [font_name](const auto& font) { return font_name_matches(font.name, font_name); });

        return it == fonts.end() ? nullptr : &(*it);
    }

    [[nodiscard]] SimpleLayout::RenderTextTexture* find_text_texture(std::vector< SimpleLayout::RenderTextTexture >& textures,
                                                                     std::size_t text_box_index) {
        const auto it = std::ranges::find_if(textures, [text_box_index](const auto& texture) { return texture.text_box_index == text_box_index; });

        return it == textures.end() ? nullptr : &(*it);
    }

    [[nodiscard]] bool pane_is_or_descends_from(const smgpc::layout::BrlytLayout& layout, std::size_t pane_index, std::string_view root_name) {
        while (pane_index < layout.panes.size()) {
            const auto& pane = layout.panes[pane_index];
            if (pane_name_matches_request(pane.name, root_name)) {
                return true;
            }
            if (pane.parent_index < 0) {
                break;
            }
            pane_index = static_cast< std::size_t >(pane.parent_index);
        }

        return false;
    }

    [[nodiscard]] bool text_box_matches_recursive(const smgpc::layout::BrlytLayout& layout, const smgpc::layout::BrlytTextBox& text_box,
                                                  std::string_view requested_name) {
        if (requested_name.empty() || pane_name_matches_request(text_box.name, requested_name)) {
            return true;
        }
        return text_box.pane_index < layout.panes.size() && pane_is_or_descends_from(layout, text_box.pane_index, requested_name);
    }

    [[nodiscard]] const smgpc::resource::RarcEntry* find_font_entry(const smgpc::resource::RarcArchive& archive, std::string_view font_name) {
        const auto candidates = font_resource_candidates(font_name);
        const auto it = std::ranges::find_if(archive.entries(), [&candidates](const auto& entry) {
            return std::ranges::find(candidates, font_resource_name(entry.path)) != candidates.end();
        });

        return it == archive.entries().end() ? nullptr : &(*it);
    }

    [[nodiscard]] std::optional< std::filesystem::path >
    find_companion_font_archive(const std::optional< std::filesystem::path >& layout_archive_path) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            if (auto path = runtime->find_layout_archive("Font")) {
                return path;
            }
        }

        if (!layout_archive_path.has_value()) {
            return std::nullopt;
        }

        auto candidate = layout_archive_path->parent_path() / "Font.arc";
        std::error_code error{};
        if (std::filesystem::is_regular_file(candidate, error)) {
            return candidate;
        }

        return std::nullopt;
    }

    [[nodiscard]] float text_factor(std::uint8_t alignment) {
        switch (alignment) {
        case 1U:
            return 0.5F;
        case 2U:
            return 1.0F;
        default:
            return 0.0F;
        }
    }

    [[nodiscard]] float glyph_advance(const smgpc::layout::BrfntGlyph& glyph, float scale_x) {
        const auto char_width = glyph.widths.char_width == 0 ? static_cast< int >(glyph.width) : static_cast< int >(glyph.widths.char_width);
        return static_cast< float >(char_width) * scale_x;
    }

    struct TextLayoutGlyph {
        std::uint16_t code = 0U;
        smgpc::layout::BrfntGlyph glyph{};
        float advance = 0.0F;
    };

    struct TextLayoutLine {
        std::vector< TextLayoutGlyph > glyphs = {};
        float width = 0.0F;
    };

    [[nodiscard]] bool is_wrap_space(std::uint16_t code) {
        return code == 0x0009U || code == 0x0020U || code == 0x3000U;
    }

    [[nodiscard]] bool is_line_break(std::uint16_t code) {
        return code == 0x000aU || code == 0x000dU;
    }

    [[nodiscard]] std::uint16_t resolve_layout_glyph_code(std::uint16_t code, const smgpc::layout::BrfntFont& font, bool use_button_icon_aliases);

    [[nodiscard]] float text_line_width(const std::vector< TextLayoutGlyph >& glyphs, float native_char_space) {
        if (glyphs.empty()) {
            return 0.0F;
        }

        auto width = 0.0F;
        for (const auto& glyph : glyphs) {
            width += glyph.advance;
        }
        width += native_char_space * static_cast< float >(glyphs.size() - 1U);
        return std::max(width, 0.0F);
    }

    void trim_trailing_wrap_spaces(std::vector< TextLayoutGlyph >& glyphs) {
        while (!glyphs.empty() && is_wrap_space(glyphs.back().code)) {
            glyphs.pop_back();
        }
    }

    void trim_leading_wrap_spaces(std::vector< TextLayoutGlyph >& glyphs) {
        while (!glyphs.empty() && is_wrap_space(glyphs.front().code)) {
            glyphs.erase(glyphs.begin());
        }
    }

    [[nodiscard]] std::optional< std::size_t > last_wrap_space_index(const std::vector< TextLayoutGlyph >& glyphs) {
        for (auto index = glyphs.size(); index > 0U; --index) {
            if (is_wrap_space(glyphs[index - 1U].code)) {
                return index - 1U;
            }
        }

        return std::nullopt;
    }

    [[nodiscard]] std::vector< TextLayoutLine > layout_text_lines(std::span< const std::uint16_t > text, const smgpc::layout::BrfntFont& font,
                                                                  float native_char_space, float native_wrap_width, bool use_button_icon_aliases) {
        auto lines = std::vector< TextLayoutLine >{};
        auto current = std::vector< TextLayoutGlyph >{};

        const auto measure_current = [&]() { return text_line_width(current, native_char_space); };
        const auto push_current = [&]() {
            trim_trailing_wrap_spaces(current);
            lines.push_back(TextLayoutLine{
                .glyphs = current,
                .width = text_line_width(current, native_char_space),
            });
            current.clear();
        };

        const auto wrap_width = native_wrap_width > 0.0F ? native_wrap_width : 4096.0F;
        for (auto index = std::size_t{}; index < text.size(); ++index) {
            const auto code = text[index];
            if (is_line_break(code)) {
                push_current();
                if (code == 0x000dU && index + 1U < text.size() && text[index + 1U] == 0x000aU) {
                    ++index;
                }
                continue;
            }

            const auto glyph_code = resolve_layout_glyph_code(code, font, use_button_icon_aliases);
            const auto glyph = font.glyph_for(glyph_code);
            if (!glyph.has_value()) {
                continue;
            }

            current.push_back(TextLayoutGlyph{
                .code = code,
                .glyph = *glyph,
                .advance = glyph_advance(*glyph, 1.0F),
            });

            if (current.size() <= 1U || measure_current() <= wrap_width) {
                continue;
            }

            if (const auto break_index = last_wrap_space_index(current); break_index.has_value() && *break_index > 0U) {
                auto next = std::vector< TextLayoutGlyph >(current.begin() + static_cast< std::ptrdiff_t >(*break_index + 1U), current.end());
                current.erase(current.begin() + static_cast< std::ptrdiff_t >(*break_index), current.end());
                trim_trailing_wrap_spaces(current);
                lines.push_back(TextLayoutLine{
                    .glyphs = current,
                    .width = text_line_width(current, native_char_space),
                });
                trim_leading_wrap_spaces(next);
                current = std::move(next);
                continue;
            }

            auto overflow = current.back();
            current.pop_back();
            push_current();
            current.push_back(overflow);
        }

        push_current();
        if (lines.empty()) {
            lines.push_back(TextLayoutLine{});
        }

        return lines;
    }

    struct ButtonIconAlias {
        std::uint16_t ascii_code = 0U;
        std::uint16_t icon_code = 0U;
    };

    constexpr std::array< ButtonIconAlias, 2U > kButtonIconAliases{
        ButtonIconAlias{.ascii_code = 'A', .icon_code = 0xe000U},
        ButtonIconAlias{.ascii_code = 'B', .icon_code = 0xe00bU},
    };

    [[nodiscard]] std::optional< std::uint16_t > fullwidth_ascii_to_ascii(std::uint16_t code) {
        if (code >= 0xff01U && code <= 0xff5eU) {
            return static_cast< std::uint16_t >(code - 0xfee0U);
        }

        return std::nullopt;
    }

    [[nodiscard]] std::uint16_t normalize_layout_ascii(std::uint16_t code) {
        if (const auto normalized = fullwidth_ascii_to_ascii(code)) {
            return *normalized;
        }

        return code;
    }

    [[nodiscard]] std::optional< std::uint16_t > button_icon_alias_for(std::uint16_t code) {
        const auto ascii_code = normalize_layout_ascii(code);
        const auto it = std::ranges::find_if(kButtonIconAliases, [ascii_code](const auto& alias) { return alias.ascii_code == ascii_code; });
        if (it == kButtonIconAliases.end()) {
            return std::nullopt;
        }

        return it->icon_code;
    }

    [[nodiscard]] bool is_button_icon_code(std::uint16_t code) {
        return std::ranges::any_of(kButtonIconAliases, [code](const auto& alias) { return alias.icon_code == code; });
    }

    [[nodiscard]] bool is_cjk_code(std::uint16_t code) {
        return (code >= 0x1100U && code <= 0x11ffU) || (code >= 0x3040U && code <= 0x30ffU) || (code >= 0x3130U && code <= 0x318fU) ||
               (code >= 0xac00U && code <= 0xd7afU);
    }

    [[nodiscard]] bool text_uses_button_icon_aliases(std::span< const std::uint16_t > text) {
        auto has_button_marker = false;
        auto has_cjk_text = false;

        for (const auto code : text) {
            if (button_icon_alias_for(code).has_value()) {
                has_button_marker = true;
                continue;
            }

            const auto normalized = normalize_layout_ascii(code);
            if (normalized < 0x80U && std::isalnum(static_cast< unsigned char >(normalized)) != 0) {
                return false;
            }

            has_cjk_text = has_cjk_text || is_cjk_code(code);
        }

        return has_button_marker && has_cjk_text;
    }

    [[nodiscard]] std::uint16_t resolve_layout_glyph_code(std::uint16_t code, const smgpc::layout::BrfntFont& font, bool use_button_icon_aliases) {
        if (use_button_icon_aliases) {
            if (const auto icon_code = button_icon_alias_for(code); icon_code.has_value() && font.glyph_for_exact(*icon_code).has_value()) {
                return *icon_code;
            }
        }

        return code;
    }

    [[nodiscard]] float base_position_x(std::uint8_t base_position, float width) {
        switch (base_position % 3U) {
        case 1U:
            return -width * 0.5F;
        case 2U:
            return -width;
        default:
            return 0.0F;
        }
    }

    [[nodiscard]] float base_position_y(std::uint8_t base_position, float height) {
        switch (base_position / 3U) {
        case 1U:
            return -height * 0.5F;
        case 2U:
            return -height;
        default:
            return 0.0F;
        }
    }

    [[nodiscard]] std::array< float, 2U > layout_location_adjust_scale() {
        return {0.75F, 1.0F};
    }

    [[nodiscard]] float gx_texture_v_to_host(float v) {
        return v;
    }

    [[nodiscard]] float brlyt_texture_v_to_host(float v, std::uint8_t wrap_t) {
        (void)wrap_t;
        return gx_texture_v_to_host(v);
    }

    [[nodiscard]] float hypot2(float x, float y) {
        return std::sqrt((x * x) + (y * y));
    }

    [[nodiscard]] std::uint16_t texture_extent(float value) {
        return static_cast< std::uint16_t >(std::clamp(std::ceil(value), 1.0F, 4096.0F));
    }

    struct LayoutWindowFrameSize {
        float left = 0.0F;
        float right = 0.0F;
        float top = 0.0F;
        float bottom = 0.0F;
    };

    struct LayoutTextureSize {
        float width = 1.0F;
        float height = 1.0F;
    };

    struct LayoutTextureFlipInfo {
        std::array< std::array< std::uint8_t, 2U >, 4U > coords{};
        std::array< std::uint8_t, 2U > axis{};
    };

    [[nodiscard]] const LayoutTextureFlipInfo& layout_texture_flip_info(std::uint8_t texture_flip) {
        static constexpr auto kFlipInfos = std::array< LayoutTextureFlipInfo, 6U >{
            LayoutTextureFlipInfo{.coords = {{{0U, 0U}, {1U, 0U}, {0U, 1U}, {1U, 1U}}}, .axis = {0U, 1U}},
            LayoutTextureFlipInfo{.coords = {{{1U, 0U}, {0U, 0U}, {1U, 1U}, {0U, 1U}}}, .axis = {0U, 1U}},
            LayoutTextureFlipInfo{.coords = {{{0U, 1U}, {1U, 1U}, {0U, 0U}, {1U, 0U}}}, .axis = {0U, 1U}},
            LayoutTextureFlipInfo{.coords = {{{0U, 1U}, {0U, 0U}, {1U, 1U}, {1U, 0U}}}, .axis = {1U, 0U}},
            LayoutTextureFlipInfo{.coords = {{{1U, 1U}, {0U, 1U}, {1U, 0U}, {0U, 0U}}}, .axis = {0U, 1U}},
            LayoutTextureFlipInfo{.coords = {{{1U, 0U}, {1U, 1U}, {0U, 0U}, {0U, 1U}}}, .axis = {1U, 0U}},
        };

        return kFlipInfos[std::min< std::size_t >(texture_flip, kFlipInfos.size() - 1U)];
    }

    [[nodiscard]] std::array< smgpc::layout::BrlytTexCoord, 4U >
    layout_quad_tex_coords_from_raw(const std::array< smgpc::layout::BrlytTexCoord, 4U >& raw) {
        return {
            raw[0U],
            raw[1U],
            raw[3U],
            raw[2U],
        };
    }

    [[nodiscard]] std::array< smgpc::layout::BrlytTexCoord, 4U > default_quad_tex_coords() {
        return {
            smgpc::layout::BrlytTexCoord{0.0F, 0.0F},
            smgpc::layout::BrlytTexCoord{1.0F, 0.0F},
            smgpc::layout::BrlytTexCoord{1.0F, 1.0F},
            smgpc::layout::BrlytTexCoord{0.0F, 1.0F},
        };
    }

    [[nodiscard]] std::vector< std::array< smgpc::layout::BrlytTexCoord, 4U > >
    layout_window_content_tex_coord_sets(std::span< const std::array< smgpc::layout::BrlytTexCoord, 4U > > raw_sets) {
        auto sets = std::vector< std::array< smgpc::layout::BrlytTexCoord, 4U > >{};
        sets.reserve(raw_sets.size());
        for (const auto& raw : raw_sets) {
            sets.push_back(layout_quad_tex_coords_from_raw(raw));
        }
        if (sets.empty()) {
            sets.push_back(default_quad_tex_coords());
        }
        return sets;
    }

    [[nodiscard]] std::array< smgpc::layout::BrlytTexCoord, 4U >
    layout_window_frame_tex_coords(std::uint8_t texture_flip, LayoutTextureSize texture_size, float width, float height,
                                   std::uint8_t anchor0, std::uint8_t anchor1, std::uint8_t anchor2, std::uint8_t anchor3) {
        const auto& info = layout_texture_flip_info(texture_flip);
        const auto ix = info.axis[0U];
        const auto iy = info.axis[1U];
        const auto tex_extent = std::array< float, 2U >{std::max(texture_size.width, 1.0F), std::max(texture_size.height, 1.0F)};
        auto raw = std::array< smgpc::layout::BrlytTexCoord, 4U >{};
        auto set_component = [&](std::uint8_t point, std::uint8_t axis, float value) {
            if (axis == 0U) {
                raw[point].u = value;
            } else {
                raw[point].v = value;
            }
        };
        auto get_component = [&](std::uint8_t coord, std::uint8_t axis) {
            return static_cast< float >(info.coords[coord][axis]);
        };

        set_component(anchor0, ix, get_component(anchor0, ix));
        set_component(anchor2, ix, get_component(anchor0, ix));
        set_component(anchor0, iy, get_component(anchor0, iy));
        set_component(anchor1, iy, get_component(anchor0, iy));

        const auto sx = get_component(anchor1, ix) - get_component(anchor0, ix);
        const auto sy = get_component(anchor2, iy) - get_component(anchor0, iy);
        const auto u_delta = sx == 0.0F ? 0.0F : width / (sx * tex_extent[ix]);
        const auto v_delta = sy == 0.0F ? 0.0F : height / (sy * tex_extent[iy]);
        set_component(anchor3, ix, get_component(anchor0, ix) + u_delta);
        set_component(anchor1, ix, get_component(anchor0, ix) + u_delta);
        set_component(anchor3, iy, get_component(anchor0, iy) + v_delta);
        set_component(anchor2, iy, get_component(anchor0, iy) + v_delta);

        return layout_quad_tex_coords_from_raw(raw);
    }

    [[nodiscard]] std::array< smgpc::layout::BrlytTexCoord, 4U >
    layout_window_frame_tex_coords_lt(std::uint8_t texture_flip, LayoutTextureSize texture_size, float width, float height) {
        return layout_window_frame_tex_coords(texture_flip, texture_size, width, height, 0U, 1U, 2U, 3U);
    }

    [[nodiscard]] std::array< smgpc::layout::BrlytTexCoord, 4U >
    layout_window_frame_tex_coords_rt(std::uint8_t texture_flip, LayoutTextureSize texture_size, float width, float height) {
        return layout_window_frame_tex_coords(texture_flip, texture_size, width, height, 1U, 0U, 3U, 2U);
    }

    [[nodiscard]] std::array< smgpc::layout::BrlytTexCoord, 4U >
    layout_window_frame_tex_coords_lb(std::uint8_t texture_flip, LayoutTextureSize texture_size, float width, float height) {
        return layout_window_frame_tex_coords(texture_flip, texture_size, width, height, 2U, 3U, 0U, 1U);
    }

    [[nodiscard]] std::array< smgpc::layout::BrlytTexCoord, 4U >
    layout_window_frame_tex_coords_rb(std::uint8_t texture_flip, LayoutTextureSize texture_size, float width, float height) {
        return layout_window_frame_tex_coords(texture_flip, texture_size, width, height, 3U, 2U, 1U, 0U);
    }

    [[nodiscard]] smgpc::layout::BrlytTexCoord transform_tex_coord(smgpc::layout::BrlytTexCoord tex_coord,
                                                                   const smgpc::layout::BrlanTextureFrame& frame) {
        constexpr auto kDegToRad = 0.017453292519943295F;
        const auto scale_s = frame.scale_s.value_or(1.0F);
        const auto scale_t = frame.scale_t.value_or(1.0F);
        const auto rotate = frame.rotate.value_or(0.0F) * kDegToRad;
        const auto translate_s = frame.translate_s.value_or(0.0F);
        const auto translate_t = frame.translate_t.value_or(0.0F);
        const auto cos_r = std::cos(rotate);
        const auto sin_r = std::sin(rotate);
        const auto centered_s = tex_coord.u - 0.5F;
        const auto centered_t = tex_coord.v - 0.5F;

        return smgpc::layout::BrlytTexCoord{
            .u = translate_s + 0.5F + cos_r * scale_s * centered_s - sin_r * scale_t * centered_t,
            .v = translate_t + 0.5F + sin_r * scale_s * centered_s + cos_r * scale_t * centered_t,
        };
    }

    [[nodiscard]] std::uint16_t tex_srt_index_for_coord_gen(const smgpc::layout::BrlytTexCoordGen& coord_gen) {
        constexpr auto gx_texmtx0 = std::uint8_t{30U};
        constexpr auto gx_identity = std::uint8_t{60U};
        if (coord_gen.tex_mtx == gx_identity || coord_gen.tex_mtx < gx_texmtx0) {
            return UINT16_MAX;
        }

        return static_cast< std::uint16_t >((coord_gen.tex_mtx - gx_texmtx0) / 3U);
    }

    [[nodiscard]] smgpc::layout::BrlytTexCoord transform_tex_coord(smgpc::layout::BrlytTexCoord tex_coord, const smgpc::layout::BrlytTexSrt& srt,
                                                                   const smgpc::layout::BrlanTextureFrame& texture_frame, bool apply_animation) {
        auto frame = smgpc::layout::BrlanTextureFrame{
            .translate_s = apply_animation ? texture_frame.translate_s : std::optional< float >{},
            .translate_t = apply_animation ? texture_frame.translate_t : std::optional< float >{},
            .rotate = apply_animation ? texture_frame.rotate : std::optional< float >{},
            .scale_s = apply_animation ? texture_frame.scale_s : std::optional< float >{},
            .scale_t = apply_animation ? texture_frame.scale_t : std::optional< float >{},
        };

        frame.translate_s = frame.translate_s.value_or(srt.translate_s);
        frame.translate_t = frame.translate_t.value_or(srt.translate_t);
        frame.rotate = frame.rotate.value_or(srt.rotate);
        frame.scale_s = frame.scale_s.value_or(srt.scale_s);
        frame.scale_t = frame.scale_t.value_or(srt.scale_t);
        return transform_tex_coord(tex_coord, frame);
    }

    [[nodiscard]] std::array< std::uint8_t, 4U > brlyt_konst_color(const smgpc::layout::BrlytMaterial& material, std::uint8_t selector) {
        constexpr std::array< std::uint8_t, 8U > constants{255U, 223U, 191U, 159U, 128U, 96U, 64U, 32U};
        if (selector < constants.size()) {
            return {constants[selector], constants[selector], constants[selector], constants[selector]};
        }
        if (selector >= 12U && selector <= 15U) {
            return material.tev_k_colors[selector - 12U];
        }
        if (selector >= 16U && selector <= 31U) {
            const auto color_index = static_cast< std::size_t >((selector - 16U) % 4U);
            const auto component = static_cast< std::size_t >((selector - 16U) / 4U);
            const auto value = material.tev_k_colors[color_index][component];
            return {value, value, value, value};
        }

        return {0U, 0U, 0U, 0U};
    }

    [[nodiscard]] std::array< std::uint8_t, 4U > brlyt_stage_konst_color(const smgpc::layout::BrlytMaterial& material,
                                                                         const smgpc::layout::BrlytTevStage& stage) {
        const auto color = brlyt_konst_color(material, stage.color.k_sel);
        const auto alpha = brlyt_konst_color(material, stage.alpha.k_sel);
        return {color[0U], color[1U], color[2U], alpha[3U]};
    }

    [[nodiscard]] bool brlyt_tev_op_can_use_gx_shader(const smgpc::layout::BrlytTevStageInOp& op, std::uint8_t max_arg) {
        return op.a <= max_arg && op.b <= max_arg && op.c <= max_arg && op.d <= max_arg && op.op <= 1U && op.bias <= 3U && op.scale <= 3U &&
               op.out_reg <= 3U;
    }

    [[nodiscard]] bool brlyt_tev_stage_uses_texture(const smgpc::layout::BrlytTevStage& stage) {
        constexpr auto gx_cc_texc = std::uint8_t{8U};
        constexpr auto gx_cc_texa = std::uint8_t{9U};
        constexpr auto gx_ca_texa = std::uint8_t{4U};
        return stage.color.a == gx_cc_texc || stage.color.b == gx_cc_texc || stage.color.c == gx_cc_texc || stage.color.d == gx_cc_texc ||
               stage.color.a == gx_cc_texa || stage.color.b == gx_cc_texa || stage.color.c == gx_cc_texa || stage.color.d == gx_cc_texa ||
               stage.alpha.a == gx_ca_texa || stage.alpha.b == gx_ca_texa || stage.alpha.c == gx_ca_texa || stage.alpha.d == gx_ca_texa;
    }

    [[nodiscard]] bool brlyt_material_can_use_gx_shader(const smgpc::layout::BrlytMaterial& material) {
        if (material.tev_stages.empty()) {
            return material.textures.size() <= smgpc::render::core::kMaxGxMaterialTextureStages2D;
        }
        if (material.tev_stages.size() > smgpc::render::core::kMaxGxMaterialTevStages2D) {
            return false;
        }

        return std::ranges::all_of(material.tev_stages, [&material](const auto& stage) {
            return (!brlyt_tev_stage_uses_texture(stage) || stage.tex_map < material.textures.size()) &&
                   brlyt_tev_op_can_use_gx_shader(stage.color, 15U) && brlyt_tev_op_can_use_gx_shader(stage.alpha, 7U);
        });
    }

    [[nodiscard]] smgpc::render::GxTevStage2D brlyt_gx_tev_stage(const smgpc::layout::BrlytMaterial& material,
                                                                 const smgpc::layout::BrlytTevStage& stage, std::uint8_t texture_coord_stage,
                                                                 std::uint8_t texture_map_stage) {
        return smgpc::render::GxTevStage2D{
            .texture_stage = texture_map_stage,
            .texture_coord_stage = texture_coord_stage,
            .texture_map_stage = texture_map_stage,
            .color_channel = stage.color_chan,
            .color_in = {stage.color.a, stage.color.b, stage.color.c, stage.color.d},
            .color_op = stage.color.op,
            .color_bias = stage.color.bias,
            .color_scale = stage.color.scale,
            .color_clamp = stage.color.clamp,
            .color_out = stage.color.out_reg,
            .alpha_in = {stage.alpha.a, stage.alpha.b, stage.alpha.c, stage.alpha.d},
            .alpha_op = stage.alpha.op,
            .alpha_bias = stage.alpha.bias,
            .alpha_scale = stage.alpha.scale,
            .alpha_clamp = stage.alpha.clamp,
            .alpha_out = stage.alpha.out_reg,
            .k_color_sel = stage.color.k_sel,
            .k_alpha_sel = stage.alpha.k_sel,
            .ras_swap = stage.ras_swap,
            .tex_swap = stage.tex_swap,
            .konst_color = brlyt_stage_konst_color(material, stage),
        };
    }

    [[nodiscard]] std::array< smgpc::render::GxTevRegisterColor2D, 4U > brlyt_initial_tev_registers(const smgpc::render::GXMaterialState& state) {
        return state.tev_registers;
    }

    [[nodiscard]] std::array< std::array< std::uint8_t, 4U >, 4U >
    brlyt_initial_tev_k_colors(const smgpc::layout::BrlytMaterial& material) {
        return material.tev_k_colors;
    }

    [[nodiscard]] smgpc::render::GxAlphaCompare2D brlyt_alpha_compare(const smgpc::layout::BrlytAlphaCompare& alpha_compare) {
        return smgpc::render::GxAlphaCompare2D{
            .comp0 = alpha_compare.comp0,
            .ref0 = alpha_compare.ref0,
            .op = alpha_compare.op,
            .comp1 = alpha_compare.comp1,
            .ref1 = alpha_compare.ref1,
            .enabled = alpha_compare.enabled,
        };
    }

    [[nodiscard]] smgpc::render::GxBlendMode2D brlyt_gx_blend_mode(const smgpc::layout::BrlytMaterial& material) {
        const auto& blend = material.gx_state.blend;
        return smgpc::render::GxBlendMode2D{
            .type = blend.type,
            .src_factor = blend.src_factor,
            .dst_factor = blend.dst_factor,
            .op = blend.op,
            .color_update = blend.color_update,
            .alpha_update = blend.alpha_update,
            .enabled = true,
        };
    }

    [[nodiscard]] std::uint8_t brlan_color_u8(float value) {
        return static_cast< std::uint8_t >(std::clamp(std::lround(value), 0L, 255L));
    }

    [[nodiscard]] std::int16_t brlan_tev_color_s10(float value) {
        return static_cast< std::int16_t >(std::clamp(std::lround(value), -1024L, 1023L));
    }

    void merge_material_frame(smgpc::layout::BrlanMaterialFrame& target, const smgpc::layout::BrlanMaterialFrame& source) {
        for (auto component = std::size_t{}; component < target.material_color.size(); ++component) {
            if (source.material_color[component].has_value()) {
                target.material_color[component] = source.material_color[component];
            }
        }
        for (auto color = std::size_t{}; color < target.tev_colors.size(); ++color) {
            for (auto component = std::size_t{}; component < target.tev_colors[color].size(); ++component) {
                if (source.tev_colors[color][component].has_value()) {
                    target.tev_colors[color][component] = source.tev_colors[color][component];
                }
            }
        }
        for (auto color = std::size_t{}; color < target.tev_k_colors.size(); ++color) {
            for (auto component = std::size_t{}; component < target.tev_k_colors[color].size(); ++component) {
                if (source.tev_k_colors[color][component].has_value()) {
                    target.tev_k_colors[color][component] = source.tev_k_colors[color][component];
                }
            }
        }
    }

    void apply_material_frame(smgpc::layout::BrlytMaterial& material, const smgpc::layout::BrlanMaterialFrame& frame) {
        for (auto component = std::size_t{}; component < material.mat_color.size(); ++component) {
            if (frame.material_color[component].has_value()) {
                material.mat_color[component] = brlan_color_u8(*frame.material_color[component]);
            }
        }
        for (auto color = std::size_t{}; color < material.tev_colors.size(); ++color) {
            for (auto component = std::size_t{}; component < material.tev_colors[color].size(); ++component) {
                if (frame.tev_colors[color][component].has_value()) {
                    material.tev_colors[color][component] = brlan_tev_color_s10(*frame.tev_colors[color][component]);
                }
            }
        }
        for (auto color = std::size_t{}; color < material.tev_k_colors.size(); ++color) {
            for (auto component = std::size_t{}; component < material.tev_k_colors[color].size(); ++component) {
                if (frame.tev_k_colors[color][component].has_value()) {
                    material.tev_k_colors[color][component] = brlan_color_u8(*frame.tev_k_colors[color][component]);
                }
            }
        }

        material.gx_state.color_channels[0U].material_color = material.mat_color;
        for (auto color = std::size_t{}; color < material.tev_colors.size(); ++color) {
            material.gx_state.tev_registers[color + 1U] = material.tev_colors[color];
        }
        material.gx_state.tev_k_colors = material.tev_k_colors;
    }

}  // namespace

SimpleLayout::SimpleLayout(const char* pName, const char* pLayoutName, u32 animLayerNum, int)
    : mName(pName), mLayoutName(pLayoutName), mAnimLayerNum(std::min< u32 >(animLayerNum, 4U)) {
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        runtime->register_layout(*this);
        mArchivePath = runtime->find_layout_archive(mLayoutName);
        if (mArchivePath.has_value()) {
            runtime->note_layout_archive(mLayoutName, *mArchivePath);
        } else {
            runtime->note_missing_layout_archive(mLayoutName);
        }
    }
}

SimpleLayout::~SimpleLayout() {
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        runtime->unregister_effect_keeper(getName());
        runtime->unregister_layout(*this);
    }
}

void SimpleLayout::initWithoutIter() {
}

void SimpleLayout::initEffectKeeper(int effectNum, const char* pEffectName, const void*) {
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        const auto group_name = pEffectName != nullptr ? std::string_view(pEffectName) : std::string_view(mLayoutName);
        runtime->register_effect_keeper(smgpc::runtime::EffectKeeperHostKind::SimpleLayout, mName, effectNum, group_name, false);
    }
}

void SimpleLayout::appear() {
    mIsDead = false;
}

void SimpleLayout::kill() {
    mIsDead = true;
}

void SimpleLayout::update() {
    if (mIsDead) {
        return;
    }

    const auto advance_animation = [](AnimationState& anim) {
        if (anim.name.empty() || anim.stopped) {
            return;
        }

        if (anim.looping) {
            anim.frame += anim.rate;
            if (anim.end > 0.0F && anim.frame >= anim.end) {
                anim.frame = std::fmod(anim.frame, anim.end);
            }
            return;
        }

        anim.frame += anim.rate;
        if (anim.frame >= anim.end || anim.frame <= 0.0F) {
            anim.frame = std::clamp(anim.frame, 0.0F, anim.end);
            anim.stopped = true;
        }
    };

    for (auto& anim : mAnimations) {
        advance_animation(anim);
    }
    for (auto& pane : mPaneAnimations) {
        for (auto& anim : pane.animations) {
            advance_animation(anim);
        }
    }
}

void SimpleLayout::setTrans(f32 x, f32 y) {
    mTransX = x;
    mTransY = y;
}

void SimpleLayout::setScale(f32 x, f32 y) {
    mScaleX = x;
    mScaleY = y;
}

bool SimpleLayout::isDead() const {
    return mIsDead;
}

const std::string& SimpleLayout::getName() const {
    return mName;
}

const std::string& SimpleLayout::getLayoutName() const {
    return mLayoutName;
}

const std::optional< std::filesystem::path >& SimpleLayout::getArchivePath() const {
    return mArchivePath;
}

void SimpleLayout::draw() {
    if (mIsDead) {
        return;
    }

    auto& renderer = smgpc::render::current_aurora_renderer();
    loadRenderData();
    ensureTextureUploads(renderer);

    const auto alpha = visualAlpha();
    if (!mBrlytLayout.drawables.empty()) {
        for (const auto& drawable : mBrlytLayout.drawables) {
            switch (drawable.kind) {
            case smgpc::layout::BrlytDrawableKind::Picture:
                drawPicture(renderer, alpha, drawable.index);
                break;
            case smgpc::layout::BrlytDrawableKind::TextBox:
                drawTextBox(renderer, alpha, drawable.index);
                break;
            case smgpc::layout::BrlytDrawableKind::Window:
                drawWindow(renderer, alpha, drawable.index);
                break;
            }
        }
        return;
    }

    for (std::size_t picture_index = 0U; picture_index < mBrlytLayout.pictures.size(); ++picture_index) {
        drawPicture(renderer, alpha, picture_index);
    }
    for (std::size_t window_index = 0U; window_index < mBrlytLayout.windows.size(); ++window_index) {
        drawWindow(renderer, alpha, window_index);
    }
    drawTextBoxes(renderer, alpha);
}

void SimpleLayout::drawPicture(smgpc::render::AuroraRenderer& renderer, float alpha, std::size_t picture_index) {
    if (picture_index >= mBrlytLayout.pictures.size()) {
        return;
    }

    const auto& picture = mBrlytLayout.pictures[picture_index];
    if (!picture.visible || picture.pane_index >= mBrlytLayout.panes.size()) {
        return;
    }

    const auto& pane = mBrlytLayout.panes[picture.pane_index];
    const auto tex_coord_sets = std::array< std::array< smgpc::layout::BrlytTexCoord, 4U >, 1U >{picture.tex_coords};
    submitLayoutQuad(renderer, alpha, picture.pane_index, picture.material_index, picture.name, picture.vertex_colors,
                     std::span< const std::array< smgpc::layout::BrlytTexCoord, 4U > >(tex_coord_sets.data(), tex_coord_sets.size()),
                     base_position_x(pane.base_position, pane.width), base_position_y(pane.base_position, pane.height), pane.width, pane.height,
                     static_cast< std::uint32_t >(picture_index));
}

void SimpleLayout::submitLayoutQuad(smgpc::render::AuroraRenderer& renderer, float alpha, std::size_t pane_index,
                                    std::uint16_t material_index, std::string_view content_name,
                                    const std::array< std::array< std::uint8_t, 4U >, 4U >& vertex_colors,
                                    std::span< const std::array< smgpc::layout::BrlytTexCoord, 4U > > tex_coord_sets,
                                    float local_left, float local_top, float width, float height, std::uint32_t trace_index) {
    if (pane_index >= mBrlytLayout.panes.size()) {
        return;
    }

    if (material_index >= mBrlytLayout.materials.size()) {
        return;
    }
    auto material = mBrlytLayout.materials[material_index];
    const auto material_frame_name = material.name.empty() ? content_name : std::string_view(material.name);
    apply_material_frame(material, materialFrameForContent(material_frame_name));

    const auto pane_state = paneRenderState(pane_index);
    if (!pane_state.visible) {
        return;
    }

    const auto& pane = mBrlytLayout.panes[pane_index];
    const auto texture_frame = textureFrameForContent(material_frame_name);
    auto raster_color_scale = std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U};
    auto channel_material_color = std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U};
    auto channel_color_src = std::uint8_t{1U};
    auto channel_alpha_src = std::uint8_t{1U};
    const auto vertex_color = [&](std::size_t index) {
        constexpr auto kNw4rQuadColorOrder = std::array< std::size_t, 4U >{0U, 1U, 3U, 2U};
        const auto& source = vertex_colors[kNw4rQuadColorOrder[index]];
        return std::array< std::uint8_t, 4U >{
            static_cast< std::uint8_t >((static_cast< std::uint16_t >(source[0U]) * raster_color_scale[0U]) / 255U),
            static_cast< std::uint8_t >((static_cast< std::uint16_t >(source[1U]) * raster_color_scale[1U]) / 255U),
            static_cast< std::uint8_t >((static_cast< std::uint16_t >(source[2U]) * raster_color_scale[2U]) / 255U),
            static_cast< std::uint8_t >(std::clamp(alpha * pane_state.alpha * (static_cast< float >(source[3U]) / 255.0F) *
                                                       (static_cast< float >(raster_color_scale[3U]) / 255.0F),
                                                   0.0F, 255.0F)),
        };
    };

    auto texture_stages = std::array< smgpc::render::GxTextureStage2D, smgpc::render::core::kMaxGxMaterialTextureStages2D >{};
    auto tev_stages = std::array< smgpc::render::GxTevStage2D, smgpc::render::core::kMaxGxMaterialTevStages2D >{};
    auto tex_coord_gen_indices = std::array< std::uint8_t, smgpc::render::core::kMaxGxMaterialTextureStages2D >{};
    auto texture_stage_wrap_t = std::array< std::uint8_t, smgpc::render::core::kMaxGxMaterialTextureStages2D >{};
    tex_coord_gen_indices.fill(0xffU);
    auto texture_stage_count = std::size_t{};
    auto tev_stage_count = std::size_t{};
#ifndef NDEBUG
    auto debug_texture_bindings = std::vector< smgpc::runtime::RuntimeContext::RenderTextureBindingTrace >{};
#endif

    const auto assign_texture_stage = [&](std::size_t slot, const smgpc::layout::BrlytMaterialTexture& material_texture,
                                          std::uint8_t tex_coord_gen_index) {
        if (slot >= texture_stages.size()) {
            return false;
        }

        auto* texture = find_texture(mRenderTextures, material_texture.texture_name);
        if (texture == nullptr || !texture->handle.is_valid()) {
            return false;
        }

        texture_stages[slot] = smgpc::render::GxTextureStage2D{
            .texture = texture->handle,
            .wrap_u = material_texture.wrap_s,
            .wrap_v = material_texture.wrap_t,
            .min_filter = material_texture.min_filter,
            .mag_filter = material_texture.mag_filter,
        };
        tex_coord_gen_indices[slot] = tex_coord_gen_index;
        texture_stage_wrap_t[slot] = material_texture.wrap_t;
        texture_stage_count = std::max(texture_stage_count, slot + 1U);
#ifndef NDEBUG
        const auto debug_slot = static_cast< std::uint8_t >(slot);
        if (std::ranges::none_of(debug_texture_bindings, [debug_slot](const auto& binding) { return binding.slot == debug_slot; })) {
            debug_texture_bindings.push_back(smgpc::runtime::RuntimeContext::RenderTextureBindingTrace{
                .slot = debug_slot,
                .texture_index = material_texture.texture_index,
                .name = texture->name,
                .width = texture->decoded.width,
                .height = texture->decoded.height,
                .format_raw = static_cast< std::uint32_t >(texture->decoded.format),
                .format_name = texture_format_name(texture->decoded.format),
            });
        }
#endif
        return true;
    };

    const auto bind_texture_map_slot = [&](std::uint8_t texture_map, std::uint8_t tex_coord_gen_index) {
        if (texture_map >= material.textures.size()) {
            return false;
        }

        return assign_texture_stage(texture_map, material.textures[texture_map], tex_coord_gen_index);
    };

    const auto ensure_tex_coord_slot = [&](std::uint8_t tex_coord_gen_index) {
        if (tex_coord_gen_index >= texture_stages.size()) {
            return false;
        }
        if (tex_coord_gen_index < texture_stage_count && texture_stages[tex_coord_gen_index].texture.is_valid()) {
            tex_coord_gen_indices[tex_coord_gen_index] = tex_coord_gen_index;
            return true;
        }
        if (material.textures.empty()) {
            return false;
        }

        return assign_texture_stage(tex_coord_gen_index, material.textures.front(), tex_coord_gen_index);
    };

    const auto reset_material_pipeline = [&]() {
        texture_stages = {};
        tev_stages = {};
        tex_coord_gen_indices.fill(0xffU);
        texture_stage_wrap_t = {};
        texture_stage_count = 0U;
        tev_stage_count = 0U;
#ifndef NDEBUG
        debug_texture_bindings.clear();
#endif
    };

    const auto add_tev_stage = [&](const smgpc::render::GxTevStage2D& stage) {
        if (tev_stage_count >= tev_stages.size()) {
            return false;
        }

        tev_stages[tev_stage_count] = stage;
        ++tev_stage_count;
        return true;
    };

    const auto use_simple_textured_material = [&]() {
        if (material.textures.empty()) {
            return false;
        }

        reset_material_pipeline();
        raster_color_scale = {255U, 255U, 255U, 255U};
        auto can_use = assign_texture_stage(0U, material.textures.front(), 0U);
        if (!tev_stages.empty()) {
            tev_stages[0U] = smgpc::render::GxTevStage2D{
                .texture_stage = 0U,
                .texture_coord_stage = 0U,
                .texture_map_stage = 0U,
                .color_channel = 0xffU,
                .color_in = {2U, 4U, 8U, 15U},
                .color_op = 0U,
                .color_bias = 0U,
                .color_scale = 0U,
                .color_clamp = true,
                .color_out = 0U,
                .alpha_in = {1U, 2U, 4U, 7U},
                .alpha_op = 0U,
                .alpha_bias = 0U,
                .alpha_scale = 0U,
                .alpha_clamp = true,
                .alpha_out = 0U,
            };
            tev_stage_count = 1U;
        } else {
            can_use = false;
        }
        return can_use;
    };

    const auto default_konst_selector = [](std::size_t texture_map_index) {
        constexpr auto kSelectors = std::array< std::uint8_t, 8U >{0x1fU, 0x1bU, 0x17U, 0x13U, 0x1eU, 0x1aU, 0x16U, 0x12U};
        return kSelectors[std::min(texture_map_index, kSelectors.size() - 1U)];
    };

    const auto default_konst_color = [&](std::size_t texture_map_index) {
        const auto selector = default_konst_selector(texture_map_index);
        const auto color = brlyt_konst_color(material, selector);
        const auto alpha_color = brlyt_konst_color(material, selector);
        return std::array< std::uint8_t, 4U >{color[0U], color[1U], color[2U], alpha_color[3U]};
    };

    const auto brlyt_default_stage_texture_replace = [](std::uint8_t texture_stage) {
        return smgpc::render::GxTevStage2D{
            .texture_stage = texture_stage,
            .texture_coord_stage = texture_stage,
            .texture_map_stage = texture_stage,
            .color_channel = 0xffU,
            .color_in = {15U, 15U, 15U, 8U},
            .color_op = 0U,
            .color_bias = 0U,
            .color_scale = 0U,
            .color_clamp = true,
            .color_out = 0U,
            .alpha_in = {7U, 7U, 7U, 4U},
            .alpha_op = 0U,
            .alpha_bias = 0U,
            .alpha_scale = 0U,
            .alpha_clamp = true,
            .alpha_out = 0U,
        };
    };

    const auto brlyt_default_stage_texture_lerp = [&](std::uint8_t texture_stage) {
        return smgpc::render::GxTevStage2D{
            .texture_stage = texture_stage,
            .texture_coord_stage = texture_stage,
            .texture_map_stage = texture_stage,
            .color_channel = 0xffU,
            .color_in = {8U, 0U, 14U, 15U},
            .color_op = 0U,
            .color_bias = 0U,
            .color_scale = 0U,
            .color_clamp = true,
            .color_out = 0U,
            .alpha_in = {4U, 0U, 6U, 7U},
            .alpha_op = 0U,
            .alpha_bias = 0U,
            .alpha_scale = 0U,
            .alpha_clamp = true,
            .alpha_out = 0U,
            .k_color_sel = default_konst_selector(texture_stage - 1U),
            .k_alpha_sel = default_konst_selector(texture_stage - 1U),
            .konst_color = default_konst_color(texture_stage - 1U),
        };
    };

    const auto brlyt_default_stage_texture_modulate = [&](std::uint8_t texture_stage) {
        return smgpc::render::GxTevStage2D{
            .texture_stage = texture_stage,
            .texture_coord_stage = texture_stage,
            .texture_map_stage = texture_stage,
            .color_channel = 0xffU,
            .color_in = {15U, 8U, 14U, static_cast< std::uint8_t >(texture_stage == 0U ? 15U : 0U)},
            .color_op = 0U,
            .color_bias = 0U,
            .color_scale = 0U,
            .color_clamp = true,
            .color_out = 0U,
            .alpha_in = {7U, 4U, 6U, static_cast< std::uint8_t >(texture_stage == 0U ? 7U : 0U)},
            .alpha_op = 0U,
            .alpha_bias = 0U,
            .alpha_scale = 0U,
            .alpha_clamp = true,
            .alpha_out = 0U,
            .k_color_sel = default_konst_selector(texture_stage),
            .k_alpha_sel = default_konst_selector(texture_stage),
            .konst_color = default_konst_color(texture_stage),
        };
    };

    const auto brlyt_default_stage_tev_color = [] {
        return smgpc::render::GxTevStage2D{
            .texture_stage = 0xffU,
            .texture_coord_stage = 0xffU,
            .texture_map_stage = 0xffU,
            .color_channel = 0xffU,
            .color_in = {2U, 4U, 0U, 15U},
            .color_op = 0U,
            .color_bias = 0U,
            .color_scale = 0U,
            .color_clamp = true,
            .color_out = 0U,
            .alpha_in = {1U, 2U, 0U, 7U},
            .alpha_op = 0U,
            .alpha_bias = 0U,
            .alpha_scale = 0U,
            .alpha_clamp = true,
            .alpha_out = 0U,
        };
    };

    const auto build_default_brlyt_material = [&]() {
        if (material.textures.size() > texture_stages.size()) {
            return false;
        }

        reset_material_pipeline();
        for (auto slot = std::size_t{}; slot < material.textures.size(); ++slot) {
            if (!assign_texture_stage(slot, material.textures[slot], static_cast< std::uint8_t >(slot))) {
                return false;
            }
        }

        const auto use_vertex_color =
            !material.has_chan_ctrl || material.chan_color_src == 1U || material.chan_alpha_src == 1U;
        const auto use_material_color =
            material.has_chan_ctrl && (material.chan_color_src == 0U || material.chan_alpha_src == 0U);
        channel_color_src = material.has_chan_ctrl ? material.chan_color_src : std::uint8_t{1U};
        channel_alpha_src = material.has_chan_ctrl ? material.chan_alpha_src : std::uint8_t{1U};
        channel_material_color = material.has_mat_color ? material.mat_color : std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U};
        if (use_material_color) {
            channel_material_color[3U] = static_cast< std::uint8_t >(std::clamp(
                (static_cast< float >(channel_material_color[3U]) * alpha * pane_state.alpha) / 255.0F, 0.0F, 255.0F));
        }
        const auto vertex_color_modulates =
            std::ranges::any_of(vertex_colors, [](const auto& color) {
                return color[0U] != 255U || color[1U] != 255U || color[2U] != 255U || color[3U] != 255U;
            }) ||
            (alpha * pane_state.alpha) < 254.5F;
        const auto material_color_modulates = material.mat_color != std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U};
        const auto use_raster_stage = (use_vertex_color && vertex_color_modulates) || (use_material_color && material_color_modulates);
        raster_color_scale = {255U, 255U, 255U, 255U};

        if (material.textures.empty()) {
            return add_tev_stage(smgpc::render::GxTevStage2D{
                .texture_stage = 0xffU,
                .texture_coord_stage = 0xffU,
                .texture_map_stage = 0xffU,
                .color_channel = 4U,
                .color_in = {15U, 4U, 10U, 15U},
                .color_op = 0U,
                .color_bias = 0U,
                .color_scale = 0U,
                .color_clamp = true,
                .color_out = 0U,
                .alpha_in = {7U, 2U, 5U, 7U},
                .alpha_op = 0U,
                .alpha_bias = 0U,
                .alpha_scale = 0U,
                .alpha_clamp = true,
                .alpha_out = 0U,
            });
        }
        if (material.textures.size() == 1U) {
            auto stage = smgpc::render::GxTevStage2D{
                .texture_stage = 0U,
                .texture_coord_stage = 0U,
                .texture_map_stage = 0U,
                .color_channel = 0xffU,
                .color_in = {2U, 4U, 8U, 15U},
                .color_op = 0U,
                .color_bias = 0U,
                .color_scale = 0U,
                .color_clamp = true,
                .color_out = 0U,
                .alpha_in = {1U, 2U, 4U, 7U},
                .alpha_op = 0U,
                .alpha_bias = 0U,
                .alpha_scale = 0U,
                .alpha_clamp = true,
                .alpha_out = 0U,
            };
            return add_tev_stage(stage) && (!use_raster_stage || add_tev_stage(smgpc::render::gx_brlyt_default_raster_modulate_stage()));
        }

        if (material.textures.size() == 2U) {
            auto can_build = add_tev_stage(brlyt_default_stage_texture_replace(0U));
            can_build = can_build && add_tev_stage(brlyt_default_stage_texture_lerp(1U));
            const auto has_animated_tev_color = material.tev_colors[0U] != smgpc::render::GXTevRegisterColor{0, 0, 0, 0} ||
                                                material.tev_colors[1U] != smgpc::render::GXTevRegisterColor{255, 255, 255, 255};
            if (has_animated_tev_color) {
                can_build = can_build && add_tev_stage(brlyt_default_stage_tev_color());
            }
            if (use_raster_stage) {
                can_build = can_build && add_tev_stage(smgpc::render::gx_brlyt_default_raster_modulate_stage());
            }
            return can_build;
        } else {
            auto can_build = true;
            for (auto slot = 0U; slot < material.textures.size(); ++slot) {
                can_build = can_build && add_tev_stage(brlyt_default_stage_texture_modulate(static_cast< std::uint8_t >(slot)));
            }

            const auto has_animated_tev_color = material.tev_colors[0U] != smgpc::render::GXTevRegisterColor{0, 0, 0, 0} ||
                                                material.tev_colors[1U] != smgpc::render::GXTevRegisterColor{255, 255, 255, 255};
            if (has_animated_tev_color) {
                can_build = can_build && add_tev_stage(brlyt_default_stage_tev_color());
            }
            if (use_raster_stage) {
                can_build = can_build && add_tev_stage(smgpc::render::gx_brlyt_default_raster_modulate_stage());
            }
            return can_build;
        }
    };

    {
        const auto use_material_color =
            material.has_chan_ctrl && (material.chan_color_src == 0U || material.chan_alpha_src == 0U);
        channel_color_src = material.has_chan_ctrl ? material.chan_color_src : std::uint8_t{1U};
        channel_alpha_src = material.has_chan_ctrl ? material.chan_alpha_src : std::uint8_t{1U};
        channel_material_color = material.has_mat_color ? material.mat_color : std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U};
        if (use_material_color) {
            channel_material_color[3U] = static_cast< std::uint8_t >(std::clamp(
                (static_cast< float >(channel_material_color[3U]) * alpha * pane_state.alpha) / 255.0F, 0.0F, 255.0F));
        }
    }

    auto can_submit = true;
    if (material.tev_stages.empty()) {
        can_submit = build_default_brlyt_material();
    } else if (!brlyt_material_can_use_gx_shader(material)) {
        can_submit = false;
    } else {
        for (const auto& stage : material.tev_stages) {
            auto texture_coord_stage = std::uint8_t{0xffU};
            auto texture_map_stage = std::uint8_t{0xffU};
            if (brlyt_tev_stage_uses_texture(stage)) {
                if (stage.tex_map > UINT8_MAX) {
                    can_submit = false;
                } else {
                    texture_coord_stage = stage.tex_coord_gen;
                    texture_map_stage = static_cast< std::uint8_t >(stage.tex_map);
                    can_submit = can_submit && bind_texture_map_slot(texture_map_stage, texture_coord_stage);
                    can_submit = can_submit && ensure_tex_coord_slot(texture_coord_stage);
                }
            }
            if (tev_stage_count < tev_stages.size()) {
                tev_stages[tev_stage_count] = brlyt_gx_tev_stage(material, stage, texture_coord_stage, texture_map_stage);
                ++tev_stage_count;
            }
        }
    }
    const auto submitted_tev_uses_texture = std::ranges::any_of(std::span< const smgpc::render::GxTevStage2D >(tev_stages.data(), tev_stage_count),
                                                                [](const auto& stage) { return stage.texture_map_stage != 0xffU; });
    if ((!can_submit && !use_simple_textured_material()) || (submitted_tev_uses_texture && texture_stage_count == 0U) || tev_stage_count == 0U) {
        return;
    }
    if (!material.textures.empty()) {
        for (auto slot = std::size_t{}; slot < texture_stage_count; ++slot) {
            if (!texture_stages[slot].texture.is_valid() && !assign_texture_stage(slot, material.textures.front(), static_cast< std::uint8_t >(slot))) {
                return;
            }
        }
    }

    const auto tex_coord = [&](std::size_t vertex_index, std::size_t texture_stage_index) {
        const auto tex_coord_gen_index = tex_coord_gen_indices[texture_stage_index];
        const auto coord_set_index = tex_coord_gen_index < tex_coord_sets.size() ? tex_coord_gen_index : std::size_t{};
        auto coord = tex_coord_sets.empty() ? default_quad_tex_coords()[vertex_index] : tex_coord_sets[coord_set_index][vertex_index];
        if (tex_coord_gen_index < material.tex_coord_gens.size()) {
            const auto tex_srt_index = tex_srt_index_for_coord_gen(material.tex_coord_gens[tex_coord_gen_index]);
            if (tex_srt_index != UINT16_MAX && tex_srt_index < material.tex_srts.size()) {
                coord = transform_tex_coord(coord, material.tex_srts[tex_srt_index], texture_frame, tex_srt_index == 0U);
            }
        } else {
            coord = transform_tex_coord(coord, texture_frame);
        }
        return std::array< float, 3U >{coord.u, brlyt_texture_v_to_host(coord.v, texture_stage_wrap_t[texture_stage_index]), 1.0F};
    };

    const auto vertex = [&](std::size_t index, float vx, float vy) {
        const auto position = panePointForAurora(pane_state, vx, vy);
        auto tex_coords = std::array< std::array< float, 3U >, smgpc::render::core::kMaxGxMaterialTextureStages2D >{};
        for (auto stage = std::size_t{}; stage < texture_stage_count && stage < tex_coords.size(); ++stage) {
            tex_coords[stage] = tex_coord(index, stage);
        }
        return smgpc::render::GxMaterialVertex2D{
            .x = position[0U],
            .y = position[1U],
            .z = 0.0F,
            .clip_w = 1.0F,
            .tex_coords = tex_coords,
            .color = vertex_color(index),
        };
    };

    const auto vertices = std::array< smgpc::render::GxMaterialVertex2D, 4U >{
        vertex(0U, local_left, local_top),
        vertex(1U, local_left + width, local_top),
        vertex(2U, local_left + width, local_top + height),
        vertex(3U, local_left, local_top + height),
    };
    constexpr auto indices = std::array< std::uint16_t, 6U >{0U, 1U, 2U, 0U, 2U, 3U};
#ifndef NDEBUG
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && runtime->should_record_render_packet_trace()) {
        runtime->record_layout_packet_trace(smgpc::runtime::RuntimeContext::LayoutRuntimePacketTrace{
            .layout_name = mLayoutName,
            .pane_name = pane.name,
            .material_name = material.name.empty() ? std::string(content_name) : material.name,
            .frame_index = runtime->frame_index(),
            .picture_index = trace_index,
            .material_index = material_index,
            .vertex_count = static_cast< std::uint32_t >(vertices.size()),
            .index_count = static_cast< std::uint32_t >(indices.size()),
            .texgen_count = static_cast< std::uint32_t >(texture_stage_count),
            .tev_stage_count = static_cast< std::uint32_t >(tev_stage_count),
            .alpha_compare_enabled = material.alpha_compare.enabled,
            .blend_enabled = material.blend_mode.enabled,
            .cull_mode = smgpc::render::CullMode::None,
            .texture_bindings = debug_texture_bindings,
        });
    }
#endif
    renderer.submit_gx_material_triangles(smgpc::render::GxMaterialTriangleBatch2D{
        .vertices = std::span< const smgpc::render::GxMaterialVertex2D >(vertices.data(), vertices.size()),
        .indices = std::span< const std::uint16_t >(indices.data(), indices.size()),
        .texture_stages = std::span< const smgpc::render::GxTextureStage2D >(texture_stages.data(), texture_stage_count),
        .tev_stages = std::span< const smgpc::render::GxTevStage2D >(tev_stages.data(), tev_stage_count),
        .initial_tev_registers = brlyt_initial_tev_registers(material.gx_state),
        .initial_tev_k_colors = brlyt_initial_tev_k_colors(material),
        .has_initial_tev_k_colors = true,
        .channel_material_color = channel_material_color,
        .channel_color_src = channel_color_src,
        .channel_alpha_src = channel_alpha_src,
        .alpha_compare = brlyt_alpha_compare(material.alpha_compare),
        .blend = brlyt_gx_blend_mode(material),
    });
}

void SimpleLayout::drawWindow(smgpc::render::AuroraRenderer& renderer, float alpha, std::size_t window_index) {
    if (window_index >= mBrlytLayout.windows.size()) {
        return;
    }

    const auto& window = mBrlytLayout.windows[window_index];
    if (!window.visible || window.pane_index >= mBrlytLayout.panes.size()) {
        return;
    }

    const auto& pane = mBrlytLayout.panes[window.pane_index];
    const auto base_left = base_position_x(pane.base_position, pane.width);
    const auto base_top = base_position_y(pane.base_position, pane.height);
    const auto white_vertex_colors = std::array< std::array< std::uint8_t, 4U >, 4U >{
        std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U},
        std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U},
        std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U},
        std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U},
    };

    const auto material_texture_size = [&](std::uint16_t material_index) {
        auto size = LayoutTextureSize{};
        if (material_index >= mBrlytLayout.materials.size() || mBrlytLayout.materials[material_index].textures.empty()) {
            return size;
        }
        const auto& texture_name = mBrlytLayout.materials[material_index].textures.front().texture_name;
        if (auto* texture = find_texture(mRenderTextures, texture_name); texture != nullptr) {
            size.width = static_cast< float >(std::max<std::uint16_t>(texture->decoded.width, 1U));
            size.height = static_cast< float >(std::max<std::uint16_t>(texture->decoded.height, 1U));
        }
        return size;
    };

    const auto frame_size_for = [&]() {
        auto frame_size = LayoutWindowFrameSize{};
        if (window.frames.empty()) {
            return frame_size;
        }
        if (window.frames.size() == 1U) {
            const auto size = material_texture_size(window.frames[0U].material_index);
            frame_size.left = size.width;
            frame_size.right = size.width;
            frame_size.top = size.height;
            frame_size.bottom = size.height;
            return frame_size;
        }
        const auto top_left_size = material_texture_size(window.frames[0U].material_index);
        const auto bottom_right_size = material_texture_size(window.frames[std::min<std::size_t>(3U, window.frames.size() - 1U)].material_index);
        frame_size.left = top_left_size.width;
        frame_size.top = top_left_size.height;
        frame_size.right = bottom_right_size.width;
        frame_size.bottom = bottom_right_size.height;
        return frame_size;
    };

    const auto frame_size = frame_size_for();
    auto content_tex_coords = layout_window_content_tex_coord_sets(window.content.tex_coord_sets);
    submitLayoutQuad(renderer, alpha, window.pane_index, window.content.material_index, window.name, window.content.vertex_colors,
                     std::span< const std::array< smgpc::layout::BrlytTexCoord, 4U > >(content_tex_coords.data(), content_tex_coords.size()),
                     base_left + frame_size.left - window.content_inflation.left,
                     base_top + frame_size.top - window.content_inflation.top,
                     window.width - frame_size.left + window.content_inflation.left - frame_size.right + window.content_inflation.right,
                     window.height - frame_size.top + window.content_inflation.top - frame_size.bottom + window.content_inflation.bottom,
                     static_cast< std::uint32_t >(0x80000000U | window_index));

    const auto draw_frame_quad = [&](std::size_t frame_index, float left, float top, float width, float height,
                                     std::array< smgpc::layout::BrlytTexCoord, 4U > tex_coords) {
        if (frame_index >= window.frames.size() || width <= 0.0F || height <= 0.0F) {
            return;
        }
        const auto& frame = window.frames[frame_index];
        if (frame.material_index >= mBrlytLayout.materials.size() || mBrlytLayout.materials[frame.material_index].textures.empty()) {
            return;
        }
        const auto tex_coord_sets = std::array< std::array< smgpc::layout::BrlytTexCoord, 4U >, 1U >{tex_coords};
        submitLayoutQuad(renderer, alpha, window.pane_index, frame.material_index, window.name, white_vertex_colors,
                         std::span< const std::array< smgpc::layout::BrlytTexCoord, 4U > >(tex_coord_sets.data(), tex_coord_sets.size()),
                         left, top, width, height, static_cast< std::uint32_t >(0x81000000U | (window_index << 8U) | frame_index));
    };

    if (window.frames.size() == 1U) {
        const auto size = material_texture_size(window.frames[0U].material_index);
        draw_frame_quad(0U, base_left, base_top, window.width - frame_size.right, frame_size.top,
                        layout_window_frame_tex_coords_lt(window.frames[0U].texture_flip, size, window.width - frame_size.right, frame_size.top));
        draw_frame_quad(0U, base_left + window.width - frame_size.right, base_top, frame_size.right, window.height - frame_size.bottom,
                        layout_window_frame_tex_coords_rt(window.frames[0U].texture_flip, size, frame_size.right, window.height - frame_size.bottom));
        draw_frame_quad(0U, base_left + frame_size.left, base_top + window.height - frame_size.bottom, window.width - frame_size.left,
                        frame_size.bottom, layout_window_frame_tex_coords_rb(window.frames[0U].texture_flip, size, window.width - frame_size.left,
                                                                            frame_size.bottom));
        draw_frame_quad(0U, base_left, base_top + frame_size.top, frame_size.left, window.height - frame_size.top,
                        layout_window_frame_tex_coords_lb(window.frames[0U].texture_flip, size, frame_size.left, window.height - frame_size.top));
        return;
    }

    if (window.frames.size() >= 4U) {
        auto size = material_texture_size(window.frames[0U].material_index);
        draw_frame_quad(0U, base_left, base_top, window.width - frame_size.right, frame_size.top,
                        layout_window_frame_tex_coords_lt(window.frames[0U].texture_flip, size, window.width - frame_size.right, frame_size.top));
        size = material_texture_size(window.frames[1U].material_index);
        draw_frame_quad(1U, base_left + window.width - frame_size.right, base_top, frame_size.right, window.height - frame_size.bottom,
                        layout_window_frame_tex_coords_rt(window.frames[1U].texture_flip, size, frame_size.right, window.height - frame_size.bottom));
        size = material_texture_size(window.frames[3U].material_index);
        draw_frame_quad(3U, base_left + frame_size.left, base_top + window.height - frame_size.bottom, window.width - frame_size.left,
                        frame_size.bottom, layout_window_frame_tex_coords_rb(window.frames[3U].texture_flip, size, window.width - frame_size.left,
                                                                            frame_size.bottom));
        size = material_texture_size(window.frames[2U].material_index);
        draw_frame_quad(2U, base_left, base_top + frame_size.top, frame_size.left, window.height - frame_size.top,
                        layout_window_frame_tex_coords_lb(window.frames[2U].texture_flip, size, frame_size.left, window.height - frame_size.top));
    }

    if (window.frames.size() >= 8U) {
        auto size = material_texture_size(window.frames[6U].material_index);
        draw_frame_quad(6U, base_left + frame_size.left, base_top, window.width - frame_size.left - frame_size.right, frame_size.top,
                        layout_window_frame_tex_coords_lt(window.frames[6U].texture_flip, size, window.width - frame_size.left - frame_size.right,
                                                          frame_size.top));
        size = material_texture_size(window.frames[5U].material_index);
        draw_frame_quad(5U, base_left + window.width - frame_size.right, base_top + frame_size.top, frame_size.right,
                        window.height - frame_size.top - frame_size.bottom,
                        layout_window_frame_tex_coords_rt(window.frames[5U].texture_flip, size, frame_size.right,
                                                          window.height - frame_size.top - frame_size.bottom));
        size = material_texture_size(window.frames[7U].material_index);
        draw_frame_quad(7U, base_left + frame_size.left, base_top + window.height - frame_size.bottom,
                        window.width - frame_size.left - frame_size.right, frame_size.bottom,
                        layout_window_frame_tex_coords_rb(window.frames[7U].texture_flip, size, window.width - frame_size.left - frame_size.right,
                                                          frame_size.bottom));
        size = material_texture_size(window.frames[4U].material_index);
        draw_frame_quad(4U, base_left, base_top + frame_size.top, frame_size.left, window.height - frame_size.top - frame_size.bottom,
                        layout_window_frame_tex_coords_lb(window.frames[4U].texture_flip, size, frame_size.left,
                                                          window.height - frame_size.top - frame_size.bottom));
    }
}

void SimpleLayout::startAnim(const char* pAnimName, u32 animLayer) {
    loadRenderData();
    auto& anim = animation(animLayer);
    commitAnimationState(anim);
    anim.name = pAnimName;
    anim.frame = 0.0f;
    anim.end = durationFor(pAnimName);
    anim.rate = 1.0f;
    anim.looping = isLoopingAnim(pAnimName);
    anim.stopped = false;
}

void SimpleLayout::setAnimFrameAndStop(f32 frame, u32 animLayer) {
    auto& anim = animation(animLayer);
    anim.frame = frame;
    anim.rate = 0.0f;
    anim.stopped = true;
}

void SimpleLayout::setAnimFrame(f32 frame, u32 animLayer) {
    animation(animLayer).frame = frame;
}

void SimpleLayout::setAnimRate(f32 rate, u32 animLayer) {
    auto& anim = animation(animLayer);
    anim.rate = rate;
    anim.stopped = rate == 0.0f;
}

f32 SimpleLayout::getAnimFrame(u32 animLayer) const {
    return animation(animLayer).frame;
}

bool SimpleLayout::isAnimStopped(u32 animLayer) {
    auto& anim = animation(animLayer);
    return anim.stopped;
}

void SimpleLayout::setTextBoxNumberRecursive(const char* pPaneName, s32 number) {
    loadRenderData();

    auto text = std::vector< std::uint16_t >{};
    const auto digits = std::to_string(number);
    text.reserve(digits.size());
    for (const auto digit : digits) {
        text.push_back(static_cast< std::uint16_t >(digit));
    }

    const auto requested_name = pPaneName != nullptr ? std::string_view(pPaneName) : std::string_view{};
    for (auto& text_box : mBrlytLayout.text_boxes) {
        if (text_box_matches_recursive(mBrlytLayout, text_box, requested_name)) {
            text_box.text = text;
            mTextBoxTemplates.erase(text_box.name);
        }
    }

    mRenderTextTextures.clear();
}

void SimpleLayout::setTextBoxStringRecursive(const char* pPaneName, std::u16string_view text) {
    loadRenderData();

    auto encoded = std::vector< std::uint16_t >{};
    encoded.reserve(text.size());
    for (const auto code : text) {
        encoded.push_back(static_cast< std::uint16_t >(code));
    }

    const auto requested_name = pPaneName != nullptr ? std::string_view(pPaneName) : std::string_view{};
    for (auto& text_box : mBrlytLayout.text_boxes) {
        if (text_box_matches_recursive(mBrlytLayout, text_box, requested_name)) {
            text_box.text = encoded;
            mTextBoxTemplates.erase(text_box.name);
        }
    }

    mRenderTextTextures.clear();
}

void SimpleLayout::setTextBoxTaggedStringRecursive(const char* pPaneName, std::u16string_view rawText, std::u16string_view displayText) {
    loadRenderData();

    const auto requested_name = pPaneName != nullptr ? std::string_view(pPaneName) : std::string_view{};
    const auto formatted = smgpc::resource::format_bmg_text(rawText, {});
    const auto text = !formatted.empty() || rawText.empty() ? std::u16string_view(formatted) : displayText;

    auto encoded = std::vector< std::uint16_t >{};
    encoded.reserve(text.size());
    for (const auto code : text) {
        encoded.push_back(static_cast< std::uint16_t >(code));
    }

    for (auto& text_box : mBrlytLayout.text_boxes) {
        if (text_box_matches_recursive(mBrlytLayout, text_box, requested_name)) {
            text_box.text = encoded;
            mTextBoxTemplates[text_box.name] = TextBoxTemplateState{
                .raw_text = std::u16string(rawText),
                .args = {},
            };
        }
    }

    mRenderTextTextures.clear();
}

void SimpleLayout::setTextBoxArgNumberRecursive(const char* pPaneName, s32 number, s32 argIndex) {
    loadRenderData();
    if (argIndex < 0) {
        return;
    }

    const auto requested_name = pPaneName != nullptr ? std::string_view(pPaneName) : std::string_view{};
    for (auto& text_box : mBrlytLayout.text_boxes) {
        if (!text_box_matches_recursive(mBrlytLayout, text_box, requested_name)) {
            continue;
        }

        const auto found = mTextBoxTemplates.find(text_box.name);
        if (found == mTextBoxTemplates.end()) {
            continue;
        }

        auto& state = found->second;
        const auto index = static_cast< std::size_t >(argIndex);
        if (state.args.size() <= index) {
            state.args.resize(index + 1U);
        }
        state.args[index] = smgpc::resource::BmgFormatArg::number(number);

        const auto formatted = smgpc::resource::format_bmg_text(state.raw_text, state.args);
        text_box.text.assign(formatted.begin(), formatted.end());
    }

    mRenderTextTextures.clear();
}

void SimpleLayout::setTextBoxArgStringRecursive(const char* pPaneName, std::u16string_view text, s32 argIndex) {
    loadRenderData();
    if (argIndex < 0) {
        return;
    }

    const auto requested_name = pPaneName != nullptr ? std::string_view(pPaneName) : std::string_view{};
    for (auto& text_box : mBrlytLayout.text_boxes) {
        if (!text_box_matches_recursive(mBrlytLayout, text_box, requested_name)) {
            continue;
        }

        const auto found = mTextBoxTemplates.find(text_box.name);
        if (found == mTextBoxTemplates.end()) {
            continue;
        }

        auto& state = found->second;
        const auto index = static_cast< std::size_t >(argIndex);
        if (state.args.size() <= index) {
            state.args.resize(index + 1U);
        }
        state.args[index] = smgpc::resource::BmgFormatArg::string(text);

        const auto formatted = smgpc::resource::format_bmg_text(state.raw_text, state.args);
        text_box.text.assign(formatted.begin(), formatted.end());
    }

    mRenderTextTextures.clear();
}

void SimpleLayout::replacePaneTexture(std::string_view paneName, const nw4r::lyt::TexMap& texMap, u8 texMapIndex) {
    loadRenderData();
    if (paneName.empty()) {
        return;
    }

    auto* render_texture = find_texture(mRenderTextures, texMap.name());
    if (render_texture == nullptr) {
        mRenderTextures.push_back(RenderTexture{
            .name = texMap.name(),
            .decoded = texMap.image(),
            .handle = {},
        });
    } else {
        render_texture->decoded = texMap.image();
        render_texture->handle = {};
    }

    const auto replace_material_texture = [&](std::uint16_t material_index) {
        if (material_index >= mBrlytLayout.materials.size()) {
            return false;
        }

        auto& material = mBrlytLayout.materials[material_index];
        if (texMapIndex >= material.textures.size()) {
            return false;
        }

        auto& material_texture = material.textures[texMapIndex];
        material_texture.texture_name = texMap.name();
        material_texture.wrap_s = texMap.wrap_s();
        material_texture.wrap_t = texMap.wrap_t();
        material_texture.min_filter = texMap.min_filter();
        material_texture.mag_filter = texMap.mag_filter();
        if (texMapIndex < material.gx_state.textures.size()) {
            material.gx_state.textures[texMapIndex].wrap_s = texMap.wrap_s();
            material.gx_state.textures[texMapIndex].wrap_t = texMap.wrap_t();
            material.gx_state.textures[texMapIndex].min_filter = texMap.min_filter();
            material.gx_state.textures[texMapIndex].mag_filter = texMap.mag_filter();
        }

        return true;
    };

    for (auto& picture : mBrlytLayout.pictures) {
        const auto matches_picture = pane_name_matches_request(picture.name, paneName);
        const auto matches_pane = picture.pane_index < mBrlytLayout.panes.size() &&
                                  pane_name_matches_request(mBrlytLayout.panes[picture.pane_index].name, paneName);
        if (!matches_picture && !matches_pane) {
            continue;
        }

        if (replace_material_texture(picture.material_index) && texMapIndex == 0U) {
            picture.texture_name = texMap.name();
            picture.wrap_s = texMap.wrap_s();
            picture.wrap_t = texMap.wrap_t();
            picture.min_filter = texMap.min_filter();
            picture.mag_filter = texMap.mag_filter();
        }
    }

    for (auto material_index = std::uint16_t{}; material_index < mBrlytLayout.materials.size(); ++material_index) {
        if (pane_name_matches_request(mBrlytLayout.materials[material_index].name, paneName)) {
            replace_material_texture(material_index);
        }
    }
}

void SimpleLayout::setPaneAlpha(std::string_view paneName, f32 alpha) {
    loadRenderData();
    if (paneName.empty()) {
        return;
    }

    const auto alpha_u8 = std::clamp(alpha, 0.0F, 1.0F) * 255.0F;
    auto matched = false;
    for (const auto& pane : mBrlytLayout.panes) {
        if (pane_name_matches_request(pane.name, paneName)) {
            mPaneAlphaOverrides[pane.name] = alpha_u8;
            matched = true;
        }
    }
    if (!matched) {
        mPaneAlphaOverrides[std::string(paneName)] = alpha_u8;
    }
}

void SimpleLayout::setPaneVisible(std::string_view paneName, bool visible) {
    loadRenderData();
    if (paneName.empty()) {
        return;
    }

    auto matched = false;
    for (const auto& pane : mBrlytLayout.panes) {
        if (pane_name_matches_request(pane.name, paneName)) {
            mPaneVisibilityOverrides[pane.name] = visible;
            matched = true;
        }
    }
    if (!matched) {
        mPaneVisibilityOverrides[std::string(paneName)] = visible;
    }
}

void SimpleLayout::setPaneVisibleRecursive(std::string_view paneName, bool visible) {
    loadRenderData();
    if (paneName.empty()) {
        return;
    }

    auto root_indices = std::vector< std::size_t >{};
    for (auto i = std::size_t{}; i < mBrlytLayout.panes.size(); ++i) {
        if (pane_name_matches_request(mBrlytLayout.panes[i].name, paneName)) {
            root_indices.push_back(i);
        }
    }

    auto is_descendant_of = [&](std::size_t pane_index, std::size_t root_index) {
        auto current = static_cast< int >(pane_index);
        while (current >= 0) {
            if (static_cast< std::size_t >(current) == root_index) {
                return true;
            }
            current = mBrlytLayout.panes[static_cast< std::size_t >(current)].parent_index;
        }
        return false;
    };

    for (auto i = std::size_t{}; i < mBrlytLayout.panes.size(); ++i) {
        if (std::ranges::any_of(root_indices, [&](std::size_t root_index) { return is_descendant_of(i, root_index); })) {
            mPaneVisibilityOverrides[mBrlytLayout.panes[i].name] = visible;
        }
    }
}

void SimpleLayout::setTextBoxHorizontalPosition(std::string_view paneName, u8 position) {
    loadRenderData();
    const auto requested_name = paneName;
    for (auto& text_box : mBrlytLayout.text_boxes) {
        if (!text_box_matches_recursive(mBrlytLayout, text_box, requested_name)) {
            continue;
        }

        text_box.text_position = static_cast< std::uint8_t >((text_box.text_position / 3U) * 3U + std::min< u8 >(position, static_cast< u8 >(2U)));
    }
}

void SimpleLayout::setTextBoxVerticalPosition(std::string_view paneName, u8 position) {
    loadRenderData();
    const auto requested_name = paneName;
    for (auto& text_box : mBrlytLayout.text_boxes) {
        if (!text_box_matches_recursive(mBrlytLayout, text_box, requested_name)) {
            continue;
        }

        text_box.text_position = static_cast< std::uint8_t >(std::min< u8 >(position, static_cast< u8 >(2U)) * 3U + text_box.text_position % 3U);
    }
}

bool SimpleLayout::isPaneVisible(std::string_view paneName) const {
    const_cast< SimpleLayout* >(this)->loadRenderData();
    if (paneName.empty()) {
        return !mIsDead;
    }

    if (const auto pane_index = find_preferred_pane_index(mBrlytLayout, paneName)) {
        return paneRenderState(*pane_index).visible;
    }

    return true;
}

bool SimpleLayout::hasPane(std::string_view paneName) const {
    if (paneName.empty()) {
        return true;
    }

    return find_preferred_pane_index(mBrlytLayout, paneName).has_value();
}

std::optional< SimpleLayout::PaneBounds > SimpleLayout::paneBounds(std::string_view paneName) const {
    const_cast< SimpleLayout* >(this)->loadRenderData();
    if (mIsDead) {
        return std::nullopt;
    }

    if (paneName.empty()) {
        const auto half_width = static_cast< f32 >(smgpc::render::core::kWiiLayoutWidth) * 0.5F;
        const auto half_height = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F;
        return PaneBounds{
            .left = -half_width,
            .top = -half_height,
            .right = half_width,
            .bottom = half_height,
        };
    }

    const auto pane_index = find_preferred_pane_index(mBrlytLayout, paneName);
    if (!pane_index.has_value()) {
        return std::nullopt;
    }

    const auto& pane = mBrlytLayout.panes[*pane_index];
    const auto pane_state = paneRenderState(*pane_index);
    if (!pane_state.visible || pane_state.alpha <= 0.0F) {
        return std::nullopt;
    }

    if (pane.width == 0.0F || pane.height == 0.0F) {
        return std::nullopt;
    }

    const auto local_left = base_position_x(pane.base_position, pane.width);
    const auto local_top = base_position_y(pane.base_position, pane.height);
    const auto corners = std::array< std::array< float, 2U >, 4U >{
        panePointForAurora(pane_state, local_left, local_top),
        panePointForAurora(pane_state, local_left + pane.width, local_top),
        panePointForAurora(pane_state, local_left + pane.width, local_top + pane.height),
        panePointForAurora(pane_state, local_left, local_top + pane.height),
    };
    auto left = corners.front()[0U];
    auto right = corners.front()[0U];
    auto top = corners.front()[1U];
    auto bottom = corners.front()[1U];
    for (const auto& corner : corners) {
        left = std::min(left, corner[0U]);
        right = std::max(right, corner[0U]);
        top = std::min(top, corner[1U]);
        bottom = std::max(bottom, corner[1U]);
    }
    return PaneBounds{
        .left = left,
        .top = top,
        .right = right,
        .bottom = bottom,
    };
}

std::optional< TVec2f > SimpleLayout::paneScale(std::string_view paneName) const {
    const_cast< SimpleLayout* >(this)->loadRenderData();
    if (paneName.empty()) {
        return TVec2f{mScaleX, mScaleY};
    }

    const auto pane_index = find_preferred_pane_index(mBrlytLayout, paneName);
    if (!pane_index.has_value()) {
        return std::nullopt;
    }

    const auto pane_state = paneRenderState(*pane_index);
    return TVec2f{
        .x = hypot2(pane_state.m00, pane_state.m10),
        .y = hypot2(pane_state.m01, pane_state.m11),
    };
}

bool SimpleLayout::copyPaneMatrix(std::string_view paneName, Mtx matrix) const {
    if (matrix == nullptr) {
        return false;
    }

    const_cast< SimpleLayout* >(this)->loadRenderData();

    auto set_matrix = [&](const PaneRenderState& pane_state) {
        matrix[0][0] = pane_state.m00;
        matrix[0][1] = pane_state.m01;
        matrix[0][2] = 0.0F;
        matrix[0][3] = mTransX + pane_state.translate_x;
        matrix[1][0] = pane_state.m10;
        matrix[1][1] = pane_state.m11;
        matrix[1][2] = 0.0F;
        matrix[1][3] = mTransY + pane_state.translate_y;
        matrix[2][0] = 0.0F;
        matrix[2][1] = 0.0F;
        matrix[2][2] = 1.0F;
        matrix[2][3] = 0.0F;
    };

    if (paneName.empty()) {
        set_matrix(PaneRenderState{
            .scale_x = mScaleX,
            .scale_y = mScaleY,
            .m00 = mScaleX,
            .m11 = mScaleY,
        });
        return true;
    }

    const auto pane_index = find_preferred_pane_index(mBrlytLayout, paneName);
    if (!pane_index.has_value()) {
        return false;
    }

    set_matrix(paneRenderState(*pane_index));
    return true;
}

bool SimpleLayout::isPointingPane(std::string_view paneName, f32 screenX, f32 screenY) const {
    const auto bounds = paneBounds(paneName);
    if (!bounds.has_value()) {
        return false;
    }

    const auto layout_x = screenX * static_cast< f32 >(smgpc::render::core::kWiiLayoutWidth) /
                              static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferWidth) -
                          static_cast< f32 >(smgpc::render::core::kWiiLayoutWidth) * 0.5F;
    const auto layout_y = -(screenY - static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F);
    return layout_x >= bounds->left && layout_x <= bounds->right && layout_y >= bounds->top && layout_y <= bounds->bottom;
}

void SimpleLayout::startPaneAnim(std::string_view paneName, const char* pAnimName, u32 animLayer) {
    if (paneName.empty() || pAnimName == nullptr) {
        return;
    }

    loadRenderData();
    auto& anim = paneAnimation(paneName).animations.at(std::min< std::size_t >(animLayer, mAnimations.size() - 1U));
    anim.name = pAnimName;
    anim.frame = 0.0F;
    anim.end = durationFor(pAnimName);
    anim.rate = 1.0F;
    anim.looping = isLoopingAnim(pAnimName);
    anim.stopped = false;
}

void SimpleLayout::stopPaneAnim(std::string_view paneName, u32 animLayer) {
    if (paneName.empty()) {
        return;
    }

    auto& pane = paneAnimation(paneName);
    auto& anim = pane.animations.at(std::min< std::size_t >(animLayer, pane.animations.size() - 1U));
    anim.rate = 0.0F;
    anim.stopped = true;
}

void SimpleLayout::setPaneAnimFrame(std::string_view paneName, f32 frame, u32 animLayer) {
    if (paneName.empty()) {
        return;
    }

    auto& anim = paneAnimation(paneName).animations.at(std::min< std::size_t >(animLayer, mAnimations.size() - 1U));
    anim.frame = frame;
}

void SimpleLayout::setPaneAnimRate(std::string_view paneName, f32 rate, u32 animLayer) {
    if (paneName.empty()) {
        return;
    }

    auto& anim = paneAnimation(paneName).animations.at(std::min< std::size_t >(animLayer, mAnimations.size() - 1U));
    anim.rate = rate;
    anim.stopped = rate == 0.0F;
}

f32 SimpleLayout::getPaneAnimFrame(std::string_view paneName, u32 animLayer) const {
    const auto* pane = findPaneAnimation(paneName);
    if (pane == nullptr) {
        return 0.0F;
    }

    return pane->animations.at(std::min< std::size_t >(animLayer, pane->animations.size() - 1U)).frame;
}

bool SimpleLayout::isPaneAnimStopped(std::string_view paneName, u32 animLayer) const {
    const auto* pane = findPaneAnimation(paneName);
    if (pane == nullptr) {
        return true;
    }

    return pane->animations.at(std::min< std::size_t >(animLayer, pane->animations.size() - 1U)).stopped;
}

f32 SimpleLayout::getAnimFrameMax(u32 animLayer) const {
    return animation(animLayer).end;
}

f32 SimpleLayout::getAnimRate(u32 animLayer) const {
    return animation(animLayer).rate;
}

f32 SimpleLayout::getAnimDuration(const char* pAnimName) const {
    if (pAnimName == nullptr || pAnimName[0] == '\0') {
        return 1.0F;
    }

    return durationFor(pAnimName);
}

bool SimpleLayout::isAnimLooping(const char* pAnimName) const {
    if (pAnimName == nullptr || pAnimName[0] == '\0') {
        return false;
    }

    return isLoopingAnim(pAnimName);
}

bool SimpleLayout::isAnimLooping(u32 animLayer) const {
    return animation(animLayer).looping;
}

#ifndef NDEBUG
f32 SimpleLayout::debugPaneAnimEndFrame(std::string_view paneName, u32 animLayer) const {
    const auto* pane = findPaneAnimation(paneName);
    if (pane == nullptr) {
        return 0.0F;
    }

    return pane->animations.at(std::min< std::size_t >(animLayer, pane->animations.size() - 1U)).end;
}

std::size_t SimpleLayout::debugAnimLayerCount() const {
    return mAnimLayerNum;
}

std::string_view SimpleLayout::debugAnimName(u32 animLayer) const {
    return animation(animLayer).name;
}

f32 SimpleLayout::debugAnimDuration(const char* pAnimName) const {
    return getAnimDuration(pAnimName);
}

bool SimpleLayout::debugAnimLooping(const char* pAnimName) const {
    return isAnimLooping(pAnimName);
}

f32 SimpleLayout::debugAnimEndFrame(u32 animLayer) const {
    return getAnimFrameMax(animLayer);
}

f32 SimpleLayout::debugAnimRate(u32 animLayer) const {
    return getAnimRate(animLayer);
}

bool SimpleLayout::debugAnimLooping(u32 animLayer) const {
    return isAnimLooping(animLayer);
}

bool SimpleLayout::debugAnimStopped(u32 animLayer) const {
    return animation(animLayer).stopped;
}

std::size_t SimpleLayout::debugPaneCount() const {
    return mBrlytLayout.panes.size();
}

std::size_t SimpleLayout::debugPictureCount() const {
    return mBrlytLayout.pictures.size();
}

std::size_t SimpleLayout::debugTextBoxCount() const {
    return mBrlytLayout.text_boxes.size();
}

std::size_t SimpleLayout::debugMaterialCount() const {
    return mBrlytLayout.materials.size();
}

std::size_t SimpleLayout::debugTextureCount() const {
    return mRenderTextures.size();
}

std::size_t SimpleLayout::debugFontCount() const {
    return mRenderFonts.size();
}

std::size_t SimpleLayout::debugCommittedPaneFrameCount() const {
    return mCommittedPaneFrames.size();
}

std::vector< SimpleLayout::DebugPaneState > SimpleLayout::debugPanes() const {
    auto states = std::vector< DebugPaneState >{};
    states.reserve(mBrlytLayout.panes.size());

    for (auto pane_index = std::size_t{}; pane_index < mBrlytLayout.panes.size(); ++pane_index) {
        const auto& pane = mBrlytLayout.panes[pane_index];
        const auto render_state = paneRenderState(pane_index);
        auto state = DebugPaneState{
            .index = pane_index,
            .name = pane.name,
            .parent_index = pane.parent_index,
            .base_visible = pane.visible,
            .effective_visible = render_state.visible,
            .translate_x = render_state.translate_x,
            .translate_y = render_state.translate_y,
            .scale_x = render_state.scale_x,
            .scale_y = render_state.scale_y,
            .alpha = render_state.alpha,
            .width = pane.width,
            .height = pane.height,
            .contents = {},
        };

        for (const auto& picture : mBrlytLayout.pictures) {
            if (picture.pane_index != pane_index) {
                continue;
            }

            auto content = DebugPaneContentState{
                .kind = "picture",
                .name = picture.name,
                .material_index = static_cast< s32 >(picture.material_index),
                .material_name = {},
                .texture_name = picture.texture_name,
                .font_name = {},
                .visible = picture.visible,
            };
            if (picture.material_index < mBrlytLayout.materials.size()) {
                const auto& material = mBrlytLayout.materials[picture.material_index];
                content.material_name = material.name;
                if (!material.textures.empty()) {
                    content.texture_name = material.textures.front().texture_name;
                }
            }
            state.contents.push_back(std::move(content));
        }

        for (const auto& text_box : mBrlytLayout.text_boxes) {
            if (text_box.pane_index != pane_index) {
                continue;
            }

            auto content = DebugPaneContentState{
                .kind = "text_box",
                .name = text_box.name,
                .material_index = static_cast< s32 >(text_box.material_index),
                .material_name = {},
                .texture_name = {},
                .font_name = text_box.font_name,
                .visible = text_box.visible,
            };
            if (text_box.material_index < mBrlytLayout.materials.size()) {
                const auto& material = mBrlytLayout.materials[text_box.material_index];
                content.material_name = material.name;
                if (!material.textures.empty()) {
                    content.texture_name = material.textures.front().texture_name;
                }
            }
            state.contents.push_back(std::move(content));
        }

        states.push_back(std::move(state));
    }

    return states;
}

std::vector< SimpleLayout::DebugMaterialState > SimpleLayout::debugMaterials() const {
    auto states = std::vector< DebugMaterialState >{};
    states.reserve(mBrlytLayout.materials.size());

    for (auto material_index = std::size_t{}; material_index < mBrlytLayout.materials.size(); ++material_index) {
        const auto& material = mBrlytLayout.materials[material_index];
        auto state = DebugMaterialState{
            .index = material_index,
            .name = material.name,
            .texture_count = material.textures.size(),
            .tex_coord_gen_count = material.tex_coord_gens.size(),
            .tev_stage_count = material.tev_stages.size(),
            .alpha_compare_enabled = material.alpha_compare.enabled,
            .blend_enabled = material.blend_mode.enabled,
            .textures = {},
        };
        state.textures.reserve(material.textures.size());
        for (auto slot = std::size_t{}; slot < material.textures.size(); ++slot) {
            const auto& texture = material.textures[slot];
            state.textures.push_back(DebugMaterialTextureState{
                .slot = slot,
                .texture_index = texture.texture_index,
                .texture_name = texture.texture_name,
                .wrap_s = texture.wrap_s,
                .wrap_t = texture.wrap_t,
                .min_filter = texture.min_filter,
                .mag_filter = texture.mag_filter,
            });
        }
        states.push_back(std::move(state));
    }

    return states;
}

std::vector< SimpleLayout::DebugTextureState > SimpleLayout::debugTextures() const {
    auto states = std::vector< DebugTextureState >{};
    states.reserve(mRenderTextures.size());

    for (auto texture_index = std::size_t{}; texture_index < mRenderTextures.size(); ++texture_index) {
        const auto& texture = mRenderTextures[texture_index];
        states.push_back(DebugTextureState{
            .index = texture_index,
            .name = texture.name,
            .width = texture.decoded.width,
            .height = texture.decoded.height,
            .format_raw = static_cast< std::uint32_t >(texture.decoded.format),
            .format_name = texture_format_name(texture.decoded.format),
            .uploaded = texture.handle.is_valid(),
            .rgba_byte_count = texture.decoded.rgba.size(),
        });
    }

    return states;
}
#endif

SimpleLayout::AnimationState& SimpleLayout::animation(u32 animLayer) {
    return mAnimations.at(std::min< std::size_t >(animLayer, mAnimations.size() - 1U));
}

const SimpleLayout::AnimationState& SimpleLayout::animation(u32 animLayer) const {
    return mAnimations.at(std::min< std::size_t >(animLayer, mAnimations.size() - 1U));
}

SimpleLayout::PaneAnimationState& SimpleLayout::paneAnimation(std::string_view paneName) {
    const auto it = std::ranges::find_if(mPaneAnimations, [paneName](const auto& pane) { return pane.pane_name == paneName; });
    if (it != mPaneAnimations.end()) {
        return *it;
    }

    mPaneAnimations.push_back(PaneAnimationState{.pane_name = std::string(paneName)});
    return mPaneAnimations.back();
}

const SimpleLayout::PaneAnimationState* SimpleLayout::findPaneAnimation(std::string_view paneName) const {
    const auto it = std::ranges::find_if(mPaneAnimations, [paneName](const auto& pane) { return pane.pane_name == paneName; });
    if (it != mPaneAnimations.end()) {
        return &*it;
    }

    const auto active = layout_locale_suffix();
    if (active.empty() || !ends_with(paneName, active) || paneName.size() <= active.size()) {
        return nullptr;
    }

    const auto base_name = paneName.substr(0U, paneName.size() - active.size());
    const auto base = std::ranges::find_if(mPaneAnimations, [base_name](const auto& pane) { return pane.pane_name == base_name; });
    return base == mPaneAnimations.end() ? nullptr : &*base;
}

void SimpleLayout::commitAnimationState(const AnimationState& anim) {
    if (anim.name.empty()) {
        return;
    }

    const auto it = mRenderAnimations.find(lower_copy(anim.name));
    if (it == mRenderAnimations.end()) {
        return;
    }

    auto frame = anim.frame;
    if (it->second.loop && it->second.frame_size > 0U) {
        frame = std::fmod(frame, static_cast< float >(it->second.frame_size));
    }

    for (const auto& content : it->second.contents) {
        const auto pane_frame = it->second.pane_frame(content.name, frame);
        auto& committed = mCommittedPaneFrames[content.name];
        if (pane_frame.translate_x.has_value()) {
            committed.translate_x = pane_frame.translate_x;
        }
        if (pane_frame.translate_y.has_value()) {
            committed.translate_y = pane_frame.translate_y;
        }
        if (pane_frame.scale_x.has_value()) {
            committed.scale_x = pane_frame.scale_x;
        }
        if (pane_frame.scale_y.has_value()) {
            committed.scale_y = pane_frame.scale_y;
        }
        if (pane_frame.rotate_z.has_value()) {
            committed.rotate_z = pane_frame.rotate_z;
        }
        if (pane_frame.alpha.has_value()) {
            committed.alpha = pane_frame.alpha;
        }
        if (pane_frame.visible.has_value()) {
            committed.visible = pane_frame.visible;
        }
        merge_material_frame(mCommittedMaterialFrames[content.name], it->second.material_frame(content.name, frame));
    }
}

void SimpleLayout::loadRenderData() {
    if (mRenderDataLoaded) {
        return;
    }

    mRenderDataLoaded = true;
    if (!mArchivePath.has_value()) {
        return;
    }

    try {
        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        auto local_archive = std::optional< smgpc::resource::RarcArchive >{};
        const auto* archive = static_cast< const smgpc::resource::RarcArchive* >(nullptr);
        if (runtime != nullptr) {
            archive = &runtime->dvd().archive_for_path(*mArchivePath);
        } else {
            local_archive = smgpc::resource::RarcArchive::from_file(*mArchivePath);
            archive = &*local_archive;
        }

        const auto brlyt_it = std::ranges::find_if(archive->entries(), [](const auto& entry) { return ends_with(entry.path, ".brlyt"); });
        if (brlyt_it == archive->entries().end()) {
            return;
        }

        mBrlytLayout = smgpc::layout::parse_brlyt_layout(archive->file_data(*brlyt_it));
        applyLayoutMessagesFromPaneUserData();
        for (const auto& entry : archive->entries()) {
            if (!ends_with(entry.path, ".brlan")) {
                continue;
            }

            try {
                mRenderAnimations[animation_name_from_path(entry.path)] = smgpc::layout::parse_brlan_animation(archive->file_data(entry));
            } catch (const std::exception& e) {
                if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
                    runtime->note_layout_texture_decode_failed(mLayoutName, entry.path, e.what());
                }
            }
        }

        for (const auto& material : mBrlytLayout.materials) {
            for (const auto& material_texture : material.textures) {
                if (material_texture.texture_name.empty() || contains_texture(mRenderTextures, material_texture.texture_name)) {
                    continue;
                }

                const auto texture_path = texture_archive_path(material_texture.texture_name);
                if (!archive->contains(texture_path)) {
                    continue;
                }

                try {
                    mRenderTextures.push_back(RenderTexture{
                        .name = material_texture.texture_name,
                        .decoded = smgpc::resource::decode_tpl_texture(archive->file_data(texture_path)),
                        .handle = {},
                    });
                } catch (const std::exception& e) {
                    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
                        runtime->note_layout_texture_decode_failed(mLayoutName, material_texture.texture_name, e.what());
                    }
                }
            }
        }

        if (!mBrlytLayout.text_boxes.empty()) {
            if (const auto font_path = find_companion_font_archive(mArchivePath)) {
                try {
                    auto local_font_archive = std::optional< smgpc::resource::RarcArchive >{};
                    const auto* font_archive = static_cast< const smgpc::resource::RarcArchive* >(nullptr);
                    if (runtime != nullptr) {
                        font_archive = &runtime->dvd().archive_for_path(*font_path);
                    } else {
                        local_font_archive = smgpc::resource::RarcArchive::from_file(*font_path);
                        font_archive = &*local_font_archive;
                    }

                    for (const auto& text_box : mBrlytLayout.text_boxes) {
                        if (contains_font(mRenderFonts, text_box.font_name)) {
                            continue;
                        }

                        const auto* font_entry = find_font_entry(*font_archive, text_box.font_name);
                        if (font_entry == nullptr) {
                            continue;
                        }

                        auto font = smgpc::layout::parse_brfnt_font(font_archive->file_data(*font_entry));
                        mRenderFonts.push_back(RenderFont{
                            .name = text_box.font_name,
                            .font = std::move(font),
                            .sheet_handles = {},
                        });
                        mRenderFonts.back().sheet_handles.resize(mRenderFonts.back().font.sheets.size());
                    }
                } catch (const std::exception& e) {
                    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
                        runtime->note_layout_texture_decode_failed(mLayoutName, "<font>", e.what());
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        mBrlytLayout = {};
        mRenderTextures.clear();
        mRenderFonts.clear();
        mRenderTextTextures.clear();
        mRenderAnimations.clear();
        mCommittedPaneFrames.clear();
        mCommittedMaterialFrames.clear();
        mTextBoxTemplates.clear();
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->note_layout_texture_decode_failed(mLayoutName, "<layout>", e.what());
        }
    }
}

void SimpleLayout::applyLayoutMessagesFromPaneUserData() {
    auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
    if (runtime == nullptr || mBrlytLayout.text_boxes.empty()) {
        return;
    }

    auto changed = false;
    for (auto& text_box : mBrlytLayout.text_boxes) {
        if (text_box.pane_index >= mBrlytLayout.panes.size()) {
            continue;
        }

        auto user_data = std::string_view{};
        auto pane_index = text_box.pane_index;
        while (pane_index < mBrlytLayout.panes.size()) {
            const auto& pane = mBrlytLayout.panes[pane_index];
            if (!pane.user_data.empty()) {
                user_data = pane.user_data;
                break;
            }
            if (pane.parent_index < 0) {
                break;
            }
            pane_index = static_cast< std::size_t >(pane.parent_index);
        }

        if (user_data.empty()) {
            continue;
        }

        const auto message_id = layout_message_id(mLayoutName, user_data);
        const auto* raw_text = runtime->messages().message_raw_utf16(message_id);
        if (raw_text == nullptr) {
            continue;
        }

        const auto* display_text = runtime->messages().message_utf16(message_id);
        const auto formatted = display_text == nullptr ? smgpc::resource::format_bmg_text(*raw_text, {}) : std::u16string{};
        const auto& visible_text = display_text == nullptr ? formatted : *display_text;
        text_box.raw_text = encode_utf16_words(*raw_text);
        text_box.text = encode_utf16_words(visible_text);
        if (const auto* control_tags = runtime->messages().message_control_tags(message_id)) {
            text_box.control_tags = *control_tags;
        } else {
            text_box.control_tags = smgpc::resource::bmg_control_tags(*raw_text);
        }
        mTextBoxTemplates[text_box.name] = TextBoxTemplateState{
            .raw_text = *raw_text,
            .args = {},
        };
        changed = true;
    }

    if (changed) {
        mRenderTextTextures.clear();
    }
}

void SimpleLayout::ensureTextureUploads(smgpc::render::AuroraRenderer& renderer) {
    for (auto& texture : mRenderTextures) {
        if (texture.handle.is_valid()) {
            continue;
        }

        texture.handle = renderer.create_rgba8_texture(texture.decoded.width, texture.decoded.height,
                                                       std::span< const std::uint8_t >(texture.decoded.rgba.data(), texture.decoded.rgba.size()));
    }

    for (auto& font : mRenderFonts) {
        for (std::size_t sheet_index = 0U; sheet_index < font.font.sheets.size(); ++sheet_index) {
            if (font.sheet_handles[sheet_index].is_valid()) {
                continue;
            }

            const auto& sheet = font.font.sheets[sheet_index];
            font.sheet_handles[sheet_index] =
                renderer.create_rgba8_texture(sheet.width, sheet.height, std::span< const std::uint8_t >(sheet.rgba.data(), sheet.rgba.size()));
        }
    }

    ensureTextTextureUploads(renderer);
}

void SimpleLayout::ensureTextTextureUploads(smgpc::render::AuroraRenderer& renderer) {
    for (std::size_t text_box_index = 0U; text_box_index < mBrlytLayout.text_boxes.size(); ++text_box_index) {
        if (find_text_texture(mRenderTextTextures, text_box_index) != nullptr) {
            continue;
        }

        const auto& text_box = mBrlytLayout.text_boxes[text_box_index];
        auto* render_font = find_font(mRenderFonts, text_box.font_name);
        if (render_font == nullptr || render_font->font.width == 0U || render_font->font.height == 0U || render_font->font.sheets.empty()) {
            continue;
        }

        auto text_texture = composeTextTexture(text_box_index, *render_font);
        text_texture.handle = renderer.create_rgba8_texture(text_texture.width, text_texture.height,
                                                            std::span< const std::uint8_t >(text_texture.rgba.data(), text_texture.rgba.size()));
        if (text_texture.handle.is_valid()) {
            mRenderTextTextures.push_back(std::move(text_texture));
        }
    }
}

SimpleLayout::RenderTextTexture SimpleLayout::composeTextTexture(std::size_t text_box_index, const RenderFont& render_font) const {
    const auto& text_box = mBrlytLayout.text_boxes[text_box_index];
    const auto& font = render_font.font;
    const auto scale_x = text_box.font_width > 0.0F ? text_box.font_width / static_cast< float >(font.width) : 1.0F;
    const auto scale_y = text_box.font_height > 0.0F ? text_box.font_height / static_cast< float >(font.height) : 1.0F;
    const auto native_char_space = scale_x > 0.0F ? text_box.char_space / scale_x : text_box.char_space;
    const auto native_line_space = scale_y > 0.0F ? text_box.line_space / scale_y : text_box.line_space;
    const auto native_line_height = static_cast< float >(std::max< std::uint16_t >(font.height, 1U));
    const auto native_line_advance = std::max(1.0F, native_line_height + native_line_space);
    const auto use_button_icon_aliases = text_uses_button_icon_aliases(std::span< const std::uint16_t >(text_box.text.data(), text_box.text.size()));

    auto box_width = text_box.width;
    if (text_box.pane_index < mBrlytLayout.panes.size()) {
        box_width = mBrlytLayout.panes[text_box.pane_index].width;
    }
    const auto native_wrap_width = scale_x > 0.0F ? box_width / scale_x : box_width;
    const auto lines = layout_text_lines(std::span< const std::uint16_t >(text_box.text.data(), text_box.text.size()), font, native_char_space,
                                         native_wrap_width, use_button_icon_aliases);

    auto max_line_width = 0.0F;
    auto has_multiline_text = lines.size() > 1U;
    for (const auto& line : lines) {
        max_line_width = std::max(max_line_width, line.width);
    }

    const auto text_horizontal_position = static_cast< std::uint8_t >(text_box.text_position % 3U);
    const auto line_alignment = static_cast< std::uint8_t >(text_box.text_alignment == 0U || text_box.text_alignment > 2U ? text_horizontal_position :
                                                                                                                            text_box.text_alignment);
    const auto native_texture_width =
        native_wrap_width > 0.0F && (has_multiline_text || line_alignment != 0U) ? std::max(native_wrap_width, max_line_width) : max_line_width;
    const auto native_texture_height = native_line_height + static_cast< float >(lines.size() - 1U) * native_line_advance;

    auto texture = RenderTextTexture{
        .text_box_index = text_box_index,
        .width = texture_extent(native_texture_width),
        .height = texture_extent(native_texture_height),
        .font_width = std::max< std::uint16_t >(font.width, 1U),
        .font_height = std::max< std::uint16_t >(font.height, 1U),
        .rgba = {},
        .handle = {},
    };
    texture.rgba.assign(static_cast< std::size_t >(texture.width) * texture.height * 4U, 0U);

    const auto mapped_color = std::array< std::uint8_t, 4U >{
        static_cast< std::uint8_t >((static_cast< std::uint16_t >(text_box.color[0U]) * text_box.color_mapping_max[0U]) / 255U),
        static_cast< std::uint8_t >((static_cast< std::uint16_t >(text_box.color[1U]) * text_box.color_mapping_max[1U]) / 255U),
        static_cast< std::uint8_t >((static_cast< std::uint16_t >(text_box.color[2U]) * text_box.color_mapping_max[2U]) / 255U),
        static_cast< std::uint8_t >((static_cast< std::uint16_t >(text_box.color[3U]) * text_box.color_mapping_max[3U]) / 255U),
    };

    for (auto line_index = std::size_t{}; line_index < lines.size(); ++line_index) {
        const auto& line = lines[line_index];
        auto cursor_x = (static_cast< float >(texture.width) - line.width) * text_factor(line_alignment);
        const auto line_y = static_cast< float >(line_index) * native_line_advance;

        for (const auto& layout_glyph : line.glyphs) {
            const auto& glyph = layout_glyph.glyph;
            if (glyph.sheet_index >= font.sheets.size()) {
                cursor_x += layout_glyph.advance + native_char_space;
                continue;
            }

            const auto glyph_code = resolve_layout_glyph_code(layout_glyph.code, font, use_button_icon_aliases);
            const auto is_button_icon = is_button_icon_code(glyph_code);
            const auto& sheet = font.sheets[glyph.sheet_index];
            const auto glyph_x = static_cast< int >(std::round(cursor_x)) + static_cast< int >(glyph.widths.left);
            const auto glyph_y = static_cast< int >(std::round(line_y));
            for (auto y = 0U; y < glyph.height; ++y) {
                for (auto x = 0U; x < glyph.width; ++x) {
                    const auto source_x = static_cast< std::uint16_t >(glyph.x + x);
                    const auto source_y = static_cast< std::uint16_t >(glyph.y + y);
                    if (source_x >= sheet.width || source_y >= sheet.height) {
                        continue;
                    }

                    const auto dest_x = glyph_x + static_cast< int >(x);
                    const auto dest_y = glyph_y + static_cast< int >(y);
                    if (dest_x < 0 || dest_y < 0 || dest_x >= texture.width || dest_y >= texture.height) {
                        continue;
                    }

                    const auto source_offset = (static_cast< std::size_t >(source_y) * sheet.width + source_x) * 4U;
                    const auto source_alpha =
                        static_cast< std::uint8_t >((static_cast< std::uint16_t >(sheet.rgba[source_offset + 3U]) * mapped_color[3U]) / 255U);
                    if (source_alpha == 0U) {
                        continue;
                    }

                    const auto dest_offset = (static_cast< std::size_t >(dest_y) * texture.width + static_cast< std::size_t >(dest_x)) * 4U;
                    const auto dest_alpha = is_button_icon ? mapped_color[3U] : source_alpha;
                    if (dest_alpha >= texture.rgba[dest_offset + 3U]) {
                        texture.rgba[dest_offset] =
                            static_cast< std::uint8_t >((static_cast< std::uint16_t >(sheet.rgba[source_offset]) * mapped_color[0U]) / 255U);
                        texture.rgba[dest_offset + 1U] =
                            static_cast< std::uint8_t >((static_cast< std::uint16_t >(sheet.rgba[source_offset + 1U]) * mapped_color[1U]) / 255U);
                        texture.rgba[dest_offset + 2U] =
                            static_cast< std::uint8_t >((static_cast< std::uint16_t >(sheet.rgba[source_offset + 2U]) * mapped_color[2U]) / 255U);
                        texture.rgba[dest_offset + 3U] = dest_alpha;
                    }
                }
            }

            cursor_x += layout_glyph.advance + native_char_space;
        }
    }

    return texture;
}

void SimpleLayout::drawTextBoxes(smgpc::render::AuroraRenderer& renderer, float alpha) {
    for (std::size_t text_box_index = 0U; text_box_index < mBrlytLayout.text_boxes.size(); ++text_box_index) {
        drawTextBox(renderer, alpha, text_box_index);
    }
}

void SimpleLayout::drawTextBox(smgpc::render::AuroraRenderer& renderer, float alpha, std::size_t text_box_index) {
    if (text_box_index >= mBrlytLayout.text_boxes.size()) {
        return;
    }
    const auto& text_box = mBrlytLayout.text_boxes[text_box_index];
    if (text_box.pane_index >= mBrlytLayout.panes.size()) {
        return;
    }

    auto* text_texture = find_text_texture(mRenderTextTextures, text_box_index);
    if (text_texture == nullptr || !text_texture->handle.is_valid()) {
        return;
    }

    const auto pane_state = paneRenderState(text_box.pane_index);
    if (!pane_state.visible) {
        return;
    }
    const auto& pane = mBrlytLayout.panes[text_box.pane_index];
    const auto box_width = pane.width;
    const auto box_height = pane.height;
    const auto box_x = base_position_x(pane.base_position, box_width);
    const auto box_y = base_position_y(pane.base_position, box_height);
    const auto font_width = text_box.font_width;
    const auto font_height = text_box.font_height;
    const auto scale_x = font_width / static_cast< float >(text_texture->font_width);
    const auto scale_y = font_height / static_cast< float >(text_texture->font_height);
    const auto line_width = static_cast< float >(text_texture->width) * scale_x;
    const auto text_height = static_cast< float >(text_texture->height) * scale_y;

    const auto horizontal_factor = text_factor(text_box.text_position % 3U);
    const auto vertical_factor = text_factor(text_box.text_position / 3U);
    const auto cursor_x = box_x + (box_width - line_width) * horizontal_factor;
    const auto cursor_y = box_y + (box_height - text_height) * vertical_factor;
    const auto color = std::array< std::uint8_t, 4U >{
        255U,
        255U,
        255U,
        static_cast< std::uint8_t >(std::clamp(alpha * pane_state.alpha, 0.0F, 255.0F)),
    };
    const auto width = line_width;
    const auto height = text_height;
    const auto top_v = gx_texture_v_to_host(0.0F);
    const auto bottom_v = gx_texture_v_to_host(1.0F);
    const auto top_left = panePointForAurora(pane_state, cursor_x, cursor_y);
    const auto top_right = panePointForAurora(pane_state, cursor_x + width, cursor_y);
    const auto bottom_right = panePointForAurora(pane_state, cursor_x + width, cursor_y + height);
    const auto bottom_left = panePointForAurora(pane_state, cursor_x, cursor_y + height);

    renderer.submit_textured_quad(
        text_texture->handle,
        smgpc::render::TexturedQuad2D{
            .vertices =
                {
                    smgpc::render::TexturedVertex2D{.x = top_left[0U], .y = top_left[1U], .z = 0.0F, .u = 0.0F, .v = top_v, .color = color},
                    smgpc::render::TexturedVertex2D{.x = top_right[0U], .y = top_right[1U], .z = 0.0F, .u = 1.0F, .v = top_v, .color = color},
                    smgpc::render::TexturedVertex2D{
                        .x = bottom_right[0U], .y = bottom_right[1U], .z = 0.0F, .u = 1.0F, .v = bottom_v, .color = color},
                    smgpc::render::TexturedVertex2D{.x = bottom_left[0U], .y = bottom_left[1U], .z = 0.0F, .u = 0.0F, .v = bottom_v, .color = color},
                },
        });
}

std::array< float, 2U > SimpleLayout::panePointForAurora(const PaneRenderState& pane_state, float local_x, float local_y) const {
    const auto translate_x = mTransX + pane_state.translate_x;
    const auto translate_y = mTransY + pane_state.translate_y;

    if (mBrlytLayout.origin_type == 1U) {
        return std::array< float, 2U >{
            translate_x + pane_state.m00 * local_x - pane_state.m01 * local_y,
            translate_y + pane_state.m10 * local_x - pane_state.m11 * local_y,
        };
    }

    const auto top_left_x = translate_x + pane_state.m00 * local_x + pane_state.m01 * local_y;
    const auto top_left_y = translate_y + pane_state.m10 * local_x + pane_state.m11 * local_y;
    return std::array< float, 2U >{
        top_left_x - mBrlytLayout.width * 0.5F,
        mBrlytLayout.height * 0.5F - top_left_y,
    };
}

SimpleLayout::PaneRenderState SimpleLayout::paneRenderState(std::size_t pane_index) const {
    const auto& pane = mBrlytLayout.panes.at(pane_index);
    auto local_translate_x = pane.translate_x;
    auto local_translate_y = pane.translate_y;
    auto local_scale_x = pane.scale_x;
    auto local_scale_y = pane.scale_y;
    auto local_rotate_z = pane.rotate_z;
    auto local_alpha = static_cast< float >(pane.alpha);
    auto local_visible = pane.visible;
    const auto local_location_adjust = pane.location_adjust;

    const auto anim = animationFrameForPane(pane.name);
    if (anim.translate_x.has_value()) {
        local_translate_x = *anim.translate_x;
    }
    if (anim.translate_y.has_value()) {
        local_translate_y = *anim.translate_y;
    }
    if (anim.scale_x.has_value()) {
        local_scale_x = *anim.scale_x;
    }
    if (anim.scale_y.has_value()) {
        local_scale_y = *anim.scale_y;
    }
    if (anim.rotate_z.has_value()) {
        local_rotate_z = *anim.rotate_z;
    }
    if (anim.alpha.has_value()) {
        local_alpha = *anim.alpha;
    }
    if (anim.visible.has_value()) {
        local_visible = *anim.visible;
    }
    if (const auto override = mPaneVisibilityOverrides.find(pane.name); override != mPaneVisibilityOverrides.end()) {
        local_visible = override->second;
    }
    if (const auto override = mPaneAlphaOverrides.find(pane.name); override != mPaneAlphaOverrides.end()) {
        local_alpha = override->second;
    }
    if (pane_name_has_inactive_locale_suffix(pane.name)) {
        local_visible = false;
    }
    if (pane_name_has_active_locale_variant(mBrlytLayout, pane.name)) {
        local_visible = false;
    }

    if (local_location_adjust) {
        const auto adjust = layout_location_adjust_scale();
        local_scale_x *= adjust[0U];
        local_scale_y *= adjust[1U];
    }

    constexpr auto kDegToRad = 3.14159265358979323846F / 180.0F;
    const auto rotation = local_rotate_z * kDegToRad;
    const auto cos_r = std::cos(rotation);
    const auto sin_r = std::sin(rotation);
    const auto local_m00 = cos_r * local_scale_x;
    const auto local_m01 = -sin_r * local_scale_y;
    const auto local_m10 = sin_r * local_scale_x;
    const auto local_m11 = cos_r * local_scale_y;

    if (pane.parent_index < 0) {
        const auto modifies_child_alpha = pane.influenced_alpha && local_alpha != 255.0F;
        return PaneRenderState{
            .translate_x = local_translate_x,
            .translate_y = local_translate_y,
            .scale_x = mScaleX * local_scale_x,
            .scale_y = mScaleY * local_scale_y,
            .rotate_z = local_rotate_z,
            .m00 = mScaleX * local_m00,
            .m01 = mScaleX * local_m01,
            .m10 = mScaleY * local_m10,
            .m11 = mScaleY * local_m11,
            .alpha = local_alpha,
            .child_alpha_scale = modifies_child_alpha ? (local_alpha / 255.0F) : 1.0F,
            .visible = local_visible,
            .location_adjust = local_location_adjust,
            .child_alpha_influenced = modifies_child_alpha,
        };
    }

    const auto parent = paneRenderState(static_cast< std::size_t >(pane.parent_index));
    const auto effective_alpha = parent.child_alpha_influenced ? local_alpha * parent.child_alpha_scale : local_alpha;
    const auto modifies_child_alpha = pane.influenced_alpha && local_alpha != 255.0F;
    const auto child_alpha_scale = modifies_child_alpha ? parent.child_alpha_scale * (local_alpha / 255.0F) : parent.child_alpha_scale;
    return PaneRenderState{
        .translate_x = parent.translate_x + parent.m00 * local_translate_x + parent.m01 * local_translate_y,
        .translate_y = parent.translate_y + parent.m10 * local_translate_x + parent.m11 * local_translate_y,
        .scale_x = parent.scale_x * local_scale_x,
        .scale_y = parent.scale_y * local_scale_y,
        .rotate_z = parent.rotate_z + local_rotate_z,
        .m00 = parent.m00 * local_m00 + parent.m01 * local_m10,
        .m01 = parent.m00 * local_m01 + parent.m01 * local_m11,
        .m10 = parent.m10 * local_m00 + parent.m11 * local_m10,
        .m11 = parent.m10 * local_m01 + parent.m11 * local_m11,
        .alpha = effective_alpha,
        .child_alpha_scale = child_alpha_scale,
        .visible = parent.visible && local_visible,
        .location_adjust = local_location_adjust,
        .child_alpha_influenced = parent.child_alpha_influenced || modifies_child_alpha,
    };
}

smgpc::layout::BrlanPaneFrame SimpleLayout::animationFrameForPane(std::string_view pane_name) const {
    auto result = smgpc::layout::BrlanPaneFrame{};
    if (const auto committed = mCommittedPaneFrames.find(std::string(pane_name)); committed != mCommittedPaneFrames.end()) {
        result = committed->second;
    }

    for (const auto& anim_state : mAnimations) {
        if (anim_state.name.empty()) {
            continue;
        }

        const auto it = mRenderAnimations.find(lower_copy(anim_state.name));
        if (it == mRenderAnimations.end()) {
            continue;
        }

        auto frame = anim_state.frame;
        if (it->second.loop && it->second.frame_size > 0U) {
            frame = std::fmod(frame, static_cast< float >(it->second.frame_size));
        }

        const auto layer_frame = it->second.pane_frame(pane_name, frame);
        if (layer_frame.translate_x.has_value()) {
            result.translate_x = layer_frame.translate_x;
        }
        if (layer_frame.translate_y.has_value()) {
            result.translate_y = layer_frame.translate_y;
        }
        if (layer_frame.scale_x.has_value()) {
            result.scale_x = layer_frame.scale_x;
        }
        if (layer_frame.scale_y.has_value()) {
            result.scale_y = layer_frame.scale_y;
        }
        if (layer_frame.rotate_z.has_value()) {
            result.rotate_z = layer_frame.rotate_z;
        }
        if (layer_frame.alpha.has_value()) {
            result.alpha = layer_frame.alpha;
        }
        if (layer_frame.visible.has_value()) {
            result.visible = layer_frame.visible;
        }
    }
    if (const auto* pane_anim = findPaneAnimation(pane_name)) {
        for (const auto& anim_state : pane_anim->animations) {
            if (anim_state.name.empty()) {
                continue;
            }

            const auto it = mRenderAnimations.find(lower_copy(anim_state.name));
            if (it == mRenderAnimations.end()) {
                continue;
            }

            auto frame = anim_state.frame;
            if (it->second.loop && it->second.frame_size > 0U) {
                frame = std::fmod(frame, static_cast< float >(it->second.frame_size));
            }

            const auto layer_frame = it->second.pane_frame(pane_name, frame);
            if (layer_frame.translate_x.has_value()) {
                result.translate_x = layer_frame.translate_x;
            }
            if (layer_frame.translate_y.has_value()) {
                result.translate_y = layer_frame.translate_y;
            }
            if (layer_frame.scale_x.has_value()) {
                result.scale_x = layer_frame.scale_x;
            }
            if (layer_frame.scale_y.has_value()) {
                result.scale_y = layer_frame.scale_y;
            }
            if (layer_frame.rotate_z.has_value()) {
                result.rotate_z = layer_frame.rotate_z;
            }
            if (layer_frame.alpha.has_value()) {
                result.alpha = layer_frame.alpha;
            }
            if (layer_frame.visible.has_value()) {
                result.visible = layer_frame.visible;
            }
        }
    }

    return result;
}

smgpc::layout::BrlanTextureFrame SimpleLayout::textureFrameForContent(std::string_view content_name) const {
    auto result = smgpc::layout::BrlanTextureFrame{};
    for (const auto& anim_state : mAnimations) {
        if (anim_state.name.empty()) {
            continue;
        }

        const auto it = mRenderAnimations.find(lower_copy(anim_state.name));
        if (it == mRenderAnimations.end()) {
            continue;
        }

        auto frame = anim_state.frame;
        if (it->second.loop && it->second.frame_size > 0U) {
            frame = std::fmod(frame, static_cast< float >(it->second.frame_size));
        }

        const auto layer_frame = it->second.texture_frame(content_name, frame);
        if (layer_frame.translate_s.has_value()) {
            result.translate_s = layer_frame.translate_s;
        }
        if (layer_frame.translate_t.has_value()) {
            result.translate_t = layer_frame.translate_t;
        }
        if (layer_frame.rotate.has_value()) {
            result.rotate = layer_frame.rotate;
        }
        if (layer_frame.scale_s.has_value()) {
            result.scale_s = layer_frame.scale_s;
        }
        if (layer_frame.scale_t.has_value()) {
            result.scale_t = layer_frame.scale_t;
        }
    }

    return result;
}

smgpc::layout::BrlanMaterialFrame SimpleLayout::materialFrameForContent(std::string_view content_name) const {
    auto result = smgpc::layout::BrlanMaterialFrame{};
    if (const auto committed = mCommittedMaterialFrames.find(std::string(content_name)); committed != mCommittedMaterialFrames.end()) {
        result = committed->second;
    }

    for (const auto& anim_state : mAnimations) {
        if (anim_state.name.empty()) {
            continue;
        }

        const auto it = mRenderAnimations.find(lower_copy(anim_state.name));
        if (it == mRenderAnimations.end()) {
            continue;
        }

        auto frame = anim_state.frame;
        if (it->second.loop && it->second.frame_size > 0U) {
            frame = std::fmod(frame, static_cast< float >(it->second.frame_size));
        }

        merge_material_frame(result, it->second.material_frame(content_name, frame));
    }

    for (const auto& pane_anim : mPaneAnimations) {
        if (pane_anim.pane_name != content_name) {
            continue;
        }

        for (const auto& anim_state : pane_anim.animations) {
            if (anim_state.name.empty()) {
                continue;
            }

            const auto it = mRenderAnimations.find(lower_copy(anim_state.name));
            if (it == mRenderAnimations.end()) {
                continue;
            }

            auto frame = anim_state.frame;
            if (it->second.loop && it->second.frame_size > 0U) {
                frame = std::fmod(frame, static_cast< float >(it->second.frame_size));
            }

            merge_material_frame(result, it->second.material_frame(content_name, frame));
        }
    }

    return result;
}

f32 SimpleLayout::durationFor(const char* pAnimName) const {
    const auto it = mRenderAnimations.find(lower_copy(pAnimName));
    if (it != mRenderAnimations.end() && it->second.frame_size > 0U) {
        return static_cast< f32 >(it->second.frame_size);
    }

    return duration_for(pAnimName);
}

bool SimpleLayout::isLoopingAnim(const char* pAnimName) const {
    const auto it = mRenderAnimations.find(lower_copy(pAnimName));
    if (it != mRenderAnimations.end()) {
        return it->second.loop;
    }

    return is_looping_anim(pAnimName);
}

float SimpleLayout::visualAlpha() const {
    for (const auto& anim_state : mAnimations) {
        if (!anim_state.name.empty() && mRenderAnimations.contains(lower_copy(anim_state.name))) {
            return 1.0F;
        }
    }

    const auto& anim = animation(0U);
    if (anim.name == "Appear" && anim.end > 0.0F) {
        return std::clamp(anim.frame / anim.end, 0.0F, 1.0F);
    }
    if ((anim.name == "Decide" || anim.name == "End") && anim.end > 0.0F) {
        return std::clamp(1.0F - anim.frame / anim.end, 0.0F, 1.0F);
    }

    return 1.0F;
}

f32 SimpleLayout::duration_for(const char* pAnimName) {
    if (std::strcmp(pAnimName, "Appear") == 0) {
        return 40.0f;
    }
    if (std::strcmp(pAnimName, "Decide") == 0 || std::strcmp(pAnimName, "End") == 0) {
        return 30.0f;
    }
    if (std::strcmp(pAnimName, "ReactionA") == 0 || std::strcmp(pAnimName, "ReactionB") == 0 || std::strcmp(pAnimName, "ButtonReaction") == 0) {
        return 12.0f;
    }

    return 1.0f;
}

bool SimpleLayout::is_looping_anim(const char* pAnimName) {
    return std::strcmp(pAnimName, "Wait") == 0;
}

SimpleEffectLayout::SimpleEffectLayout(const char* pName, const char* pLayoutName, u32 animLayerNum, int drawType)
    : SimpleLayout(pName, pLayoutName, animLayerNum, drawType) {
    initEffectKeeper(0, nullptr, nullptr);
}
