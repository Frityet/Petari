#pragma once

#include <cstdint>
#include <memory>

#include <revolution/types.h>

class JUTTexture;

namespace smgpc::compat {

    struct CapturedFrameBlurStats {
        std::uint64_t draw_count = 0U;
        std::uint64_t history_capture_count = 0U;
        bool history_valid = false;
    };

    class CapturedFrameBlurService final {
    public:
        CapturedFrameBlurService(std::uint16_t history_width = 128U,
                                 std::uint16_t history_height = 64U);
        CapturedFrameBlurService(const CapturedFrameBlurService&) = delete;
        CapturedFrameBlurService& operator=(const CapturedFrameBlurService&) = delete;
        ~CapturedFrameBlurService();

        void draw(JUTTexture& captured_frame, float coordinate_width,
                  float coordinate_height, std::uint16_t framebuffer_width,
                  std::uint16_t framebuffer_height, float current_expand,
                  float history_expand, u8 current_alpha, u8 history_alpha);

        [[nodiscard]] const CapturedFrameBlurStats& stats() const noexcept;
        [[nodiscard]] JUTTexture* history_texture() noexcept;
        [[nodiscard]] std::uint16_t history_width() const noexcept;
        [[nodiscard]] std::uint16_t history_height() const noexcept;

    private:
        void capture_history(JUTTexture& captured_frame);
        JUTTexture& ensure_history_texture();

        std::uint16_t _history_width;
        std::uint16_t _history_height;
        std::unique_ptr<JUTTexture> _history_texture;
        CapturedFrameBlurStats _stats;
    };

}  // namespace smgpc::compat
