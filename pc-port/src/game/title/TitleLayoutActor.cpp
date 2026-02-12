#include "TitleLayoutActor.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include "layout/Binary.hpp"

namespace smgpc::game::title {
namespace {

[[nodiscard]] std::string baseWithoutExtension(std::string text) {
    const auto slash = text.find_last_of("/\\");
    if (slash != std::string::npos) {
        text = text.substr(slash + 1U);
    }

    const auto dot = text.find_last_of('.');
    if (dot != std::string::npos) {
        text = text.substr(0U, dot);
    }

    return text;
}

}  // namespace

TitleLayoutActor::TitleLayoutActor(const TitleLayoutResource *resource)
    : mResource(resource) {
    if (mResource == nullptr) {
        return;
    }

    mBaseStates.reserve(mResource->layout.panes.size());
    mCurrentStates.reserve(mResource->layout.panes.size());

    for (std::size_t paneIndex = 0; paneIndex < mResource->layout.panes.size(); ++paneIndex) {
        const auto &pane = mResource->layout.panes[paneIndex];

        RuntimePaneState state {};
        state.visible = pane.visible;
        state.alpha = pane.alpha;
        state.tx = pane.translate.x;
        state.ty = pane.translate.y;
        state.tz = pane.translate.z;
        state.sx = pane.scale.x;
        state.sy = pane.scale.y;
        state.width = pane.size.x;
        state.height = pane.size.y;

        for (std::size_t i = 0; i < pane.vertex_colors.size(); ++i) {
            const auto &color = pane.vertex_colors[i];
            state.vertexColor[i * 4U + 0U] = color.r;
            state.vertexColor[i * 4U + 1U] = color.g;
            state.vertexColor[i * 4U + 2U] = color.b;
            state.vertexColor[i * 4U + 3U] = color.a;
        }

        mBaseStates.push_back(state);
        mCurrentStates.push_back(state);

        mPaneIndexByName.emplace(normalizeName(pane.name), paneIndex);
    }

    mIsAlive = false;
}

std::string TitleLayoutActor::normalizeName(std::string name) {
    return assets::layout::binary::to_lower_ascii(std::move(name));
}

float TitleLayoutActor::anchorOffsetX(std::uint8_t basePosition, float width) {
    switch (basePosition % 3U) {
    case 0U:
        return 0.0F;
    case 1U:
        return -width * 0.5F;
    case 2U:
        return -width;
    default:
        return 0.0F;
    }
}

float TitleLayoutActor::anchorOffsetY(std::uint8_t basePosition, float height) {
    const auto row = static_cast<std::uint8_t>((basePosition / 3U) % 3U);
    switch (row) {
    case 0U:
        return 0.0F;
    case 1U:
        return -height * 0.5F;
    case 2U:
        return -height;
    default:
        return 0.0F;
    }
}

std::uint8_t TitleLayoutActor::clampU8(float value) {
    if (value <= 0.0F) {
        return 0U;
    }
    if (value >= 255.0F) {
        return 255U;
    }
    return static_cast<std::uint8_t>(std::lround(value));
}

const assets::layout::PaneDefinition *TitleLayoutActor::paneDefinition(std::size_t index) const {
    if (mResource == nullptr || index >= mResource->layout.panes.size()) {
        return nullptr;
    }
    return &mResource->layout.panes[index];
}

const TitleLayoutActor::RuntimePaneState *TitleLayoutActor::paneState(std::size_t index) const {
    if (index >= mCurrentStates.size()) {
        return nullptr;
    }
    return &mCurrentStates[index];
}

void TitleLayoutActor::resetPose() {
    mCurrentStates = mBaseStates;
}

void TitleLayoutActor::rebuildPose() {
    for (const auto &slot : mAnimationLayers) {
        if (slot.animation == nullptr) {
            continue;
        }

        applyAnimation(*slot.animation, slot.frame);
    }
}

void TitleLayoutActor::applyAnimation(const assets::layout::BrlanAnimation &animation, float frame) {
    const auto sampled_frame = animation.normalize_frame(frame);

    for (const auto &track : animation.tracks) {
        const auto pane_found = mPaneIndexByName.find(normalizeName(track.pane_name));
        if (pane_found == mPaneIndexByName.end()) {
            continue;
        }

        const auto paneIndex = pane_found->second;
        if (paneIndex >= mCurrentStates.size()) {
            continue;
        }

        auto &state = mCurrentStates[paneIndex];
        const auto value = track.sample(sampled_frame);

        if (track.kind == "RLPA") {
            switch (track.target) {
            case 0U:
                state.tx = value;
                break;
            case 1U:
                state.ty = value;
                break;
            case 2U:
                state.tz = value;
                break;
            case 6U:
                state.sx = value;
                break;
            case 7U:
                state.sy = value;
                break;
            default:
                break;
            }
            continue;
        }

        if (track.kind == "RLVC") {
            if (track.target == 16U) {
                state.alpha = clampU8(value);
                continue;
            }

            if (track.target < 16U) {
                state.vertexColor[track.target] = clampU8(value);
            }
            continue;
        }

        if (track.kind == "RLMC") {
            if (track.target == 3U || track.target == 7U || track.target == 11U || track.target == 15U) {
                state.alpha = clampU8(value);
                continue;
            }

            const auto component = static_cast<std::size_t>(track.target % 4U);
            for (std::size_t vertex = 0; vertex < 4U; ++vertex) {
                state.vertexColor[vertex * 4U + component] = clampU8(value);
            }
            continue;
        }

        if (track.kind == "RLTS") {
            switch (track.target) {
            case 0U:
                state.texOffsetU = value;
                break;
            case 1U:
                state.texOffsetV = value;
                break;
            case 3U:
                state.texScaleU = value;
                break;
            case 4U:
                state.texScaleV = value;
                break;
            default:
                break;
            }
            continue;
        }

        if (track.kind == "RLVI") {
            if (track.target == 0U) {
                state.visible = value > 0.5F;
            }
            continue;
        }
    }
}

void TitleLayoutActor::appear() {
    mIsAlive = true;
    for (auto &slot : mAnimationLayers) {
        slot = AnimationSlot {};
    }
    resetPose();
}

void TitleLayoutActor::kill() {
    mIsAlive = false;
    for (auto &slot : mAnimationLayers) {
        slot = AnimationSlot {};
    }
    resetPose();
}

bool TitleLayoutActor::isDead() const {
    return not mIsAlive;
}

void TitleLayoutActor::startAnim(const char *pAnimName, std::uint32_t layer) {
    if (mResource == nullptr || pAnimName == nullptr || layer >= mAnimationLayers.size()) {
        return;
    }

    const auto key = normalizeName(pAnimName);
    const auto found = mResource->animations_by_name.find(key);
    if (found == mResource->animations_by_name.end()) {
        return;
    }

    mAnimationLayers[layer].animation = &found->second;
    mAnimationLayers[layer].frame = 0.0F;
    mAnimationLayers[layer].paused = false;

    rebuildPose();
}

bool TitleLayoutActor::isAnimStopped(std::uint32_t layer) const {
    if (layer >= mAnimationLayers.size()) {
        return true;
    }

    const auto &slot = mAnimationLayers[layer];
    if (slot.animation == nullptr) {
        return true;
    }
    if (slot.animation->loop) {
        return false;
    }

    return slot.frame >= static_cast<float>(slot.animation->frame_size);
}

void TitleLayoutActor::setAnimFrameAndStop(float frame, std::uint32_t layer) {
    if (layer >= mAnimationLayers.size()) {
        return;
    }

    auto &slot = mAnimationLayers[layer];
    if (slot.animation == nullptr) {
        return;
    }

    slot.frame = frame;
    slot.paused = true;

    rebuildPose();
}

void TitleLayoutActor::update(float deltaFrames) {
    if (not mIsAlive) {
        return;
    }

    if (deltaFrames < 0.0F) {
        deltaFrames = 0.0F;
    }

    for (auto &slot : mAnimationLayers) {
        if (slot.animation == nullptr || slot.paused || deltaFrames <= 0.0F) {
            continue;
        }

        slot.frame += deltaFrames;
        if (not slot.animation->loop) {
            const auto end_frame = static_cast<float>(slot.animation->frame_size);
            if (slot.frame > end_frame) {
                slot.frame = end_frame;
            }
        }
    }

    rebuildPose();
}

void TitleLayoutActor::emitEffect(const char *) {
}

void TitleLayoutActor::deleteEffectAll() {
}

const assets::layout::tpl::DecodedImage *TitleLayoutActor::resolveTextureForMaterial(std::int32_t materialIndex) const {
    if (mResource == nullptr) {
        return nullptr;
    }

    if (materialIndex < 0 || static_cast<std::size_t>(materialIndex) >= mResource->layout.materials.size()) {
        return nullptr;
    }

    const auto &material = mResource->layout.materials[static_cast<std::size_t>(materialIndex)];
    return resolveTextureByLayoutIndex(material.texture_index);
}

const assets::layout::tpl::DecodedImage *TitleLayoutActor::resolveTextureByLayoutIndex(std::int32_t textureIndex) const {
    if (mResource == nullptr) {
        return nullptr;
    }
    if (textureIndex < 0 || static_cast<std::size_t>(textureIndex) >= mResource->layout.texture_names.size()) {
        return nullptr;
    }

    const auto textureName = baseWithoutExtension(mResource->layout.texture_names[static_cast<std::size_t>(textureIndex)]);
    const auto found = mResource->textures_by_name.find(normalizeName(textureName));
    if (found == mResource->textures_by_name.end()) {
        return nullptr;
    }
    return &found->second;
}

bool TitleLayoutActor::hasVariableAlpha(const assets::layout::tpl::DecodedImage &image) {
    if (image.rgba8.size() < 8U) {
        return false;
    }

    const auto alpha0 = image.rgba8[3U];
    for (std::size_t i = 7U; i < image.rgba8.size(); i += 4U) {
        if (image.rgba8[i] != alpha0) {
            return true;
        }
    }
    return false;
}

void TitleLayoutActor::rebuildPicLogoGalaxyComposite(const assets::layout::tpl::DecodedImage &baseTexture, const assets::layout::tpl::DecodedImage &maskTexture) const {
    mPicLogoGalaxyComposite.width = baseTexture.width;
    mPicLogoGalaxyComposite.height = baseTexture.height;
    mPicLogoGalaxyComposite.rgba8 = baseTexture.rgba8;

    const bool useMaskLuma = not hasVariableAlpha(maskTexture);
    if (baseTexture.width == 0U || baseTexture.height == 0U || maskTexture.width == 0U || maskTexture.height == 0U) {
        return;
    }

    for (std::uint32_t y = 0U; y < baseTexture.height; ++y) {
        const std::uint32_t maskY = (y * static_cast<std::uint32_t>(maskTexture.height)) / static_cast<std::uint32_t>(baseTexture.height);
        for (std::uint32_t x = 0U; x < baseTexture.width; ++x) {
            const std::uint32_t maskX = (x * static_cast<std::uint32_t>(maskTexture.width)) / static_cast<std::uint32_t>(baseTexture.width);

            const std::size_t baseComponentIndex = (static_cast<std::size_t>(y) * baseTexture.width + x) * 4U;
            const std::size_t maskComponentIndex = (static_cast<std::size_t>(maskY) * maskTexture.width + maskX) * 4U;

            std::uint32_t maskAlpha = maskTexture.rgba8[maskComponentIndex + 3U];
            if (useMaskLuma) {
                maskAlpha = (
                    static_cast<std::uint32_t>(maskTexture.rgba8[maskComponentIndex + 0U]) +
                    static_cast<std::uint32_t>(maskTexture.rgba8[maskComponentIndex + 1U]) +
                    static_cast<std::uint32_t>(maskTexture.rgba8[maskComponentIndex + 2U])) / 3U;
            }
            maskAlpha = 255U - maskAlpha;

            const std::uint32_t baseAlpha = mPicLogoGalaxyComposite.rgba8[baseComponentIndex + 3U];
            mPicLogoGalaxyComposite.rgba8[baseComponentIndex + 3U] = static_cast<std::uint8_t>((baseAlpha * maskAlpha + 127U) / 255U);
        }
    }
}

void TitleLayoutActor::applyPicture(
    std::size_t paneIndex,
    const TransformContext &parent,
    render::layout::LayoutDrawList *pDrawList) const {
    if (mResource == nullptr || pDrawList == nullptr) {
        return;
    }

    const auto *pane = paneDefinition(paneIndex);
    const auto *state = paneState(paneIndex);
    if (pane == nullptr || state == nullptr) {
        return;
    }

    const float world_scale_x = parent.scaleX * state->sx;
    const float world_scale_y = parent.scaleY * state->sy;

    const float pane_origin_x = parent.originX + state->tx * parent.scaleX;
    const float pane_origin_y = parent.originY - state->ty * parent.scaleY;

    const float draw_x = pane_origin_x + anchorOffsetX(pane->base_position, state->width) * world_scale_x;
    const float draw_y = pane_origin_y + anchorOffsetY(pane->base_position, state->height) * world_scale_y;
    const float draw_w = state->width * world_scale_x;
    const float draw_h = state->height * world_scale_y;

    if (std::fabs(draw_w) < 0.00001F || std::fabs(draw_h) < 0.00001F) {
        return;
    }

    const float alpha = parent.alpha * (static_cast<float>(state->alpha) / 255.0F);

    const auto *material = (pane->material_index >= 0 && static_cast<std::size_t>(pane->material_index) < mResource->layout.materials.size())
        ? &mResource->layout.materials[static_cast<std::size_t>(pane->material_index)]
        : nullptr;
    const auto *texture = resolveTextureForMaterial(pane->material_index);
    if (material != nullptr && material->name == "PicLogoGalaxy" && material->texture_indices.size() >= 2U) {
        const auto *maskTexture = resolveTextureByLayoutIndex(material->texture_indices[1U]);
        if (texture != nullptr && maskTexture != nullptr) {
            if (mPicLogoGalaxyBaseTexture != texture || mPicLogoGalaxyMaskTexture != maskTexture || mPicLogoGalaxyComposite.empty()) {
                rebuildPicLogoGalaxyComposite(*texture, *maskTexture);
                mPicLogoGalaxyBaseTexture = texture;
                mPicLogoGalaxyMaskTexture = maskTexture;
            }

            if (not mPicLogoGalaxyComposite.empty()) {
                texture = &mPicLogoGalaxyComposite;
            }
        }
    }

    const auto blendMode = material != nullptr && material->blend_mode == assets::layout::MaterialBlendMode::Additive
        ? render::layout::BlendMode::Additive
        : render::layout::BlendMode::Alpha;

    const auto color_for_vertex = [&](std::size_t vertex_index) {
        const auto base = vertex_index * 4U;
        const auto source_r = static_cast<float>(state->vertexColor[base + 0U]) / 255.0F;
        const auto source_g = static_cast<float>(state->vertexColor[base + 1U]) / 255.0F;
        const auto source_b = static_cast<float>(state->vertexColor[base + 2U]) / 255.0F;
        const auto source_a = static_cast<float>(state->vertexColor[base + 3U]) / 255.0F;

        const auto mat_r = material != nullptr ? static_cast<float>(material->mat_color[0U]) / 255.0F : 1.0F;
        const auto mat_g = material != nullptr ? static_cast<float>(material->mat_color[1U]) / 255.0F : 1.0F;
        const auto mat_b = material != nullptr ? static_cast<float>(material->mat_color[2U]) / 255.0F : 1.0F;
        const auto mat_a = material != nullptr ? static_cast<float>(material->mat_color[3U]) / 255.0F : 1.0F;

        return render::layout::pack_abgr(
            clampU8(source_r * mat_r * 255.0F),
            clampU8(source_g * mat_g * 255.0F),
            clampU8(source_b * mat_b * 255.0F),
            clampU8(source_a * mat_a * alpha * 255.0F));
    };

    const auto u0 = state->texOffsetU + pane->tex_coords[0U] * state->texScaleU;
    const auto v0 = state->texOffsetV + pane->tex_coords[1U] * state->texScaleV;
    const auto u1 = state->texOffsetU + pane->tex_coords[6U] * state->texScaleU;
    const auto v1 = state->texOffsetV + pane->tex_coords[7U] * state->texScaleV;

    pDrawList->push_quad(render::layout::QuadCommand {
        .x0 = draw_x,
        .y0 = draw_y,
        .x1 = draw_x + draw_w,
        .y1 = draw_y + draw_h,
        .u0 = u0,
        .v0 = v0,
        .u1 = u1,
        .v1 = v1,
        .color_tl = color_for_vertex(0U),
        .color_tr = color_for_vertex(1U),
        .color_bl = color_for_vertex(2U),
        .color_br = color_for_vertex(3U),
        .blend_mode = blendMode,
        .texture = render::layout::TextureRef {
            .id = texture != nullptr ? static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(texture)) : 0U,
            .rgba8 = texture != nullptr && not texture->rgba8.empty() ? texture->rgba8.data() : nullptr,
            .width = texture != nullptr ? texture->width : static_cast<std::uint16_t>(0U),
            .height = texture != nullptr ? texture->height : static_cast<std::uint16_t>(0U),
        },
    });
}

void TitleLayoutActor::applyText(
    std::size_t paneIndex,
    const TransformContext &parent,
    render::layout::LayoutDrawList *pDrawList,
    const std::unordered_map<std::string, assets::layout::BrfntFont> &rFontsByName) const {
    if (mResource == nullptr || pDrawList == nullptr) {
        return;
    }

    const auto *pane = paneDefinition(paneIndex);
    const auto *state = paneState(paneIndex);
    if (pane == nullptr || state == nullptr) {
        return;
    }

    if (pane->font_index < 0 || static_cast<std::size_t>(pane->font_index) >= mResource->layout.font_names.size()) {
        return;
    }

    const auto font_name_key = normalizeName(baseWithoutExtension(mResource->layout.font_names[static_cast<std::size_t>(pane->font_index)]));
    const auto font_found = rFontsByName.find(font_name_key);
    if (font_found == rFontsByName.end()) {
        return;
    }

    const auto &font = font_found->second;
    if (font.sheets().empty() || font.cell_width() == 0U || font.cell_height() == 0U) {
        return;
    }

    const float world_scale_x = parent.scaleX * state->sx;
    const float world_scale_y = parent.scaleY * state->sy;

    const float pane_origin_x = parent.originX + state->tx * parent.scaleX;
    const float pane_origin_y = parent.originY - state->ty * parent.scaleY;

    const float draw_x = pane_origin_x + anchorOffsetX(pane->base_position, state->width) * world_scale_x;
    const float draw_y = pane_origin_y + anchorOffsetY(pane->base_position, state->height) * world_scale_y;

    const float alpha = parent.alpha * (static_cast<float>(state->alpha) / 255.0F);

    const float text_scale_x = pane->text_font_size.x > 0.0F
        ? pane->text_font_size.x / static_cast<float>(font.cell_width())
        : 1.0F;
    const float text_scale_y = pane->text_font_size.y > 0.0F
        ? pane->text_font_size.y / static_cast<float>(font.cell_height())
        : 1.0F;

    const auto top_color = pane->text_colors[0U];
    const auto packed_color = render::layout::pack_abgr(
        clampU8(static_cast<float>(top_color.r)),
        clampU8(static_cast<float>(top_color.g)),
        clampU8(static_cast<float>(top_color.b)),
        clampU8(static_cast<float>(top_color.a) * alpha));
    const auto line_advance = static_cast<float>(font.line_feed()) * text_scale_y * world_scale_y + pane->text_line_space * world_scale_y;
    const auto pane_width = state->width * world_scale_x;
    const auto pane_height = state->height * world_scale_y;
    const auto glyph_height = std::max(1, static_cast<int>(font.cell_height()));
    const auto line_glyph_height = static_cast<float>(glyph_height) * text_scale_y * world_scale_y;

    std::vector<std::pair<std::size_t, std::size_t>> line_ranges {};
    line_ranges.reserve(4U);
    std::size_t line_begin = 0U;
    for (std::size_t i = 0U; i < pane->text.size(); ++i) {
        if (pane->text[i] == u'\n') {
            line_ranges.emplace_back(line_begin, i);
            line_begin = i + 1U;
        }
    }
    line_ranges.emplace_back(line_begin, pane->text.size());

    const auto measure_line_width = [&](std::size_t begin, std::size_t end) {
        float width = 0.0F;
        for (std::size_t i = begin; i < end; ++i) {
            assets::layout::BrfntGlyph glyph {};
            if (not font.get_glyph(static_cast<std::uint16_t>(pane->text[i]), &glyph)) {
                continue;
            }
            width += (static_cast<float>(glyph.widths.char_width) + pane->text_char_space) * text_scale_x * world_scale_x;
        }
        return width;
    };

    const auto text_block_height = line_ranges.empty()
        ? 0.0F
        : line_glyph_height + static_cast<float>(line_ranges.size() - 1U) * line_advance;
    float block_origin_y = draw_y;
    const auto text_anchor_row = static_cast<std::uint8_t>((pane->text_position / 3U) % 3U);
    if (text_anchor_row == 1U) {
        block_origin_y += (pane_height - text_block_height) * 0.5F;
    } else if (text_anchor_row == 2U) {
        block_origin_y += pane_height - text_block_height;
    }

    float line_origin_y = block_origin_y;
    const auto text_anchor_col = static_cast<std::uint8_t>(pane->text_position % 3U);
    for (const auto &[begin, end] : line_ranges) {
        const auto line_width = measure_line_width(begin, end);
        float line_origin_x = draw_x;
        if (text_anchor_col == 1U) {
            line_origin_x += (pane_width - line_width) * 0.5F;
        } else if (text_anchor_col == 2U) {
            line_origin_x += pane_width - line_width;
        }

        float pen_x = line_origin_x;
        for (std::size_t i = begin; i < end; ++i) {
            assets::layout::BrfntGlyph glyph {};
            if (not font.get_glyph(static_cast<std::uint16_t>(pane->text[i]), &glyph)) {
                continue;
            }

            if (glyph.sheet_index >= font.sheets().size()) {
                continue;
            }

            const auto &sheet = font.sheets()[glyph.sheet_index];
            if (sheet.empty()) {
                continue;
            }

            const auto glyph_width = std::max(1, static_cast<int>(glyph.widths.glyph_width));

            const auto quad_x0 = pen_x + static_cast<float>(glyph.widths.left) * text_scale_x * world_scale_x;
            const auto quad_y0 = line_origin_y;
            const auto quad_x1 = quad_x0 + static_cast<float>(glyph_width) * text_scale_x * world_scale_x;
            const auto quad_y1 = quad_y0 + static_cast<float>(glyph_height) * text_scale_y * world_scale_y;

            const auto u0 = static_cast<float>(glyph.cell_x) / static_cast<float>(sheet.width);
            const auto v0 = static_cast<float>(glyph.cell_y) / static_cast<float>(sheet.height);
            const auto u1 = static_cast<float>(glyph.cell_x + font.cell_width()) / static_cast<float>(sheet.width);
            const auto v1 = static_cast<float>(glyph.cell_y + font.cell_height()) / static_cast<float>(sheet.height);

            pDrawList->push_quad(render::layout::QuadCommand {
                .x0 = quad_x0,
                .y0 = quad_y0,
                .x1 = quad_x1,
                .y1 = quad_y1,
                .u0 = u0,
                .v0 = v0,
                .u1 = u1,
                .v1 = v1,
                .color_tl = packed_color,
                .color_tr = packed_color,
                .color_bl = packed_color,
                .color_br = packed_color,
                .texture = render::layout::TextureRef {
                    .id = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(&sheet)),
                    .rgba8 = sheet.rgba8.data(),
                    .width = sheet.width,
                    .height = sheet.height,
                },
            });

            pen_x += (static_cast<float>(glyph.widths.char_width) + pane->text_char_space) * text_scale_x * world_scale_x;
        }

        line_origin_y += line_advance;
    }
}

void TitleLayoutActor::appendPaneRecursive(
    std::size_t paneIndex,
    const TransformContext &parent,
    render::layout::LayoutDrawList *pDrawList,
    const std::unordered_map<std::string, assets::layout::BrfntFont> &rFontsByName) const {
    if (mResource == nullptr || pDrawList == nullptr || paneIndex >= mResource->layout.panes.size() || paneIndex >= mCurrentStates.size()) {
        return;
    }

    const auto &pane = mResource->layout.panes[paneIndex];
    const auto &state = mCurrentStates[paneIndex];
    if (not parent.visible || not state.visible) {
        return;
    }

    if (pane.type == assets::layout::PaneType::Picture) {
        applyPicture(paneIndex, parent, pDrawList);
    } else if (pane.type == assets::layout::PaneType::Text) {
        applyText(paneIndex, parent, pDrawList, rFontsByName);
    }

    const float world_scale_x = parent.scaleX * state.sx;
    const float world_scale_y = parent.scaleY * state.sy;

    const float pane_origin_x = parent.originX + state.tx * parent.scaleX;
    const float pane_origin_y = parent.originY - state.ty * parent.scaleY;

    const TransformContext child {
        .originX = pane_origin_x,
        .originY = pane_origin_y,
        .scaleX = world_scale_x,
        .scaleY = world_scale_y,
        .alpha = parent.alpha * (static_cast<float>(state.alpha) / 255.0F),
        .visible = true,
    };

    for (const auto child_index : pane.children) {
        if (child_index < 0) {
            continue;
        }
        appendPaneRecursive(static_cast<std::size_t>(child_index), child, pDrawList, rFontsByName);
    }
}

void TitleLayoutActor::appendDrawCommands(
    render::layout::LayoutDrawList *pDrawList,
    const std::unordered_map<std::string, assets::layout::BrfntFont> &rFontsByName) const {
    if (not mIsAlive || mResource == nullptr || pDrawList == nullptr) {
        return;
    }

    const auto root_index = mResource->layout.root_pane;
    if (root_index < 0 || static_cast<std::size_t>(root_index) >= mResource->layout.panes.size()) {
        return;
    }

    const TransformContext root {
        .originX = mResource->layout.center_origin ? (mResource->layout.size.x * 0.5F) : 0.0F,
        .originY = mResource->layout.center_origin ? (mResource->layout.size.y * 0.5F) : 0.0F,
        .scaleX = 1.0F,
        .scaleY = 1.0F,
        .alpha = 1.0F,
        .visible = true,
    };

    appendPaneRecursive(static_cast<std::size_t>(root_index), root, pDrawList, rFontsByName);
}

}  // namespace smgpc::game::title
