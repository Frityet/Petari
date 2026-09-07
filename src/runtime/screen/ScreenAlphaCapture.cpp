#include "Game/Screen/ScreenAlphaCapture.hpp"
#include "runtime/ScreenAlphaCaptureService.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>

#include <JSystem/JUtility/JUTTexture.hpp>
#include <JSystem/JUtility/JUTVideo.hpp>

namespace {
    constexpr auto cScreenAlphaCaptureCount = 4U;

    smgpc::runtime::ScreenAlphaCaptureService* s_service = nullptr;

    [[nodiscard]] std::size_t texture_index(s32 index) {
        return static_cast<std::size_t>(std::clamp(index, 0, static_cast<s32>(cScreenAlphaCaptureCount - 1U)));
    }

}  // namespace

namespace smgpc::runtime {
    ScreenAlphaCaptureService::ScreenAlphaCaptureService() {
        if (s_service) throw std::logic_error("A screen-alpha texture owner is already installed");
        s_service = this;
    }

    ScreenAlphaCaptureService::~ScreenAlphaCaptureService() {
        s_service = nullptr;
    }

    ScreenAlphaCaptureService& ScreenAlphaCaptureService::active() {
        if (!s_service) throw std::logic_error("Screen-alpha textures require an active runtime owner");
        return *s_service;
    }

    JUTTexture* ScreenAlphaCaptureService::texture(s32 index) const {
        return _textures[texture_index(index)].get();
    }

    void ScreenAlphaCaptureService::ensure_texture(s32 index, f32 scale) {
        const auto slot = texture_index(index);
        if (_textures[slot] != nullptr) {
            return;
        }

        const auto *render_mode = JUTVideo::getManager()->getRenderMode();
        const auto width = std::max(1, static_cast<int>(static_cast<f32>(render_mode->fbWidth) * scale));
        const auto height = std::max(1, static_cast<int>(static_cast<f32>(render_mode->efbHeight) * scale));
        _textures[slot] = std::make_unique<JUTTexture>(width, height, GX_CTF_A8);
    }
}  // namespace smgpc::runtime

namespace MR {
    void createScreenAlphaSceneObj(s32 index, f32 scale) {
        smgpc::runtime::ScreenAlphaCaptureService::active().ensure_texture(index, scale);
    }

    void captureScreenAlpha(s32 index) {
        smgpc::runtime::ScreenAlphaCaptureService::active().ensure_texture(index, 1.0F);
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
        return smgpc::runtime::ScreenAlphaCaptureService::active().texture(index);
    }
}  // namespace MR
