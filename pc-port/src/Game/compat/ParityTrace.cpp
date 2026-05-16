#include "Game/compat/ParityTrace.hpp"

#include <array>
#include <filesystem>
#include <span>
#include <string>

#include <revolution.h>

#include "DumpJson.hpp"
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
            case EffectEventKind::DeleteAll:
                return "DeleteAll";
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
                {"animations", layout_animations_json(entry.animations)},
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

        [[nodiscard]] Json gx_blend_json(const render::GxBlendMode2D &blend) {
            return Json{
                {"enabled", blend.enabled},
                {"type", blend.type},
                {"src_factor", blend.src_factor},
                {"dst_factor", blend.dst_factor},
                {"op", blend.op},
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

        [[nodiscard]] Json tev_registers_json(const std::array<render::GxTevRegisterColor2D, 4U> &registers) {
            auto out = Json::array();
            for (const auto &reg : registers) {
                out.push_back(Json::array({reg[0U], reg[1U], reg[2U], reg[3U]}));
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
                {"lights", gx_lights_json(state)},
                {"active_tev_stage_count", state.active_tev_stage_count},
                {"tev_order_count", state.tev_order_count},
                {"tev_stage_count", state.tev_stage_count},
                {"texgen_count", state.texgen_count},
                {"indirect_stage_count", state.indirect_stage_count},
                {"active_indirect_tev_stage_count", state.active_indirect_tev_stage_count},
                {"indirect_matrix_count", state.indirect_matrix_count},
                {"indirect_texture_order_count", state.indirect_texture_order_count},
                {"indirect_texture_scale_count", state.indirect_texture_scale_count},
                {"mdl3_packet_bytes", state.mdl3_packet_bytes},
                {"mdl3_command_count", state.mdl3_command_count},
                {"mdl3_bp_load_count", state.mdl3_bp_load_count},
                {"mdl3_xf_load_count", state.mdl3_xf_load_count},
                {"source_vertex_count", state.source_vertex_count},
                {"source_triangle_count", state.source_triangle_count},
                {"project_source_vertices", state.project_source_vertices},
                {"evaluate_material_per_vertex", state.evaluate_material_per_vertex},
                {"blend", state.blend},
                {"blend_mode", blend_mode_name(state.blend_mode)},
                {"gx_blend", gx_blend_json(state.gx_blend)},
                {"gx_alpha_compare", gx_alpha_compare_json(state.gx_alpha_compare)},
                {"gx_initial_tev_registers", tev_registers_json(state.gx_initial_tev_registers)},
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
            {"scene_snapshot", scene_entries_json(runtime.scheduler().snapshot())},
            {"scene_trace", scene_entries_json(runtime.scheduler().last_execution_trace())},
            {"layout_runtime", layout_runtime_entries_json(runtime.scheduler().debug_layout_runtime_snapshot())},
            {"render_packets", j3d_packet_traces_json(runtime.j3d_packet_trace())},
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
