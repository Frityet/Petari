#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>

namespace smgpc::render::core {

    inline constexpr std::uint16_t kWiiLogicalFramebufferWidth = 640U;
    inline constexpr std::uint16_t kWiiLogicalFramebufferHeight = 456U;

    struct WindowConfiguration {
        int width = kWiiLogicalFramebufferWidth;
        int height = kWiiLogicalFramebufferHeight;
        std::string title = "SMG PC Port";
    };

    struct RenderInitDesc : WindowConfiguration {
        std::uint16_t logical_width = kWiiLogicalFramebufferWidth;
        std::uint16_t logical_height = kWiiLogicalFramebufferHeight;
        bool enable_vsync = true;
        void* native_window_handle = nullptr;
        void* native_display_handle = nullptr;
    };

    struct FramebufferInfo {
        std::uint16_t width = 1U;
        std::uint16_t height = 1U;
    };

    struct FrameContext {
        std::uint64_t frame_index = 0;
        double frame_time_seconds = 0;
        double frame_delta_seconds = 0;
        FramebufferInfo framebuffer = {};
        bool has_focus = true;
        bool is_minimized = false;
    };

    struct NativeWindowHandle {
        void* window_handle = nullptr;
        void* display_handle = nullptr;
    };

    enum class InputButton {
        CORE_PAD_A,
        CORE_PAD_B,
    };

    enum class BlendMode {
        Opaque,
        Alpha,
        Additive,
    };

    enum class DepthCompare {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always,
    };

    struct TextureHandle {
        static constexpr std::uint32_t INVALID_VALUE = UINT32_MAX;

        std::uint32_t value = INVALID_VALUE;

        [[nodiscard]] bool is_valid() const {
            return value != INVALID_VALUE;
        }
    };

    struct TexturedVertex2D {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
        float u = 0.0F;
        float v = 0.0F;
        std::array< std::uint8_t, 4U > color{255U, 255U, 255U, 255U};
    };

    struct TexturedQuad2D {
        std::array< TexturedVertex2D, 4U > vertices{};
        bool wrap_u = false;
        bool wrap_v = false;
        bool blend = true;
        BlendMode blend_mode = BlendMode::Alpha;
        bool depth_test = false;
        bool depth_write = false;
        DepthCompare depth_compare = DepthCompare::LessEqual;
    };

    struct TexturedTriangleBatch2D {
        std::span< const TexturedVertex2D > vertices{};
        std::span< const std::uint16_t > indices{};
        bool wrap_u = false;
        bool wrap_v = false;
        bool blend = true;
        BlendMode blend_mode = BlendMode::Alpha;
        bool depth_test = false;
        bool depth_write = false;
        DepthCompare depth_compare = DepthCompare::LessEqual;
    };

}  // namespace smgpc::render::core
