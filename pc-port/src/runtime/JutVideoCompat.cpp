#include <JSystem/JUtility/JUTVideo.hpp>

#include <memory>

#include "RendererService.hpp"

namespace {
    [[nodiscard]] GXRenderModeObj default_render_mode() {
        auto mode = GXRenderModeObj{};
        mode.fbWidth = smgpc::render::core::kWiiLogicalFramebufferWidth;
        mode.efbHeight = smgpc::render::core::kWiiLogicalFramebufferHeight;
        mode.xfbHeight = smgpc::render::core::kWiiLogicalFramebufferHeight;
        mode.viWidth = smgpc::render::core::kWiiLogicalFramebufferWidth;
        mode.viHeight = smgpc::render::core::kWiiLogicalFramebufferHeight;
        mode.vfilter[0U] = 0U;
        mode.vfilter[1U] = 0U;
        mode.vfilter[2U] = 21U;
        mode.vfilter[3U] = 22U;
        mode.vfilter[4U] = 21U;
        mode.vfilter[5U] = 0U;
        mode.vfilter[6U] = 0U;
        return mode;
    }

    std::unique_ptr<JUTVideo> s_manager;
}  // namespace

JUTVideo::JUTVideo(const GXRenderModeObj *render_mode) {
    setRenderMode(render_mode);
}

JUTVideo::~JUTVideo() = default;

JUTVideo *JUTVideo::createManager(const GXRenderModeObj *render_mode) {
    s_manager = std::make_unique<JUTVideo>(render_mode);
    return s_manager.get();
}

void JUTVideo::destroyManager() {
    s_manager.reset();
}

void JUTVideo::drawDoneStart() {
}

void JUTVideo::dummyNoDrawWait() {
}

void JUTVideo::setRenderMode(const GXRenderModeObj *render_mode) {
    mRenderObjStorage = render_mode != nullptr ? *render_mode : default_render_mode();
    mRenderObj = &mRenderObjStorage;
}

void JUTVideo::waitRetraceIfNeed() {
}

void JUTVideo::preRetraceProc(u32) {
}

void JUTVideo::postRetraceProc(u32) {
}

void JUTVideo::drawDoneCallback() {
}

JUTVideo *JUTVideo::getManager() {
    if (s_manager == nullptr) {
        createManager(nullptr);
    }

    return s_manager.get();
}
