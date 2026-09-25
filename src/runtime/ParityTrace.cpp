#include "render/GXState.hpp"
#include "camera/CameraDirectorRuntime.hpp"
#include "runtime/ParityTrace.hpp"

#ifndef NDEBUG

#include <array>
#include <filesystem>
#include <span>
#include <string>
#include <utility>

#include <revolution.h>

#include "DumpJson.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "TraceStore.hpp"
#include "runtime/RuntimeContext.hpp"
#include "resource/TextEncoding.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/ImageEffectSystemHolder.hpp"
#include "Game/Screen/ImageEffectDirector.hpp"
#include "Game/Screen/ImageEffectState.hpp"
#include "Game/Screen/ImageEffectBase.hpp"

namespace smgpc::runtime {
    namespace {

        using dump::Json;

        [[nodiscard]] const char *entry_kind_name(SceneEntryKind kind) {
            switch (kind) {
            case SceneEntryKind::NameObj:
                return "NameObj";
            case SceneEntryKind::Layout:
                return "Layout";
            case SceneEntryKind::LayoutActor:
                return "LayoutActor";
            case SceneEntryKind::LiveActorModel:
                return "LiveActorModel";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *phase_name(SceneSchedulerPhase phase) {
            switch (phase) {
            case SceneSchedulerPhase::None:
                return "None";
            case SceneSchedulerPhase::Movement:
                return "Movement";
            case SceneSchedulerPhase::CalcAnim:
                return "CalcAnim";
            case SceneSchedulerPhase::CalcViewAndEntry:
                return "CalcViewAndEntry";
            case SceneSchedulerPhase::DrawBufferOpa:
                return "DrawBufferOpa";
            case SceneSchedulerPhase::DrawBufferXlu:
                return "DrawBufferXlu";
            case SceneSchedulerPhase::DrawType:
                return "DrawType";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *draw_buffer_pass_name(SceneDrawBufferPass pass) {
            switch (pass) {
            case SceneDrawBufferPass::None:
                return "None";
            case SceneDrawBufferPass::Opaque:
                return "Opaque";
            case SceneDrawBufferPass::Translucent:
                return "Translucent";
            }

            return "Unknown";
        }


        [[nodiscard]] const char *rumble_request_kind_name(RumbleRequestKind kind) {
            switch (kind) {
            case RumbleRequestKind::Named:
                return "Named";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *wipe_event_kind_name(WipeEventKind kind) {
            switch (kind) {
            case WipeEventKind::Open:
                return "Open";
            case WipeEventKind::Close:
                return "Close";
            case WipeEventKind::ForceOpen:
                return "ForceOpen";
            case WipeEventKind::ForceClose:
                return "ForceClose";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *wipe_state_name(WipeState state) {
            switch (state) {
            case WipeState::Open:
                return "Open";
            case WipeState::Closed:
                return "Closed";
            case WipeState::Opening:
                return "Opening";
            case WipeState::Closing:
                return "Closing";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *sequence_request_kind_name(SequenceRequestKind kind) {
            switch (kind) {
            case SequenceRequestKind::ChangeStageInGameAfterLoadingGameData:
                return "ChangeStageInGameAfterLoadingGameData";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *sequence_request_semantic_name(SequenceRequestKind kind) {
            switch (kind) {
            case SequenceRequestKind::ChangeStageInGameAfterLoadingGameData:
                return "change_stage_in_game_after_loading_game_data";
            }

            return "unknown_sequence_request";
        }

        [[nodiscard]] const char *cull_mode_name(render::CullMode mode) {
            switch (mode) {
            case render::CullMode::None:
                return "None";
            case render::CullMode::Front:
                return "Front";
            case render::CullMode::Back:
                return "Back";
            case render::CullMode::FrontAndBack:
                return "FrontAndBack";
            }

            return "Unknown";
        }

        [[nodiscard]] Json vec3_json(const smgpc::camera::CameraParamVec3 &value) {
            return Json{{"x", value.x}, {"y", value.y}, {"z", value.z}};
        }

        [[nodiscard]] Json camera_pose_json(const smgpc::camera::CameraPose &pose) {
            return Json{
                {"eye", vec3_json(pose.eye)},
                {"watch", vec3_json(pose.watch)},
                {"up", vec3_json(pose.up)},
                {"fovy_degrees", pose.fovy_degrees},
                {"aspect_ratio", pose.aspect_ratio},
                {"near_clip", pose.near_clip},
                {"far_clip", pose.far_clip},
                {"projection_offset_x", pose.projection_offset_x},
                {"projection_offset_y", pose.projection_offset_y},
            };
        }

        [[nodiscard]] Json color_json(const smgpc::render::GXColorValue &color) {
            return Json::array({color[0U], color[1U], color[2U], color[3U]});
        }

        [[nodiscard]] Json float3_json(const std::array<float, 3U> &values) {
            return Json::array({values[0U], values[1U], values[2U]});
        }

        [[nodiscard]] Json float12_json(const std::array<float, 12U> &values) {
            auto out = Json::array();
            for (const auto value : values) {
                out.push_back(value);
            }
            return out;
        }

        [[nodiscard]] Json u8_array_json(std::span<const std::uint8_t> values) {
            auto out = Json::array();
            for (const auto value : values) {
                out.push_back(static_cast<int>(value));
            }
            return out;
        }

        [[nodiscard]] Json copy_sample_pattern_json(const std::array<std::array<std::uint8_t, 2U>, 12U> &pattern) {
            auto out = Json::array();
            for (const auto &sample : pattern) {
                out.push_back(Json::array({sample[0U], sample[1U]}));
            }
            return out;
        }

        [[nodiscard]] Json wpad_button_names_json(std::uint32_t mask) {
            constexpr auto buttons = std::array{
                std::pair{WPAD_BUTTON_A, "A"},
                std::pair{WPAD_BUTTON_B, "B"},
                std::pair{WPAD_BUTTON_UP, "UP"},
                std::pair{WPAD_BUTTON_DOWN, "DOWN"},
                std::pair{WPAD_BUTTON_LEFT, "LEFT"},
                std::pair{WPAD_BUTTON_RIGHT, "RIGHT"},
                std::pair{WPAD_BUTTON_PLUS, "PLUS"},
                std::pair{WPAD_BUTTON_MINUS, "MINUS"},
                std::pair{WPAD_BUTTON_HOME, "HOME"},
                std::pair{WPAD_BUTTON_C, "C"},
                std::pair{WPAD_BUTTON_Z, "Z"},
                std::pair{WPAD_BUTTON_1, "ONE"},
                std::pair{WPAD_BUTTON_2, "TWO"},
            };

            auto out = Json::array();
            for (const auto &[button_mask, name] : buttons) {
                if ((mask & button_mask) != 0U) {
                    out.push_back(name);
                }
            }
            return out;
        }

        [[nodiscard]] bool pointer_on_screen(float x, float y, bool valid, const render::FrameContext &frame_context) {
            return valid && x >= 0.0F && y >= 0.0F && x <= static_cast<float>(frame_context.framebuffer.width) &&
                   y <= static_cast<float>(frame_context.framebuffer.height);
        }

        [[nodiscard]] Json input_pointer_json(const render::InputPointerState &pointer, const render::FrameContext &frame_context) {
            return Json{
                {"x", pointer.x},
                {"y", pointer.y},
                {"valid", pointer.valid},
                {"on_screen", pointer_on_screen(pointer.x, pointer.y, pointer.valid, frame_context)},
            };
        }

        [[nodiscard]] Json wpad_pointer_json(const WpadPointerState &pointer, const render::FrameContext &frame_context) {
            return Json{
                {"x", pointer.x},
                {"y", pointer.y},
                {"valid", pointer.valid},
                {"on_screen", pointer_on_screen(pointer.x, pointer.y, pointer.valid, frame_context)},
            };
        }

        [[nodiscard]] Json host_input_json(const RuntimeContext::HostInputTraceState &input, const render::FrameContext &frame_context) {
            return Json{
                {"frame_index", input.frame_index},
                {"raw_hold_mask", input.raw_hold_mask},
                {"effective_hold_mask", input.effective_hold_mask},
                {"raw_buttons", wpad_button_names_json(input.raw_hold_mask)},
                {"effective_buttons", wpad_button_names_json(input.effective_hold_mask)},
                {"raw_pointer", input_pointer_json(input.raw_pointer, frame_context)},
                {"effective_pointer", input_pointer_json(input.effective_pointer, frame_context)},
                {"debug_button_script_applied", input.debug_button_script_applied},
                {"debug_pointer_script_applied", input.debug_pointer_script_applied},
            };
        }

        [[nodiscard]] Json wpad_channel_json(const WpadService &wpad, s32 channel, const render::FrameContext &frame_context) {
            const auto *state = wpad.channel_state(channel);
            if (state == nullptr) {
                return Json{{"connected", false}};
            }

            return Json{
                {"connected", state->connected},
                {"hold_mask", state->hold},
                {"trigger_mask", state->trigger},
                {"release_mask", state->release},
                {"repeat_mask", state->repeat},
                {"hold_frame_count", state->hold_frame_count},
                {"held_buttons", wpad_button_names_json(state->hold)},
                {"triggered_buttons", wpad_button_names_json(state->trigger)},
                {"released_buttons", wpad_button_names_json(state->release)},
                {"repeated_buttons", wpad_button_names_json(state->repeat)},
                {"pointer", wpad_pointer_json(state->pointer, frame_context)},
                {"previous_pointer", wpad_pointer_json(wpad.past_pointer(channel, 1U), frame_context)},
                {"pointer_history_count", state->pointer_history_count},
                {"distance_to_display", state->distance_to_display},
                {"button_a_held", wpad.is_button_held(channel, WPAD_BUTTON_A)},
                {"button_b_held", wpad.is_button_held(channel, WPAD_BUTTON_B)},
                {"button_a_triggered", wpad.is_button_triggered(channel, WPAD_BUTTON_A)},
                {"button_b_triggered", wpad.is_button_triggered(channel, WPAD_BUTTON_B)},
            };
        }

        [[nodiscard]] const char *copy_event_kind_name(render::CopyEventKind kind) {
            switch (kind) {
            case render::CopyEventKind::Texture:
                return "texture";
            case render::CopyEventKind::Xfb:
                return "xfb";
            case render::CopyEventKind::Present:
                return "present";
            }

            return "unknown";
        }

        [[nodiscard]] render::CopyRect frame_rect(std::uint16_t width, std::uint16_t height) {
            return render::CopyRect{
                .left = 0,
                .top = 0,
                .right = width,
                .bottom = height,
                .width = width,
                .height = height,
            };
        }

        [[nodiscard]] render::CopyViewport copy_viewport_from_frame(const render::FrameContext &frame_context) {
            return render::CopyViewport{
                .left = 0.0F,
                .right = static_cast<float>(frame_context.framebuffer.width),
                .top = 0.0F,
                .bottom = static_cast<float>(frame_context.framebuffer.height),
                .near_depth = 0.0F,
                .far_depth = 1.0F,
            };
        }

        [[nodiscard]] Json copy_rect_json(const render::CopyRect &rect) {
            return Json{
                {"left", rect.left},
                {"top", rect.top},
                {"right", rect.right},
                {"bottom", rect.bottom},
                {"width", rect.width},
                {"height", rect.height},
            };
        }

        [[nodiscard]] Json copy_viewport_json(const render::CopyViewport &viewport) {
            return Json{
                {"left", viewport.left},
                {"right", viewport.right},
                {"top", viewport.top},
                {"bottom", viewport.bottom},
                {"near_depth", viewport.near_depth},
                {"far_depth", viewport.far_depth},
            };
        }

        [[nodiscard]] Json framebuffer_json(const render::FramebufferInfo &framebuffer) {
            return Json{
                {"width", framebuffer.width},
                {"height", framebuffer.height},
            };
        }

        [[nodiscard]] Json frame_viewport_json(const render::FrameContext &frame_context) {
            return copy_viewport_json(copy_viewport_from_frame(frame_context));
        }

        [[nodiscard]] Json frame_scissor_json(const render::FrameContext &frame_context) {
            const auto width = frame_context.framebuffer.width;
            const auto height = frame_context.framebuffer.height;
            return copy_rect_json(frame_rect(width, height));
        }

        [[nodiscard]] Json copy_event_json(const render::CopyEvent &event, std::uint64_t index) {
            return Json{
                {"index", index},
                {"event_index", event.event_index},
                {"presenter_frame_count", event.presenter_frame_count},
                {"kind", copy_event_kind_name(event.kind)},
                {"copy_to_xfb", event.copy_to_xfb},
                {"depth_copy", event.depth_copy},
                {"clear", event.clear},
                {"half_scale", event.half_scale},
                {"scale_invert", event.scale_invert},
                {"clamp_top", event.clamp_top},
                {"clamp_bottom", event.clamp_bottom},
                {"intensity_format", event.intensity_format},
                {"auto_conversion", event.auto_conversion},
                {"clear_color", u8_array_json(event.clear_color)},
                {"clear_depth", event.clear_depth},
                {"copy_filter_aa", event.copy_filter_aa},
                {"copy_filter_vertical", event.copy_filter_vertical},
                {"copy_filter_sample_pattern", copy_sample_pattern_json(event.copy_filter_sample_pattern)},
                {"copy_filter_vfilter", u8_array_json(event.copy_filter_vfilter)},
                {"dest_addr", event.dest_addr},
                {"dest_stride", event.dest_stride},
                {"source_rect", copy_rect_json(event.source_rect)},
                {"output_size", framebuffer_json(event.output_size)},
                {"target_pixel_format", event.target_pixel_format},
                {"real_format", event.real_format},
                {"frame_to_field", event.frame_to_field},
                {"gamma_index", event.gamma_index},
                {"gamma_value", event.gamma_value},
                {"y_scale", event.y_scale},
                {"dispcopyyscale", event.dispcopyyscale},
                {"scissor", copy_rect_json(event.scissor)},
                {"viewport", copy_viewport_json(event.viewport)},
                {"backbuffer", framebuffer_json(event.backbuffer)},
                {"target_rect", copy_rect_json(event.target_rect)},
                {"render_pass", event.render_pass},
                {"view_id", event.view_id},
            };
        }

        [[nodiscard]] Json pc_copy_events_json(const RuntimeContext &runtime) {
            auto out = Json::array();
            for (const auto &event : runtime.copy_events()) {
                out.push_back(copy_event_json(event, out.size()));
            }
            return out;
        }

        [[nodiscard]] Json live_actor_state_json(const SceneSchedulerEntryState &entry) {
            if (!entry.has_live_actor_state) {
                return Json(nullptr);
            }

            return Json{
                {"nerve_step", entry.live_actor_nerve_step},
                {"position", Json::array({entry.live_actor_position[0U], entry.live_actor_position[1U], entry.live_actor_position[2U]})},
                {"rotation", Json::array({entry.live_actor_rotation[0U], entry.live_actor_rotation[1U], entry.live_actor_rotation[2U]})},
                {"scale", Json::array({entry.live_actor_scale[0U], entry.live_actor_scale[1U], entry.live_actor_scale[2U]})},
                {"base_matrix", float12_json(entry.live_actor_base_matrix)},
                {"bck", entry.live_actor_bck_name},
                {"brk", entry.live_actor_brk_name},
                {"btk", entry.live_actor_btk_name},
            };
        }

        [[nodiscard]] Json scene_entry_json(const SceneSchedulerEntryState &entry, std::size_t index) {
            return Json{
                {"index", index},
                {"kind", entry_kind_name(entry.kind)},
                {"phase", phase_name(entry.phase)},
                {"name", entry.name},
                {"movement_type", entry.movement_type},
                {"calc_anim_type", entry.calc_anim_type},
                {"draw_buffer_type", entry.draw_buffer_type},
                {"draw_type", entry.draw_type},
                {"draw_buffer_pass", draw_buffer_pass_name(entry.draw_buffer_pass)},
                {"order", entry.order},
                {"suspended", entry.suspended},
                {"dead", entry.dead},
                {"live_actor", live_actor_state_json(entry)},
            };
        }

        [[nodiscard]] Json scene_entries_json(std::span<const SceneSchedulerEntryState> entries) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < entries.size(); ++i) {
                out.push_back(scene_entry_json(entries[i], i));
            }
            return out;
        }

        [[nodiscard]] Json layout_animation_json(const SceneLayoutAnimationDebugState &animation) {
            auto out = Json{
                {"layer_index", animation.layer_index},
                {"active", animation.active},
                {"name", animation.name},
            };
            if (animation.active) {
                out["frame"] = animation.frame;
                out["end_frame"] = animation.end_frame;
                out["rate"] = animation.rate;
                out["stopped"] = animation.stopped;
                out["looping"] = animation.looping;
            }
            return out;
        }

        [[nodiscard]] Json layout_animations_json(std::span<const SceneLayoutAnimationDebugState> animations) {
            auto out = Json::array();
            for (const auto &animation : animations) {
                out.push_back(layout_animation_json(animation));
            }
            return out;
        }

        [[nodiscard]] Json layout_pane_control_animation_json(const SceneLayoutPaneControlAnimationDebugState &animation) {
            return Json{
                {"layer_index", animation.layer_index},
                {"name", animation.name},
                {"frame", animation.frame},
                {"end_frame", animation.end_frame},
                {"rate", animation.rate},
                {"stopped", animation.stopped},
                {"looping", animation.looping},
            };
        }

        [[nodiscard]] Json layout_pane_control_animations_json(std::span<const SceneLayoutPaneControlAnimationDebugState> animations) {
            auto out = Json::array();
            for (const auto &animation : animations) {
                out.push_back(layout_pane_control_animation_json(animation));
            }
            return out;
        }

        [[nodiscard]] Json layout_pane_control_json(const SceneLayoutPaneControlDebugState &pane) {
            return Json{
                {"pane_name", pane.pane_name},
                {"exists_in_layout", pane.exists_in_layout},
                {"visible", pane.visible},
                {"animations", layout_pane_control_animations_json(pane.animations)},
            };
        }

        [[nodiscard]] Json layout_pane_controls_json(std::span<const SceneLayoutPaneControlDebugState> panes) {
            auto out = Json::array();
            for (const auto &pane : panes) {
                out.push_back(layout_pane_control_json(pane));
            }
            return out;
        }

        [[nodiscard]] Json layout_button_controller_json(const SceneLayoutButtonControllerDebugState &button) {
            return Json{
                {"pane_name", button.pane_name},
                {"bounding_pane_name", button.bounding_pane_name},
                {"nerve", button.nerve},
                {"anim_layer", button.anim_layer},
                {"active", button.active},
                {"selected", button.selected},
                {"pointing", button.pointing},
                {"appearance_enabled", button.appearance_enabled},
                {"decide_enabled", button.decide_enabled},
                {"pointing_anim_start_frame", button.pointing_anim_start_frame},
            };
        }

        [[nodiscard]] Json layout_button_controllers_json(std::span<const SceneLayoutButtonControllerDebugState> buttons) {
            auto out = Json::array();
            for (const auto &button : buttons) {
                out.push_back(layout_button_controller_json(button));
            }
            return out;
        }

        [[nodiscard]] Json layout_pane_content_json(const SceneLayoutPaneContentDebugState &content) {
            return Json{
                {"kind", content.kind},
                {"name", content.name},
                {"material_index", content.material_index},
                {"material_name", content.material_name},
                {"texture_name", content.texture_name},
                {"font_name", content.font_name},
                {"visible", content.visible},
            };
        }

        [[nodiscard]] Json layout_pane_contents_json(std::span<const SceneLayoutPaneContentDebugState> contents) {
            auto out = Json::array();
            for (const auto &content : contents) {
                out.push_back(layout_pane_content_json(content));
            }
            return out;
        }

        [[nodiscard]] Json layout_pane_runtime_json(const SceneLayoutPaneRuntimeDebugState &pane) {
            return Json{
                {"index", pane.index},
                {"name", pane.name},
                {"parent_index", pane.parent_index},
                {"base_visible", pane.base_visible},
                {"effective_visible", pane.effective_visible},
                {"translate_x", pane.translate_x},
                {"translate_y", pane.translate_y},
                {"scale_x", pane.scale_x},
                {"scale_y", pane.scale_y},
                {"alpha", pane.alpha},
                {"width", pane.width},
                {"height", pane.height},
                {"contents", layout_pane_contents_json(pane.contents)},
            };
        }

        [[nodiscard]] Json layout_panes_runtime_json(std::span<const SceneLayoutPaneRuntimeDebugState> panes) {
            auto out = Json::array();
            for (const auto &pane : panes) {
                out.push_back(layout_pane_runtime_json(pane));
            }
            return out;
        }

        [[nodiscard]] Json layout_material_texture_json(const SceneLayoutMaterialTextureDebugState &texture) {
            return Json{
                {"slot", texture.slot},
                {"texture_index", texture.texture_index},
                {"texture_name", texture.texture_name},
                {"wrap_s", texture.wrap_s},
                {"wrap_t", texture.wrap_t},
                {"min_filter", texture.min_filter},
                {"mag_filter", texture.mag_filter},
            };
        }

        [[nodiscard]] Json layout_material_textures_json(std::span<const SceneLayoutMaterialTextureDebugState> textures) {
            auto out = Json::array();
            for (const auto &texture : textures) {
                out.push_back(layout_material_texture_json(texture));
            }
            return out;
        }

        [[nodiscard]] Json layout_material_json(const SceneLayoutMaterialDebugState &material) {
            return Json{
                {"index", material.index},
                {"name", material.name},
                {"texture_count", material.texture_count},
                {"tex_coord_gen_count", material.tex_coord_gen_count},
                {"tev_stage_count", material.tev_stage_count},
                {"alpha_compare_enabled", material.alpha_compare_enabled},
                {"blend_enabled", material.blend_enabled},
                {"textures", layout_material_textures_json(material.textures)},
            };
        }

        [[nodiscard]] Json layout_materials_json(std::span<const SceneLayoutMaterialDebugState> materials) {
            auto out = Json::array();
            for (const auto &material : materials) {
                out.push_back(layout_material_json(material));
            }
            return out;
        }

        [[nodiscard]] Json layout_texture_json(const SceneLayoutTextureDebugState &texture) {
            return Json{
                {"index", texture.index},
                {"name", texture.name},
                {"width", texture.width},
                {"height", texture.height},
                {"format_raw", texture.format_raw},
                {"format", texture.format_name},
                {"uploaded", texture.uploaded},
                {"rgba_byte_count", texture.rgba_byte_count},
            };
        }

        [[nodiscard]] Json layout_textures_json(std::span<const SceneLayoutTextureDebugState> textures) {
            auto out = Json::array();
            for (const auto &texture : textures) {
                out.push_back(layout_texture_json(texture));
            }
            return out;
        }

        [[nodiscard]] Json layout_runtime_json(const SceneLayoutRuntimeDebugState &entry, std::size_t index) {
            return Json{
                {"index", index},
                {"name", entry.name},
                {"layout_name", entry.layout_name},
                {"archive_path", entry.has_archive_path ? Json(entry.archive_path) : Json(nullptr)},
                {"movement_type", entry.movement_type},
                {"calc_anim_type", entry.calc_anim_type},
                {"draw_type", entry.draw_type},
                {"order", entry.order},
                {"suspended", entry.suspended},
                {"dead", entry.dead},
                {"pane_count", entry.pane_count},
                {"picture_count", entry.picture_count},
                {"text_box_count", entry.text_box_count},
                {"material_count", entry.material_count},
                {"texture_count", entry.texture_count},
                {"font_count", entry.font_count},
                {"committed_pane_frame_count", entry.committed_pane_frame_count},
                {"animations", layout_animations_json(entry.animations)},
                {"pane_controls", layout_pane_controls_json(entry.pane_controls)},
                {"button_controllers", layout_button_controllers_json(entry.button_controllers)},
                {"panes", layout_panes_runtime_json(entry.panes)},
                {"materials", layout_materials_json(entry.materials)},
                {"textures", layout_textures_json(entry.textures)},
            };
        }

        [[nodiscard]] Json layout_runtime_entries_json(std::span<const SceneLayoutRuntimeDebugState> entries) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < entries.size(); ++i) {
                out.push_back(layout_runtime_json(entries[i], i));
            }
            return out;
        }


        [[nodiscard]] Json wipe_events_json(std::span<const WipeEvent> events) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < events.size(); ++i) {
                const auto &event = events[i];
                out.push_back(Json{
                    {"index", i},
                    {"kind", wipe_event_kind_name(event.kind)},
                    {"name", event.name},
                    {"frame_count", event.frame_count},
                    {"frame_index", event.frame_index},
                });
            }
            return out;
        }

        [[nodiscard]] Json wipe_service_json(const WipeService &wipe) {
            return Json{
                {"state", wipe_state_name(wipe.state())},
                {"name", wipe.current_name()},
                {"active", wipe.is_active()},
                {"blank", wipe.is_blank()},
                {"open", wipe.is_open()},
                {"remaining_frames", wipe.remaining_frames()},
                {"duration_frames", wipe.duration_frames()},
                {"events", wipe_events_json(wipe.events())},
            };
        }

        [[nodiscard]] Json image_effect_state_json() {
            auto* holder = MR::getSceneObjHolder();
            if (!holder || !holder->isExist(SceneObj_ImageEffectSystemHolder)) return Json{{"available", false}};
            auto* system = static_cast<ImageEffectSystemHolder*>(holder->getObj(SceneObj_ImageEffectSystemHolder));
            if (!system->mDirector) return Json{{"available", false}};
            const auto& director = *system->mDirector;
            const char* state = "Unknown";
            if (director.mState == director.mStateNull) state = "None";
            else if (director.mState == director.mStateBloomNormal) state = "BloomNormal";
            else if (director.mState == director.mStateBloomSimple) state = "BloomSimple";
            else if (director.mState == director.mStateScreenBlur) state = "ScreenBlur";
            else if (director.mState == director.mStateDepthOfField) state = "DepthOfField";
            Json effect = nullptr;
            if (const auto* current = director.mCurrentEffect) {
                effect = Json{{"name", smgpc::resource::decode_cp932(current->mName)}, {"requested", current->_C},
                              {"active", current->_D}, {"intensity", current->_10}};
            }
            return Json{{"available", true}, {"control_auto", director.mIsAuto},
                        {"player_sync", director.mIsPlayerSync}, {"player_sync_intensity", director.mPlayerSyncIntensity},
                        {"depth_of_field_intensity", director.mDepthOfFieldIntensity},
                        {"state", state}, {"current_effect", std::move(effect)}};
        }

        [[nodiscard]] Json rumble_request_events_json(std::span<const RumbleRequestEvent> events) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < events.size(); ++i) {
                const auto &event = events[i];
                out.push_back(Json{
                    {"index", i},
                    {"kind", rumble_request_kind_name(event.kind)},
                    {"pattern_name", smgpc::resource::decode_cp932(event.pattern_name)},
                    {"channel", event.channel},
                    {"frame_index", event.frame_index},
                });
            }
            return out;
        }

        [[nodiscard]] Json sequence_request_events_json(std::span<const SequenceRequestEvent> events) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < events.size(); ++i) {
                const auto &event = events[i];
                out.push_back(Json{
                    {"index", i},
                    {"kind", sequence_request_kind_name(event.kind)},
                    {"frame_index", event.frame_index},
                });
            }
            return out;
        }

        [[nodiscard]] Json semantic_trace_event_json(const RuntimeContext::SemanticTraceEvent &event) {
            return Json{
                {"index", event.index},
                {"frame_index", event.frame_index},
                {"category", event.category},
                {"name", event.name},
                {"detail", event.detail},
                {"stage", event.stage_name},
                {"source", "runtime"},
            };
        }

        [[nodiscard]] Json sequence_request_semantic_event_json(const SequenceRequestEvent &event, std::size_t index,
                                                                std::string_view stage_name) {
            return Json{
                {"index", index},
                {"frame_index", event.frame_index},
                {"category", "sequence"},
                {"name", sequence_request_semantic_name(event.kind)},
                {"detail", sequence_request_kind_name(event.kind)},
                {"stage", std::string(stage_name)},
                {"source", "sequence_requests"},
            };
        }

        [[nodiscard]] Json semantic_trace_events_json(const RuntimeContext &runtime) {
            auto out = Json::array();
            for (const auto &event : runtime.semantic_trace_events()) {
                out.push_back(semantic_trace_event_json(event));
            }
            for (const auto &event : runtime.sequence_requests().events()) {
                out.push_back(sequence_request_semantic_event_json(event, out.size(), runtime.current_stage_name()));
            }
            return out;
        }

        [[nodiscard]] Json rfl_miis_json(std::span<const RflMiiEntry> entries) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < entries.size(); ++i) {
                const auto &entry = entries[i];
                out.push_back(Json{
                    {"index", i},
                    {"rfl_index", entry.index},
                    {"source", static_cast<int>(entry.source)},
                    {"name", entry.name},
                    {"creator", entry.creator},
                    {"favorite", entry.favorite},
                });
            }
            return out;
        }

        [[nodiscard]] std::string_view rfl_operation_kind_name(RflOperationKind kind) {
            switch (kind) {
            case RflOperationKind::LoadBegin:
                return "load_begin";
            case RflOperationKind::LoadComplete:
                return "load_complete";
            case RflOperationKind::LoadFailed:
                return "load_failed";
            case RflOperationKind::AdditionalInfo:
                return "additional_info";
            case RflOperationKind::SearchOfficial:
                return "search_official";
            case RflOperationKind::CheckAvailable:
                return "check_available";
            case RflOperationKind::InitResource:
                return "init_resource";
            case RflOperationKind::InitCharModel:
                return "init_char_model";
            case RflOperationKind::MakeIcon:
                return "make_icon";
            case RflOperationKind::MiiSelectPage:
                return "mii_select_page";
            }
            return "unknown";
        }

        [[nodiscard]] Json rfl_trace_json(std::span<const RflOperationTrace> entries) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < entries.size(); ++i) {
                const auto &entry = entries[i];
                out.push_back(Json{
                    {"index", i},
                    {"kind", std::string(rfl_operation_kind_name(entry.kind))},
                    {"frame_index", entry.frame_index},
                    {"path", entry.path},
                    {"source", static_cast<int>(entry.source)},
                    {"rfl_index", entry.index},
                    {"result", static_cast<int>(entry.result)},
                    {"byte_count", entry.byte_count},
                    {"entry_count", entry.entry_count},
                    {"db_present", entry.db_present},
                    {"async_pending", entry.async_pending},
                    {"texture_available", entry.texture_available},
                    {"width", entry.width},
                    {"height", entry.height},
                    {"expression", static_cast<int>(entry.expression)},
                    {"expression_flags", entry.expression_flags},
                    {"page_index", entry.page_index},
                    {"page_count", entry.page_count},
                    {"icon_count", entry.icon_count},
                    {"selected", entry.selected},
                    {"prohibited", entry.prohibited},
                });
            }
            return out;
        }

        [[nodiscard]] Json rfl_db_status_json(const RflDbStatus &status) {
            return Json{
                {"nand_bound", status.nand_bound},
                {"db_present", status.db_present},
                {"async_pending", status.async_pending},
                {"resource_initialized", status.resource_initialized},
                {"deluxe_textures", status.deluxe_textures},
                {"byte_count", status.byte_count},
                {"entry_count", status.entry_count},
                {"loaded_frame", status.loaded_frame},
                {"last_error", static_cast<int>(status.last_error)},
                {"last_reason", status.last_reason},
            };
        }

        [[nodiscard]] Json scene_lights_json(const SceneLightService &lights) {
            auto out = Json::array();
            const auto entries = lights.lights();
            for (auto light_index = std::size_t{}; light_index < entries.size(); ++light_index) {
                const auto &light = entries[light_index];
                if (!light.loaded) {
                    continue;
                }
                out.push_back(Json{
                    {"index", light_index},
                    {"color", color_json(light.color)},
                    {"cosine_attenuation", float3_json(light.cosine_attenuation)},
                    {"distance_attenuation", float3_json(light.distance_attenuation)},
                    {"position", float3_json(light.position)},
                    {"direction", float3_json(light.direction)},
                });
            }
            return out;
        }

        [[nodiscard]] Json dvd_file_read_trace_json(const DvdFileReadTrace &entry, std::size_t index) {
            return Json{
                {"index", index},
                {"requested_path", entry.requested_path},
                {"resolved_path", entry.resolved_path},
                {"byte_count", entry.byte_count},
            };
        }

        [[nodiscard]] Json dvd_file_read_traces_json(std::span<const DvdFileReadTrace> entries) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < entries.size(); ++i) {
                out.push_back(dvd_file_read_trace_json(entries[i], i));
            }
            return out;
        }

        [[nodiscard]] Json dvd_archive_load_trace_json(const DvdArchiveLoadTrace &entry, std::size_t index) {
            return Json{
                {"index", index},
                {"requested_path", entry.requested_path},
                {"resolved_path", entry.resolved_path},
                {"cache_hit", entry.cache_hit},
                {"load_count", entry.load_count},
                {"cached_archive_count", entry.cached_archive_count},
                {"resource_count", entry.resource_count},
            };
        }

        [[nodiscard]] Json dvd_archive_load_traces_json(std::span<const DvdArchiveLoadTrace> entries) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < entries.size(); ++i) {
                out.push_back(dvd_archive_load_trace_json(entries[i], i));
            }
            return out;
        }

        [[nodiscard]] Json runtime_services_json(const RuntimeContext &runtime) {
            const auto* camera = smgpc::camera::current_camera_director_runtime();
            return Json{
                {"dvd",
                 Json{
                     {"root", runtime.dvd().root().generic_string()},
                     {"cached_archive_count", runtime.dvd().cached_archive_count()},
                     {"file_reads", dvd_file_read_traces_json(runtime.dvd().file_read_trace())},
                     {"archive_loads", dvd_archive_load_traces_json(runtime.dvd().archive_load_trace())},
                 }},
                {"rfl",
                 Json{
                     {"initialized", runtime.rfl().is_initialized()},
                     {"error", runtime.rfl().has_error()},
                     {"db_status", rfl_db_status_json(runtime.rfl().db_status())},
                     {"valid_mii_count", runtime.rfl().valid_miis().size()},
                     {"valid_miis", rfl_miis_json(runtime.rfl().valid_miis())},
                     {"trace", rfl_trace_json(runtime.rfl().trace())},
                 }},
                {"save",
                 Json{
                     {"file_count", runtime.save_data().file_count()},
                     {"host_directory",
                      runtime.save_data().host_directory().has_value() ? Json(runtime.save_data().host_directory()->string()) : Json(nullptr)},
                     {"valid_game_data_container", runtime.save_data().has_valid_game_data_container()},
                 }},
                {"scene_lights",
                 Json{
                     {"loaded_mask", runtime.scene_lights().loaded_mask()},
                     {"lights", scene_lights_json(runtime.scene_lights())},
                 }},
                {"wipe",
                 Json{
                     {"scene", wipe_service_json(runtime.scene_wipe())},
                     {"system", wipe_service_json(runtime.system_wipe())},
                 }},
                {"image_effects", image_effect_state_json()},
                {"camera",
                 Json{
                     {"effective_camera_pose", camera != nullptr ? camera_pose_json(camera->pose()) : Json(nullptr)},
                     {"presented_camera_pose",
                      runtime.scene_camera_pose().has_value() ? camera_pose_json(*runtime.scene_camera_pose()) : Json(nullptr)},
                 }},
                {"rumble",
                 Json{
                     {"events", rumble_request_events_json(runtime.rumble().events())},
                 }},
                {"sequence_requests",
                 Json{
                     {"change_stage_in_game_after_loading_game_data",
                      runtime.sequence_requests().is_change_stage_in_game_after_loading_game_data_requested()},
                     {"events", sequence_request_events_json(runtime.sequence_requests().events())},
                 }},
            };
        }

        [[nodiscard]] Json render_texture_binding_json(const RuntimeContext::RenderTextureBindingTrace &texture) {
            return Json{
                {"slot", texture.slot},
                {"texture_index", texture.texture_index},
                {"name", texture.name},
                {"identity_name", texture.name},
                {"width", texture.width},
                {"height", texture.height},
                {"format", texture.format_name},
                {"format_raw", texture.format_raw},
            };
        }

        [[nodiscard]] Json render_texture_bindings_json(std::span<const RuntimeContext::RenderTextureBindingTrace> textures) {
            auto out = Json::array();
            for (const auto &texture : textures) {
                out.push_back(render_texture_binding_json(texture));
            }
            return out;
        }

        [[nodiscard]] std::uint32_t used_textures_mask(std::span<const RuntimeContext::RenderTextureBindingTrace> textures) {
            auto mask = std::uint32_t{};
            for (const auto &texture : textures) {
                if (texture.slot < 8U) {
                    mask |= 1U << texture.slot;
                }
            }
            return mask;
        }

        [[nodiscard]] Json used_texture_slots_json(std::span<const RuntimeContext::RenderTextureBindingTrace> textures) {
            auto slots = std::array<bool, 8U>{};
            for (const auto &texture : textures) {
                if (texture.slot < slots.size()) {
                    slots[texture.slot] = true;
                }
            }

            auto out = Json::array();
            for (auto slot = std::size_t{}; slot < slots.size(); ++slot) {
                if (slots[slot]) {
                    out.push_back(slot);
                }
            }
            return out;
        }

        [[nodiscard]] Json layout_packet_trace_json(const RuntimeContext::LayoutRuntimePacketTrace &packet, std::size_t index) {
            return Json{
                {"index", index},
                {"model_name", packet.layout_name},
                {"layout_name", packet.layout_name},
                {"pane_name", packet.pane_name},
                {"frame_index", packet.frame_index},
                {"draw_pass", "2d_layout"},
                {"render_pass", "2d_layout"},
                {"view_id", 0},
                {"material_name", packet.material_name},
                {"picture_index", packet.picture_index},
                {"material_index", packet.material_index},
                {"packet_mode", "BrlytGxMaterial2D"},
                {"primitive_type", "triangles"},
                {"source_vertex_count", packet.vertex_count},
                {"source_triangle_count", packet.index_count / 3U},
                {"num_indices", packet.index_count},
                {"texgen_count", packet.texgen_count},
                {"color_channel_count", 1U},
                {"active_tev_stage_count", packet.tev_stage_count},
                {"tev_stage_count", packet.tev_stage_count},
                {"indirect_stage_count", 0U},
                {"cull_mode", cull_mode_name(packet.cull_mode)},
                {"used_textures_mask", used_textures_mask(packet.texture_bindings)},
                {"used_texture_slots", used_texture_slots_json(packet.texture_bindings)},
                {"texture_bindings", render_texture_bindings_json(packet.texture_bindings)},
                {"alpha_compare_enabled", packet.alpha_compare_enabled},
                {"blend_enabled", packet.blend_enabled},
            };
        }

        [[nodiscard]] Json runtime_render_packets_json(const RuntimeContext &runtime) {
            auto out = Json::array();
            auto index = std::size_t{};
            for (const auto &packet : runtime.layout_packet_trace()) {
                out.push_back(layout_packet_trace_json(packet, index));
                ++index;
            }

            return out;
        }

    }  // namespace

    dump::Json runtime_parity_trace_json(const render::FrameContext &frame_context, const RuntimeContext &runtime) {
        auto trace = Json{
            {"schema", "smgpc-runtime-parity-trace-v1"},
            {"frame",
             Json{
                 {"index", frame_context.frame_index},
                 {"runtime_index", runtime.frame_index()},
                 {"time_seconds", frame_context.frame_time_seconds},
                 {"delta_seconds", frame_context.frame_delta_seconds},
                 {"framebuffer", Json{{"width", frame_context.framebuffer.width}, {"height", frame_context.framebuffer.height}}},
                 {"viewport", frame_viewport_json(frame_context)},
                 {"scissor", frame_scissor_json(frame_context)},
                 {"has_focus", frame_context.has_focus},
                 {"is_minimized", frame_context.is_minimized},
             }},
            {"camera_pose", runtime.last_camera_pose().has_value() ? camera_pose_json(*runtime.last_camera_pose()) : Json(nullptr)},
            {"host_input", host_input_json(runtime.host_input_trace(), frame_context)},
            {"wpad0", wpad_channel_json(runtime.wpad(), WPAD_CHAN0, frame_context)},
            {"runtime_services", runtime_services_json(runtime)},
            {"scene_snapshot", scene_entries_json(runtime.scheduler().snapshot())},
            {"scene_trace", scene_entries_json(runtime.scheduler().last_execution_trace())},
            {"semantic_events", semantic_trace_events_json(runtime)},
            {"layout_runtime", layout_runtime_entries_json(runtime.scheduler().debug_layout_runtime_snapshot())},
            {"render_packets", runtime_render_packets_json(runtime)},
            {"copy_events", pc_copy_events_json(runtime)},
        };
        return trace;
    }

    void write_runtime_parity_trace(const std::filesystem::path &path, const render::FrameContext &frame_context, const RuntimeContext &runtime) {
        (void)trace::write_trace_sqlite_file(path, runtime_parity_trace_json(frame_context, runtime), "pc-port");
    }

    dump::Json load_runtime_parity_trace(const std::filesystem::path &path) {
        return trace::load_trace_sqlite_file(path);
    }

}  // namespace smgpc::runtime

#endif
