#include "LayoutRuntimeActor.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include "layout/Binary.hpp"

namespace smgpc::game::layout {
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

[[nodiscard]] bool has_non_opaque_alpha(const assets::layout::tpl::DecodedImage &image) {
    if (image.empty() || image.rgba8.size() < 4U) {
        return false;
    }

    const std::size_t pixel_count = static_cast<std::size_t>(image.width) * image.height;
    const std::size_t sample_count = std::min<std::size_t>(pixel_count, 2048U);
    const std::size_t sample_stride = std::max<std::size_t>(1U, pixel_count / sample_count);
    std::size_t non_opaque = 0U;
    std::size_t sampled = 0U;

    for (std::size_t pixel_index = 0U; pixel_index < pixel_count; pixel_index += sample_stride) {
        const std::size_t rgba_index = pixel_index * 4U;
        if (rgba_index + 3U >= image.rgba8.size()) {
            break;
        }
        if (image.rgba8[rgba_index + 3U] != 255U) {
            ++non_opaque;
        }
        ++sampled;
    }

    if (sampled == 0U) {
        return false;
    }
    return (static_cast<float>(non_opaque) / static_cast<float>(sampled)) > 0.05F;
}

struct MaskChannelStats {
    float minimum {1.0F};
    float maximum {};
    float average {};
};

[[nodiscard]] MaskChannelStats estimate_mask_channel_stats(const assets::layout::tpl::DecodedImage &image, bool use_alpha_channel) {
    MaskChannelStats stats {};
    if (image.empty() || image.rgba8.size() < 4U) {
        return stats;
    }

    const std::size_t pixel_count = static_cast<std::size_t>(image.width) * image.height;
    const std::size_t sample_count = std::min<std::size_t>(pixel_count, 4096U);
    const std::size_t sample_stride = std::max<std::size_t>(1U, pixel_count / sample_count);
    float sum = 0.0F;
    std::size_t sampled = 0U;

    for (std::size_t pixel_index = 0U; pixel_index < pixel_count; pixel_index += sample_stride) {
        const std::size_t rgba_index = pixel_index * 4U;
        if (rgba_index + 3U >= image.rgba8.size()) {
            break;
        }

        const float channel = use_alpha_channel
            ? static_cast<float>(image.rgba8[rgba_index + 3U]) / 255.0F
            : (static_cast<float>(image.rgba8[rgba_index + 0U]) +
               static_cast<float>(image.rgba8[rgba_index + 1U]) +
               static_cast<float>(image.rgba8[rgba_index + 2U])) / (255.0F * 3.0F);

        stats.minimum = std::min(stats.minimum, channel);
        stats.maximum = std::max(stats.maximum, channel);
        sum += channel;
        ++sampled;
    }

    if (sampled > 0U) {
        stats.average = sum / static_cast<float>(sampled);
    }
    return stats;
}

[[nodiscard]] float estimate_colorfulness(const assets::layout::tpl::DecodedImage &image) {
    if (image.empty() || image.rgba8.size() < 4U) {
        return 0.0F;
    }

    const std::size_t pixel_count = static_cast<std::size_t>(image.width) * image.height;
    const std::size_t sample_count = std::min<std::size_t>(pixel_count, 4096U);
    const std::size_t sample_stride = std::max<std::size_t>(1U, pixel_count / sample_count);
    float diff_sum = 0.0F;
    std::size_t sampled = 0U;

    for (std::size_t pixel_index = 0U; pixel_index < pixel_count; pixel_index += sample_stride) {
        const std::size_t rgba_index = pixel_index * 4U;
        if (rgba_index + 2U >= image.rgba8.size()) {
            break;
        }

        const float r = static_cast<float>(image.rgba8[rgba_index + 0U]) / 255.0F;
        const float g = static_cast<float>(image.rgba8[rgba_index + 1U]) / 255.0F;
        const float b = static_cast<float>(image.rgba8[rgba_index + 2U]) / 255.0F;
        diff_sum += (std::fabs(r - g) + std::fabs(g - b) + std::fabs(b - r)) / 3.0F;
        ++sampled;
    }

    if (sampled == 0U) {
        return 0.0F;
    }
    return diff_sum / static_cast<float>(sampled);
}

[[nodiscard]] bool should_invert_mask_alpha(const assets::layout::tpl::DecodedImage &image, bool use_alpha_channel) {
    if (image.empty() || image.rgba8.size() < 4U) {
        return false;
    }

    const std::size_t pixel_count = static_cast<std::size_t>(image.width) * image.height;
    const std::size_t sample_count = std::min<std::size_t>(pixel_count, 4096U);
    const std::size_t sample_stride = std::max<std::size_t>(1U, pixel_count / sample_count);
    std::uint64_t alpha_sum = 0U;
    std::size_t sampled = 0U;

    for (std::size_t pixel_index = 0U; pixel_index < pixel_count; pixel_index += sample_stride) {
        const std::size_t rgba_index = pixel_index * 4U;
        if (rgba_index + 3U >= image.rgba8.size()) {
            break;
        }

        const std::uint8_t alpha = use_alpha_channel
            ? image.rgba8[rgba_index + 3U]
            : static_cast<std::uint8_t>(
                (static_cast<std::uint32_t>(image.rgba8[rgba_index + 0U]) +
                 static_cast<std::uint32_t>(image.rgba8[rgba_index + 1U]) +
                 static_cast<std::uint32_t>(image.rgba8[rgba_index + 2U])) / 3U);
        alpha_sum += alpha;
        ++sampled;
    }

    if (sampled == 0U) {
        return false;
    }

    const float average_alpha = static_cast<float>(alpha_sum) / static_cast<float>(sampled) / 255.0F;
    return average_alpha > 0.5F;
}

}  // namespace

LayoutRuntimeActor::LayoutRuntimeActor(std::shared_ptr<const LayoutArchiveData> resource)
    : mResource(std::move(resource)) {
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

std::string LayoutRuntimeActor::normalizeName(std::string name) {
    return assets::layout::binary::to_lower_ascii(std::move(name));
}

float LayoutRuntimeActor::anchorOffsetX(std::uint8_t basePosition, float width) {
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

float LayoutRuntimeActor::anchorOffsetY(std::uint8_t basePosition, float height) {
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

std::uint8_t LayoutRuntimeActor::clampU8(float value) {
    if (value <= 0.0F) {
        return 0U;
    }
    if (value >= 255.0F) {
        return 255U;
    }
    return static_cast<std::uint8_t>(std::lround(value));
}

const assets::layout::PaneDefinition *LayoutRuntimeActor::paneDefinition(std::size_t index) const {
    if (mResource == nullptr || index >= mResource->layout.panes.size()) {
        return nullptr;
    }
    return &mResource->layout.panes[index];
}

const LayoutRuntimeActor::RuntimePaneState *LayoutRuntimeActor::paneState(std::size_t index) const {
    if (index >= mCurrentStates.size()) {
        return nullptr;
    }
    return &mCurrentStates[index];
}

void LayoutRuntimeActor::resetPose() {
    mCurrentStates = mBaseStates;
}

void LayoutRuntimeActor::rebuildPose() {
    for (const auto &slot : mAnimationLayers) {
        if (slot.animation == nullptr) {
            continue;
        }

        applyAnimation(*slot.animation, slot.frame);
    }
}

void LayoutRuntimeActor::applyAnimation(const assets::layout::BrlanAnimation &animation, float frame) {
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
            const std::size_t texture_stage = static_cast<std::size_t>(track.target / 5U);
            const auto stage_target = static_cast<std::uint16_t>(track.target % 5U);
            if (texture_stage < state.texOffsetU.size()) {
                switch (stage_target) {
                case 0U:
                    state.texOffsetU[texture_stage] = value;
                    break;
                case 1U:
                    state.texOffsetV[texture_stage] = value;
                    break;
                case 3U:
                    state.texScaleU[texture_stage] = value;
                    break;
                case 4U:
                    state.texScaleV[texture_stage] = value;
                    break;
                default:
                    break;
                }
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

void LayoutRuntimeActor::appear() {
    mIsAlive = true;
    for (auto &slot : mAnimationLayers) {
        slot = AnimationSlot {};
    }
    resetPose();
}

void LayoutRuntimeActor::kill() {
    mIsAlive = false;
    for (auto &slot : mAnimationLayers) {
        slot = AnimationSlot {};
    }
    resetPose();
}

bool LayoutRuntimeActor::isDead() const {
    return not mIsAlive;
}

void LayoutRuntimeActor::startAnim(const char *pAnimName, std::uint32_t layer) {
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

bool LayoutRuntimeActor::isAnimStopped(std::uint32_t layer) const {
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

void LayoutRuntimeActor::setAnimFrameAndStop(float frame, std::uint32_t layer) {
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

void LayoutRuntimeActor::update(float deltaFrames) {
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

void LayoutRuntimeActor::emitEffect(const char *) {
}

void LayoutRuntimeActor::deleteEffectAll() {
}

const assets::layout::tpl::DecodedImage *LayoutRuntimeActor::resolveTextureForMaterial(std::int32_t materialIndex) const {
    if (mResource == nullptr) {
        return nullptr;
    }

    if (materialIndex < 0 || static_cast<std::size_t>(materialIndex) >= mResource->layout.materials.size()) {
        return nullptr;
    }

    const auto &material = mResource->layout.materials[static_cast<std::size_t>(materialIndex)];
    return resolveTextureByLayoutIndex(material.texture_index);
}

const assets::layout::tpl::DecodedImage *LayoutRuntimeActor::resolveTextureByLayoutIndex(std::int32_t textureIndex) const {
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

bool LayoutRuntimeActor::chooseColorAndMaskTextures(
    std::int32_t materialIndex,
    const assets::layout::tpl::DecodedImage **colorTexture,
    std::size_t *colorStage,
    const assets::layout::tpl::DecodedImage **maskTexture,
    std::size_t *maskStage,
    bool *invertMask,
    bool *maskUsesAlpha) const {
    if (colorTexture == nullptr || colorStage == nullptr || maskTexture == nullptr || maskStage == nullptr || invertMask == nullptr || maskUsesAlpha == nullptr) {
        return false;
    }

    // Keep the caller-provided color texture as the fallback when mask selection is rejected.
    *colorStage = 0U;
    *maskTexture = nullptr;
    *maskStage = 0U;
    *invertMask = false;
    *maskUsesAlpha = false;

    if (mResource == nullptr) {
        return false;
    }
    if (materialIndex < 0 || static_cast<std::size_t>(materialIndex) >= mResource->layout.materials.size()) {
        return false;
    }

    const auto &material = mResource->layout.materials[static_cast<std::size_t>(materialIndex)];
    if (material.texture_indices.size() < 2U) {
        return false;
    }

    const auto *stage0 = resolveTextureByLayoutIndex(material.texture_indices[0U]);
    const auto *stage1 = resolveTextureByLayoutIndex(material.texture_indices[1U]);
    if (stage0 == nullptr || stage1 == nullptr || stage0->empty() || stage1->empty()) {
        return false;
    }

    const bool stage0_uses_alpha = has_non_opaque_alpha(*stage0);
    const bool stage1_uses_alpha = has_non_opaque_alpha(*stage1);
    const float colorfulness0 = estimate_colorfulness(*stage0);
    const float colorfulness1 = estimate_colorfulness(*stage1);

    const bool stage0_is_mask = (stage0_uses_alpha != stage1_uses_alpha)
        ? stage0_uses_alpha
        : (colorfulness0 <= colorfulness1);
    const auto *selected_mask = stage0_is_mask ? stage0 : stage1;
    const auto *selected_color = stage0_is_mask ? stage1 : stage0;
    const std::size_t selected_mask_stage = stage0_is_mask ? 0U : 1U;
    const std::size_t selected_color_stage = stage0_is_mask ? 1U : 0U;

    const bool use_alpha_channel = has_non_opaque_alpha(*selected_mask);
    const float mask_colorfulness = estimate_colorfulness(*selected_mask);
    const float color_texture_colorfulness = estimate_colorfulness(*selected_color);
    const auto channel_stats = estimate_mask_channel_stats(*selected_mask, use_alpha_channel);

    // Keep mask usage conservative to avoid accidentally zeroing unrelated panes.
    if ((channel_stats.maximum - channel_stats.minimum) < 0.10F) {
        return false;
    }
    if (not use_alpha_channel && mask_colorfulness > 0.12F) {
        return false;
    }
    if (not use_alpha_channel && std::fabs(color_texture_colorfulness - mask_colorfulness) < 0.015F) {
        return false;
    }
    if (color_texture_colorfulness + 0.01F < mask_colorfulness && not use_alpha_channel) {
        return false;
    }

    *colorTexture = selected_color;
    *colorStage = selected_color_stage;
    *maskTexture = selected_mask;
    *maskStage = selected_mask_stage;
    *maskUsesAlpha = use_alpha_channel;
    *invertMask = should_invert_mask_alpha(*selected_mask, use_alpha_channel);
    return true;
}

void LayoutRuntimeActor::applyPicture(
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

    const assets::layout::tpl::DecodedImage *colorTexture = texture;
    const assets::layout::tpl::DecodedImage *maskTexture = nullptr;
    std::size_t colorStage = 0U;
    std::size_t maskStage = 0U;
    bool invertMask = false;
    bool maskUsesAlpha = false;
    const bool hasMask = chooseColorAndMaskTextures(
        pane->material_index,
        &colorTexture,
        &colorStage,
        &maskTexture,
        &maskStage,
        &invertMask,
        &maskUsesAlpha);
    if (not hasMask && colorTexture == nullptr) {
        colorTexture = texture;
        colorStage = 0U;
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

    const auto sample_u0 = [&](std::size_t stage) {
        return state->texOffsetU[stage] + pane->tex_coords[0U] * state->texScaleU[stage];
    };
    const auto sample_v0 = [&](std::size_t stage) {
        return state->texOffsetV[stage] + pane->tex_coords[1U] * state->texScaleV[stage];
    };
    const auto sample_u1 = [&](std::size_t stage) {
        return state->texOffsetU[stage] + pane->tex_coords[6U] * state->texScaleU[stage];
    };
    const auto sample_v1 = [&](std::size_t stage) {
        return state->texOffsetV[stage] + pane->tex_coords[7U] * state->texScaleV[stage];
    };

    const std::size_t primaryStage = std::min<std::size_t>(colorStage, state->texOffsetU.size() - 1U);
    const std::size_t secondaryStage = std::min<std::size_t>(maskStage, state->texOffsetU.size() - 1U);

    pDrawList->push_quad(render::layout::QuadCommand {
        .x0 = draw_x,
        .y0 = draw_y,
        .x1 = draw_x + draw_w,
        .y1 = draw_y + draw_h,
        .u0 = sample_u0(primaryStage),
        .v0 = sample_v0(primaryStage),
        .u1 = sample_u1(primaryStage),
        .v1 = sample_v1(primaryStage),
        .u0_secondary = sample_u0(secondaryStage),
        .v0_secondary = sample_v0(secondaryStage),
        .u1_secondary = sample_u1(secondaryStage),
        .v1_secondary = sample_v1(secondaryStage),
        .color_tl = color_for_vertex(0U),
        .color_tr = color_for_vertex(1U),
        .color_bl = color_for_vertex(2U),
        .color_br = color_for_vertex(3U),
        .blend_mode = blendMode,
        .use_mask_texture = hasMask,
        .invert_mask = invertMask,
        .mask_uses_alpha = maskUsesAlpha,
        .texture = render::layout::TextureRef {
            .id = colorTexture != nullptr ? static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(colorTexture)) : 0U,
            .rgba8 = colorTexture != nullptr && not colorTexture->rgba8.empty() ? colorTexture->rgba8.data() : nullptr,
            .width = colorTexture != nullptr ? colorTexture->width : static_cast<std::uint16_t>(0U),
            .height = colorTexture != nullptr ? colorTexture->height : static_cast<std::uint16_t>(0U),
        },
        .mask_texture = render::layout::TextureRef {
            .id = maskTexture != nullptr ? static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(maskTexture)) : 0U,
            .rgba8 = maskTexture != nullptr && not maskTexture->rgba8.empty() ? maskTexture->rgba8.data() : nullptr,
            .width = maskTexture != nullptr ? maskTexture->width : static_cast<std::uint16_t>(0U),
            .height = maskTexture != nullptr ? maskTexture->height : static_cast<std::uint16_t>(0U),
        },
    });
}

void LayoutRuntimeActor::applyText(
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

    if (pane->font_index < 0 || static_cast<std::size_t>(pane->font_index) >= mResource->layout.font_names.size()) {
        return;
    }

    const auto font_name_key = normalizeName(baseWithoutExtension(mResource->layout.font_names[static_cast<std::size_t>(pane->font_index)]));
    const auto font_found = mResource->fonts_by_name.find(font_name_key);
    if (font_found == mResource->fonts_by_name.end()) {
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

void LayoutRuntimeActor::appendPaneRecursive(
    std::size_t paneIndex,
    const TransformContext &parent,
    render::layout::LayoutDrawList *pDrawList) const {
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
        applyText(paneIndex, parent, pDrawList);
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
        appendPaneRecursive(static_cast<std::size_t>(child_index), child, pDrawList);
    }
}

void LayoutRuntimeActor::appendDrawCommands(render::layout::LayoutDrawList *pDrawList) const {
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

    appendPaneRecursive(static_cast<std::size_t>(root_index), root, pDrawList);
}

const LayoutArchiveData *LayoutRuntimeActor::resource() const {
    return mResource.get();
}

}  // namespace smgpc::game::layout
