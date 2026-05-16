#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <span>

#include "RenderTypes.hpp"

namespace smgpc::render::core {

struct RenderClearCommand {
    std::uint8_t view_id {0U};
    bool clear_color {true};
    bool clear_depth {true};
    bool clear_stencil {false};
    std::uint32_t color_value {0x000000ffU};
    float depth_value {1.0F};
    std::uint8_t stencil_value {0U};
};

struct RenderViewportCommand {
    std::uint8_t view_id {0U};
    std::uint16_t x {0U};
    std::uint16_t y {0U};
    std::uint16_t width {1U};
    std::uint16_t height {1U};
    float min_depth {0.0F};
    float max_depth {1.0F};
};

struct RenderScissorCommand {
    std::uint8_t view_id {0U};
    std::uint16_t x {0U};
    std::uint16_t y {0U};
    std::uint16_t width {1U};
    std::uint16_t height {1U};
};

struct RenderTextureRef {
    std::uint64_t id {0U};
    const std::uint8_t *rgba8 {nullptr};
    std::uint16_t width {0U};
    std::uint16_t height {0U};
    std::uint8_t wrap_s {0U};
    std::uint8_t wrap_t {0U};
};

struct RenderLayoutQuad {
    float x0 {};
    float y0 {};
    float x1 {};
    float y1 {};
    float coordinate_width {};
    float coordinate_height {};
    bool use_custom_vertices {};
    float x_tl {};
    float y_tl {};
    float x_tr {};
    float y_tr {};
    float x_bl {};
    float y_bl {};
    float x_br {};
    float y_br {};

    float u0 {};
    float v0 {};
    float q0 {1.0F};
    float u1 {};
    float v1 {};
    float q1 {1.0F};

    float u0_secondary {};
    float v0_secondary {};
    float q0_secondary {1.0F};
    float u1_secondary {};
    float v1_secondary {};
    float q1_secondary {1.0F};

    bool use_custom_tex_coords {};
    float u_tl {};
    float v_tl {};
    float q_tl {1.0F};
    float u_tr {};
    float v_tr {};
    float q_tr {1.0F};
    float u_bl {};
    float v_bl {};
    float q_bl {1.0F};
    float u_br {};
    float v_br {};
    float q_br {1.0F};
    float u_tl_secondary {};
    float v_tl_secondary {};
    float q_tl_secondary {1.0F};
    float u_tr_secondary {};
    float v_tr_secondary {};
    float q_tr_secondary {1.0F};
    float u_bl_secondary {};
    float v_bl_secondary {};
    float q_bl_secondary {1.0F};
    float u_br_secondary {};
    float v_br_secondary {};
    float q_br_secondary {1.0F};

    std::uint32_t color_tl {};
    std::uint32_t color_tr {};
    std::uint32_t color_bl {};
    std::uint32_t color_br {};
    RenderBlendMode blend_mode {RenderBlendMode::Alpha};

    bool use_mask_texture {};
    bool invert_mask {};
    bool mask_uses_alpha {};
    bool texture_alpha_only {};
    bool texture_color_lerp {};
    std::uint32_t tev_color0 {};
    std::uint32_t tev_color1 {0xFFFFFFFFU};
    float tev_color_scale {1.0F};

    RenderTextureRef texture {};
    RenderTextureRef mask_texture {};
};

struct RenderLayoutTriangleVertex {
    float x {};
    float y {};
    float z {};
    float u {};
    float v {};
    float q {1.0F};
    float u_secondary {};
    float v_secondary {};
    float q_secondary {1.0F};
    std::uint32_t color {};
};

struct RenderLayoutTriangleTevStageArgs {
    std::uint8_t a {};
    std::uint8_t b {};
    std::uint8_t c {};
    std::uint8_t d {};
};

struct RenderLayoutTriangleTevStageOp {
    std::uint8_t op {};
    std::uint8_t bias {};
    std::uint8_t scale {};
    std::uint8_t clamp {};
    std::uint8_t output_register {};
};

struct RenderLayoutTriangleTevStage {
    RenderLayoutTriangleTevStageArgs color_args {};
    RenderLayoutTriangleTevStageOp color_op {};
};

struct RenderLayoutTriangleBatch {
    float coordinate_width {};
    float coordinate_height {};
    RenderBlendMode blend_mode {RenderBlendMode::Alpha};
    RenderTriangleTextureCombineMode secondary_texture_mode {RenderTriangleTextureCombineMode::None};
    std::uint32_t tev_color0 {};
    std::uint32_t tev_color1 {0xFFFFFFFFU};
    std::uint8_t tev_stage_count {};
    std::array<RenderLayoutTriangleTevStage, 2U> tev_stages {};
    RenderTextureRef texture {};
    RenderTextureRef secondary_texture {};
    std::vector<RenderLayoutTriangleVertex> vertices {};
};

enum class RenderLayoutDrawItemKind : std::uint8_t {
    Quad,
    TriangleBatch,
};

struct RenderLayoutDrawItem {
    RenderLayoutDrawItemKind kind {RenderLayoutDrawItemKind::Quad};
    std::size_t index {};
};

struct RenderDrawLayoutCommand {
    std::uint8_t view_id {0U};
    std::uint16_t framebuffer_width {1U};
    std::uint16_t framebuffer_height {1U};
    float layout_width {1.0F};
    float layout_height {1.0F};

    bool debug_solid_quad {false};
    bool debug_force_touch {false};
    bool debug_touch_only {false};
    bool debug_trace {false};

    std::vector<RenderLayoutQuad> quads {};
    std::vector<RenderLayoutTriangleBatch> triangle_batches {};
    std::vector<RenderLayoutDrawItem> draw_order {};
};

struct RenderJ3dVertex {
    float x {};
    float y {};
    float z {};
    float u {};
    float v {};
    float q {1.0F};
    float u_secondary {};
    float v_secondary {};
    float q_secondary {1.0F};
    float u2 {};
    float v2 {};
    float q2 {1.0F};
    float u3 {};
    float v3 {};
    float q3 {1.0F};
    std::uint32_t color {0xFFFFFFFFU};
};

struct RenderJ3dTevStageArgs {
    std::uint8_t a {};
    std::uint8_t b {};
    std::uint8_t c {};
    std::uint8_t d {};
};

struct RenderJ3dTevStageOp {
    std::uint8_t op {};
    std::uint8_t bias {};
    std::uint8_t scale {};
    std::uint8_t clamp {};
    std::uint8_t output_register {};
};

struct RenderJ3dTevStage {
    RenderJ3dTevStageArgs color_args {};
    RenderJ3dTevStageOp color_op {};
    RenderJ3dTevStageArgs alpha_args {};
    RenderJ3dTevStageOp alpha_op {};
};

struct RenderJ3dTevSwapChannels {
    std::array<std::uint8_t, 4U> channels {0U, 1U, 2U, 3U};
};

struct RenderJ3dAlphaCompare {
    bool valid {};
    std::uint8_t comp0 {};
    std::uint8_t ref0 {};
    std::uint8_t op {};
    std::uint8_t comp1 {};
    std::uint8_t ref1 {};
};

struct RenderJ3dIndirectTextureOrder {
    bool valid {};
    std::uint8_t texture_index {};
    std::uint8_t texture_coordinate {};
    std::uint8_t texture_map {};
    std::uint8_t scale_s {};
    std::uint8_t scale_t {};
};

struct RenderJ3dIndirectTextureMatrix {
    bool valid {};
    std::array<float, 6U> values {0.5F, 0.0F, 0.0F, 0.0F, 0.5F, 0.0F};
    std::int8_t scale_exponent {1};
};

struct RenderJ3dIndirectTevStage {
    bool valid {};
    std::uint8_t ind_stage {};
    std::uint8_t format {};
    std::uint8_t bias {};
    std::uint8_t matrix {};
    std::uint8_t wrap_s {};
    std::uint8_t wrap_t {};
    std::uint8_t add_prev {};
    std::uint8_t alpha {};
};

struct RenderJ3dMaterialBatch {
    RenderBlendMode blend_mode {RenderBlendMode::Alpha};
    RenderCullMode cull_mode {RenderCullMode::Back};
    RenderDepthMode depth_mode {RenderDepthMode::LessEqual};
    bool write_depth {true};
    RenderTriangleTextureCombineMode secondary_texture_mode {RenderTriangleTextureCombineMode::None};
    std::uint32_t tev_color0 {};
    std::uint32_t tev_color1 {0xFFFFFFFFU};
    std::array<std::uint32_t, 4U> tev_colors {};
    std::array<std::uint32_t, 4U> tev_k_colors {};
    std::uint8_t tev_stage_count {};
    std::array<RenderJ3dTevStage, 4U> tev_stages {};
    std::array<std::uint8_t, 4U> tev_stage_k_color_selectors {};
    std::array<std::uint8_t, 4U> tev_stage_k_alpha_selectors {};
    std::array<RenderJ3dTevSwapChannels, 4U> tev_stage_texture_swizzles {};
    std::array<RenderJ3dTevSwapChannels, 4U> tev_stage_raster_swizzles {};
    RenderJ3dAlphaCompare alpha_compare {};
    std::uint8_t indirect_texture_stage_count {};
    std::array<RenderJ3dIndirectTextureOrder, 4U> indirect_texture_orders {};
    std::array<RenderJ3dIndirectTextureMatrix, 3U> indirect_texture_matrices {};
    std::array<RenderJ3dIndirectTevStage, 4U> indirect_tev_stages {};
    std::uint8_t texture_count {};
    std::array<RenderTextureRef, 4U> textures {};
    std::array<std::uint8_t, 4U> tev_stage_texture_indices {0U, 1U, 0U, 1U};
    RenderTextureRef texture {};
    RenderTextureRef secondary_texture {};
    std::vector<RenderJ3dVertex> vertices {};
};

struct RenderDrawJ3dCommand {
    std::uint8_t view_id {0U};
    std::uint16_t framebuffer_width {1U};
    std::uint16_t framebuffer_height {1U};
    bool use_camera {false};
    std::array<float, 3U> camera_eye {0.0F, 0.0F, 1.0F};
    std::array<float, 3U> camera_target {0.0F, 0.0F, 0.0F};
    std::array<float, 3U> camera_up {0.0F, 1.0F, 0.0F};
    float camera_fovy_degrees {60.0F};
    float camera_near {64.0F};
    float camera_far {1000000.0F};
    std::array<float, 16U> view_matrix {
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F,
    };
    std::array<float, 16U> projection_matrix {
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F,
    };
    std::vector<RenderJ3dMaterialBatch> batches {};
};

struct RenderBindTextureCommand {
    std::uint8_t view_id {0U};
    std::uint8_t sampler_slot {0U};
    RenderTextureRef texture {};
};

struct RenderMarkerCommand {
    std::string label {};
};

struct RenderBarrierCommand {
    std::uint8_t view_id {0U};
};

enum class RenderCommandType {
    Marker,
    Clear,
    SetViewport,
    SetScissor,
    DrawLayout,
    DrawJ3d,
    BindTexture,
    Barrier,
};

using RenderCommandPayload = std::variant<
    RenderMarkerCommand,
    RenderClearCommand,
    RenderViewportCommand,
    RenderScissorCommand,
    RenderDrawLayoutCommand,
    RenderDrawJ3dCommand,
    RenderBindTextureCommand,
    RenderBarrierCommand>;

struct RenderCommand {
    RenderCommandType type {RenderCommandType::Marker};
    RenderCommandPayload payload;
};

class RenderCommandBuffer {
public:
    using CommandList = std::vector<RenderCommand>;

    void clear() {
        _commands.clear();
    }

    void reserve(std::size_t command_count) {
        _commands.reserve(command_count);
    }

    void marker(std::string_view label) {
        _commands.push_back(RenderCommand {
            .type = RenderCommandType::Marker,
            .payload = RenderMarkerCommand {.label = std::string(label)},
        });
    }

    void clear_view(std::uint8_t view_id, std::uint32_t color, float depth = 1.0F, std::uint8_t stencil = 0U) {
        _commands.push_back(RenderCommand {
            .type = RenderCommandType::Clear,
            .payload = RenderClearCommand {
                .view_id = view_id,
                .color_value = color,
                .depth_value = depth,
                .stencil_value = stencil,
            },
        });
    }

    void set_viewport(
        std::uint8_t view_id,
        std::uint16_t x,
        std::uint16_t y,
        std::uint16_t width,
        std::uint16_t height,
        float min_depth = 0.0F,
        float max_depth = 1.0F) {
        _commands.push_back(RenderCommand {
            .type = RenderCommandType::SetViewport,
            .payload = RenderViewportCommand {
                .view_id = view_id,
                .x = x,
                .y = y,
                .width = width,
                .height = height,
                .min_depth = min_depth,
                .max_depth = max_depth,
            },
        });
    }

    void set_scissor(std::uint8_t view_id, std::uint16_t x, std::uint16_t y, std::uint16_t width, std::uint16_t height) {
        _commands.push_back(RenderCommand {
            .type = RenderCommandType::SetScissor,
            .payload = RenderScissorCommand {
                .view_id = view_id,
                .x = x,
                .y = y,
                .width = width,
                .height = height,
            },
        });
    }

    void draw_layout(const RenderDrawLayoutCommand &command) {
        _commands.push_back(RenderCommand {
            .type = RenderCommandType::DrawLayout,
            .payload = command,
        });
    }

    void draw_j3d(const RenderDrawJ3dCommand &command) {
        _commands.push_back(RenderCommand {
            .type = RenderCommandType::DrawJ3d,
            .payload = command,
        });
    }

    void bind_texture(std::uint8_t view_id, std::uint8_t sampler_slot, const RenderTextureRef &texture) {
        _commands.push_back(RenderCommand {
            .type = RenderCommandType::BindTexture,
            .payload = RenderBindTextureCommand {
                .view_id = view_id,
                .sampler_slot = sampler_slot,
                .texture = texture,
            },
        });
    }

    [[nodiscard]] std::span<const RenderCommand> commands() const {
        return std::span(_commands.data(), _commands.size());
    }

private:
    CommandList _commands {};
};

}  // namespace smgpc::render::core
