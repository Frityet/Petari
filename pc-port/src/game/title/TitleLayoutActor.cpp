#include "TitleLayoutActor.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include "layout/Binary.hpp"

namespace smgpc::game::title {
namespace {

[[nodiscard]] std::string base_without_extension(std::string text) {
    const auto slash = text.find_last_of("/\\");
    if (slash != std::string::npos) {
        text = text.substr(slash + 1U);
    }

    const auto dot = text.find_last_of('.');
    if (dot != std::string::npos) {
        text = text.substr(0U, dot);
    }

    return text;
}

}  // namespace

TitleLayoutActor::TitleLayoutActor(const TitleLayoutResource *resource)
    : _resource(resource) {
    if (_resource == nullptr) {
        return;
    }

    _base_states.reserve(_resource->layout.panes.size());
    _current_states.reserve(_resource->layout.panes.size());

    for (std::size_t pane_index = 0; pane_index < _resource->layout.panes.size(); ++pane_index) {
        const auto &pane = _resource->layout.panes[pane_index];

        RuntimePaneState state {};
        state.visible = pane.visible;
        state.alpha = pane.alpha;
        state.tx = pane.translate.x;
        state.ty = pane.translate.y;
        state.tz = pane.translate.z;
        state.sx = pane.scale.x;
        state.sy = pane.scale.y;
        state.width = pane.size.x;
        state.height = pane.size.y;

        for (std::size_t i = 0; i < pane.vertex_colors.size(); ++i) {
            const auto &color = pane.vertex_colors[i];
            state.vertex_color[i * 4U + 0U] = color.r;
            state.vertex_color[i * 4U + 1U] = color.g;
            state.vertex_color[i * 4U + 2U] = color.b;
            state.vertex_color[i * 4U + 3U] = color.a;
        }

        _base_states.push_back(state);
        _current_states.push_back(state);

        _pane_index_by_name.emplace(normalize_name(pane.name), pane_index);
    }

    _alive = false;
}

std::string TitleLayoutActor::normalize_name(std::string name) {
    return assets::layout::binary::to_lower_ascii(std::move(name));
}

float TitleLayoutActor::anchor_offset_x(std::uint8_t base_position, float width) {
    switch (base_position % 3U) {
    case 0U:
        return 0.0F;
    case 1U:
        return -width * 0.5F;
    case 2U:
        return -width;
    default:
        return 0.0F;
    }
}

float TitleLayoutActor::anchor_offset_y(std::uint8_t base_position, float height) {
    const auto row = static_cast<std::uint8_t>((base_position / 3U) % 3U);
    switch (row) {
    case 0U:
        return 0.0F;
    case 1U:
        return -height * 0.5F;
    case 2U:
        return -height;
    default:
        return 0.0F;
    }
}

std::uint8_t TitleLayoutActor::clamp_u8(float value) {
    if (value <= 0.0F) {
        return 0U;
    }
    if (value >= 255.0F) {
        return 255U;
    }
    return static_cast<std::uint8_t>(std::lround(value));
}

const assets::layout::PaneDefinition *TitleLayoutActor::pane_definition(std::size_t index) const {
    if (_resource == nullptr || index >= _resource->layout.panes.size()) {
        return nullptr;
    }
    return &_resource->layout.panes[index];
}

const TitleLayoutActor::RuntimePaneState *TitleLayoutActor::pane_state(std::size_t index) const {
    if (index >= _current_states.size()) {
        return nullptr;
    }
    return &_current_states[index];
}

void TitleLayoutActor::reset_pose() {
    _current_states = _base_states;
}

void TitleLayoutActor::rebuild_pose() {
    reset_pose();

    for (const auto &slot : _animation_layers) {
        if (slot.animation == nullptr) {
            continue;
        }

        apply_animation(*slot.animation, slot.frame);
    }
}

void TitleLayoutActor::apply_animation(const assets::layout::BrlanAnimation &animation, float frame) {
    const auto sampled_frame = animation.normalize_frame(frame);

    for (const auto &track : animation.tracks) {
        const auto pane_found = _pane_index_by_name.find(normalize_name(track.pane_name));
        if (pane_found == _pane_index_by_name.end()) {
            continue;
        }

        const auto pane_index = pane_found->second;
        if (pane_index >= _current_states.size()) {
            continue;
        }

        auto &state = _current_states[pane_index];
        const auto value = track.sample(sampled_frame);

        if (track.kind == "RLPA") {
            switch (track.target) {
            case 0U:
                state.tx = value;
                break;
            case 1U:
                state.ty = value;
                break;
            case 2U:
                state.tz = value;
                break;
            case 6U:
                state.sx = value;
                break;
            case 7U:
                state.sy = value;
                break;
            default:
                break;
            }
            continue;
        }

        if (track.kind == "RLVC") {
            if (track.target == 16U) {
                state.alpha = clamp_u8(value);
                continue;
            }

            if (track.target < 16U) {
                state.vertex_color[track.target] = clamp_u8(value);
            }
            continue;
        }

        if (track.kind == "RLMC") {
            if (track.target == 3U || track.target == 7U || track.target == 11U || track.target == 15U) {
                state.alpha = clamp_u8(value);
                continue;
            }

            const auto component = static_cast<std::size_t>(track.target % 4U);
            for (std::size_t vertex = 0; vertex < 4U; ++vertex) {
                state.vertex_color[vertex * 4U + component] = clamp_u8(value);
            }
            continue;
        }

        if (track.kind == "RLTS") {
            switch (track.target) {
            case 0U:
                state.tex_offset_u = value;
                break;
            case 1U:
                state.tex_offset_v = value;
                break;
            case 3U:
                state.tex_scale_u = value;
                break;
            case 4U:
                state.tex_scale_v = value;
                break;
            default:
                break;
            }
            continue;
        }

        if (track.kind == "RLVI") {
            if (track.target == 0U) {
                state.visible = value > 0.5F;
            }
            continue;
        }
    }
}

void TitleLayoutActor::appear() {
    _alive = true;
    for (auto &slot : _animation_layers) {
        slot = AnimationSlot {};
    }
    reset_pose();
}

void TitleLayoutActor::kill() {
    _alive = false;
    for (auto &slot : _animation_layers) {
        slot = AnimationSlot {};
    }
    reset_pose();
}

bool TitleLayoutActor::is_dead() const {
    return not _alive;
}

void TitleLayoutActor::start_anim(const char *animation_name, std::uint32_t layer) {
    if (_resource == nullptr || animation_name == nullptr || layer >= _animation_layers.size()) {
        return;
    }

    const auto key = normalize_name(animation_name);
    const auto found = _resource->animations_by_name.find(key);
    if (found == _resource->animations_by_name.end()) {
        return;
    }

    _animation_layers[layer].animation = &found->second;
    _animation_layers[layer].frame = 0.0F;
    _animation_layers[layer].paused = false;

    rebuild_pose();
}

bool TitleLayoutActor::is_anim_stopped(std::uint32_t layer) const {
    if (layer >= _animation_layers.size()) {
        return true;
    }

    const auto &slot = _animation_layers[layer];
    if (slot.animation == nullptr) {
        return true;
    }
    if (slot.animation->loop) {
        return false;
    }

    return slot.frame >= static_cast<float>(slot.animation->frame_size);
}

void TitleLayoutActor::set_anim_frame_and_stop(float frame, std::uint32_t layer) {
    if (layer >= _animation_layers.size()) {
        return;
    }

    auto &slot = _animation_layers[layer];
    if (slot.animation == nullptr) {
        return;
    }

    slot.frame = frame;
    slot.paused = true;

    rebuild_pose();
}

void TitleLayoutActor::update(float delta_frames) {
    if (not _alive) {
        return;
    }

    if (delta_frames < 0.0F) {
        delta_frames = 0.0F;
    }

    for (auto &slot : _animation_layers) {
        if (slot.animation == nullptr || slot.paused || delta_frames <= 0.0F) {
            continue;
        }

        slot.frame += delta_frames;
        if (not slot.animation->loop) {
            const auto end_frame = static_cast<float>(slot.animation->frame_size);
            if (slot.frame > end_frame) {
                slot.frame = end_frame;
            }
        }
    }

    rebuild_pose();
}

void TitleLayoutActor::emit_effect(const char *) {
}

void TitleLayoutActor::delete_effect_all() {
}

const assets::layout::tpl::DecodedImage *TitleLayoutActor::resolve_texture_for_material(std::int32_t material_index) const {
    if (_resource == nullptr) {
        return nullptr;
    }

    if (material_index < 0 || static_cast<std::size_t>(material_index) >= _resource->layout.materials.size()) {
        return nullptr;
    }

    const auto &material = _resource->layout.materials[static_cast<std::size_t>(material_index)];
    if (material.texture_index < 0 || static_cast<std::size_t>(material.texture_index) >= _resource->layout.texture_names.size()) {
        return nullptr;
    }

    const auto texture_name = base_without_extension(_resource->layout.texture_names[static_cast<std::size_t>(material.texture_index)]);
    const auto found = _resource->textures_by_name.find(normalize_name(texture_name));
    if (found == _resource->textures_by_name.end()) {
        return nullptr;
    }

    return &found->second;
}

void TitleLayoutActor::apply_picture(
    std::size_t pane_index,
    const TransformContext &parent,
    render::layout::LayoutDrawList *draw_list) const {
    if (_resource == nullptr || draw_list == nullptr) {
        return;
    }

    const auto *pane = pane_definition(pane_index);
    const auto *state = pane_state(pane_index);
    if (pane == nullptr || state == nullptr) {
        return;
    }

    const float world_scale_x = parent.scale_x * state->sx;
    const float world_scale_y = parent.scale_y * state->sy;

    const float pane_origin_x = parent.origin_x + state->tx * parent.scale_x;
    const float pane_origin_y = parent.origin_y - state->ty * parent.scale_y;

    const float draw_x = pane_origin_x + anchor_offset_x(pane->base_position, state->width) * world_scale_x;
    const float draw_y = pane_origin_y + anchor_offset_y(pane->base_position, state->height) * world_scale_y;
    const float draw_w = state->width * world_scale_x;
    const float draw_h = state->height * world_scale_y;

    if (std::fabs(draw_w) < 0.00001F || std::fabs(draw_h) < 0.00001F) {
        return;
    }

    const float alpha = parent.alpha * (static_cast<float>(state->alpha) / 255.0F);

    const auto *material = (pane->material_index >= 0 && static_cast<std::size_t>(pane->material_index) < _resource->layout.materials.size())
        ? &_resource->layout.materials[static_cast<std::size_t>(pane->material_index)]
        : nullptr;

    const auto *texture = resolve_texture_for_material(pane->material_index);

    const auto color_for_vertex = [&](std::size_t vertex_index) {
        const auto base = vertex_index * 4U;
        const auto source_r = static_cast<float>(state->vertex_color[base + 0U]) / 255.0F;
        const auto source_g = static_cast<float>(state->vertex_color[base + 1U]) / 255.0F;
        const auto source_b = static_cast<float>(state->vertex_color[base + 2U]) / 255.0F;
        const auto source_a = static_cast<float>(state->vertex_color[base + 3U]) / 255.0F;

        const auto mat_r = material != nullptr ? static_cast<float>(material->mat_color[0U]) / 255.0F : 1.0F;
        const auto mat_g = material != nullptr ? static_cast<float>(material->mat_color[1U]) / 255.0F : 1.0F;
        const auto mat_b = material != nullptr ? static_cast<float>(material->mat_color[2U]) / 255.0F : 1.0F;
        const auto mat_a = material != nullptr ? static_cast<float>(material->mat_color[3U]) / 255.0F : 1.0F;

        return render::layout::pack_abgr(
            clamp_u8(source_r * mat_r * 255.0F),
            clamp_u8(source_g * mat_g * 255.0F),
            clamp_u8(source_b * mat_b * 255.0F),
            clamp_u8(source_a * mat_a * alpha * 255.0F));
    };

    const auto u0 = state->tex_offset_u + pane->tex_coords[0U] * state->tex_scale_u;
    const auto v0 = state->tex_offset_v + pane->tex_coords[1U] * state->tex_scale_v;
    const auto u1 = state->tex_offset_u + pane->tex_coords[6U] * state->tex_scale_u;
    const auto v1 = state->tex_offset_v + pane->tex_coords[7U] * state->tex_scale_v;

    draw_list->push_quad(render::layout::QuadCommand {
        .x0 = draw_x,
        .y0 = draw_y,
        .x1 = draw_x + draw_w,
        .y1 = draw_y + draw_h,
        .u0 = u0,
        .v0 = v0,
        .u1 = u1,
        .v1 = v1,
        .color_tl = color_for_vertex(0U),
        .color_tr = color_for_vertex(1U),
        .color_bl = color_for_vertex(2U),
        .color_br = color_for_vertex(3U),
        .texture = render::layout::TextureRef {
            .id = texture != nullptr ? static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(texture)) : 0U,
            .rgba8 = texture != nullptr && not texture->rgba8.empty() ? texture->rgba8.data() : nullptr,
            .width = texture != nullptr ? texture->width : static_cast<std::uint16_t>(0U),
            .height = texture != nullptr ? texture->height : static_cast<std::uint16_t>(0U),
        },
    });
}

void TitleLayoutActor::apply_text(
    std::size_t pane_index,
    const TransformContext &parent,
    render::layout::LayoutDrawList *draw_list,
    const std::unordered_map<std::string, assets::layout::BrfntFont> &fonts_by_name) const {
    if (_resource == nullptr || draw_list == nullptr) {
        return;
    }

    const auto *pane = pane_definition(pane_index);
    const auto *state = pane_state(pane_index);
    if (pane == nullptr || state == nullptr) {
        return;
    }

    if (pane->font_index < 0 || static_cast<std::size_t>(pane->font_index) >= _resource->layout.font_names.size()) {
        return;
    }

    const auto font_name_key = normalize_name(base_without_extension(_resource->layout.font_names[static_cast<std::size_t>(pane->font_index)]));
    const auto font_found = fonts_by_name.find(font_name_key);
    if (font_found == fonts_by_name.end()) {
        return;
    }

    const auto &font = font_found->second;
    if (font.sheets().empty() || font.cell_width() == 0U || font.cell_height() == 0U) {
        return;
    }

    const float world_scale_x = parent.scale_x * state->sx;
    const float world_scale_y = parent.scale_y * state->sy;

    const float pane_origin_x = parent.origin_x + state->tx * parent.scale_x;
    const float pane_origin_y = parent.origin_y - state->ty * parent.scale_y;

    const float draw_x = pane_origin_x + anchor_offset_x(pane->base_position, state->width) * world_scale_x;
    const float draw_y = pane_origin_y + anchor_offset_y(pane->base_position, state->height) * world_scale_y;

    const float alpha = parent.alpha * (static_cast<float>(state->alpha) / 255.0F);

    const float text_scale_x = pane->text_font_size.x > 0.0F
        ? pane->text_font_size.x / static_cast<float>(font.cell_width())
        : 1.0F;
    const float text_scale_y = pane->text_font_size.y > 0.0F
        ? pane->text_font_size.y / static_cast<float>(font.cell_height())
        : 1.0F;

    auto pen_x = draw_x;
    auto pen_y = draw_y;

    const auto top_color = pane->text_colors[0U];
    const auto packed_color = render::layout::pack_abgr(
        clamp_u8(static_cast<float>(top_color.r)),
        clamp_u8(static_cast<float>(top_color.g)),
        clamp_u8(static_cast<float>(top_color.b)),
        clamp_u8(static_cast<float>(top_color.a) * alpha));

    for (const auto code_unit : pane->text) {
        if (code_unit == u'\n') {
            pen_x = draw_x;
            pen_y += static_cast<float>(font.line_feed()) * text_scale_y * world_scale_y + pane->text_line_space * world_scale_y;
            continue;
        }

        assets::layout::BrfntGlyph glyph {};
        if (not font.get_glyph(static_cast<std::uint16_t>(code_unit), &glyph)) {
            continue;
        }

        if (glyph.sheet_index >= font.sheets().size()) {
            continue;
        }

        const auto &sheet = font.sheets()[glyph.sheet_index];
        if (sheet.empty()) {
            continue;
        }

        const auto glyph_width = std::max(1, static_cast<int>(glyph.widths.glyph_width));
        const auto glyph_height = std::max(1, static_cast<int>(font.cell_height()));

        const auto quad_x0 = pen_x + static_cast<float>(glyph.widths.left) * text_scale_x * world_scale_x;
        const auto quad_y0 = pen_y;
        const auto quad_x1 = quad_x0 + static_cast<float>(glyph_width) * text_scale_x * world_scale_x;
        const auto quad_y1 = quad_y0 + static_cast<float>(glyph_height) * text_scale_y * world_scale_y;

        const auto u0 = static_cast<float>(glyph.cell_x) / static_cast<float>(sheet.width);
        const auto v0 = static_cast<float>(glyph.cell_y) / static_cast<float>(sheet.height);
        const auto u1 = static_cast<float>(glyph.cell_x + font.cell_width()) / static_cast<float>(sheet.width);
        const auto v1 = static_cast<float>(glyph.cell_y + font.cell_height()) / static_cast<float>(sheet.height);

        draw_list->push_quad(render::layout::QuadCommand {
            .x0 = quad_x0,
            .y0 = quad_y0,
            .x1 = quad_x1,
            .y1 = quad_y1,
            .u0 = u0,
            .v0 = v0,
            .u1 = u1,
            .v1 = v1,
            .color_tl = packed_color,
            .color_tr = packed_color,
            .color_bl = packed_color,
            .color_br = packed_color,
            .texture = render::layout::TextureRef {
                .id = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(&sheet)),
                .rgba8 = sheet.rgba8.data(),
                .width = sheet.width,
                .height = sheet.height,
            },
        });

        pen_x += (static_cast<float>(glyph.widths.char_width) + pane->text_char_space) * text_scale_x * world_scale_x;
    }
}

void TitleLayoutActor::append_pane_recursive(
    std::size_t pane_index,
    const TransformContext &parent,
    render::layout::LayoutDrawList *draw_list,
    const std::unordered_map<std::string, assets::layout::BrfntFont> &fonts_by_name) const {
    if (_resource == nullptr || draw_list == nullptr || pane_index >= _resource->layout.panes.size() || pane_index >= _current_states.size()) {
        return;
    }

    const auto &pane = _resource->layout.panes[pane_index];
    const auto &state = _current_states[pane_index];
    if (not parent.visible || not state.visible) {
        return;
    }

    if (pane.type == assets::layout::PaneType::Picture) {
        apply_picture(pane_index, parent, draw_list);
    } else if (pane.type == assets::layout::PaneType::Text) {
        apply_text(pane_index, parent, draw_list, fonts_by_name);
    }

    const float world_scale_x = parent.scale_x * state.sx;
    const float world_scale_y = parent.scale_y * state.sy;

    const float pane_origin_x = parent.origin_x + state.tx * parent.scale_x;
    const float pane_origin_y = parent.origin_y - state.ty * parent.scale_y;

    const TransformContext child {
        .origin_x = pane_origin_x,
        .origin_y = pane_origin_y,
        .scale_x = world_scale_x,
        .scale_y = world_scale_y,
        .alpha = parent.alpha * (static_cast<float>(state.alpha) / 255.0F),
        .visible = true,
    };

    for (const auto child_index : pane.children) {
        if (child_index < 0) {
            continue;
        }
        append_pane_recursive(static_cast<std::size_t>(child_index), child, draw_list, fonts_by_name);
    }
}

void TitleLayoutActor::append_draw_commands(
    render::layout::LayoutDrawList *draw_list,
    const std::unordered_map<std::string, assets::layout::BrfntFont> &fonts_by_name) const {
    if (not _alive || _resource == nullptr || draw_list == nullptr) {
        return;
    }

    const auto root_index = _resource->layout.root_pane;
    if (root_index < 0 || static_cast<std::size_t>(root_index) >= _resource->layout.panes.size()) {
        return;
    }

    const TransformContext root {
        .origin_x = _resource->layout.center_origin ? (_resource->layout.size.x * 0.5F) : 0.0F,
        .origin_y = _resource->layout.center_origin ? (_resource->layout.size.y * 0.5F) : 0.0F,
        .scale_x = 1.0F,
        .scale_y = 1.0F,
        .alpha = 1.0F,
        .visible = true,
    };

    append_pane_recursive(static_cast<std::size_t>(root_index), root, draw_list, fonts_by_name);
}

}  // namespace smgpc::game::title
