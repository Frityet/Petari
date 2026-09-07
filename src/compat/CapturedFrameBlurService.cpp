#include "compat/CapturedFrameBlurService.hpp"

#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/CenterScreenBlur.hpp"
#include "Game/Screen/FullScreenBlur.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include "JSystem/JUtility/JUTVideo.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <algorithm>
#include <stdexcept>

#include <dolphin/gx.h>
#include <dolphin/gx/GXAurora.h>
#include <dolphin/mtx.h>

namespace smgpc::compat {
    namespace {
        class ScopedOffscreenFramebuffer final {
        public:
            ScopedOffscreenFramebuffer(std::uint16_t width, std::uint16_t height) {
                GXCreateFrameBuffer(width, height);
            }

            ScopedOffscreenFramebuffer(const ScopedOffscreenFramebuffer&) = delete;
            ScopedOffscreenFramebuffer& operator=(const ScopedOffscreenFramebuffer&) = delete;

            ~ScopedOffscreenFramebuffer() {
                GXRestoreFrameBuffer();
            }
        };

        void setup_textured_pass(float coordinate_width, float coordinate_height,
                                 std::uint16_t framebuffer_width,
                                 std::uint16_t framebuffer_height, bool blend) {
            GXSetViewport(0.0F, 0.0F, static_cast<float>(framebuffer_width),
                          static_cast<float>(framebuffer_height), 0.0F, 1.0F);
            GXSetScissor(0U, 0U, framebuffer_width, framebuffer_height);

            Mtx44 projection;
            C_MTXOrtho(projection, 0.0F, coordinate_height, 0.0F, coordinate_width,
                       0.0F, -1.0F);
            GXSetProjection(projection, GX_ORTHOGRAPHIC);

            Mtx model;
            PSMTXIdentity(model);
            GXLoadPosMtxImm(model, GX_PNMTX0);
            GXSetCurrentMtx(GX_PNMTX0);

            GXSetCullMode(GX_CULL_NONE);
            GXSetClipMode(GX_CLIP_ENABLE);
            GXSetCoPlanar(GX_DISABLE);
            GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
            GXSetZCompLoc(GX_FALSE);
            GXSetColorUpdate(GX_TRUE);
            GXSetAlphaUpdate(GX_FALSE);
            GXSetNumChans(0U);
            GXSetNumIndStages(0U);
            GXSetNumTexGens(1U);
            GXSetNumTevStages(1U);
            GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0,
                              GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
            GXSetTevDirect(GX_TEVSTAGE0);
            GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
            GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
            GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
                            GX_CA_A0);
            GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
                            GX_TRUE, GX_TEVPREV);
            GXSetBlendMode(blend ? GX_BM_BLEND : GX_BM_NONE, GX_BL_SRCALPHA,
                           GX_BL_INVSRCALPHA, GX_LO_CLEAR);
            GXSetAlphaCompare(GX_ALWAYS, 0U, GX_AOP_AND, GX_ALWAYS, 0U);
            GXSetFog(GX_FOG_NONE, 0.0F, 0.0F, 0.0F, 0.0F,
                     GXColor{0U, 0U, 0U, 0U});
            GXSetFogRangeAdj(GX_FALSE, 0U, nullptr);

            GXClearVtxDesc();
            GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
            GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0U);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0U);
        }

        void set_texture_alpha(u8 alpha) {
            const auto color = GXColorS10{0, 0, 0, static_cast<s16>(alpha)};
            GXSetTevColorS10(GX_TEVREG0, color);
        }

        void draw_textured_quad(JUTTexture& texture, float left, float top,
                                float width, float height) {
            if (texture.getTexInfo() == nullptr || texture.mImage == nullptr) {
                throw std::logic_error(
                    "Captured-frame blur requires a real sampled texture.");
            }

            texture.load(GX_TEXMAP0);
            GXBegin(GX_QUADS, GX_VTXFMT0, 4U);
            GXPosition3f32(left, top, 0.0F);
            GXTexCoord2f32(0.0F, 0.0F);
            GXPosition3f32(left + width, top, 0.0F);
            GXTexCoord2f32(1.0F, 0.0F);
            GXPosition3f32(left + width, top + height, 0.0F);
            GXTexCoord2f32(1.0F, 1.0F);
            GXPosition3f32(left, top + height, 0.0F);
            GXTexCoord2f32(0.0F, 1.0F);
            GXEnd();
        }
    }  // namespace

    CapturedFrameBlurService::CapturedFrameBlurService(std::uint16_t history_width,
                                                       std::uint16_t history_height)
        : _history_width(std::max<std::uint16_t>(history_width, 1U)),
          _history_height(std::max<std::uint16_t>(history_height, 1U)),
          _history_texture(), _stats() {
    }

    CapturedFrameBlurService::~CapturedFrameBlurService() = default;

    void CapturedFrameBlurService::draw(JUTTexture& captured_frame,
                                        float coordinate_width,
                                        float coordinate_height,
                                        std::uint16_t framebuffer_width,
                                        std::uint16_t framebuffer_height,
                                        float current_expand,
                                        float history_expand, u8 current_alpha,
                                        u8 history_alpha) {
        if (AuroraIsFrameActive() == GX_FALSE) {
            throw std::logic_error(
                "Captured-frame blur requires an active Aurora GX frame.");
        }
        if (captured_frame.mImage == nullptr ||
            AuroraHasTextureCopy(captured_frame.mImage) == GX_FALSE) {
            throw std::logic_error(
                "Captured-frame blur requires a completed GPU screen capture.");
        }
        if (!(coordinate_width > 0.0F) || !(coordinate_height > 0.0F) ||
            framebuffer_width == 0U || framebuffer_height == 0U) {
            throw std::invalid_argument(
                "Captured-frame blur dimensions must be positive.");
        }

        setup_textured_pass(coordinate_width, coordinate_height, framebuffer_width,
                            framebuffer_height, true);
        set_texture_alpha(current_alpha);
        draw_textured_quad(captured_frame, -current_expand, -current_expand,
                           coordinate_width + current_expand * 2.0F,
                           coordinate_height + current_expand * 2.0F);

        if (_stats.history_valid) {
            auto& history = ensure_history_texture();
            set_texture_alpha(history_alpha);
            draw_textured_quad(history, -history_expand, -history_expand,
                               coordinate_width + history_expand * 2.0F,
                               coordinate_height + history_expand * 2.0F);
        }

        capture_history(captured_frame);
        ++_stats.draw_count;
    }

    const CapturedFrameBlurStats& CapturedFrameBlurService::stats() const noexcept {
        return _stats;
    }

    JUTTexture* CapturedFrameBlurService::history_texture() noexcept {
        return _history_texture.get();
    }

    std::uint16_t CapturedFrameBlurService::history_width() const noexcept {
        return _history_width;
    }

    std::uint16_t CapturedFrameBlurService::history_height() const noexcept {
        return _history_height;
    }

    void CapturedFrameBlurService::capture_history(JUTTexture& captured_frame) {
        auto& history = ensure_history_texture();
        {
            const auto framebuffer =
                ScopedOffscreenFramebuffer(_history_width, _history_height);
            setup_textured_pass(static_cast<float>(_history_width),
                                static_cast<float>(_history_height), _history_width,
                                _history_height, false);
            set_texture_alpha(255U);
            draw_textured_quad(captured_frame, 0.0F, 0.0F,
                               static_cast<float>(_history_width),
                               static_cast<float>(_history_height));
            history.capture(0, 0, GX_TF_RGB565, false, 0U);
            GXPixModeSync();
        }

        if (AuroraHasTextureCopy(history.mImage) == GX_FALSE) {
            throw std::logic_error(
                "Aurora did not materialize the captured-frame blur history texture.");
        }
        _stats.history_valid = true;
        ++_stats.history_capture_count;
    }

    JUTTexture& CapturedFrameBlurService::ensure_history_texture() {
        if (_history_texture == nullptr) {
            _history_texture = std::make_unique<JUTTexture>(
                _history_width, _history_height, GX_TF_RGB565);
        }
        return *_history_texture;
    }

}  // namespace smgpc::compat

namespace MR {

    void drawFullScreenBlur(f32 expand) {
        drawFullScreenBlur(expand, expand, static_cast<u8>(expand / 30.0F),
                           static_cast<u8>((expand / 30.0F) / 2.0F));
    }

    void drawFullScreenBlur(f32 current_expand, f32 history_expand,
                            u8 current_alpha, u8 history_alpha) {
        auto* service = smgpc::scene::current_captured_frame_blur_service();
        if (service == nullptr) {
            throw std::logic_error(
                "Full-screen blur requires a scene-owned captured-frame blur service.");
        }

        const auto* source = getScreenResTIMG();
        auto* source_image = getScreenTexImage();
        auto* video = JUTVideo::getManager();
        if (source == nullptr || source_image == nullptr || video == nullptr ||
            video->getRenderMode() == nullptr) {
            throw std::logic_error(
                "Full-screen blur requires the real CaptureScreenDirector texture.");
        }

        auto captured_frame = JUTTexture(source, 0U);
        const auto* render_mode = video->getRenderMode();
        service->draw(captured_frame, 608.0F,
                      static_cast<float>(render_mode->efbHeight),
                      render_mode->fbWidth, render_mode->efbHeight, current_expand,
                      history_expand, current_alpha, history_alpha);
    }

    void createCenterScreenBlur() {
        if (createSceneObj(SceneObj_CenterScreenBlur) == nullptr) {
            throw std::logic_error(
                "CenterScreenBlur requires a scene-owned SceneObjHolder.");
        }
    }

    void startCenterScreenBlur(s32 time, f32 offset, u8 alpha, s32 fade_in,
                               s32 fade_out) {
        auto* holder = getSceneObjHolder();
        auto* blur = holder != nullptr
                         ? dynamic_cast<CenterScreenBlur*>(
                               holder->getObj(SceneObj_CenterScreenBlur))
                         : nullptr;
        if (blur == nullptr) {
            throw std::logic_error(
                "CenterScreenBlur must be created before it can be started.");
        }
        blur->start(time, offset, alpha, fade_in, fade_out);
    }

}  // namespace MR
