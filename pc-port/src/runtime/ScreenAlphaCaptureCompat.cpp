#include "Game/Screen/ScreenAlphaCapture.hpp"

#include <algorithm>
#include <array>
#include <memory>

#include <JSystem/JUtility/JUTTexture.hpp>
#include <JSystem/JUtility/JUTVideo.hpp>

namespace {
    constexpr auto cScreenAlphaCaptureCount = 4U;

    std::array<std::unique_ptr<JUTTexture>, cScreenAlphaCaptureCount> s_textures;

    [[nodiscard]] std::size_t texture_index(s32 index) {
        return static_cast<std::size_t>(std::clamp(index, 0, static_cast<s32>(cScreenAlphaCaptureCount - 1U)));
    }

    void ensure_texture(s32 index, f32 scale) {
        const auto slot = texture_index(index);
        if (s_textures[slot] != nullptr) {
            return;
        }

        const auto *render_mode = JUTVideo::getManager()->getRenderMode();
        const auto width = std::max(1, static_cast<int>(static_cast<f32>(render_mode->fbWidth) * scale));
        const auto height = std::max(1, static_cast<int>(static_cast<f32>(render_mode->efbHeight) * scale));
        s_textures[slot] = std::make_unique<JUTTexture>(width, height, GX_CTF_A8);
    }
}  // namespace

namespace MR {
    void createScreenAlphaSceneObj(s32 index, f32 scale) {
        ensure_texture(index, scale);
    }

    void captureScreenAlpha(s32 index) {
        ensure_texture(index, 1.0F);
        if (auto *texture = getScreenAlphaTexture(index)) {
            texture->capture(0, 0, GX_CTF_A8, false, GX_FALSE);
        }
    }

    void loadScreenAlphaTexture(s32 index, GXTexMapID tex_map_id) {
        if (auto *texture = getScreenAlphaTexture(index)) {
            texture->load(tex_map_id);
        }
    }

    JUTTexture *getScreenAlphaTexture(s32 index) {
        return s_textures[texture_index(index)].get();
    }
}  // namespace MR
