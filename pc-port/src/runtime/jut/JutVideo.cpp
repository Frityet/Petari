#include <JSystem/JUtility/JUTVideo.hpp>

#include <memory>

#include "runtime/RuntimeContext.hpp"
#include "runtime/WiiVideoService.hpp"

namespace {
    std::unique_ptr<JUTVideo> s_manager;

    smgpc::runtime::WiiVideoService &active_video_service() {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            return runtime->wii_video();
        }

        static auto fallback = smgpc::runtime::WiiVideoService();
        return fallback;
    }
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
    active_video_service().draw_done_start();
}

void JUTVideo::dummyNoDrawWait() {
    active_video_service().dummy_no_draw_wait();
}

void JUTVideo::setRenderMode(const GXRenderModeObj *render_mode) {
    active_video_service().configure(render_mode);
}

void JUTVideo::waitRetraceIfNeed() {
    active_video_service().wait_for_retrace();
}

void JUTVideo::preRetraceProc(u32) {
    active_video_service().pre_retrace_proc(VIGetRetraceCount());
}

void JUTVideo::postRetraceProc(u32) {
    active_video_service().post_retrace_proc(VIGetRetraceCount());
}

void JUTVideo::drawDoneCallback() {
    active_video_service().draw_done_callback();
}

JUTVideo *JUTVideo::getManager() {
    if (s_manager == nullptr) {
        createManager(nullptr);
    }

    return s_manager.get();
}

GXRenderModeObj *JUTVideo::getRenderMode() const {
    return &active_video_service().render_mode();
}
