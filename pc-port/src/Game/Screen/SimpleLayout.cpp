#include "Game/Screen/SimpleLayout.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>

#include "Game/compat/RarcArchive.hpp"
#include "Game/compat/RuntimeContext.hpp"

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

    [[nodiscard]] const SimpleLayout::RenderTexture* find_texture(const std::vector< SimpleLayout::RenderTexture >& textures,
                                                                  std::string_view texture_name) {
        const auto it = std::ranges::find_if(textures, [texture_name](const auto& texture) { return texture.name == texture_name; });

        return it == textures.end() ? nullptr : &(*it);
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

    [[nodiscard]] SimpleLayout::RenderMaterialTexture* find_material_texture(std::vector< SimpleLayout::RenderMaterialTexture >& textures,
                                                                             std::size_t picture_index) {
        const auto it = std::ranges::find_if(textures, [picture_index](const auto& texture) { return texture.picture_index == picture_index; });

        return it == textures.end() ? nullptr : &(*it);
    }

    [[nodiscard]] const smgpc::game::RarcEntry* find_font_entry(const smgpc::game::RarcArchive& archive, std::string_view font_name) {
        const auto candidates = font_resource_candidates(font_name);
        const auto it = std::ranges::find_if(archive.entries(), [&candidates](const auto& entry) {
            return std::ranges::find(candidates, font_resource_name(entry.path)) != candidates.end();
        });

        return it == archive.entries().end() ? nullptr : &(*it);
    }

    [[nodiscard]] std::optional< std::filesystem::path >
    find_companion_font_archive(const std::optional< std::filesystem::path >& layout_archive_path) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
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

    [[nodiscard]] float glyph_advance(const smgpc::game::BrfntGlyph& glyph, float scale_x) {
        const auto char_width = glyph.widths.char_width == 0 ? static_cast< int >(glyph.width) : static_cast< int >(glyph.widths.char_width);
        return static_cast< float >(char_width) * scale_x;
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

    [[nodiscard]] std::uint16_t resolve_layout_glyph_code(std::uint16_t code, const smgpc::game::BrfntFont& font, bool use_button_icon_aliases) {
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

    [[nodiscard]] float gx_texture_v_to_host(float v) {
        return 1.0F - v;
    }

    [[nodiscard]] std::uint16_t texture_extent(float value) {
        return static_cast< std::uint16_t >(std::clamp(std::ceil(value), 1.0F, 4096.0F));
    }

    [[nodiscard]] smgpc::game::BrlytTexCoord transform_tex_coord(smgpc::game::BrlytTexCoord tex_coord, const smgpc::game::BrlanTextureFrame& frame) {
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

        return smgpc::game::BrlytTexCoord{
            .u = translate_s + 0.5F + cos_r * scale_s * centered_s - sin_r * scale_t * centered_t,
            .v = translate_t + 0.5F + sin_r * scale_s * centered_s + cos_r * scale_t * centered_t,
        };
    }

    [[nodiscard]] bool same_optional_float(const std::optional< float >& lhs, const std::optional< float >& rhs) {
        if (lhs.has_value() != rhs.has_value()) {
            return false;
        }
        if (!lhs.has_value()) {
            return true;
        }

        return std::abs(*lhs - *rhs) < 0.0001F;
    }

    [[nodiscard]] bool same_texture_frame(const smgpc::game::BrlanTextureFrame& lhs, const smgpc::game::BrlanTextureFrame& rhs) {
        return same_optional_float(lhs.translate_s, rhs.translate_s) && same_optional_float(lhs.translate_t, rhs.translate_t) &&
               same_optional_float(lhs.rotate, rhs.rotate) && same_optional_float(lhs.scale_s, rhs.scale_s) &&
               same_optional_float(lhs.scale_t, rhs.scale_t);
    }

    [[nodiscard]] bool material_requires_cpu_composition(const smgpc::game::BrlytMaterial& material) {
        return material.textures.size() != 1U || !material.tev_stages.empty();
    }

    [[nodiscard]] std::uint16_t tex_srt_index_for_coord_gen(const smgpc::game::BrlytTexCoordGen& coord_gen) {
        constexpr auto gx_texmtx0 = std::uint8_t{30U};
        constexpr auto gx_identity = std::uint8_t{60U};
        if (coord_gen.tex_mtx == gx_identity || coord_gen.tex_mtx < gx_texmtx0) {
            return UINT16_MAX;
        }

        return static_cast< std::uint16_t >((coord_gen.tex_mtx - gx_texmtx0) / 3U);
    }

    [[nodiscard]] smgpc::game::BrlytTexCoord transform_tex_coord(smgpc::game::BrlytTexCoord tex_coord, const smgpc::game::BrlytTexSrt& srt,
                                                                 const smgpc::game::BrlanTextureFrame& texture_frame, bool apply_animation) {
        auto frame = smgpc::game::BrlanTextureFrame{
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

    [[nodiscard]] float wrap_or_clamp(float value, bool wrap) {
        if (wrap) {
            value -= std::floor(value);
            if (value < 0.0F) {
                value += 1.0F;
            }
            return value;
        }

        return std::clamp(value, 0.0F, 1.0F);
    }

    [[nodiscard]] std::array< int, 4U > sample_texture(const smgpc::game::DecodedTexture& texture, smgpc::game::BrlytTexCoord tex_coord, bool wrap_s,
                                                       bool wrap_t) {
        if (texture.width == 0U || texture.height == 0U || texture.rgba.empty()) {
            return {0, 0, 0, 0};
        }

        const auto u = wrap_or_clamp(tex_coord.u, wrap_s);
        const auto v = wrap_or_clamp(tex_coord.v, wrap_t);
        const auto x = static_cast< std::uint32_t >(
            std::clamp(std::round(u * static_cast< float >(texture.width - 1U)), 0.0F, static_cast< float >(texture.width - 1U)));
        const auto y = static_cast< std::uint32_t >(
            std::clamp(std::round(v * static_cast< float >(texture.height - 1U)), 0.0F, static_cast< float >(texture.height - 1U)));
        const auto offset = (static_cast< std::size_t >(y) * texture.width + x) * 4U;
        return {
            texture.rgba[offset],
            texture.rgba[offset + 1U],
            texture.rgba[offset + 2U],
            texture.rgba[offset + 3U],
        };
    }

    [[nodiscard]] std::array< int, 4U > konst_color(const smgpc::game::BrlytMaterial& material, std::uint8_t selector) {
        constexpr std::array< int, 8U > constants{255, 223, 191, 159, 128, 96, 64, 32};
        if (selector < constants.size()) {
            return {constants[selector], constants[selector], constants[selector], constants[selector]};
        }
        if (selector >= 12U && selector <= 15U) {
            const auto& color = material.tev_k_colors[selector - 12U];
            return {color[0U], color[1U], color[2U], color[3U]};
        }
        if (selector >= 16U && selector <= 31U) {
            const auto color_index = (selector - 16U) % 4U;
            const auto component = (selector - 16U) / 4U;
            const auto value = material.tev_k_colors[color_index][component];
            return {value, value, value, value};
        }

        return {0, 0, 0, 0};
    }

    [[nodiscard]] int color_arg_value(std::uint8_t arg, std::size_t component, const std::array< std::array< int, 4U >, 4U >& registers,
                                      const std::array< int, 4U >& texture, const std::array< int, 4U >& ras, const std::array< int, 4U >& konst) {
        switch (arg) {
        case 0U:
            return registers[0U][component];
        case 1U:
            return registers[0U][3U];
        case 2U:
            return registers[1U][component];
        case 3U:
            return registers[1U][3U];
        case 4U:
            return registers[2U][component];
        case 5U:
            return registers[2U][3U];
        case 6U:
            return registers[3U][component];
        case 7U:
            return registers[3U][3U];
        case 8U:
            return texture[component];
        case 9U:
            return texture[3U];
        case 10U:
            return ras[component];
        case 11U:
            return ras[3U];
        case 12U:
            return 255;
        case 13U:
            return 128;
        case 14U:
            return konst[component];
        default:
            return 0;
        }
    }

    [[nodiscard]] int alpha_arg_value(std::uint8_t arg, const std::array< std::array< int, 4U >, 4U >& registers,
                                      const std::array< int, 4U >& texture, const std::array< int, 4U >& ras, const std::array< int, 4U >& konst) {
        switch (arg) {
        case 0U:
            return registers[0U][3U];
        case 1U:
            return registers[1U][3U];
        case 2U:
            return registers[2U][3U];
        case 3U:
            return registers[3U][3U];
        case 4U:
            return texture[3U];
        case 5U:
            return ras[3U];
        case 6U:
            return konst[3U];
        default:
            return 0;
        }
    }

    [[nodiscard]] int tev_regular(const smgpc::game::BrlytTevStageInOp& op, int a, int b, int c, int d) {
        constexpr std::array< int, 4U > bias{0, 128, -128, 0};
        constexpr std::array< int, 4U > scale_lshift{0, 1, 2, 0};
        constexpr std::array< int, 4U > scale_rshift{0, 0, 0, 1};
        const auto scale = std::min< std::uint8_t >(op.scale, 3U);
        const auto c256 = c + (c >> 7);
        auto temp = a * (256 - c256) + b * c256;
        temp <<= scale_lshift[scale];
        if (scale != 3U) {
            temp += op.op == 1U ? 127 : 128;
        }
        temp >>= 8;
        if (op.op == 1U) {
            temp = -temp;
        }

        auto result = ((d + bias[std::min< std::uint8_t >(op.bias, 3U)]) << scale_lshift[scale]) + temp;
        result >>= scale_rshift[scale];
        if (op.clamp) {
            return std::clamp(result, 0, 255);
        }

        return std::clamp(result, -1024, 1023);
    }

    [[nodiscard]] bool compare_alpha(int alpha, std::uint8_t compare, std::uint8_t reference) {
        switch (compare) {
        case 0U:
            return false;
        case 1U:
            return alpha < reference;
        case 2U:
            return alpha == reference;
        case 3U:
            return alpha <= reference;
        case 4U:
            return alpha > reference;
        case 5U:
            return alpha != reference;
        case 6U:
            return alpha >= reference;
        default:
            return true;
        }
    }

    [[nodiscard]] bool passes_alpha_compare(const smgpc::game::BrlytAlphaCompare& alpha_compare, int alpha) {
        if (!alpha_compare.enabled) {
            return true;
        }

        const auto lhs = compare_alpha(alpha, alpha_compare.comp0, alpha_compare.ref0);
        const auto rhs = compare_alpha(alpha, alpha_compare.comp1, alpha_compare.ref1);
        switch (alpha_compare.op) {
        case 1U:
            return lhs || rhs;
        case 2U:
            return lhs != rhs;
        case 3U:
            return lhs == rhs;
        default:
            return lhs && rhs;
        }
    }

}  // namespace

SimpleLayout::SimpleLayout(const char* pName, const char* pLayoutName, u32 animLayerNum, int)
    : mName(pName), mLayoutName(pLayoutName), mAnimLayerNum(std::min< u32 >(animLayerNum, 4U)) {
    if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
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
    if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->unregister_layout(*this);
    }
}

void SimpleLayout::initWithoutIter() {
}

void SimpleLayout::initEffectKeeper(int, const char*, const void*) {
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

    for (auto& anim : mAnimations) {
        if (anim.name.empty() || anim.stopped) {
            continue;
        }

        if (anim.looping) {
            anim.frame += anim.rate;
            if (anim.end > 0.0F && anim.frame >= anim.end) {
                anim.frame = std::fmod(anim.frame, anim.end);
            }
            continue;
        }

        anim.frame += anim.rate;
        if (anim.frame >= anim.end || anim.frame <= 0.0F) {
            anim.frame = std::clamp(anim.frame, 0.0F, anim.end);
            anim.stopped = true;
        }
    }
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

void SimpleLayout::draw(smgpc::render::IRendererEngine& renderer) {
    if (mIsDead) {
        return;
    }

    loadRenderData();
    ensureTextureUploads(renderer);

    const auto alpha = visualAlpha();
    for (std::size_t picture_index = 0U; picture_index < mBrlytLayout.pictures.size(); ++picture_index) {
        const auto& picture = mBrlytLayout.pictures[picture_index];
        if (!picture.visible || picture.pane_index >= mBrlytLayout.panes.size()) {
            continue;
        }

        if (picture.material_index >= mBrlytLayout.materials.size()) {
            continue;
        }
        const auto& material = mBrlytLayout.materials[picture.material_index];
        const auto content_name = material.name.empty() ? std::string_view(picture.name) : std::string_view(material.name);

        const auto pane_state = paneRenderState(picture.pane_index);
        if (!pane_state.visible) {
            continue;
        }

        const auto& pane = mBrlytLayout.panes[picture.pane_index];
        const auto width = pane.width * pane_state.scale_x;
        const auto height = pane.height * pane_state.scale_y;
        const auto x = pane_state.translate_x + base_position_x(pane.base_position, width);
        const auto y = pane_state.translate_y + base_position_y(pane.base_position, height);
        const auto texture_frame = textureFrameForContent(content_name);
        const auto vertex_color = [&](std::size_t index) {
            const auto& source = picture.vertex_colors[index];
            return std::array< std::uint8_t, 4U >{
                source[0U],
                source[1U],
                source[2U],
                static_cast< std::uint8_t >(std::clamp(alpha * pane_state.alpha * (static_cast< float >(source[3U]) / 255.0F), 0.0F, 255.0F)),
            };
        };

        if (material_requires_cpu_composition(material)) {
            auto* material_texture = ensureMaterialTextureUpload(renderer, picture_index, texture_frame);
            if (material_texture == nullptr || !material_texture->handle.is_valid()) {
                continue;
            }

            const auto color = std::array< std::uint8_t, 4U >{
                255U,
                255U,
                255U,
                static_cast< std::uint8_t >(std::clamp(alpha * pane_state.alpha, 0.0F, 255.0F)),
            };
            const auto top_v = gx_texture_v_to_host(0.0F);
            const auto bottom_v = gx_texture_v_to_host(1.0F);
            renderer.submit_textured_quad(
                material_texture->handle,
                smgpc::render::TexturedQuad2D{
                    .vertices =
                        {
                            smgpc::render::TexturedVertex2D{.x = x, .y = y, .z = 0.0F, .u = 0.0F, .v = top_v, .color = color},
                            smgpc::render::TexturedVertex2D{.x = x + width, .y = y, .z = 0.0F, .u = 1.0F, .v = top_v, .color = color},
                            smgpc::render::TexturedVertex2D{.x = x + width, .y = y + height, .z = 0.0F, .u = 1.0F, .v = bottom_v, .color = color},
                            smgpc::render::TexturedVertex2D{.x = x, .y = y + height, .z = 0.0F, .u = 0.0F, .v = bottom_v, .color = color},
                        },
                });
            continue;
        }

        auto* texture = find_texture(mRenderTextures, picture.texture_name);
        if (texture == nullptr || !texture->handle.is_valid()) {
            continue;
        }

        const auto vertex = [&](std::size_t index, float vx, float vy) {
            const auto tex_coord = transform_tex_coord(picture.tex_coords[index], texture_frame);
            return smgpc::render::TexturedVertex2D{
                .x = vx,
                .y = vy,
                .u = tex_coord.u,
                .v = gx_texture_v_to_host(tex_coord.v),
                .color = vertex_color(index),
            };
        };

        renderer.submit_textured_quad(texture->handle, smgpc::render::TexturedQuad2D{
                                                           .vertices =
                                                               {
                                                                   vertex(0U, x, y),
                                                                   vertex(1U, x + width, y),
                                                                   vertex(2U, x + width, y + height),
                                                                   vertex(3U, x, y + height),
                                                               },
                                                           .wrap_u = picture.wrap_s != 0U,
                                                           .wrap_v = picture.wrap_t != 0U,
                                                       });
    }

    drawTextBoxes(renderer, alpha);
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

SimpleLayout::AnimationState& SimpleLayout::animation(u32 animLayer) {
    return mAnimations.at(std::min< std::size_t >(animLayer, mAnimations.size() - 1U));
}

const SimpleLayout::AnimationState& SimpleLayout::animation(u32 animLayer) const {
    return mAnimations.at(std::min< std::size_t >(animLayer, mAnimations.size() - 1U));
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
        if (pane_frame.alpha.has_value()) {
            committed.alpha = pane_frame.alpha;
        }
        if (pane_frame.visible.has_value()) {
            committed.visible = pane_frame.visible;
        }
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
        const auto archive = smgpc::game::RarcArchive::from_file(*mArchivePath);
        const auto brlyt_it = std::ranges::find_if(archive.entries(), [](const auto& entry) { return ends_with(entry.path, ".brlyt"); });
        if (brlyt_it == archive.entries().end()) {
            return;
        }

        mBrlytLayout = smgpc::game::parse_brlyt_layout(archive.file_data(*brlyt_it));
        for (const auto& entry : archive.entries()) {
            if (!ends_with(entry.path, ".brlan")) {
                continue;
            }

            try {
                mRenderAnimations[animation_name_from_path(entry.path)] = smgpc::game::parse_brlan_animation(archive.file_data(entry));
            } catch (const std::exception& e) {
                if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
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
                if (!archive.contains(texture_path)) {
                    continue;
                }

                try {
                    mRenderTextures.push_back(RenderTexture{
                        .name = material_texture.texture_name,
                        .decoded = smgpc::game::decode_tpl_texture(archive.file_data(texture_path)),
                        .handle = {},
                    });
                } catch (const std::exception& e) {
                    if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
                        runtime->note_layout_texture_decode_failed(mLayoutName, material_texture.texture_name, e.what());
                    }
                }
            }
        }

        if (!mBrlytLayout.text_boxes.empty()) {
            if (const auto font_path = find_companion_font_archive(mArchivePath)) {
                try {
                    const auto font_archive = smgpc::game::RarcArchive::from_file(*font_path);
                    for (const auto& text_box : mBrlytLayout.text_boxes) {
                        if (contains_font(mRenderFonts, text_box.font_name)) {
                            continue;
                        }

                        const auto* font_entry = find_font_entry(font_archive, text_box.font_name);
                        if (font_entry == nullptr) {
                            continue;
                        }

                        auto font = smgpc::game::parse_brfnt_font(font_archive.file_data(*font_entry));
                        mRenderFonts.push_back(RenderFont{
                            .name = text_box.font_name,
                            .font = std::move(font),
                            .sheet_handles = {},
                        });
                        mRenderFonts.back().sheet_handles.resize(mRenderFonts.back().font.sheets.size());
                    }
                } catch (const std::exception& e) {
                    if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
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
        mRenderMaterialTextures.clear();
        mRenderAnimations.clear();
        mCommittedPaneFrames.clear();
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_layout_texture_decode_failed(mLayoutName, "<layout>", e.what());
        }
    }
}

void SimpleLayout::ensureTextureUploads(smgpc::render::IRendererEngine& renderer) {
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

void SimpleLayout::ensureTextTextureUploads(smgpc::render::IRendererEngine& renderer) {
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

SimpleLayout::RenderMaterialTexture SimpleLayout::composeMaterialTexture(std::size_t picture_index,
                                                                         const smgpc::game::BrlanTextureFrame& texture_frame) const {
    const auto& picture = mBrlytLayout.pictures.at(picture_index);
    const auto& material = mBrlytLayout.materials.at(picture.material_index);
    const auto width = texture_extent(std::max(1.0F, mBrlytLayout.panes.at(picture.pane_index).width));
    const auto height = texture_extent(std::max(1.0F, mBrlytLayout.panes.at(picture.pane_index).height));

    auto texture = RenderMaterialTexture{
        .picture_index = picture_index,
        .texture_frame = texture_frame,
        .width = width,
        .height = height,
        .rgba = {},
        .handle = {},
    };
    texture.rgba.assign(static_cast< std::size_t >(texture.width) * texture.height * 4U, 0U);

    const auto interpolate_tex_coord = [&](float x, float y) {
        const auto top = smgpc::game::BrlytTexCoord{
            .u = picture.tex_coords[0U].u + (picture.tex_coords[1U].u - picture.tex_coords[0U].u) * x,
            .v = picture.tex_coords[0U].v + (picture.tex_coords[1U].v - picture.tex_coords[0U].v) * x,
        };
        const auto bottom = smgpc::game::BrlytTexCoord{
            .u = picture.tex_coords[3U].u + (picture.tex_coords[2U].u - picture.tex_coords[3U].u) * x,
            .v = picture.tex_coords[3U].v + (picture.tex_coords[2U].v - picture.tex_coords[3U].v) * x,
        };
        return smgpc::game::BrlytTexCoord{
            .u = top.u + (bottom.u - top.u) * y,
            .v = top.v + (bottom.v - top.v) * y,
        };
    };

    const auto interpolate_vertex_color = [&](float x, float y) {
        auto color = std::array< int, 4U >{};
        for (auto component = 0U; component < color.size(); ++component) {
            const auto top =
                static_cast< float >(picture.vertex_colors[0U][component]) +
                (static_cast< float >(picture.vertex_colors[1U][component]) - static_cast< float >(picture.vertex_colors[0U][component])) * x;
            const auto bottom =
                static_cast< float >(picture.vertex_colors[3U][component]) +
                (static_cast< float >(picture.vertex_colors[2U][component]) - static_cast< float >(picture.vertex_colors[3U][component])) * x;
            color[component] = static_cast< int >(std::round(top + (bottom - top) * y));
        }
        return color;
    };

    const auto sample_material_texture = [&](const smgpc::game::BrlytMaterialTexture& material_texture, smgpc::game::BrlytTexCoord base_coord,
                                             std::size_t tex_coord_gen_index) {
        auto tex_coord = base_coord;
        if (tex_coord_gen_index < material.tex_coord_gens.size()) {
            const auto tex_srt_index = tex_srt_index_for_coord_gen(material.tex_coord_gens[tex_coord_gen_index]);
            if (tex_srt_index != UINT16_MAX && tex_srt_index < material.tex_srts.size()) {
                tex_coord = transform_tex_coord(base_coord, material.tex_srts[tex_srt_index], texture_frame, tex_srt_index == 0U);
            }
        }

        const auto* render_texture = find_texture(mRenderTextures, material_texture.texture_name);
        if (render_texture == nullptr) {
            return std::array< int, 4U >{0, 0, 0, 0};
        }

        return sample_texture(render_texture->decoded, tex_coord, material_texture.wrap_s != 0U, material_texture.wrap_t != 0U);
    };

    for (auto y = 0U; y < texture.height; ++y) {
        const auto normalized_y = (static_cast< float >(y) + 0.5F) / static_cast< float >(texture.height);
        for (auto x = 0U; x < texture.width; ++x) {
            const auto normalized_x = (static_cast< float >(x) + 0.5F) / static_cast< float >(texture.width);
            const auto base_coord = interpolate_tex_coord(normalized_x, normalized_y);
            const auto ras = interpolate_vertex_color(normalized_x, normalized_y);

            auto registers = std::array< std::array< int, 4U >, 4U >{};
            for (auto component = 0U; component < 4U; ++component) {
                registers[1U][component] = material.tev_colors[0U][component];
                registers[2U][component] = material.tev_colors[1U][component];
                registers[3U][component] = material.tev_colors[2U][component];
            }

            auto output = std::array< int, 4U >{0, 0, 0, 0};
            if (material.tev_stages.empty() && !material.textures.empty()) {
                output = sample_material_texture(material.textures.front(), base_coord, 0U);
            } else {
                std::uint8_t last_color_register = 0U;
                std::uint8_t last_alpha_register = 0U;
                for (const auto& stage : material.tev_stages) {
                    auto sampled = std::array< int, 4U >{0, 0, 0, 0};
                    if (stage.tex_map < material.textures.size()) {
                        sampled = sample_material_texture(material.textures[stage.tex_map], base_coord, stage.tex_coord_gen);
                    }

                    const auto color_konst = konst_color(material, stage.color.k_sel);
                    const auto alpha_konst = konst_color(material, stage.alpha.k_sel);
                    const auto stage_konst = std::array< int, 4U >{color_konst[0U], color_konst[1U], color_konst[2U], alpha_konst[3U]};
                    for (auto component = 0U; component < 3U; ++component) {
                        const auto a = color_arg_value(stage.color.a, component, registers, sampled, ras, stage_konst);
                        const auto b = color_arg_value(stage.color.b, component, registers, sampled, ras, stage_konst);
                        const auto c = color_arg_value(stage.color.c, component, registers, sampled, ras, stage_konst);
                        const auto d = color_arg_value(stage.color.d, component, registers, sampled, ras, stage_konst);
                        registers[stage.color.out_reg][component] = tev_regular(stage.color, a, b, c, d);
                    }

                    const auto alpha_a = alpha_arg_value(stage.alpha.a, registers, sampled, ras, stage_konst);
                    const auto alpha_b = alpha_arg_value(stage.alpha.b, registers, sampled, ras, stage_konst);
                    const auto alpha_c = alpha_arg_value(stage.alpha.c, registers, sampled, ras, stage_konst);
                    const auto alpha_d = alpha_arg_value(stage.alpha.d, registers, sampled, ras, stage_konst);
                    registers[stage.alpha.out_reg][3U] = tev_regular(stage.alpha, alpha_a, alpha_b, alpha_c, alpha_d);
                    last_color_register = stage.color.out_reg;
                    last_alpha_register = stage.alpha.out_reg;
                }

                output = registers[last_color_register];
                output[3U] = registers[last_alpha_register][3U];
            }

            if (!passes_alpha_compare(material.alpha_compare, output[3U])) {
                output = {0, 0, 0, 0};
            }

            const auto offset = (static_cast< std::size_t >(y) * texture.width + x) * 4U;
            texture.rgba[offset] = static_cast< std::uint8_t >(std::clamp(output[0U], 0, 255));
            texture.rgba[offset + 1U] = static_cast< std::uint8_t >(std::clamp(output[1U], 0, 255));
            texture.rgba[offset + 2U] = static_cast< std::uint8_t >(std::clamp(output[2U], 0, 255));
            texture.rgba[offset + 3U] = static_cast< std::uint8_t >(std::clamp(output[3U], 0, 255));
        }
    }

    return texture;
}

SimpleLayout::RenderMaterialTexture* SimpleLayout::ensureMaterialTextureUpload(smgpc::render::IRendererEngine& renderer, std::size_t picture_index,
                                                                               const smgpc::game::BrlanTextureFrame& texture_frame) {
    auto* existing = find_material_texture(mRenderMaterialTextures, picture_index);
    if (existing != nullptr && existing->handle.is_valid() && same_texture_frame(existing->texture_frame, texture_frame)) {
        return existing;
    }

    if (existing != nullptr && existing->handle.is_valid()) {
        renderer.destroy_texture(existing->handle);
        existing->handle = {};
    }

    auto composed = composeMaterialTexture(picture_index, texture_frame);
    composed.handle =
        renderer.create_rgba8_texture(composed.width, composed.height, std::span< const std::uint8_t >(composed.rgba.data(), composed.rgba.size()));
    if (existing != nullptr) {
        *existing = std::move(composed);
        return existing;
    }

    mRenderMaterialTextures.push_back(std::move(composed));
    return &mRenderMaterialTextures.back();
}

SimpleLayout::RenderTextTexture SimpleLayout::composeTextTexture(std::size_t text_box_index, const RenderFont& render_font) const {
    const auto& text_box = mBrlytLayout.text_boxes[text_box_index];
    const auto& font = render_font.font;
    const auto scale_x = text_box.font_width > 0.0F ? text_box.font_width / static_cast< float >(font.width) : 1.0F;
    const auto native_char_space = scale_x > 0.0F ? text_box.char_space / scale_x : text_box.char_space;
    const auto use_button_icon_aliases = text_uses_button_icon_aliases(std::span< const std::uint16_t >(text_box.text.data(), text_box.text.size()));

    auto native_line_width = 0.0F;
    for (const auto code : text_box.text) {
        if (const auto glyph = font.glyph_for(resolve_layout_glyph_code(code, font, use_button_icon_aliases))) {
            native_line_width += glyph_advance(*glyph, 1.0F) + native_char_space;
        }
    }
    if (!text_box.text.empty()) {
        native_line_width -= native_char_space;
    }

    auto texture = RenderTextTexture{
        .text_box_index = text_box_index,
        .width = texture_extent(native_line_width),
        .height = std::max< std::uint16_t >(font.height, 1U),
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

    auto cursor_x = 0.0F;
    for (const auto code : text_box.text) {
        const auto glyph_code = resolve_layout_glyph_code(code, font, use_button_icon_aliases);
        const auto glyph = font.glyph_for(glyph_code);
        if (!glyph.has_value() || glyph->sheet_index >= font.sheets.size()) {
            continue;
        }

        const auto is_button_icon = is_button_icon_code(glyph_code);
        const auto& sheet = font.sheets[glyph->sheet_index];
        const auto glyph_x = static_cast< int >(std::round(cursor_x)) + static_cast< int >(glyph->widths.left);
        for (auto y = 0U; y < glyph->height; ++y) {
            for (auto x = 0U; x < glyph->width; ++x) {
                const auto source_x = static_cast< std::uint16_t >(glyph->x + x);
                const auto source_y = static_cast< std::uint16_t >(glyph->y + y);
                if (source_x >= sheet.width || source_y >= sheet.height) {
                    continue;
                }

                const auto dest_x = glyph_x + static_cast< int >(x);
                const auto dest_y = static_cast< int >(y);
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
                    if (is_button_icon) {
                        texture.rgba[dest_offset] =
                            static_cast< std::uint8_t >((static_cast< std::uint16_t >(sheet.rgba[source_offset]) * mapped_color[0U]) / 255U);
                        texture.rgba[dest_offset + 1U] =
                            static_cast< std::uint8_t >((static_cast< std::uint16_t >(sheet.rgba[source_offset + 1U]) * mapped_color[1U]) / 255U);
                        texture.rgba[dest_offset + 2U] =
                            static_cast< std::uint8_t >((static_cast< std::uint16_t >(sheet.rgba[source_offset + 2U]) * mapped_color[2U]) / 255U);
                        texture.rgba[dest_offset + 3U] = dest_alpha;
                        continue;
                    }

                    texture.rgba[dest_offset] = mapped_color[0U];
                    texture.rgba[dest_offset + 1U] = mapped_color[1U];
                    texture.rgba[dest_offset + 2U] = mapped_color[2U];
                    texture.rgba[dest_offset + 3U] = dest_alpha;
                }
            }
        }

        cursor_x += glyph_advance(*glyph, 1.0F) + native_char_space;
    }

    return texture;
}

void SimpleLayout::drawTextBoxes(smgpc::render::IRendererEngine& renderer, float alpha) {
    for (std::size_t text_box_index = 0U; text_box_index < mBrlytLayout.text_boxes.size(); ++text_box_index) {
        const auto& text_box = mBrlytLayout.text_boxes[text_box_index];
        if (!text_box.visible || text_box.pane_index >= mBrlytLayout.panes.size()) {
            continue;
        }

        auto* text_texture = find_text_texture(mRenderTextTextures, text_box_index);
        if (text_texture == nullptr || !text_texture->handle.is_valid()) {
            continue;
        }

        const auto pane_state = paneRenderState(text_box.pane_index);
        if (!pane_state.visible) {
            continue;
        }

        const auto& pane = mBrlytLayout.panes[text_box.pane_index];
        const auto box_width = pane.width * pane_state.scale_x;
        const auto box_height = pane.height * pane_state.scale_y;
        const auto box_x = pane_state.translate_x + base_position_x(pane.base_position, box_width);
        const auto box_y = pane_state.translate_y + base_position_y(pane.base_position, box_height);
        const auto font_width = text_box.font_width * pane_state.scale_x;
        const auto font_height = text_box.font_height * pane_state.scale_y;
        const auto scale_x = font_width / static_cast< float >(text_texture->font_width);
        const auto scale_y = font_height / static_cast< float >(text_texture->font_height);
        const auto line_width = static_cast< float >(text_texture->width) * scale_x;

        const auto horizontal_factor = text_factor(text_box.text_position % 3U);
        const auto vertical_factor = text_factor(text_box.text_position / 3U);
        const auto cursor_x = box_x + (box_width - line_width) * horizontal_factor;
        const auto cursor_y = box_y + (box_height - font_height) * vertical_factor;
        const auto color = std::array< std::uint8_t, 4U >{
            255U,
            255U,
            255U,
            static_cast< std::uint8_t >(std::clamp(alpha * pane_state.alpha, 0.0F, 255.0F)),
        };
        const auto width = line_width;
        const auto height = static_cast< float >(text_texture->height) * scale_y;
        const auto top_v = gx_texture_v_to_host(0.0F);
        const auto bottom_v = gx_texture_v_to_host(1.0F);

        renderer.submit_textured_quad(
            text_texture->handle,
            smgpc::render::TexturedQuad2D{
                .vertices =
                    {
                        smgpc::render::TexturedVertex2D{.x = cursor_x, .y = cursor_y, .z = 0.0F, .u = 0.0F, .v = top_v, .color = color},
                        smgpc::render::TexturedVertex2D{.x = cursor_x + width, .y = cursor_y, .z = 0.0F, .u = 1.0F, .v = top_v, .color = color},
                        smgpc::render::TexturedVertex2D{
                            .x = cursor_x + width, .y = cursor_y + height, .z = 0.0F, .u = 1.0F, .v = bottom_v, .color = color},
                        smgpc::render::TexturedVertex2D{.x = cursor_x, .y = cursor_y + height, .z = 0.0F, .u = 0.0F, .v = bottom_v, .color = color},
                    },
            });
    }
}

SimpleLayout::PaneRenderState SimpleLayout::paneRenderState(std::size_t pane_index) const {
    const auto& pane = mBrlytLayout.panes.at(pane_index);
    auto local_translate_x = pane.translate_x;
    auto local_translate_y = pane.translate_y;
    auto local_scale_x = pane.scale_x;
    auto local_scale_y = pane.scale_y;
    auto local_alpha = static_cast< float >(pane.alpha);
    auto local_visible = pane.visible;

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
    if (anim.alpha.has_value()) {
        local_alpha = *anim.alpha;
    }
    if (anim.visible.has_value()) {
        local_visible = *anim.visible;
    }

    if (pane.parent_index < 0) {
        return PaneRenderState{
            .translate_x = local_translate_x,
            .translate_y = local_translate_y,
            .scale_x = local_scale_x,
            .scale_y = local_scale_y,
            .alpha = local_alpha,
            .visible = local_visible,
        };
    }

    const auto parent = paneRenderState(static_cast< std::size_t >(pane.parent_index));
    return PaneRenderState{
        .translate_x = parent.translate_x + local_translate_x * parent.scale_x,
        .translate_y = parent.translate_y + local_translate_y * parent.scale_y,
        .scale_x = parent.scale_x * local_scale_x,
        .scale_y = parent.scale_y * local_scale_y,
        .alpha = parent.alpha * (local_alpha / 255.0F),
        .visible = parent.visible && local_visible,
    };
}

smgpc::game::BrlanPaneFrame SimpleLayout::animationFrameForPane(std::string_view pane_name) const {
    auto result = smgpc::game::BrlanPaneFrame{};
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
        if (layer_frame.alpha.has_value()) {
            result.alpha = layer_frame.alpha;
        }
        if (layer_frame.visible.has_value()) {
            result.visible = layer_frame.visible;
        }
    }

    return result;
}

smgpc::game::BrlanTextureFrame SimpleLayout::textureFrameForContent(std::string_view content_name) const {
    auto result = smgpc::game::BrlanTextureFrame{};
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
