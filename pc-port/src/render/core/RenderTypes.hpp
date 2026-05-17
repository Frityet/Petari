#pragma once

#include <array>
#include <cstddef>
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

    enum class CullMode {
        None,
        Front,
        Back,
        FrontAndBack,
    };

    struct GxAlphaCompare2D {
        std::uint8_t comp0 = 7U;
        std::uint8_t ref0 = 0U;
        std::uint8_t op = 0U;
        std::uint8_t comp1 = 7U;
        std::uint8_t ref1 = 0U;
        bool enabled = false;
    };

    struct GxBlendMode2D {
        std::uint8_t type = 0U;
        std::uint8_t src_factor = 1U;
        std::uint8_t dst_factor = 0U;
        std::uint8_t op = 3U;
        bool color_update = true;
        bool alpha_update = true;
        bool enabled = false;
    };

    struct GxFog2D {
        bool enabled = false;
        std::uint8_t type = 0U;
        std::uint8_t projection = 0U;
        bool range_adjust_enabled = false;
        std::array<std::uint8_t, 4U> color{};
        float a = 0.0F;
        float c = 0.0F;
        std::uint32_t b_magnitude = 0U;
        std::uint32_t b_shift = 0U;
    };

    using GxTevRegisterColor2D = std::array< std::int16_t, 4U >;

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

    inline constexpr std::size_t kMaxGxMaterialTextureStages2D = 3U;
    inline constexpr std::size_t kMaxGxMaterialTevStages2D = 3U;

    struct GxMaterialVertex2D {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
        float clip_w = 1.0F;
        std::array< std::array< float, 3U >, kMaxGxMaterialTextureStages2D > tex_coords{};
        std::array< std::uint8_t, 4U > color{255U, 255U, 255U, 255U};
    };

    struct GxTextureStage2D {
        TextureHandle texture{};
        bool wrap_u = false;
        bool wrap_v = false;
    };

    struct GxTevStage2D {
        std::uint8_t texture_stage = 0U;
        std::array< std::uint8_t, 4U > color_in{};
        std::uint8_t color_op = 0U;
        std::uint8_t color_bias = 0U;
        std::uint8_t color_scale = 0U;
        bool color_clamp = true;
        std::uint8_t color_out = 0U;
        std::array< std::uint8_t, 4U > alpha_in{};
        std::uint8_t alpha_op = 0U;
        std::uint8_t alpha_bias = 0U;
        std::uint8_t alpha_scale = 0U;
        bool alpha_clamp = true;
        std::uint8_t alpha_out = 0U;
        std::array< std::uint8_t, 4U > konst_color{0U, 0U, 0U, 0U};
    };

    struct TexturedQuad2D {
        std::array< TexturedVertex2D, 4U > vertices{};
        bool wrap_u = false;
        bool wrap_v = false;
        bool blend = true;
        BlendMode blend_mode = BlendMode::Alpha;
        GxBlendMode2D gx_blend{};
        bool depth_test = false;
        bool depth_write = false;
        DepthCompare depth_compare = DepthCompare::LessEqual;
        CullMode cull_mode = CullMode::None;
        GxFog2D fog{};
    };

    struct TexturedTriangleBatch2D {
        std::span< const TexturedVertex2D > vertices{};
        std::span< const std::uint16_t > indices{};
        bool wrap_u = false;
        bool wrap_v = false;
        bool blend = true;
        BlendMode blend_mode = BlendMode::Alpha;
        GxBlendMode2D gx_blend{};
        bool depth_test = false;
        bool depth_write = false;
        DepthCompare depth_compare = DepthCompare::LessEqual;
        CullMode cull_mode = CullMode::None;
        GxFog2D fog{};
    };

    struct GxMaterialTriangleBatch2D {
        std::span< const GxMaterialVertex2D > vertices{};
        std::span< const std::uint16_t > indices{};
        std::span< const GxTextureStage2D > texture_stages{};
        std::span< const GxTevStage2D > tev_stages{};
        std::array< GxTevRegisterColor2D, 4U > initial_tev_registers{};
        GxAlphaCompare2D alpha_compare{};
        GxBlendMode2D blend{};
        bool depth_test = false;
        bool depth_write = false;
        DepthCompare depth_compare = DepthCompare::LessEqual;
        CullMode cull_mode = CullMode::None;
        GxFog2D fog{};
    };

}  // namespace smgpc::render::core
