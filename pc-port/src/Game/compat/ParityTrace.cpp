#include "Game/compat/ParityTrace.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string_view>

#include <revolution.h>

#include "Game/compat/RuntimeContext.hpp"

namespace smgpc::game {
    namespace {

        void write_json_string(std::ostream &out, std::string_view value) {
            out << '"';
            for (const auto ch : value) {
                switch (ch) {
                case '\\':
                    out << "\\\\";
                    break;
                case '"':
                    out << "\\\"";
                    break;
                case '\b':
                    out << "\\b";
                    break;
                case '\f':
                    out << "\\f";
                    break;
                case '\n':
                    out << "\\n";
                    break;
                case '\r':
                    out << "\\r";
                    break;
                case '\t':
                    out << "\\t";
                    break;
                default:
                    if (static_cast<unsigned char>(ch) < 0x20U) {
                        out << "\\u00";
                        constexpr auto hex = std::string_view{"0123456789abcdef"};
                        out << hex[(static_cast<unsigned char>(ch) >> 4U) & 0x0fU] << hex[static_cast<unsigned char>(ch) & 0x0fU];
                    } else {
                        out << ch;
                    }
                    break;
                }
            }
            out << '"';
        }

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

        void write_vec3(std::ostream &out, const CameraParamVec3 &value) {
            out << "{\"x\":" << value.x << ",\"y\":" << value.y << ",\"z\":" << value.z << '}';
        }

        void write_camera_pose(std::ostream &out, const CameraPoseCompat &pose) {
            out << "{\"eye\":";
            write_vec3(out, pose.eye);
            out << ",\"watch\":";
            write_vec3(out, pose.watch);
            out << ",\"up\":";
            write_vec3(out, pose.up);
            out << ",\"fovy_degrees\":" << pose.fovy_degrees << ",\"aspect_ratio\":" << pose.aspect_ratio << ",\"near_clip\":" << pose.near_clip
                << ",\"far_clip\":" << pose.far_clip << '}';
        }

        void write_scene_entry(std::ostream &out, const SceneSchedulerEntryState &entry, std::size_t index) {
            out << "{\"index\":" << index << ",\"kind\":";
            write_json_string(out, entry_kind_name(entry.kind));
            out << ",\"phase\":";
            write_json_string(out, phase_name(entry.phase));
            out << ",\"name\":";
            write_json_string(out, entry.name);
            out << ",\"movement_type\":" << entry.movement_type << ",\"calc_anim_type\":" << entry.calc_anim_type
                << ",\"draw_buffer_type\":" << entry.draw_buffer_type << ",\"draw_type\":" << entry.draw_type << ",\"draw_buffer_pass\":";
            write_json_string(out, draw_buffer_pass_name(entry.draw_buffer_pass));
            out << ",\"order\":" << entry.order << ",\"suspended\":" << (entry.suspended ? "true" : "false") << ",\"dead\":"
                << (entry.dead ? "true" : "false") << '}';
        }

        void write_scene_entries(std::ostream &out, std::span<const SceneSchedulerEntryState> entries) {
            out << '[';
            for (auto i = std::size_t{}; i < entries.size(); ++i) {
                if (i != 0U) {
                    out << ',';
                }
                write_scene_entry(out, entries[i], i);
            }
            out << ']';
        }

        void write_layout_animation_state(std::ostream &out, const SceneLayoutAnimationDebugState &animation) {
            out << "{\"layer_index\":" << animation.layer_index << ",\"name\":";
            write_json_string(out, animation.name);
            out << ",\"frame\":" << animation.frame << ",\"end_frame\":" << animation.end_frame << ",\"rate\":" << animation.rate
                << ",\"stopped\":" << (animation.stopped ? "true" : "false") << ",\"looping\":"
                << (animation.looping ? "true" : "false") << '}';
        }

        void write_layout_animation_states(std::ostream &out, std::span<const SceneLayoutAnimationDebugState> animations) {
            out << '[';
            for (auto i = std::size_t{}; i < animations.size(); ++i) {
                if (i != 0U) {
                    out << ',';
                }
                write_layout_animation_state(out, animations[i]);
            }
            out << ']';
        }

        void write_layout_runtime_entry(std::ostream &out, const SceneLayoutRuntimeDebugState &entry, std::size_t index) {
            out << "{\"index\":" << index << ",\"name\":";
            write_json_string(out, entry.name);
            out << ",\"layout_name\":";
            write_json_string(out, entry.layout_name);
            out << ",\"archive_path\":";
            if (entry.has_archive_path) {
                write_json_string(out, entry.archive_path);
            } else {
                out << "null";
            }
            out << ",\"movement_type\":" << entry.movement_type << ",\"calc_anim_type\":" << entry.calc_anim_type << ",\"draw_type\":"
                << entry.draw_type << ",\"order\":" << entry.order << ",\"suspended\":" << (entry.suspended ? "true" : "false")
                << ",\"dead\":" << (entry.dead ? "true" : "false") << ",\"animations\":";
            write_layout_animation_states(out, entry.animations);
            out << '}';
        }

        void write_layout_runtime_entries(std::ostream &out, std::span<const SceneLayoutRuntimeDebugState> entries) {
            out << '[';
            for (auto i = std::size_t{}; i < entries.size(); ++i) {
                if (i != 0U) {
                    out << ',';
                }
                write_layout_runtime_entry(out, entries[i], i);
            }
            out << ']';
        }

        void write_audio_events(std::ostream &out, std::span<const AudioEvent> events) {
            out << '[';
            for (auto i = std::size_t{}; i < events.size(); ++i) {
                if (i != 0U) {
                    out << ',';
                }
                const auto &event = events[i];
                out << "{\"index\":" << i << ",\"kind\":";
                write_json_string(out, audio_event_kind_name(event.kind));
                out << ",\"name\":";
                write_json_string(out, event.name);
                out << ",\"fade_frames\":" << event.fade_frames << ",\"frame_index\":" << event.frame_index << '}';
            }
            out << ']';
        }

        void write_effect_events(std::ostream &out, std::span<const EffectEvent> events) {
            out << '[';
            for (auto i = std::size_t{}; i < events.size(); ++i) {
                if (i != 0U) {
                    out << ',';
                }
                const auto &event = events[i];
                out << "{\"index\":" << i << ",\"kind\":";
                write_json_string(out, effect_event_kind_name(event.kind));
                out << ",\"actor_name\":";
                write_json_string(out, event.actor_name);
                out << ",\"effect_name\":";
                write_json_string(out, event.effect_name);
                out << ",\"frame_index\":" << event.frame_index << '}';
            }
            out << ']';
        }

        void write_gx_blend(std::ostream &out, const render::GxBlendMode2D &blend) {
            out << "{\"enabled\":" << (blend.enabled ? "true" : "false") << ",\"type\":" << static_cast<unsigned>(blend.type)
                << ",\"src_factor\":" << static_cast<unsigned>(blend.src_factor) << ",\"dst_factor\":"
                << static_cast<unsigned>(blend.dst_factor) << ",\"op\":" << static_cast<unsigned>(blend.op) << '}';
        }

        void write_gx_alpha_compare(std::ostream &out, const render::GxAlphaCompare2D &alpha_compare) {
            out << "{\"enabled\":" << (alpha_compare.enabled ? "true" : "false") << ",\"comp0\":"
                << static_cast<unsigned>(alpha_compare.comp0) << ",\"ref0\":" << static_cast<unsigned>(alpha_compare.ref0) << ",\"op\":"
                << static_cast<unsigned>(alpha_compare.op) << ",\"comp1\":" << static_cast<unsigned>(alpha_compare.comp1) << ",\"ref1\":"
                << static_cast<unsigned>(alpha_compare.ref1) << '}';
        }

        void write_tev_registers(std::ostream &out, const std::array<render::GxTevRegisterColor2D, 4U> &registers) {
            out << '[';
            for (auto register_index = std::size_t{}; register_index < registers.size(); ++register_index) {
                if (register_index != 0U) {
                    out << ',';
                }
                out << '[';
                for (auto channel_index = std::size_t{}; channel_index < registers[register_index].size(); ++channel_index) {
                    if (channel_index != 0U) {
                        out << ',';
                    }
                    out << registers[register_index][channel_index];
                }
                out << ']';
            }
            out << ']';
        }

        void write_j3d_packet_trace(std::ostream &out, const RuntimeContext::J3dRuntimePacketTrace &packet, std::size_t index) {
            const auto &state = packet.state;
            out << "{\"index\":" << index << ",\"model_name\":";
            write_json_string(out, packet.model_name);
            out << ",\"frame_index\":" << packet.frame_index << ",\"draw_pass\":";
            write_json_string(out, packet.draw_pass);
            out << ",\"material_name\":";
            write_json_string(out, state.material_name);
            out << ",\"shape_index\":" << state.shape_index << ",\"shape_draw_order\":" << state.shape_draw_order
                << ",\"material_index\":" << state.material_index << ",\"joint_index\":" << state.joint_index << ",\"pass_order\":"
                << static_cast<unsigned>(state.pass_order) << ",\"packet_mode\":";
            write_json_string(out, j3d_packet_mode_name(state.packet_mode));
            out << ",\"material_pass_count\":" << state.material_pass_count << ",\"shader_texture_stage_count\":" << state.shader_texture_stage_count
                << ",\"color_channel_count\":" << static_cast<unsigned>(state.color_channel_count) << ",\"declared_tev_stage_count\":"
                << static_cast<unsigned>(state.declared_tev_stage_count) << ",\"active_tev_stage_count\":" << state.active_tev_stage_count
                << ",\"tev_order_count\":" << state.tev_order_count << ",\"tev_stage_count\":" << state.tev_stage_count << ",\"texgen_count\":"
                << state.texgen_count << ",\"indirect_stage_count\":" << static_cast<unsigned>(state.indirect_stage_count)
                << ",\"active_indirect_tev_stage_count\":" << state.active_indirect_tev_stage_count << ",\"indirect_matrix_count\":"
                << state.indirect_matrix_count << ",\"indirect_texture_order_count\":" << state.indirect_texture_order_count
                << ",\"indirect_texture_scale_count\":" << state.indirect_texture_scale_count << ",\"mdl3_packet_bytes\":"
                << state.mdl3_packet_bytes << ",\"mdl3_command_count\":" << state.mdl3_command_count << ",\"mdl3_bp_load_count\":"
                << state.mdl3_bp_load_count << ",\"mdl3_xf_load_count\":" << state.mdl3_xf_load_count << ",\"source_vertex_count\":"
                << state.source_vertex_count << ",\"source_triangle_count\":" << state.source_triangle_count
                << ",\"project_source_vertices\":" << (state.project_source_vertices ? "true" : "false")
                << ",\"evaluate_material_per_vertex\":" << (state.evaluate_material_per_vertex ? "true" : "false")
                << ",\"blend\":" << (state.blend ? "true" : "false") << ",\"blend_mode\":";
            write_json_string(out, blend_mode_name(state.blend_mode));
            out << ",\"gx_blend\":";
            write_gx_blend(out, state.gx_blend);
            out << ",\"gx_alpha_compare\":";
            write_gx_alpha_compare(out, state.gx_alpha_compare);
            out << ",\"gx_initial_tev_registers\":";
            write_tev_registers(out, state.gx_initial_tev_registers);
            out << ",\"depth_test\":" << (state.depth_test ? "true" : "false") << ",\"depth_write\":"
                << (state.depth_write ? "true" : "false") << ",\"depth_compare\":";
            write_json_string(out, depth_compare_name(state.depth_compare));
            out << ",\"cull_mode\":";
            write_json_string(out, cull_mode_name(state.cull_mode));
            out << ",\"fog_enabled\":" << (state.fog_enabled ? "true" : "false") << ",\"fog_type\":" << static_cast<unsigned>(state.fog_type)
                << ",\"fog_projection\":" << static_cast<unsigned>(state.fog_projection)
                << ",\"fog_range_adjust_enabled\":" << (state.fog_range_adjust_enabled ? "true" : "false") << ",\"fog_color\":["
                << static_cast<unsigned>(state.fog_color[0U]) << ',' << static_cast<unsigned>(state.fog_color[1U]) << ','
                << static_cast<unsigned>(state.fog_color[2U]) << ',' << static_cast<unsigned>(state.fog_color[3U]) << ']';
            out << ",\"bck_active\":" << (state.bck_active ? "true" : "false") << ",\"bck_frame\":" << state.bck_frame
                << ",\"bck_normalized_frame\":" << state.bck_normalized_frame << ",\"bck_frame_max\":" << state.bck_frame_max
                << ",\"bck_joint_count\":" << state.bck_joint_count << ",\"btk_active\":" << (state.btk_active ? "true" : "false")
                << ",\"btk_frame\":" << state.btk_frame << ",\"btk_normalized_frame\":" << state.btk_normalized_frame
                << ",\"btk_frame_max\":" << state.btk_frame_max << ",\"btk_material_count\":" << state.btk_material_count;
            out << '}';
        }

        void write_j3d_packet_traces(std::ostream &out, std::span<const RuntimeContext::J3dRuntimePacketTrace> packets) {
            out << '[';
            for (auto i = std::size_t{}; i < packets.size(); ++i) {
                if (i != 0U) {
                    out << ',';
                }
                write_j3d_packet_trace(out, packets[i], i);
            }
            out << ']';
        }

    }  // namespace

    void write_runtime_parity_trace(const std::filesystem::path &path, const render::FrameContext &frame_context, const RuntimeContext &runtime) {
        if (!path.parent_path().empty()) {
            std::filesystem::create_directories(path.parent_path());
        }

        auto out = std::ofstream(path, std::ios::binary);
        if (!out) {
            throw std::runtime_error("could not open parity trace output " + path.string());
        }

        out << "{\"schema\":\"smgpc-runtime-parity-trace-v1\"";
        out << ",\"frame\":{\"index\":" << frame_context.frame_index << ",\"runtime_index\":" << runtime.frame_index()
            << ",\"time_seconds\":" << frame_context.frame_time_seconds << ",\"delta_seconds\":" << frame_context.frame_delta_seconds
            << ",\"framebuffer\":{\"width\":" << frame_context.framebuffer.width << ",\"height\":" << frame_context.framebuffer.height
            << "},\"has_focus\":" << (frame_context.has_focus ? "true" : "false") << ",\"is_minimized\":"
            << (frame_context.is_minimized ? "true" : "false") << '}';
        out << ",\"camera_pose\":";
        if (runtime.last_camera_pose().has_value()) {
            write_camera_pose(out, *runtime.last_camera_pose());
        } else {
            out << "null";
        }
        out << ",\"wpad0\":{\"connected\":" << (runtime.wpad().is_connected(WPAD_CHAN0) ? "true" : "false")
            << ",\"button_a_held\":" << (runtime.wpad().is_button_held(WPAD_CHAN0, WPAD_BUTTON_A) ? "true" : "false")
            << ",\"button_b_held\":" << (runtime.wpad().is_button_held(WPAD_CHAN0, WPAD_BUTTON_B) ? "true" : "false")
            << ",\"button_a_triggered\":" << (runtime.wpad().is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A) ? "true" : "false")
            << ",\"button_b_triggered\":" << (runtime.wpad().is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_B) ? "true" : "false") << '}';
        out << ",\"audio\":{\"stage_bgm\":";
        write_json_string(out, runtime.current_stage_bgm_name());
        out << ",\"stage_bgm_prepared\":" << (runtime.is_stage_bgm_prepared() ? "true" : "false") << ",\"events\":";
        write_audio_events(out, runtime.audio().events());
        out << '}';
        out << ",\"effects\":{\"events\":";
        write_effect_events(out, runtime.effects().events());
        out << '}';
        out << ",\"scene_snapshot\":";
        write_scene_entries(out, runtime.scheduler().snapshot());
        out << ",\"scene_trace\":";
        write_scene_entries(out, runtime.scheduler().last_execution_trace());
        const auto layout_runtime = runtime.scheduler().debug_layout_runtime_snapshot();
        out << ",\"layout_runtime\":";
        write_layout_runtime_entries(out, layout_runtime);
        out << ",\"render_packets\":";
        write_j3d_packet_traces(out, runtime.j3d_packet_trace());
        out << "}\n";
    }

}  // namespace smgpc::game
