#include "Game/compat/ParityTrace.hpp"

#include <array>
#include <filesystem>
#include <span>
#include <string>

#include <revolution.h>

#include "DumpJson.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/compat/RuntimeContext.hpp"

namespace smgpc::game {
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

        [[nodiscard]] const char *audio_event_kind_name(AudioEventKind kind) {
            switch (kind) {
            case AudioEventKind::StageBgmStart:
                return "StageBgmStart";
            case AudioEventKind::StageBgmUnlock:
                return "StageBgmUnlock";
            case AudioEventKind::StageBgmStop:
                return "StageBgmStop";
            case AudioEventKind::StageBgmStateChange:
                return "StageBgmStateChange";
            case AudioEventKind::SystemSoundStart:
                return "SystemSoundStart";
            case AudioEventKind::ControllerSpeakerSoundStart:
                return "ControllerSpeakerSoundStart";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *effect_event_kind_name(EffectEventKind kind) {
            switch (kind) {
            case EffectEventKind::Emit:
                return "Emit";
            case EffectEventKind::Delete:
                return "Delete";
            case EffectEventKind::DeleteAll:
                return "DeleteAll";
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

        [[nodiscard]] const char *j3d_packet_mode_name(J3dRendererPacketMode mode) {
            switch (mode) {
            case J3dRendererPacketMode::ConstantBackdrop:
                return "ConstantBackdrop";
            case J3dRendererPacketMode::ConstantMaterial:
                return "ConstantMaterial";
            case J3dRendererPacketMode::ComposedMaterial:
                return "ComposedMaterial";
            case J3dRendererPacketMode::CpuTevPerVertex:
                return "CpuTevPerVertex";
            case J3dRendererPacketMode::ShaderGxTev:
                return "ShaderGxTev";
            case J3dRendererPacketMode::TexturePass:
                return "TexturePass";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *texture_format_name(TplTextureFormat format) {
            switch (format) {
            case TplTextureFormat::I4:
                return "I4";
            case TplTextureFormat::I8:
                return "I8";
            case TplTextureFormat::IA4:
                return "IA4";
            case TplTextureFormat::IA8:
                return "IA8";
            case TplTextureFormat::RGB565:
                return "RGB565";
            case TplTextureFormat::RGB5A3:
                return "RGB5A3";
            case TplTextureFormat::RGBA8:
                return "RGBA8";
            case TplTextureFormat::C4:
                return "C4";
            case TplTextureFormat::C8:
                return "C8";
            case TplTextureFormat::C14X2:
                return "C14X2";
            case TplTextureFormat::CMPR:
                return "CMPR";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *register_space_name(GXRegisterSpace space) {
            switch (space) {
            case GXRegisterSpace::BP:
                return "BP";
            case GXRegisterSpace::CP:
                return "CP";
            case GXRegisterSpace::XF:
                return "XF";
            case GXRegisterSpace::IndexedA:
                return "IndexedA";
            case GXRegisterSpace::IndexedB:
                return "IndexedB";
            case GXRegisterSpace::IndexedC:
                return "IndexedC";
            case GXRegisterSpace::IndexedD:
                return "IndexedD";
            case GXRegisterSpace::Unknown:
                return "Unknown";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *blend_mode_name(render::BlendMode mode) {
            switch (mode) {
            case render::BlendMode::Opaque:
                return "Opaque";
            case render::BlendMode::Alpha:
                return "Alpha";
            case render::BlendMode::Additive:
                return "Additive";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *depth_compare_name(render::DepthCompare compare) {
            switch (compare) {
            case render::DepthCompare::Never:
                return "Never";
            case render::DepthCompare::Less:
                return "Less";
            case render::DepthCompare::Equal:
                return "Equal";
            case render::DepthCompare::LessEqual:
                return "LessEqual";
            case render::DepthCompare::Greater:
                return "Greater";
            case render::DepthCompare::NotEqual:
                return "NotEqual";
            case render::DepthCompare::GreaterEqual:
                return "GreaterEqual";
            case render::DepthCompare::Always:
                return "Always";
            }

            return "Unknown";
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

        [[nodiscard]] Json vec3_json(const CameraParamVec3 &value) {
            return Json{{"x", value.x}, {"y", value.y}, {"z", value.z}};
        }

        [[nodiscard]] Json camera_pose_json(const CameraPoseCompat &pose) {
            return Json{
                {"eye", vec3_json(pose.eye)},
                {"watch", vec3_json(pose.watch)},
                {"up", vec3_json(pose.up)},
                {"fovy_degrees", pose.fovy_degrees},
                {"aspect_ratio", pose.aspect_ratio},
                {"near_clip", pose.near_clip},
                {"far_clip", pose.far_clip},
            };
        }

        [[nodiscard]] Json color_json(const GXColorValue &color) {
            return Json::array({color[0U], color[1U], color[2U], color[3U]});
        }

        [[nodiscard]] Json float3_json(const std::array<float, 3U> &values) {
            return Json::array({values[0U], values[1U], values[2U]});
        }

        [[nodiscard]] Json float16_json(const std::array<float, 16U> &values) {
            auto out = Json::array();
            for (const auto value : values) {
                out.push_back(value);
            }
            return out;
        }

        [[nodiscard]] Json float12_json(const std::array<float, 12U> &values) {
            auto out = Json::array();
            for (const auto value : values) {
                out.push_back(value);
            }
            return out;
        }

        [[nodiscard]] Json float_array_json(std::span<const float> values) {
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

        [[nodiscard]] Json u32_array_json(std::span<const std::uint32_t> values) {
            auto out = Json::array();
            for (const auto value : values) {
                out.push_back(value);
            }
            return out;
        }

        [[nodiscard]] Json frame_rect_json(std::uint16_t width, std::uint16_t height) {
            return Json{
                {"left", 0},
                {"top", 0},
                {"right", width},
                {"bottom", height},
                {"width", width},
                {"height", height},
            };
        }

        [[nodiscard]] Json frame_viewport_json(const render::FrameContext &frame_context) {
            return Json{
                {"left", 0.0F},
                {"right", static_cast<float>(frame_context.framebuffer.width)},
                {"top", 0.0F},
                {"bottom", static_cast<float>(frame_context.framebuffer.height)},
                {"near_depth", 0.0F},
                {"far_depth", 1.0F},
            };
        }

        [[nodiscard]] Json frame_scissor_json(const render::FrameContext &frame_context) {
            const auto width = frame_context.framebuffer.width;
            const auto height = frame_context.framebuffer.height;
            return Json{
                {"left", 0},
                {"top", 0},
                {"right", width},
                {"bottom", height},
                {"width", width},
                {"height", height},
            };
        }

        [[nodiscard]] Json pc_copy_events_json(const render::FrameContext &frame_context) {
            const auto width = frame_context.framebuffer.width;
            const auto height = frame_context.framebuffer.height;
            return Json::array({
                Json{
                    {"index", 0},
                    {"event_index", frame_context.frame_index},
                    {"presenter_frame_count", frame_context.frame_index},
                    {"kind", "present"},
                    {"copy_to_xfb", false},
                    {"depth_copy", false},
                    {"clear", false},
                    {"half_scale", false},
                    {"scale_invert", false},
                    {"clamp_top", true},
                    {"clamp_bottom", true},
                    {"intensity_format", false},
                    {"auto_conversion", false},
                    {"dest_addr", 0},
                    {"dest_stride", static_cast<std::uint32_t>(width) * 4U},
                    {"source_rect", frame_rect_json(width, height)},
                    {"output_size", Json{{"width", width}, {"height", height}}},
                    {"target_pixel_format", 0},
                    {"real_format", 0},
                    {"frame_to_field", 0},
                    {"gamma_index", 0},
                    {"gamma_value", 1.0F},
                    {"y_scale", 1.0F},
                    {"dispcopyyscale", 256},
                    {"scissor", frame_scissor_json(frame_context)},
                    {"viewport", frame_viewport_json(frame_context)},
                    {"backbuffer", Json{{"width", width}, {"height", height}}},
                    {"target_rect", frame_rect_json(width, height)},
                    {"render_pass", "FinalPresent"},
                    {"view_id", 0},
                },
            });
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

        [[nodiscard]] Json scene_message_json(const SceneSchedulerMessageTraceEntry &entry, std::size_t index) {
            return Json{
                {"index", index},
                {"sequence", entry.sequence},
                {"message", entry.message},
                {"message_name", MR::getActorMessageName(entry.message)},
                {"target_name", entry.target_name},
                {"target_kind", entry_kind_name(entry.target_kind)},
                {"target_movement_type", entry.target_movement_type},
                {"target_calc_anim_type", entry.target_calc_anim_type},
                {"target_draw_buffer_type", entry.target_draw_buffer_type},
                {"target_draw_type", entry.target_draw_type},
                {"target_order", entry.target_order},
                {"target_dead", entry.target_dead},
                {"target_suspended", entry.target_suspended},
                {"excluded", entry.excluded},
                {"delivered", entry.delivered},
                {"accepted", entry.accepted},
                {"sender_sensor_present", entry.sender_sensor_present},
                {"receiver_sensor_present", entry.receiver_sensor_present},
                {"sender_sensor_type", entry.sender_sensor_type},
                {"receiver_sensor_type", entry.receiver_sensor_type},
                {"sender_sensor_host_name", entry.sender_sensor_host_name},
                {"receiver_sensor_host_name", entry.receiver_sensor_host_name},
            };
        }

        [[nodiscard]] Json scene_messages_json(std::span<const SceneSchedulerMessageTraceEntry> entries) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < entries.size(); ++i) {
                out.push_back(scene_message_json(entries[i], i));
            }
            return out;
        }

        [[nodiscard]] Json layout_animation_json(const SceneLayoutAnimationDebugState &animation) {
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
            };
        }

        [[nodiscard]] Json layout_runtime_entries_json(std::span<const SceneLayoutRuntimeDebugState> entries) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < entries.size(); ++i) {
                out.push_back(layout_runtime_json(entries[i], i));
            }
            return out;
        }

        [[nodiscard]] Json audio_events_json(std::span<const AudioEvent> events) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < events.size(); ++i) {
                const auto &event = events[i];
                out.push_back(Json{
                    {"index", i},
                    {"kind", audio_event_kind_name(event.kind)},
                    {"name", event.name},
                    {"fade_frames", event.fade_frames},
                    {"state", event.state},
                    {"change_frames", event.change_frames},
                    {"frame_index", event.frame_index},
                });
            }
            return out;
        }

        [[nodiscard]] Json effect_events_json(std::span<const EffectEvent> events) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < events.size(); ++i) {
                const auto &event = events[i];
                out.push_back(Json{
                    {"index", i},
                    {"kind", effect_event_kind_name(event.kind)},
                    {"actor_name", event.actor_name},
                    {"effect_name", event.effect_name},
                    {"frame_index", event.frame_index},
                });
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

        [[nodiscard]] Json rfl_miis_json(std::span<const RflMiiEntry> entries) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < entries.size(); ++i) {
                const auto &entry = entries[i];
                out.push_back(Json{
                    {"index", i},
                    {"rfl_index", entry.index},
                    {"name", entry.name},
                });
            }
            return out;
        }

        [[nodiscard]] Json save_slot_json(const SaveDataService::SlotState &slot) {
            return Json{
                {"slot_index", slot.slot_index},
                {"created", slot.created},
                {"game_data_corrupted", slot.game_data_corrupted},
                {"config_data_corrupted", slot.config_data_corrupted},
                {"last_loaded_mario", slot.last_loaded_mario},
                {"power_star_num", slot.power_star_num},
                {"star_piece_num", slot.star_piece_num},
                {"player_miss_num", slot.player_miss_num},
                {"has_mii_id", slot.has_mii_id},
                {"rfl_mii_index", slot.rfl_mii_index.has_value() ? Json(*slot.rfl_mii_index) : Json(nullptr)},
                {"icon_id", slot.icon_id.has_value() ? Json(*slot.icon_id) : Json(nullptr)},
                {"view_normal_ending", slot.view_normal_ending},
                {"view_complete_ending", slot.view_complete_ending},
                {"complete_ending_mario_and_luigi", slot.complete_ending_mario_and_luigi},
                {"last_modified", slot.last_modified},
            };
        }

        [[nodiscard]] Json save_slots_json(std::span<const SaveDataService::SlotState> slots) {
            auto out = Json::array();
            for (const auto &slot : slots) {
                out.push_back(save_slot_json(slot));
            }
            return out;
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

        [[nodiscard]] Json runtime_services_json(const RuntimeContext &runtime) {
            return Json{
                {"rfl",
                 Json{
                     {"initialized", runtime.rfl().is_initialized()},
                     {"error", runtime.rfl().has_error()},
                     {"valid_mii_count", runtime.rfl().valid_miis().size()},
                     {"valid_miis", rfl_miis_json(runtime.rfl().valid_miis())},
                 }},
                {"save",
                 Json{
                     {"file_count", runtime.save_data().file_count()},
                     {"host_directory",
                      runtime.save_data().host_directory().has_value() ? Json(runtime.save_data().host_directory()->string()) : Json(nullptr)},
                     {"sys_config",
                      Json{
                          {"time_announced", runtime.save_data().sys_config_time_announced()},
                          {"time_sent", runtime.save_data().sys_config_time_sent()},
                          {"sent_bytes", runtime.save_data().sys_config_sent_bytes()},
                      }},
                     {"slot_count", runtime.save_data().slot_states().size()},
                     {"slots", save_slots_json(runtime.save_data().slot_states())},
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
                {"sequence_requests",
                 Json{
                     {"change_stage_in_game_after_loading_game_data",
                      runtime.sequence_requests().is_change_stage_in_game_after_loading_game_data_requested()},
                     {"events", sequence_request_events_json(runtime.sequence_requests().events())},
                 }},
            };
        }

        [[nodiscard]] Json gx_blend_json(const render::GxBlendMode2D &blend) {
            return Json{
                {"enabled", blend.enabled},
                {"type", blend.type},
                {"src_factor", blend.src_factor},
                {"dst_factor", blend.dst_factor},
                {"op", blend.op},
                {"color_update", blend.color_update},
                {"alpha_update", blend.alpha_update},
            };
        }

        [[nodiscard]] Json gx_alpha_compare_json(const render::GxAlphaCompare2D &alpha_compare) {
            return Json{
                {"enabled", alpha_compare.enabled},
                {"comp0", alpha_compare.comp0},
                {"ref0", alpha_compare.ref0},
                {"op", alpha_compare.op},
                {"comp1", alpha_compare.comp1},
                {"ref1", alpha_compare.ref1},
            };
        }

        [[nodiscard]] Json gx_color_channel_control_json(const GXColorChannelControlState &control) {
            return Json{
                {"raw", control.raw},
                {"material_source", control.material_source},
                {"lighting_enabled", control.lighting_enabled},
                {"light_mask", control.light_mask},
                {"ambient_source", control.ambient_source},
                {"diffuse_function", control.diffuse_function},
                {"attenuation_function", control.attenuation_function},
                {"attenuation_mode", control.attenuation_mode},
            };
        }

        [[nodiscard]] Json gx_color_channels_json(const J3dRendererPacketState &state) {
            auto out = Json::array();
            for (auto channel = std::size_t{}; channel < state.color_channel_material_colors.size(); ++channel) {
                out.push_back(Json{
                    {"index", channel},
                    {"material_color", color_json(state.color_channel_material_colors[channel])},
                    {"ambient_color", color_json(state.color_channel_ambient_colors[channel])},
                    {"color_control", gx_color_channel_control_json(state.color_channel_controls[channel])},
                    {"alpha_control", gx_color_channel_control_json(state.alpha_channel_controls[channel])},
                });
            }
            return out;
        }

        [[nodiscard]] Json gx_lights_json(const J3dRendererPacketState &state) {
            auto out = Json::array();
            for (auto light_index = std::size_t{}; light_index < state.lights.size(); ++light_index) {
                const auto &light = state.lights[light_index];
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

        [[nodiscard]] Json gx_texture_binding_json(const J3dRendererTextureState &texture) {
            return Json{
                {"slot", texture.slot},
                {"texture_index", texture.texture_index},
                {"name", texture.name},
                {"has_source_texture", texture.has_source_texture},
                {"width", texture.width},
                {"height", texture.height},
                {"format", texture_format_name(texture.format)},
                {"format_raw", static_cast<std::uint32_t>(texture.format)},
                {"wrap_s", texture.wrap_s},
                {"wrap_t", texture.wrap_t},
                {"min_filter", texture.min_filter},
                {"mag_filter", texture.mag_filter},
                {"host_texture_handle", texture.host_handle.is_valid() ? Json(texture.host_handle.value) : Json(nullptr)},
            };
        }

        [[nodiscard]] Json gx_texture_bindings_json(std::span<const J3dRendererTextureState> textures) {
            auto out = Json::array();
            for (const auto &texture : textures) {
                out.push_back(gx_texture_binding_json(texture));
            }
            return out;
        }

        [[nodiscard]] std::uint32_t used_textures_mask(std::span<const J3dRendererTextureState> textures) {
            auto mask = std::uint32_t{};
            for (const auto &texture : textures) {
                if (texture.slot < 8U) {
                    mask |= 1U << texture.slot;
                }
            }
            return mask;
        }

        [[nodiscard]] Json used_texture_slots_json(std::span<const J3dRendererTextureState> textures) {
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

        [[nodiscard]] Json gx_tex_coord_gen_json(const GXTexCoordGenState &gen) {
            return Json{
                {"slot", gen.slot},
                {"type", gen.type},
                {"source", gen.source},
                {"matrix", gen.matrix},
            };
        }

        [[nodiscard]] Json gx_tex_coord_gens_json(std::span<const GXTexCoordGenState> gens) {
            auto out = Json::array();
            for (const auto &gen : gens) {
                out.push_back(gx_tex_coord_gen_json(gen));
            }
            return out;
        }

        [[nodiscard]] Json gx_tex_matrix_json(const GXTexMatrixState &matrix) {
            return Json{
                {"slot", matrix.slot},
                {"projection", matrix.projection},
                {"info", matrix.info},
                {"center", float3_json(matrix.center)},
                {"scale_s", matrix.scale_s},
                {"scale_t", matrix.scale_t},
                {"rotation", matrix.rotation},
                {"translate_s", matrix.translate_s},
                {"translate_t", matrix.translate_t},
                {"effect_matrix", float16_json(matrix.effect_matrix)},
            };
        }

        [[nodiscard]] Json gx_tex_matrices_json(std::span<const GXTexMatrixState> matrices) {
            auto out = Json::array();
            for (const auto &matrix : matrices) {
                out.push_back(gx_tex_matrix_json(matrix));
            }
            return out;
        }

        [[nodiscard]] Json gx_tev_order_json(const GXTevOrderState &order) {
            return Json{
                {"stage", order.stage},
                {"tex_coord", order.tex_coord},
                {"tex_map", order.tex_map},
                {"color_channel", order.color_channel},
            };
        }

        [[nodiscard]] Json gx_tev_orders_json(std::span<const GXTevOrderState> orders) {
            auto out = Json::array();
            for (const auto &order : orders) {
                out.push_back(gx_tev_order_json(order));
            }
            return out;
        }

        [[nodiscard]] Json gx_tev_stage_json(const GXTevStageState &stage) {
            return Json{
                {"stage", stage.stage},
                {"raw", u8_array_json(stage.raw)},
                {"color_in", u8_array_json(stage.color_in)},
                {"color_op", stage.color_op},
                {"color_bias", stage.color_bias},
                {"color_scale", stage.color_scale},
                {"color_clamp", stage.color_clamp},
                {"color_out", stage.color_out},
                {"k_color_sel", stage.k_color_sel},
                {"alpha_in", u8_array_json(stage.alpha_in)},
                {"alpha_op", stage.alpha_op},
                {"alpha_bias", stage.alpha_bias},
                {"alpha_scale", stage.alpha_scale},
                {"alpha_clamp", stage.alpha_clamp},
                {"alpha_out", stage.alpha_out},
                {"k_alpha_sel", stage.k_alpha_sel},
            };
        }

        [[nodiscard]] Json gx_tev_stages_json(std::span<const GXTevStageState> stages) {
            auto out = Json::array();
            for (const auto &stage : stages) {
                out.push_back(gx_tev_stage_json(stage));
            }
            return out;
        }

        [[nodiscard]] Json tev_registers_json(const std::array<render::GxTevRegisterColor2D, 4U> &registers) {
            auto out = Json::array();
            for (const auto &reg : registers) {
                out.push_back(Json::array({reg[0U], reg[1U], reg[2U], reg[3U]}));
            }
            return out;
        }

        [[nodiscard]] Json tev_k_colors_json(const std::array<GXColorValue, 4U> &colors) {
            auto out = Json::array();
            for (const auto &color : colors) {
                out.push_back(color_json(color));
            }
            return out;
        }

        [[nodiscard]] Json gx_z_mode_json(const GXZModeState &z_mode) {
            return Json{
                {"enabled", z_mode.enabled},
                {"compare_enable", z_mode.compare_enable},
                {"function", z_mode.function},
                {"update_enable", z_mode.update_enable},
            };
        }

        [[nodiscard]] Json gx_fog_json(const GXFogState &fog) {
            return Json{
                {"enabled", fog.enabled},
                {"type", fog.type},
                {"projection", fog.projection},
                {"range_adjust_enabled", fog.range_adjust_enabled},
                {"range_center", fog.range_center},
                {"a", fog.a},
                {"c", fog.c},
                {"b_magnitude", fog.b_magnitude},
                {"b_shift", fog.b_shift},
                {"color", color_json(fog.color)},
                {"range_k", float_array_json(fog.range_k)},
                {"raw", u8_array_json(fog.raw)},
            };
        }

        [[nodiscard]] Json gx_indirect_texture_orders_json(std::span<const GXIndirectTextureOrderState> orders) {
            auto out = Json::array();
            for (const auto &order : orders) {
                out.push_back(Json{
                    {"stage", order.stage},
                    {"tex_map", order.tex_map},
                    {"tex_coord", order.tex_coord},
                });
            }
            return out;
        }

        [[nodiscard]] Json gx_indirect_texture_matrices_json(std::span<const GXIndirectTextureMatrixState> matrices) {
            auto out = Json::array();
            for (const auto &matrix : matrices) {
                out.push_back(Json{
                    {"matrix", matrix.matrix},
                    {"ma", matrix.ma},
                    {"mb", matrix.mb},
                    {"mc", matrix.mc},
                    {"md", matrix.md},
                    {"me", matrix.me},
                    {"mf", matrix.mf},
                    {"scale", matrix.scale},
                    {"raw", u32_array_json(matrix.raw)},
                });
            }
            return out;
        }

        [[nodiscard]] Json gx_indirect_texture_coord_scales_json(std::span<const GXIndirectTextureCoordScaleState> scales) {
            auto out = Json::array();
            for (const auto &scale : scales) {
                out.push_back(Json{
                    {"stage", scale.stage},
                    {"scale_s", scale.scale_s},
                    {"scale_t", scale.scale_t},
                });
            }
            return out;
        }

        [[nodiscard]] Json gx_indirect_tev_stages_json(std::span<const GXIndirectTevStageState> stages) {
            auto out = Json::array();
            for (const auto &stage : stages) {
                out.push_back(Json{
                    {"tev_stage", stage.tev_stage},
                    {"ind_stage", stage.ind_stage},
                    {"format", stage.format},
                    {"bias", stage.bias},
                    {"bump_alpha", stage.bump_alpha},
                    {"matrix_index", stage.matrix_index},
                    {"matrix_id", stage.matrix_id},
                    {"wrap_s", stage.wrap_s},
                    {"wrap_t", stage.wrap_t},
                    {"use_original_lod", stage.use_original_lod},
                    {"add_previous", stage.add_previous},
                    {"active", stage.active},
                    {"raw", stage.raw},
                });
            }
            return out;
        }

        [[nodiscard]] Json gx_indirect_json(const GXIndirectState &indirect) {
            return Json{
                {"stage_count", indirect.stage_count},
                {"texture_orders", gx_indirect_texture_orders_json(indirect.texture_orders)},
                {"texture_matrices", gx_indirect_texture_matrices_json(indirect.texture_matrices)},
                {"texture_coord_scales", gx_indirect_texture_coord_scales_json(indirect.texture_coord_scales)},
                {"tev_stages", gx_indirect_tev_stages_json(indirect.tev_stages)},
            };
        }

        [[nodiscard]] Json gx_register_load_json(const GXRegisterLoadState &load, std::size_t index) {
            return Json{
                {"index", index},
                {"space", register_space_name(load.space)},
                {"byte_offset", load.byte_offset},
                {"address", load.address},
                {"count", load.count},
                {"value", load.value},
            };
        }

        [[nodiscard]] Json gx_register_loads_json(std::span<const GXRegisterLoadState> loads) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < loads.size(); ++i) {
                out.push_back(gx_register_load_json(loads[i], i));
            }
            return out;
        }

        [[nodiscard]] Json j3d_packet_trace_json(const RuntimeContext::J3dRuntimePacketTrace &packet, std::size_t index) {
            const auto &state = packet.state;
            return Json{
                {"index", index},
                {"model_name", packet.model_name},
                {"frame_index", packet.frame_index},
                {"draw_pass", packet.draw_pass},
                {"render_pass", packet.draw_pass},
                {"view_id", 0},
                {"material_name", state.material_name},
                {"shape_index", state.shape_index},
                {"shape_draw_order", state.shape_draw_order},
                {"material_index", state.material_index},
                {"joint_index", state.joint_index},
                {"matrix_group_index", state.matrix_group_index},
                {"matrix_group_count", state.matrix_group_count},
                {"use_matrix_index", state.use_matrix_index},
                {"use_matrix_count", state.use_matrix_count},
                {"first_matrix_table_index", state.first_matrix_table_index},
                {"matrix_table_count", state.matrix_table_count},
                {"display_list_offset", state.display_list_offset},
                {"display_list_size", state.display_list_size},
                {"parsed_display_list_bytes", state.parsed_display_list_bytes},
                {"draw_packet_triangle_count", state.draw_packet_triangle_count},
                {"pass_order", state.pass_order},
                {"packet_mode", j3d_packet_mode_name(state.packet_mode)},
                {"material_pass_count", state.material_pass_count},
                {"shader_texture_stage_count", state.shader_texture_stage_count},
                {"color_channel_count", state.color_channel_count},
                {"declared_tev_stage_count", state.declared_tev_stage_count},
                {"color_channels", gx_color_channels_json(state)},
                {"loaded_light_mask", state.loaded_light_mask},
                {"material_loaded_light_mask", state.material_loaded_light_mask},
                {"scene_loaded_light_mask", state.scene_loaded_light_mask},
                {"requested_light_mask", state.requested_light_mask},
                {"unsatisfied_light_mask", state.unsatisfied_light_mask},
                {"lights", gx_lights_json(state)},
                {"used_textures_mask", used_textures_mask(state.texture_bindings)},
                {"used_texture_slots", used_texture_slots_json(state.texture_bindings)},
                {"texture_bindings", gx_texture_bindings_json(state.texture_bindings)},
                {"tex_coord_gens", gx_tex_coord_gens_json(state.tex_coord_gens)},
                {"tex_matrices", gx_tex_matrices_json(state.tex_matrices)},
                {"tev_orders", gx_tev_orders_json(state.tev_orders)},
                {"tev_stages", gx_tev_stages_json(state.tev_stages)},
                {"tev_k_colors", tev_k_colors_json(state.tev_k_colors)},
                {"active_tev_stage_count", state.active_tev_stage_count},
                {"tev_order_count", state.tev_order_count},
                {"tev_stage_count", state.tev_stage_count},
                {"texgen_count", state.texgen_count},
                {"indirect_stage_count", state.indirect_stage_count},
                {"active_indirect_tev_stage_count", state.active_indirect_tev_stage_count},
                {"indirect_matrix_count", state.indirect_matrix_count},
                {"indirect_texture_order_count", state.indirect_texture_order_count},
                {"indirect_texture_scale_count", state.indirect_texture_scale_count},
                {"indirect", gx_indirect_json(state.indirect)},
                {"mdl3_packet_bytes", state.mdl3_packet_bytes},
                {"mdl3_command_count", state.mdl3_command_count},
                {"mdl3_bp_load_count", state.mdl3_bp_load_count},
                {"mdl3_xf_load_count", state.mdl3_xf_load_count},
                {"mdl3_register_loads", gx_register_loads_json(state.mdl3_register_loads)},
                {"source_vertex_count", state.source_vertex_count},
                {"source_triangle_count", state.source_triangle_count},
                {"project_source_vertices", state.project_source_vertices},
                {"evaluate_material_per_vertex", state.evaluate_material_per_vertex},
                {"blend", state.blend},
                {"blend_mode", blend_mode_name(state.blend_mode)},
                {"gx_blend", gx_blend_json(state.gx_blend)},
                {"gx_alpha_compare", gx_alpha_compare_json(state.gx_alpha_compare)},
                {"gx_initial_tev_registers", tev_registers_json(state.gx_initial_tev_registers)},
                {"gx_z_mode", gx_z_mode_json(state.gx_z_mode)},
                {"gx_fog", gx_fog_json(state.gx_fog)},
                {"depth_test", state.depth_test},
                {"depth_write", state.depth_write},
                {"depth_compare", depth_compare_name(state.depth_compare)},
                {"cull_mode", cull_mode_name(state.cull_mode)},
                {"fog_enabled", state.fog_enabled},
                {"fog_type", state.fog_type},
                {"fog_projection", state.fog_projection},
                {"fog_range_adjust_enabled", state.fog_range_adjust_enabled},
                {"fog_color", color_json(state.fog_color)},
                {"bck_active", state.bck_active},
                {"bck_frame", state.bck_frame},
                {"bck_normalized_frame", state.bck_normalized_frame},
                {"bck_frame_max", state.bck_frame_max},
                {"bck_joint_count", state.bck_joint_count},
                {"btk_active", state.btk_active},
                {"btk_frame", state.btk_frame},
                {"btk_normalized_frame", state.btk_normalized_frame},
                {"btk_frame_max", state.btk_frame_max},
                {"btk_material_count", state.btk_material_count},
            };
        }

        [[nodiscard]] Json j3d_packet_traces_json(std::span<const RuntimeContext::J3dRuntimePacketTrace> packets) {
            auto out = Json::array();
            for (auto i = std::size_t{}; i < packets.size(); ++i) {
                out.push_back(j3d_packet_trace_json(packets[i], i));
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
            {"wpad0",
             Json{
                 {"connected", runtime.wpad().is_connected(WPAD_CHAN0)},
                 {"button_a_held", runtime.wpad().is_button_held(WPAD_CHAN0, WPAD_BUTTON_A)},
                 {"button_b_held", runtime.wpad().is_button_held(WPAD_CHAN0, WPAD_BUTTON_B)},
                 {"button_a_triggered", runtime.wpad().is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A)},
                 {"button_b_triggered", runtime.wpad().is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_B)},
             }},
            {"audio",
             Json{
                 {"stage_bgm", runtime.current_stage_bgm_name()},
                 {"stage_bgm_prepared", runtime.is_stage_bgm_prepared()},
                 {"events", audio_events_json(runtime.audio().events())},
             }},
            {"effects", Json{{"events", effect_events_json(runtime.effects().events())}}},
            {"runtime_services", runtime_services_json(runtime)},
            {"scene_snapshot", scene_entries_json(runtime.scheduler().snapshot())},
            {"scene_trace", scene_entries_json(runtime.scheduler().last_execution_trace())},
            {"scene_messages", scene_messages_json(runtime.scheduler().message_trace())},
            {"layout_runtime", layout_runtime_entries_json(runtime.scheduler().debug_layout_runtime_snapshot())},
            {"render_packets", j3d_packet_traces_json(runtime.j3d_packet_trace())},
            {"copy_events", pc_copy_events_json(frame_context)},
        };
        return trace;
    }

    void write_runtime_parity_trace(const std::filesystem::path &path, const render::FrameContext &frame_context, const RuntimeContext &runtime) {
        dump::write_json_file(path, runtime_parity_trace_json(frame_context, runtime));
    }

    dump::Json load_runtime_parity_trace(const std::filesystem::path &path) {
        return dump::load_json_file(path);
    }

}  // namespace smgpc::game
