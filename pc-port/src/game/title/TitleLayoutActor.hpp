#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "layout/Brfnt.hpp"
#include "TitleAssets.hpp"
#include "layout/LayoutDrawList.hpp"

namespace smgpc::game::title {

class TitleLayoutActor {
public:
    explicit TitleLayoutActor(const TitleLayoutResource *resource);

    void appear();
    void kill();
    [[nodiscard]] bool is_dead() const;

    void start_anim(const char *animation_name, std::uint32_t layer);
    [[nodiscard]] bool is_anim_stopped(std::uint32_t layer) const;
    void set_anim_frame_and_stop(float frame, std::uint32_t layer);

    void update(float delta_frames);

    void emit_effect(const char *);
    void delete_effect_all();

    void append_draw_commands(
        render::layout::LayoutDrawList *draw_list,
        const std::unordered_map<std::string, assets::layout::BrfntFont> &fonts_by_name) const;

private:
    struct RuntimePaneState {
        bool visible {true};
        std::uint8_t alpha {255U};
        std::array<std::uint8_t, 16> vertex_color {};

        float tx {};
        float ty {};
        float tz {};
        float sx {1.0F};
        float sy {1.0F};
        float width {};
        float height {};

        float tex_offset_u {};
        float tex_offset_v {};
        float tex_scale_u {1.0F};
        float tex_scale_v {1.0F};
    };

    struct AnimationSlot {
        const assets::layout::BrlanAnimation *animation {};
        float frame {};
        bool paused {};
    };

    struct TransformContext {
        float origin_x {};
        float origin_y {};
        float scale_x {1.0F};
        float scale_y {1.0F};
        float alpha {1.0F};
        bool visible {true};
    };

    [[nodiscard]] static std::string normalize_name(std::string name);
    [[nodiscard]] static float anchor_offset_x(std::uint8_t base_position, float width);
    [[nodiscard]] static float anchor_offset_y(std::uint8_t base_position, float height);
    [[nodiscard]] static std::uint8_t clamp_u8(float value);

    [[nodiscard]] const assets::layout::PaneDefinition *pane_definition(std::size_t index) const;
    [[nodiscard]] const RuntimePaneState *pane_state(std::size_t index) const;

    void reset_pose();
    void rebuild_pose();
    void apply_animation(const assets::layout::BrlanAnimation &animation, float frame);

    void apply_picture(
        std::size_t pane_index,
        const TransformContext &parent,
        render::layout::LayoutDrawList *draw_list) const;

    void apply_text(
        std::size_t pane_index,
        const TransformContext &parent,
        render::layout::LayoutDrawList *draw_list,
        const std::unordered_map<std::string, assets::layout::BrfntFont> &fonts_by_name) const;

    void append_pane_recursive(
        std::size_t pane_index,
        const TransformContext &parent,
        render::layout::LayoutDrawList *draw_list,
        const std::unordered_map<std::string, assets::layout::BrfntFont> &fonts_by_name) const;

    [[nodiscard]] const assets::layout::tpl::DecodedImage *resolve_texture_for_material(std::int32_t material_index) const;

    const TitleLayoutResource *_resource {};

    bool _alive {};
    std::array<AnimationSlot, 2> _animation_layers {};

    std::vector<RuntimePaneState> _base_states {};
    std::vector<RuntimePaneState> _current_states {};
    std::unordered_map<std::string, std::size_t> _pane_index_by_name {};
};

}  // namespace smgpc::game::title
