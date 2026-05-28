#include "runtime/WiiVideoService.hpp"

#include <algorithm>
#include <limits>

#include "RendererService.hpp"
#include "runtime/RuntimeContext.hpp"

namespace smgpc::runtime {
    namespace {
        [[nodiscard]] GXRenderModeObj default_render_mode() {
            auto mode = GXRenderModeObj {};
            mode.viTVmode = VI_TVMODE_NTSC_INT;
            mode.fbWidth = smgpc::render::core::kWiiLogicalFramebufferWidth;
            mode.efbHeight = smgpc::render::core::kWiiLogicalFramebufferHeight;
            mode.xfbHeight = smgpc::render::core::kWiiLogicalFramebufferHeight;
            mode.viWidth = smgpc::render::core::kWiiLogicalFramebufferWidth;
            mode.viHeight = smgpc::render::core::kWiiLogicalFramebufferHeight;
            mode.xFBmode = VI_XFBMODE_SF;
            mode.vfilter[0U] = 0U;
            mode.vfilter[1U] = 0U;
            mode.vfilter[2U] = 21U;
            mode.vfilter[3U] = 22U;
            mode.vfilter[4U] = 21U;
            mode.vfilter[5U] = 0U;
            mode.vfilter[6U] = 0U;
            return mode;
        }

        [[nodiscard]] u32 saturating_u32_increment(u32 value) {
            return value == std::numeric_limits<u32>::max() ? value : value + 1U;
        }
    }  // namespace

    WiiVideoService::WiiVideoService() : _render_mode(default_render_mode()) {
    }

    void WiiVideoService::reset() {
        _render_mode = default_render_mode();
        _frame_index = 0U;
        _retrace_count = 0U;
        _black = FALSE;
        _flushed = FALSE;
        _dimming_enabled = TRUE;
        _next_frame_buffer = nullptr;
        push_event(WiiVideoEventKind::Init);
    }

    void WiiVideoService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
        _retrace_count = std::max(_retrace_count, static_cast<u32>(std::min<std::uint64_t>(frame_index, std::numeric_limits<u32>::max())));
    }

    void WiiVideoService::configure(const GXRenderModeObj *render_mode) {
        _render_mode = render_mode != nullptr ? *render_mode : default_render_mode();
        _flushed = FALSE;
        push_event(WiiVideoEventKind::Configure);
    }

    void WiiVideoService::configure_pan(u16 x_origin, u16 y_origin, u16 width, u16 height) {
        _render_mode.viXOrigin = x_origin;
        _render_mode.viYOrigin = y_origin;
        _render_mode.viWidth = width;
        _render_mode.viHeight = height;
        _flushed = FALSE;
        push_event(WiiVideoEventKind::ConfigurePan);
    }

    void WiiVideoService::set_black(BOOL black) {
        _black = black != FALSE ? TRUE : FALSE;
        _flushed = FALSE;
        push_event(WiiVideoEventKind::SetBlack);
    }

    void WiiVideoService::flush() {
        _flushed = TRUE;
        push_event(WiiVideoEventKind::Flush);
    }

    void WiiVideoService::wait_for_retrace() {
        _retrace_count = saturating_u32_increment(_retrace_count);
        push_event(WiiVideoEventKind::WaitRetrace);
    }

    void WiiVideoService::set_next_frame_buffer(void *frame_buffer) {
        _next_frame_buffer = frame_buffer;
        _flushed = FALSE;
        push_event(WiiVideoEventKind::SetNextFrameBuffer);
    }

    void *WiiVideoService::next_frame_buffer() const {
        return _next_frame_buffer;
    }

    BOOL WiiVideoService::enable_dimming(BOOL enabled) {
        const auto previous = _dimming_enabled;
        _dimming_enabled = enabled != FALSE ? TRUE : FALSE;
        push_event(WiiVideoEventKind::EnableDimming);
        return previous;
    }

    BOOL WiiVideoService::reset_dimming_count() {
        push_event(WiiVideoEventKind::EnableDimming);
        return TRUE;
    }

    void WiiVideoService::draw_done_start() {
        push_event(WiiVideoEventKind::DrawDoneStart);
    }

    void WiiVideoService::dummy_no_draw_wait() {
        push_event(WiiVideoEventKind::DummyNoDrawWait);
    }

    void WiiVideoService::draw_done_callback() {
        push_event(WiiVideoEventKind::DrawDoneCallback);
    }

    void WiiVideoService::pre_retrace_proc(u32 retrace_count) {
        _retrace_count = std::max(_retrace_count, retrace_count);
        push_event(WiiVideoEventKind::PreRetrace);
    }

    void WiiVideoService::post_retrace_proc(u32 retrace_count) {
        _retrace_count = std::max(_retrace_count, retrace_count);
        push_event(WiiVideoEventKind::PostRetrace);
    }

    GXRenderModeObj &WiiVideoService::render_mode() {
        return _render_mode;
    }

    const GXRenderModeObj &WiiVideoService::render_mode() const {
        return _render_mode;
    }

    u32 WiiVideoService::retrace_count() const {
        return _retrace_count;
    }

    u32 WiiVideoService::tv_format() const {
        return _render_mode.viTVmode >> 2U;
    }

    u32 WiiVideoService::scan_mode() const {
        return _render_mode.viTVmode & 0x3U;
    }

    u32 WiiVideoService::dtv_status() const {
        return scan_mode() == VI_PROGRESSIVE ? 1U : 0U;
    }

    u32 WiiVideoService::current_line() const {
        const auto height = std::max<u32>(_render_mode.viHeight, 1U);
        return _retrace_count % height;
    }

    BOOL WiiVideoService::is_black() const {
        return _black;
    }

    BOOL WiiVideoService::is_flushed() const {
        return _flushed;
    }

    BOOL WiiVideoService::is_dimming_enabled() const {
        return _dimming_enabled;
    }

    std::span<const WiiVideoEvent> WiiVideoService::events() const {
        return _events;
    }

    void WiiVideoService::clear_trace() {
        _events.clear();
    }

    void WiiVideoService::push_event(WiiVideoEventKind kind) {
        _events.push_back(WiiVideoEvent {
            .kind = kind,
            .frame_index = _frame_index,
            .retrace_count = _retrace_count,
            .vi_tv_mode = _render_mode.viTVmode,
            .tv_format = tv_format(),
            .scan_mode = scan_mode(),
            .black = _black,
            .flushed = _flushed,
            .dimming_enabled = _dimming_enabled,
            .frame_buffer = _next_frame_buffer,
        });
    }

}  // namespace smgpc::runtime

namespace {
    smgpc::runtime::WiiVideoService &active_video_service() {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            return runtime->wii_video();
        }

        static auto fallback = smgpc::runtime::WiiVideoService();
        return fallback;
    }
}  // namespace

void VIInit() {
    active_video_service().reset();
}

void VIConfigure(const GXRenderModeObj *render_mode) {
    active_video_service().configure(render_mode);
}

void VIConfigurePan(u16 x_origin, u16 y_origin, u16 width, u16 height) {
    active_video_service().configure_pan(x_origin, y_origin, width, height);
}

u32 VIGetDTVStatus() {
    return active_video_service().dtv_status();
}

u32 VIGetTvFormat() {
    return active_video_service().tv_format();
}

u32 VIGetCurrentLine() {
    return active_video_service().current_line();
}

u32 VIGetScanMode() {
    return active_video_service().scan_mode();
}

void VISetBlack(BOOL black) {
    active_video_service().set_black(black);
}

void VIFlush() {
    active_video_service().flush();
}

void VIWaitForRetrace() {
    active_video_service().wait_for_retrace();
}

u32 VIGetRetraceCount() {
    return active_video_service().retrace_count();
}

void VISetNextFrameBuffer(void *frame_buffer) {
    active_video_service().set_next_frame_buffer(frame_buffer);
}

void *VIGetNextFrameBuffer() {
    return active_video_service().next_frame_buffer();
}

BOOL VIEnableDimming(BOOL enabled) {
    return active_video_service().enable_dimming(enabled);
}

BOOL VIResetDimmingCount() {
    return active_video_service().reset_dimming_count();
}
