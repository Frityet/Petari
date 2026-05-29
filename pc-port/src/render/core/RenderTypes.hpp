#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <dolphin/gx.h>

namespace smgpc::render::core {

    inline constexpr std::uint16_t kWiiLogicalFramebufferWidth = 640U;
    inline constexpr std::uint16_t kWiiLogicalFramebufferHeight = 456U;
    inline constexpr std::uint16_t kWiiLayoutWidth = 608U;

    struct WindowConfiguration {
        int width = kWiiLogicalFramebufferWidth;
        int height = kWiiLogicalFramebufferHeight;
        std::string title = "SMG PC Port";
    };

    struct RenderInitDesc : WindowConfiguration {
        std::uint16_t logical_width = kWiiLogicalFramebufferWidth;
        std::uint16_t logical_height = kWiiLogicalFramebufferHeight;
        bool enable_vsync = true;
        void *native_window_handle = nullptr;
        void *native_display_handle = nullptr;
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

    enum class CopyEventKind {
        Texture,
        Xfb,
        Present,
    };

    struct CopyRect {
        std::int32_t left = 0;
        std::int32_t top = 0;
        std::int32_t right = 0;
        std::int32_t bottom = 0;
        std::int32_t width = 0;
        std::int32_t height = 0;
    };

    struct CopyViewport {
        float left = 0.0F;
        float right = 0.0F;
        float top = 0.0F;
        float bottom = 0.0F;
        float near_depth = 0.0F;
        float far_depth = 1.0F;
    };

    struct CopyEvent {
        std::uint64_t index = 0;
        std::uint64_t event_index = 0;
        std::uint64_t presenter_frame_count = 0;
        CopyEventKind kind = CopyEventKind::Present;
        bool copy_to_xfb = false;
        bool depth_copy = false;
        bool clear = false;
        bool half_scale = false;
        bool scale_invert = false;
        bool clamp_top = true;
        bool clamp_bottom = true;
        bool intensity_format = false;
        bool auto_conversion = false;
        std::array<std::uint8_t, 4U> clear_color = {0U, 0U, 0U, 0U};
        std::uint32_t clear_depth = 0U;
        bool copy_filter_aa = false;
        bool copy_filter_vertical = false;
        std::array<std::array<std::uint8_t, 2U>, 12U> copy_filter_sample_pattern = {};
        std::array<std::uint8_t, 7U> copy_filter_vfilter = {};
        std::uint32_t dest_addr = 0U;
        std::uint32_t dest_stride = 0U;
        CopyRect source_rect = {};
        FramebufferInfo output_size = {};
        std::uint32_t target_pixel_format = 0U;
        std::uint32_t real_format = 0U;
        std::uint32_t frame_to_field = 0U;
        std::uint32_t gamma_index = 0U;
        float gamma_value = 1.0F;
        float y_scale = 1.0F;
        std::uint32_t dispcopyyscale = 256U;
        CopyRect scissor = {};
        CopyViewport viewport = {};
        FramebufferInfo backbuffer = {};
        CopyRect target_rect = {};
        std::string render_pass;
        std::uint16_t view_id = 0U;
    };

    struct NativeWindowHandle {
        void *window_handle = nullptr;
        void *display_handle = nullptr;
    };

    struct InputPointerState {
        float x = 0.0F;
        float y = 0.0F;
        bool valid = false;
    };

    enum class InputButton {
        CORE_PAD_A,
        CORE_PAD_B,
        CORE_PAD_UP,
        CORE_PAD_DOWN,
        CORE_PAD_LEFT,
        CORE_PAD_RIGHT,
        CORE_PAD_PLUS,
        CORE_PAD_MINUS,
        CORE_PAD_HOME,
        CORE_PAD_C,
        CORE_PAD_Z,
        COUNT,
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

    enum class PrimitiveTopology {
        Triangles,
        TriangleStrip,
    };

    enum class RenderSpace2D {
        Layout,
        CenteredFramebuffer,
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
        std::array<std::uint8_t, 4U> color = {0U, 0U, 0U, 0U};
        float a = 0.0F;
        float c = 0.0F;
        std::uint32_t b_magnitude = 0U;
        std::uint32_t b_shift = 0U;
    };

    using GxTevRegisterColor2D = std::array<std::int16_t, 4U>;

    struct AuroraTexture {
        std::uint16_t width = 0U;
        std::uint16_t height = 0U;
        std::vector<std::uint8_t> rgba = {};
        GXTexObj object {};
        GXTexFmt format = GX_TF_RGBA8_PC;
        bool mipmap = false;
        float min_lod = 0.0F;
        float max_lod = 0.0F;
        float lod_bias = 0.0F;
        bool bias_clamp = false;
        bool edge_lod = false;
        GXAnisotropy max_anisotropy = GX_ANISO_1;
        bool alive = false;
    };

    struct TextureHandle {
        std::shared_ptr<AuroraTexture> texture = {};

        [[nodiscard]] bool is_valid() const {
            return texture != nullptr && texture->alive;
        }

        [[nodiscard]] std::uintptr_t debug_id() const {
            return reinterpret_cast<std::uintptr_t>(texture.get());
        }
    };

    struct TexturedVertex2D {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
        float u = 0.0F;
        float v = 0.0F;
        std::array<std::uint8_t, 4U> color = {255U, 255U, 255U, 255U};
    };

    inline constexpr std::size_t kMaxGxMaterialTextureStages2D = 8U;
    inline constexpr std::size_t kMaxGxMaterialTevStages2D = 16U;

    struct GxMaterialVertex2D {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
        float clip_w = 1.0F;
        std::array<std::array<float, 3U>, kMaxGxMaterialTextureStages2D> tex_coords{};
        std::array<std::uint8_t, 4U> color = {255U, 255U, 255U, 255U};
    };

    struct GxTextureStage2D {
        TextureHandle texture = {};
        std::uint8_t wrap_u = 0U;
        std::uint8_t wrap_v = 0U;
        std::uint8_t min_filter = 1U;
        std::uint8_t mag_filter = 1U;
        GXTexGenType texgen_type = GX_TG_MTX2x4;
        GXTexGenSrc texgen_source = GX_TG_TEX0;
        GXTexMtx texgen_matrix = GX_IDENTITY;
        GXTexMtxType texgen_matrix_type = GX_MTX2x4;
        bool has_texgen_matrix = false;
        std::array<float, 12U> texgen_matrix_values = {};
    };

    struct GxTevStage2D {
        std::uint8_t texture_stage = 0U;
        std::uint8_t texture_coord_stage = 0xffU;
        std::uint8_t texture_map_stage = 0xffU;
        std::uint8_t color_channel = 4U;
        std::array<std::uint8_t, 4U> color_in = {};
        std::uint8_t color_op = 0U;
        std::uint8_t color_bias = 0U;
        std::uint8_t color_scale = 0U;
        bool color_clamp = true;
        std::uint8_t color_out = 0U;
        std::array<std::uint8_t, 4U> alpha_in = {};
        std::uint8_t alpha_op = 0U;
        std::uint8_t alpha_bias = 0U;
        std::uint8_t alpha_scale = 0U;
        bool alpha_clamp = true;
        std::uint8_t alpha_out = 0U;
        std::uint8_t k_color_sel = 0xffU;
        std::uint8_t k_alpha_sel = 0xffU;
        std::uint8_t ras_swap = 0U;
        std::uint8_t tex_swap = 0U;
        std::array<std::uint8_t, 4U> konst_color = {0U, 0U, 0U, 0U};
    };

    inline constexpr std::size_t kMaxGxIndirectStages2D = 4U;
    inline constexpr std::size_t kMaxGxIndirectMatrices2D = 3U;

    struct GxIndirectTextureOrder2D {
        std::uint8_t stage = 0U;
        std::uint8_t tex_map = 0xffU;
        std::uint8_t tex_coord = 0xffU;
    };

    struct GxIndirectTextureMatrix2D {
        std::uint8_t matrix = 0U;
        std::int16_t ma = 0;
        std::int16_t mb = 0;
        std::int16_t mc = 0;
        std::int16_t md = 0;
        std::int16_t me = 0;
        std::int16_t mf = 0;
        std::uint8_t scale = 0U;
    };

    struct GxIndirectTextureCoordScale2D {
        std::uint8_t stage = 0U;
        std::uint8_t scale_s = 0U;
        std::uint8_t scale_t = 0U;
    };

    struct GxIndirectTevStage2D {
        std::uint8_t tev_stage = 0U;
        std::uint8_t ind_stage = 0U;
        std::uint8_t format = 0U;
        std::uint8_t bias = 0U;
        std::uint8_t bump_alpha = 0U;
        std::uint8_t matrix_index = 0U;
        std::uint8_t matrix_id = 0U;
        std::uint8_t wrap_s = 0U;
        std::uint8_t wrap_t = 0U;
        bool use_original_lod = false;
        bool add_previous = false;
        bool active = false;
    };

    struct TexturedQuad2D {
        std::array<TexturedVertex2D, 4U> vertices = {};
        RenderSpace2D space = RenderSpace2D::Layout;
        std::uint8_t wrap_u = 0U;
        std::uint8_t wrap_v = 0U;
        std::uint8_t min_filter = 1U;
        std::uint8_t mag_filter = 1U;
        bool blend = true;
        BlendMode blend_mode = BlendMode::Alpha;
        GxBlendMode2D gx_blend = {};
        bool depth_test = false;
        bool depth_write = false;
        DepthCompare depth_compare = DepthCompare::LessEqual;
        CullMode cull_mode = CullMode::None;
        GxFog2D fog{};
    };

    struct TexturedTriangleBatch2D {
        std::span<const TexturedVertex2D> vertices = {};
        std::span<const std::uint16_t> indices = {};
        PrimitiveTopology primitive_topology = PrimitiveTopology::Triangles;
        RenderSpace2D space = RenderSpace2D::Layout;
        std::uint8_t wrap_u = 0U;
        std::uint8_t wrap_v = 0U;
        std::uint8_t min_filter = 1U;
        std::uint8_t mag_filter = 1U;
        bool blend = true;
        BlendMode blend_mode = BlendMode::Alpha;
        GxBlendMode2D gx_blend = {};
        bool depth_test = false;
        bool depth_write = false;
        DepthCompare depth_compare = DepthCompare::LessEqual;
        CullMode cull_mode = CullMode::None;
        GxFog2D fog{};
    };

    struct GxMaterialTriangleBatch2D {
        std::span<const GxMaterialVertex2D> vertices = {};
        std::span<const std::uint16_t> indices = {};
        PrimitiveTopology primitive_topology = PrimitiveTopology::Triangles;
        RenderSpace2D space = RenderSpace2D::Layout;
        std::span<const GxTextureStage2D> texture_stages = {};
        std::span<const GxTevStage2D> tev_stages = {};
        std::uint8_t indirect_stage_count = 0U;
        std::span<const GxIndirectTextureOrder2D> indirect_texture_orders = {};
        std::span<const GxIndirectTextureMatrix2D> indirect_texture_matrices = {};
        std::span<const GxIndirectTextureCoordScale2D> indirect_texture_scales = {};
        std::span<const GxIndirectTevStage2D> indirect_tev_stages = {};
        std::array<GxTevRegisterColor2D, 4U> initial_tev_registers = {};
        std::array<std::array<std::uint8_t, 4U>, 4U> initial_tev_k_colors = {};
        bool has_initial_tev_k_colors = false;
        std::array<std::uint8_t, 4U> channel_material_color = {255U, 255U, 255U, 255U};
        std::uint8_t channel_color_src = 1U;
        std::uint8_t channel_alpha_src = 1U;
        GxAlphaCompare2D alpha_compare = {};
        GxBlendMode2D blend = {};
        bool depth_test = false;
        bool depth_write = false;
        DepthCompare depth_compare = DepthCompare::LessEqual;
        CullMode cull_mode = CullMode::None;
        GxFog2D fog = {};
    };

}  // namespace smgpc::render::core
