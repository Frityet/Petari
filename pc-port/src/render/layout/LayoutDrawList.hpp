#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace smgpc::render::layout {

struct TextureRef {
    std::uint64_t id {};
    const std::uint8_t *rgba8 {};
    std::uint16_t width {};
    std::uint16_t height {};
    std::uint8_t wrap_s {};
    std::uint8_t wrap_t {};
};

enum class BlendMode : std::uint8_t {
    Alpha,
    Additive,
};

enum class TriangleTextureCombineMode : std::uint8_t {
    None,
    Multiply,
    Add,
    Screen,
    J3dTevColorStages,
};

enum class DrawCommandKind : std::uint8_t {
    Quad,
    TriangleBatch,
};

struct DrawOrderEntry {
    DrawCommandKind kind {DrawCommandKind::Quad};
    std::size_t index {};
};

struct QuadCommand {
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

    BlendMode blend_mode {BlendMode::Alpha};

    bool use_mask_texture {};
    bool invert_mask {};
    bool mask_uses_alpha {};
    bool texture_alpha_only {};
    bool texture_color_lerp {};
    std::uint32_t tev_color0 {};
    std::uint32_t tev_color1 {0xFFFFFFFFU};
    float tev_color_scale {1.0F};
    TextureRef texture {};
    TextureRef mask_texture {};
};

struct TriangleVertex {
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

struct TriangleTevStageArgs {
    std::uint8_t a {};
    std::uint8_t b {};
    std::uint8_t c {};
    std::uint8_t d {};
};

struct TriangleTevStageOp {
    std::uint8_t op {};
    std::uint8_t bias {};
    std::uint8_t scale {};
    std::uint8_t clamp {};
    std::uint8_t output_register {};
};

struct TriangleTevStage {
    TriangleTevStageArgs color_args {};
    TriangleTevStageOp color_op {};
};

struct TriangleBatchCommand {
    float coordinate_width {};
    float coordinate_height {};
    BlendMode blend_mode {BlendMode::Alpha};
    TriangleTextureCombineMode secondary_texture_mode {TriangleTextureCombineMode::None};
    std::uint32_t tev_color0 {};
    std::uint32_t tev_color1 {0xFFFFFFFFU};
    std::uint8_t tev_stage_count {};
    std::array<TriangleTevStage, 2U> tev_stages {};
    TextureRef texture {};
    TextureRef secondary_texture {};
    std::vector<TriangleVertex> vertices {};
};

class LayoutDrawList {
public:
    void clear() {
        _quads.clear();
        _triangle_batches.clear();
        _draw_order.clear();
    }

    void reserve(std::size_t count) {
        _quads.reserve(count);
    }

    void push_quad(const QuadCommand &quad) {
        _quads.push_back(quad);
        _draw_order.push_back(DrawOrderEntry{
            .kind = DrawCommandKind::Quad,
            .index = _quads.size() - 1U,
        });
    }

    void push_triangle_batch(TriangleBatchCommand batch) {
        _triangle_batches.push_back(std::move(batch));
        _draw_order.push_back(DrawOrderEntry{
            .kind = DrawCommandKind::TriangleBatch,
            .index = _triangle_batches.size() - 1U,
        });
    }

    [[nodiscard]] const std::vector<QuadCommand> &quads() const {
        return _quads;
    }

    [[nodiscard]] const std::vector<TriangleBatchCommand> &triangle_batches() const {
        return _triangle_batches;
    }

    [[nodiscard]] const std::vector<DrawOrderEntry> &draw_order() const {
        return _draw_order;
    }

private:
    std::vector<QuadCommand> _quads {};
    std::vector<TriangleBatchCommand> _triangle_batches {};
    std::vector<DrawOrderEntry> _draw_order {};
};

[[nodiscard]] inline std::uint32_t pack_abgr(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    return (static_cast<std::uint32_t>(a) << 24U) |
           (static_cast<std::uint32_t>(b) << 16U) |
           (static_cast<std::uint32_t>(g) << 8U) |
           static_cast<std::uint32_t>(r);
}

}  // namespace smgpc::render::layout
