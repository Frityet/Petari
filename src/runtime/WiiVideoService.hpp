#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include <revolution.h>

namespace smgpc::runtime {

    enum class WiiVideoEventKind {
        Init,
        Configure,
        ConfigurePan,
        SetBlack,
        Flush,
        WaitRetrace,
        SetNextFrameBuffer,
        EnableDimming,
        DrawDoneStart,
        DummyNoDrawWait,
        DrawDoneCallback,
        PreRetrace,
        PostRetrace,
    };

    struct WiiVideoEvent {
        WiiVideoEventKind kind = WiiVideoEventKind::Init;
        std::uint64_t frame_index = 0U;
        u32 retrace_count = 0U;
        u32 vi_tv_mode = 0U;
        u32 tv_format = VI_NTSC;
        u32 scan_mode = VI_INTERLACE;
        BOOL black = FALSE;
        BOOL flushed = FALSE;
        BOOL dimming_enabled = TRUE;
        void *frame_buffer = nullptr;
    };

    class WiiVideoService final {
    public:
        WiiVideoService();

        void reset();
        void begin_frame(std::uint64_t frame_index);
        void configure(const GXRenderModeObj *render_mode);
        void configure_pan(u16 x_origin, u16 y_origin, u16 width, u16 height);
        void set_black(BOOL black);
        void flush();
        void wait_for_retrace();
        void set_next_frame_buffer(void *frame_buffer);
        [[nodiscard]] void *next_frame_buffer() const;
        [[nodiscard]] BOOL enable_dimming(BOOL enabled);
        [[nodiscard]] BOOL reset_dimming_count();
        void draw_done_start();
        void dummy_no_draw_wait();
        void draw_done_callback();
        void pre_retrace_proc(u32 retrace_count);
        void post_retrace_proc(u32 retrace_count);

        [[nodiscard]] GXRenderModeObj &render_mode();
        [[nodiscard]] const GXRenderModeObj &render_mode() const;
        [[nodiscard]] u32 retrace_count() const;
        [[nodiscard]] u32 tv_format() const;
        [[nodiscard]] u32 scan_mode() const;
        [[nodiscard]] u32 dtv_status() const;
        [[nodiscard]] u32 current_line() const;
        [[nodiscard]] BOOL is_black() const;
        [[nodiscard]] BOOL is_flushed() const;
        [[nodiscard]] BOOL is_dimming_enabled() const;
        [[nodiscard]] std::span<const WiiVideoEvent> events() const;
        void clear_trace();

    private:
        void push_event(WiiVideoEventKind kind);

        GXRenderModeObj _render_mode{};
        std::uint64_t _frame_index = 0U;
        u32 _retrace_count = 0U;
        BOOL _black = FALSE;
        BOOL _flushed = FALSE;
        BOOL _dimming_enabled = TRUE;
        void *_next_frame_buffer = nullptr;
        std::vector<WiiVideoEvent> _events;
    };

}  // namespace smgpc::runtime
