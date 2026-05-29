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
            case AudioEventKind::SystemSoundStop:
                return "SystemSoundStop";
            case AudioEventKind::SystemLevelSoundStart:
                return "SystemLevelSoundStart";
            case AudioEventKind::LevelSoundSubmit:
                return "LevelSoundSubmit";
            case AudioEventKind::LevelSoundPermit:
                return "LevelSoundPermit";
            case AudioEventKind::AtmosphereSoundStart:
                return "AtmosphereSoundStart";
            case AudioEventKind::SystemMEStart:
                return "SystemMEStart";
            case AudioEventKind::ControllerSpeakerSoundStart:
                return "ControllerSpeakerSoundStart";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *camera_shake_request_kind_name(CameraSystemService::ShakeRequestKind kind) {
            switch (kind) {
            case CameraSystemService::ShakeRequestKind::Normal:
                return "Normal";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *rumble_request_kind_name(RumbleRequestKind kind) {
            switch (kind) {
            case RumbleRequestKind::Strong:
                return "Strong";
            case RumbleRequestKind::Middle:
                return "Middle";
            case RumbleRequestKind::Weak:
                return "Weak";
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

        [[nodiscard]] const char *effect_keeper_host_kind_name(EffectKeeperHostKind kind) {
            switch (kind) {
            case EffectKeeperHostKind::LiveActor:
                return "LiveActor";
            case EffectKeeperHostKind::LayoutActor:
                return "LayoutActor";
            case EffectKeeperHostKind::SimpleLayout:
                return "SimpleLayout";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *effect_host_binding_source_name(EffectHostBindingSource source) {
            switch (source) {
            case EffectHostBindingSource::LiveActorBaseMatrix:
                return "LiveActorBaseMatrix";
            case EffectHostBindingSource::LayoutActorTransform:
                return "LayoutActorTransform";
            case EffectHostBindingSource::SimpleLayoutOrigin:
                return "SimpleLayoutOrigin";
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

        [[nodiscard]] const char *image_effect_control_kind_name(ImageEffectControlKind kind) {
            switch (kind) {
            case ImageEffectControlKind::ForceOff:
                return "ForceOff";
            case ImageEffectControlKind::ControlAuto:
                return "ControlAuto";
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

        [[nodiscard]] const char *j3d_packet_mode_name(smgpc::render::J3dRendererPacketMode mode) {
            switch (mode) {
            case smgpc::render::J3dRendererPacketMode::ConstantBackdrop:
                return "ConstantBackdrop";
            case smgpc::render::J3dRendererPacketMode::ConstantMaterial:
                return "ConstantMaterial";
            case smgpc::render::J3dRendererPacketMode::ComposedMaterial:
                return "ComposedMaterial";
            case smgpc::render::J3dRendererPacketMode::CpuTevPerVertex:
                return "CpuTevPerVertex";
            case smgpc::render::J3dRendererPacketMode::ShaderGxTev:
                return "ShaderGxTev";
            case smgpc::render::J3dRendererPacketMode::TexturePass:
                return "TexturePass";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *texture_format_name(smgpc::resource::TplTextureFormat format) {
            switch (format) {
            case smgpc::resource::TplTextureFormat::I4:
                return "I4";
            case smgpc::resource::TplTextureFormat::I8:
                return "I8";
            case smgpc::resource::TplTextureFormat::IA4:
                return "IA4";
            case smgpc::resource::TplTextureFormat::IA8:
                return "IA8";
            case smgpc::resource::TplTextureFormat::RGB565:
                return "RGB565";
            case smgpc::resource::TplTextureFormat::RGB5A3:
                return "RGB5A3";
            case smgpc::resource::TplTextureFormat::RGBA8:
                return "RGBA8";
            case smgpc::resource::TplTextureFormat::C4:
                return "C4";
            case smgpc::resource::TplTextureFormat::C8:
                return "C8";
            case smgpc::resource::TplTextureFormat::C14X2:
                return "C14X2";
            case smgpc::resource::TplTextureFormat::CMPR:
                return "CMPR";
            }

            return "Unknown";
        }

        [[nodiscard]] const char *register_space_name(smgpc::render::GXRegisterSpace space) {
            switch (space) {
            case smgpc::render::GXRegisterSpace::BP:
                return "BP";
            case smgpc::render::GXRegisterSpace::CP:
                return "CP";
            case smgpc::render::GXRegisterSpace::XF:
                return "XF";
            case smgpc::render::GXRegisterSpace::IndexedA:
                return "IndexedA";
            case smgpc::render::GXRegisterSpace::IndexedB:
                return "IndexedB";
            case smgpc::render::GXRegisterSpace::IndexedC:
                return "IndexedC";
            case smgpc::render::GXRegisterSpace::IndexedD:
                return "IndexedD";
            case smgpc::render::GXRegisterSpace::Unknown:
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

        [[nodiscard]] Json vec3_json(const smgpc::camera::CameraParamVec3 &value) {
            return Json {{"x", value.x}, {"y", value.y}, {"z", value.z}};
        }

        [[nodiscard]] Json camera_pose_json(const smgpc::camera::CameraPose &pose) {
            return Json {
                {"eye", vec3_json(pose.eye)},
                {"watch", vec3_json(pose.watch)},
                {"up", vec3_json(pose.up)},
                {"fovy_degrees", pose.fovy_degrees},
                {"aspect_ratio", pose.aspect_ratio},
                {"near_clip", pose.near_clip},
                {"far_clip", pose.far_clip},
            };
        }

        [[nodiscard]] Json color_json(const smgpc::render::GXColorValue &color) {
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

        [[nodiscard]] Json copy_sample_pattern_json(const std::array<std::array<std::uint8_t, 2U>, 12U> &pattern) {
            auto out = Json::array();
            for (const auto &sample : pattern) {
                out.push_back(Json::array({sample[0U], sample[1U]}));
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

        [[nodiscard]] Json wpad_button_names_json(std::uint32_t mask) {
            constexpr auto buttons = std::array {
                std::pair {WPAD_BUTTON_A, "A"},
                std::pair {WPAD_BUTTON_B, "B"},
                std::pair {WPAD_BUTTON_UP, "UP"},
                std::pair {WPAD_BUTTON_DOWN, "DOWN"},
                std::pair {WPAD_BUTTON_LEFT, "LEFT"},
                std::pair {WPAD_BUTTON_RIGHT, "RIGHT"},
                std::pair {WPAD_BUTTON_PLUS, "PLUS"},
                std::pair {WPAD_BUTTON_MINUS, "MINUS"},
                std::pair {WPAD_BUTTON_HOME, "HOME"},
                std::pair {WPAD_BUTTON_C, "C"},
                std::pair {WPAD_BUTTON_Z, "Z"},
                std::pair {WPAD_BUTTON_1, "ONE"},
                std::pair {WPAD_BUTTON_2, "TWO"},
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
            return Json {
                {"x", pointer.x},
                {"y", pointer.y},
                {"valid", pointer.valid},
                {"on_screen", pointer_on_screen(pointer.x, pointer.y, pointer.valid, frame_context)},
            };
        }

        [[nodiscard]] Json wpad_pointer_json(const WpadPointerState &pointer, const render::FrameContext &frame_context) {
            return Json {
                {"x", pointer.x},
                {"y", pointer.y},
                {"valid", pointer.valid},
                {"on_screen", pointer_on_screen(pointer.x, pointer.y, pointer.valid, frame_context)},
            };
        }

        [[nodiscard]] Json host_input_json(const RuntimeContext::HostInputTraceState &input, const render::FrameContext &frame_context) {
            return Json {
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
                return Json {{"connected", false}};
            }

            return Json {
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
            return render::CopyRect {
                .left = 0,
                .top = 0,
                .right = width,
                .bottom = height,
                .width = width,
                .height = height,
            };
        }

        [[nodiscard]] render::CopyViewport copy_viewport_from_frame(const render::FrameContext &frame_context) {
            return render::CopyViewport {
                .left = 0.0F,
                .right = static_cast<float>(frame_context.framebuffer.width),
                .top = 0.0F,
                .bottom = static_cast<float>(frame_context.framebuffer.height),
                .near_depth = 0.0F,
                .far_depth = 1.0F,
            };
        }

        [[nodiscard]] Json copy_rect_json(const render::CopyRect &rect) {
            return Json {
                {"left", rect.left},
                {"top", rect.top},
                {"right", rect.right},
                {"bottom", rect.bottom},
                {"width", rect.width},
                {"height", rect.height},
            };
        }

        [[nodiscard]] Json copy_viewport_json(const render::CopyViewport &viewport) {
            return Json {
                {"left", viewport.left},
                {"right", viewport.right},
                {"top", viewport.top},
                {"bottom", viewport.bottom},
                {"near_depth", viewport.near_depth},
                {"far_depth", viewport.far_depth},
            };
        }

        [[nodiscard]] Json framebuffer_json(const render::FramebufferInfo &framebuffer) {
            return Json {
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
            return Json {
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

            return Json {
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
            return Json {
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
            for (auto i = std::size_t {}; i < entries.size(); ++i) {
                out.push_back(scene_entry_json(entries[i], i));
            }
            return out;
        }

        [[nodiscard]] Json scene_message_json(const SceneSchedulerMessageTraceEntry &entry, std::size_t index) {
            return Json {
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
            for (auto i = std::size_t {}; i < entries.size(); ++i) {
                out.push_back(scene_message_json(entries[i], i));
            }
            return out;
        }

        [[nodiscard]] Json layout_animation_json(const SceneLayoutAnimationDebugState &animation) {
            return Json {
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
            return Json {
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
            return Json {
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
            return Json {
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
            return Json {
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
            return Json {
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
            return Json {
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
            return Json {
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
            return Json {
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
            return Json {
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
            for (auto i = std::size_t {}; i < entries.size(); ++i) {
                out.push_back(layout_runtime_json(entries[i], i));
            }
            return out;
        }

        [[nodiscard]] Json audio_events_json(std::span<const AudioEvent> events) {
            auto out = Json::array();
            for (auto i = std::size_t {}; i < events.size(); ++i) {
                const auto &event = events[i];
                out.push_back(Json {
                    {"index", i},
                    {"kind", audio_event_kind_name(event.kind)},
                    {"name", event.name},
                    {"fade_frames", event.fade_frames},
                    {"state", event.state},
                    {"change_frames", event.change_frames},
                    {"delay_frames", event.delay_frames},
                    {"frame_index", event.frame_index},
                });
            }
            return out;
        }

        [[nodiscard]] Json effect_texture_metadata_json(const smgpc::render::effects::JpcTextureMetadata &texture) {
            return Json {
                {"index", texture.index},
                {"name", texture.name},
                {"width", texture.width},
                {"height", texture.height},
                {"format", texture_format_name(texture.format)},
                {"format_raw", static_cast<std::uint32_t>(texture.format)},
                {"wrap_s", texture.wrap_s},
                {"wrap_t", texture.wrap_t},
                {"min_filter", texture.min_filter},
                {"mag_filter", texture.mag_filter},
            };
        }

        [[nodiscard]] Json effect_texture_metadata_json(std::span<const smgpc::render::effects::JpcTextureMetadata> textures) {
            auto out = Json::array();
            for (const auto &texture : textures) {
                out.push_back(effect_texture_metadata_json(texture));
            }
            return out;
        }

        [[nodiscard]] Json effect_resource_block_tags_json(const smgpc::render::effects::JpcResourceMetadata *resource) {
            auto out = Json::array();
            if (resource == nullptr) {
                return out;
            }
            for (const auto &tag : resource->block_tags) {
                out.push_back(tag);
            }
            return out;
        }

        [[nodiscard]] Json effect_dynamics_json(const smgpc::render::effects::JpcResourceMetadata *resource) {
            if (resource == nullptr || !resource->dynamics.has_value()) {
                return nullptr;
            }

            const auto &dynamics = *resource->dynamics;
            return Json {
                {"flags", dynamics.flags},
                {"volume_type", dynamics.volume_type},
                {"fixed_density", dynamics.fixed_density},
                {"fixed_interval", dynamics.fixed_interval},
                {"inherit_scale", dynamics.inherit_scale},
                {"follow_emitter", dynamics.follow_emitter},
                {"follow_emitter_child", dynamics.follow_emitter_child},
                {"rate", dynamics.rate},
                {"rate_random", dynamics.rate_random},
                {"lifetime_random", dynamics.lifetime_random},
                {"start_frame", dynamics.start_frame},
                {"max_frame", dynamics.max_frame},
                {"lifetime", dynamics.lifetime},
                {"volume_size", dynamics.volume_size},
                {"div_number", dynamics.div_number},
                {"rate_step", dynamics.rate_step},
            };
        }

        [[nodiscard]] Json effect_base_shape_json(const smgpc::render::effects::JpcResourceMetadata *resource) {
            if (resource == nullptr || !resource->base_shape.has_value()) {
                return nullptr;
            }

            const auto &base_shape = *resource->base_shape;
            return Json {
                {"flags", base_shape.flags},
                {"shape_type", base_shape.shape_type},
                {"direction_type", base_shape.direction_type},
                {"rotation_type", base_shape.rotation_type},
                {"base_plane_type", base_shape.base_plane_type},
                {"base_size_x", base_shape.base_size_x},
                {"base_size_y", base_shape.base_size_y},
                {"texture_flags", base_shape.texture_flags},
                {"texture_count", base_shape.texture_count},
                {"texture_slot", base_shape.texture_slot},
                {"texture_coordinate_animation", base_shape.texture_coordinate_animation},
                {"blend_mode_config", base_shape.blend_mode_config},
                {"alpha_compare_config", base_shape.alpha_compare_config},
                {"alpha_ref0", base_shape.alpha_ref0},
                {"alpha_ref1", base_shape.alpha_ref1},
                {"z_mode_config", base_shape.z_mode_config},
                {"prm_color", Json::array({base_shape.prm_color[0U], base_shape.prm_color[1U], base_shape.prm_color[2U], base_shape.prm_color[3U]})},
                {"env_color", Json::array({base_shape.env_color[0U], base_shape.env_color[1U], base_shape.env_color[2U], base_shape.env_color[3U]})},
            };
        }

        [[nodiscard]] Json effect_child_shape_json(const smgpc::render::effects::JpcResourceMetadata *resource) {
            if (resource == nullptr || !resource->child_shape.has_value()) {
                return nullptr;
            }

            const auto &child_shape = *resource->child_shape;
            return Json {
                {"flags", child_shape.flags},
                {"shape_type", child_shape.shape_type},
                {"direction_type", child_shape.direction_type},
                {"rotation_type", child_shape.rotation_type},
                {"base_plane_type", child_shape.base_plane_type},
                {"position_random", child_shape.position_random},
                {"base_velocity", child_shape.base_velocity},
                {"base_velocity_random", child_shape.base_velocity_random},
                {"velocity_inherit_rate", child_shape.velocity_inherit_rate},
                {"gravity", child_shape.gravity},
                {"scale_x", child_shape.scale_x},
                {"scale_y", child_shape.scale_y},
                {"inherit_scale", child_shape.inherit_scale},
                {"inherit_alpha", child_shape.inherit_alpha},
                {"inherit_rgb", child_shape.inherit_rgb},
                {"prm_color", Json::array({child_shape.prm_color[0U], child_shape.prm_color[1U], child_shape.prm_color[2U], child_shape.prm_color[3U]})},
                {"env_color", Json::array({child_shape.env_color[0U], child_shape.env_color[1U], child_shape.env_color[2U], child_shape.env_color[3U]})},
                {"timing", child_shape.timing},
                {"lifetime", child_shape.lifetime},
                {"rate", child_shape.rate},
                {"step", child_shape.step},
                {"texture_slot", child_shape.texture_slot},
                {"rotation_speed", child_shape.rotation_speed},
            };
        }

        [[nodiscard]] Json effect_key_blocks_json(const smgpc::render::effects::JpcResourceMetadata *resource) {
            auto out = Json::array();
            if (resource == nullptr) {
                return out;
            }

            for (const auto &key_block : resource->key_blocks) {
                auto keys = Json::array();
                for (const auto &key : key_block.keys) {
                    keys.push_back(Json {
                        {"time", key.time},
                        {"value", key.value},
                        {"tangent_in", key.tangent_in},
                        {"tangent_out", key.tangent_out},
                    });
                }
                out.push_back(Json {
                    {"id", key_block.id},
                    {"loop", key_block.loop},
                    {"key_count", key_block.keys.size()},
                    {"keys", std::move(keys)},
                });
            }
            return out;
        }

        [[nodiscard]] Json resolved_effect_resources_json(std::span<const smgpc::render::effects::ResolvedEffectResource> resources) {
            auto out = Json::array();
            for (const auto &resource : resources) {
                out.push_back(Json {
                    {"requested_name", resource.requested_name},
                    {"particle_name", resource.particle_name},
                    {"user_index", resource.user_index},
                    {"auto_effect_group_name", resource.auto_effect_group_name},
                    {"auto_effect_unique_name", resource.auto_effect_unique_name},
                    {"auto_effect_parent_name", resource.auto_effect_parent_name},
                    {"auto_effect_joint_name", resource.auto_effect_joint_name},
                    {"auto_effect_draw_order", resource.auto_effect_draw_order},
                    {"field_block_count", resource.resource != nullptr ? resource.resource->field_block_count : 0U},
                    {"key_block_count", resource.resource != nullptr ? resource.resource->key_block_count : 0U},
                    {"texture_reference_count", resource.resource != nullptr ? resource.resource->texture_reference_count : 0U},
                    {"block_tags", effect_resource_block_tags_json(resource.resource)},
                    {"dynamics", effect_dynamics_json(resource.resource)},
                    {"base_shape", effect_base_shape_json(resource.resource)},
                    {"child_shape", effect_child_shape_json(resource.resource)},
                    {"key_blocks", effect_key_blocks_json(resource.resource)},
                    {"textures", effect_texture_metadata_json(resource.textures)},
                });
            }
            return out;
        }

        [[nodiscard]] Json effect_keeper_registration_json(const EffectKeeperRegistration &keeper) {
            return Json {
                {"host_kind", effect_keeper_host_kind_name(keeper.host_kind)},
                {"host_name", keeper.host_name},
                {"resource_group_name", keeper.resource_group_name},
                {"requested_capacity", keeper.requested_capacity},
                {"sort_enabled", keeper.sort_enabled},
                {"frame_index", keeper.frame_index},
            };
        }

        [[nodiscard]] Json effect_keeper_registration_json(const std::optional<EffectKeeperRegistration> &keeper) {
            return keeper.has_value() ? effect_keeper_registration_json(*keeper) : Json(nullptr);
        }

        [[nodiscard]] Json effect_host_binding_json(const EffectHostBinding &binding) {
            return Json {
                {"host_kind", effect_keeper_host_kind_name(binding.host_kind)},
                {"source", effect_host_binding_source_name(binding.source)},
                {"host_name", binding.host_name},
                {"matrix", float_array_json(binding.matrix)},
                {"translation", Json::array({binding.translation[0U], binding.translation[1U], binding.translation[2U]})},
                {"host_dead", binding.host_dead},
                {"frame_index", binding.frame_index},
            };
        }

        [[nodiscard]] Json effect_host_binding_json(const std::optional<EffectHostBinding> &binding) {
            return binding.has_value() ? effect_host_binding_json(*binding) : Json(nullptr);
        }

        [[nodiscard]] Json registered_effect_keepers_json(const EffectService &effects) {
            auto out = Json::array();
            const auto keepers = effects.registered_keepers();
            for (auto i = std::size_t {}; i < keepers.size(); ++i) {
                auto keeper = effect_keeper_registration_json(keepers[i]);
                keeper["index"] = i;
                out.push_back(std::move(keeper));
            }
            return out;
        }

        [[nodiscard]] Json effect_events_json(std::span<const EffectEvent> events) {
            auto out = Json::array();
            for (auto i = std::size_t {}; i < events.size(); ++i) {
                const auto &event = events[i];
                out.push_back(Json {
                    {"index", i},
                    {"kind", effect_event_kind_name(event.kind)},
                    {"actor_name", event.actor_name},
                    {"effect_name", event.effect_name},
                    {"frame_index", event.frame_index},
                    {"keeper", effect_keeper_registration_json(event.keeper)},
                    {"resolved_resources", resolved_effect_resources_json(event.resolved_resources)},
                });
            }
            return out;
        }

        [[nodiscard]] Json effect_particle_instance_json(const JpcEffectParticleInstance &particle, std::size_t index) {
            return Json {
                {"index", index},
                {"id", particle.id},
                {"age", particle.age},
                {"lifetime", particle.lifetime},
                {"child", particle.child},
                {"position", Json::array({particle.x, particle.y, particle.z})},
                {"velocity", Json::array({particle.velocity_x, particle.velocity_y, particle.velocity_z})},
                {"momentum", particle.momentum},
                {"scale", Json::array({particle.scale_x, particle.scale_y})},
                {"alpha", particle.alpha},
            };
        }

        [[nodiscard]] Json effect_particle_instances_json(std::span<const JpcEffectParticleInstance> particles) {
            auto out = Json::array();
            for (auto i = std::size_t {}; i < particles.size(); ++i) {
                out.push_back(effect_particle_instance_json(particles[i], i));
            }
            return out;
        }

        [[nodiscard]] Json effect_emitter_instance_json(const JpcEffectEmitterInstance &emitter, std::size_t index) {
            return Json {
                {"index", index},
                {"user_index", emitter.user_index},
                {"particle_name", emitter.particle_name},
                {"start_frame_index", emitter.start_frame_index},
                {"next_update_frame_index", emitter.next_update_frame_index},
                {"random_seed", emitter.random_seed},
                {"fractional_emit_count", emitter.fractional_emit_count},
                {"rate_step_timer", emitter.rate_step_timer},
                {"first_emit", emitter.first_emit},
                {"rate_step_emit", emitter.rate_step_emit},
                {"next_particle_id", emitter.next_particle_id},
                {"live_particle_count", emitter.particles.size()},
                {"particles", effect_particle_instances_json(emitter.particles)},
            };
        }

        [[nodiscard]] Json effect_emitter_instances_json(std::span<const JpcEffectEmitterInstance> emitters) {
            auto out = Json::array();
            for (auto i = std::size_t {}; i < emitters.size(); ++i) {
                out.push_back(effect_emitter_instance_json(emitters[i], i));
            }
            return out;
        }

        [[nodiscard]] Json active_effect_instance_json(const ActiveEffectInstance &active, std::size_t index) {
            return Json {
                {"index", index},
                {"actor_name", active.actor_name},
                {"effect_name", active.effect_name},
                {"start_frame_index", active.start_frame_index},
                {"keeper", effect_keeper_registration_json(active.keeper)},
                {"host_binding", effect_host_binding_json(active.host_binding)},
                {"resolved_resources", resolved_effect_resources_json(active.resolved_resources)},
                {"emitter_count", active.emitters.size()},
                {"emitters", effect_emitter_instances_json(active.emitters)},
            };
        }

        [[nodiscard]] Json active_effect_instances_json(std::span<const ActiveEffectInstance> active_effects) {
            auto out = Json::array();
            for (auto i = std::size_t {}; i < active_effects.size(); ++i) {
                out.push_back(active_effect_instance_json(active_effects[i], i));
            }
            return out;
        }

        [[nodiscard]] Json wipe_events_json(std::span<const WipeEvent> events) {
            auto out = Json::array();
            for (auto i = std::size_t {}; i < events.size(); ++i) {
                const auto &event = events[i];
                out.push_back(Json {
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
            return Json {
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

        [[nodiscard]] Json image_effect_events_json(std::span<const ImageEffectControlEvent> events) {
            auto out = Json::array();
            for (auto i = std::size_t {}; i < events.size(); ++i) {
                const auto &event = events[i];
                out.push_back(Json {
                    {"index", i},
                    {"kind", image_effect_control_kind_name(event.kind)},
                    {"frame_index", event.frame_index},
                });
            }
            return out;
        }

        [[nodiscard]] Json image_effect_service_json(const ImageEffectService &image_effects) {
            return Json {
                {"forced_off", image_effects.is_forced_off()},
                {"control_auto", image_effects.is_control_auto()},
                {"events", image_effect_events_json(image_effects.events())},
            };
        }

        [[nodiscard]] Json camera_shake_request_events_json(std::span<const CameraSystemService::ShakeRequestEvent> events) {
            auto out = Json::array();
            for (auto i = std::size_t {}; i < events.size(); ++i) {
                const auto &event = events[i];
                out.push_back(Json {
                    {"index", i},
                    {"kind", camera_shake_request_kind_name(event.kind)},
                    {"frame_index", event.frame_index},
                });
            }
            return out;
        }

        [[nodiscard]] Json rumble_request_events_json(std::span<const RumbleRequestEvent> events) {
            auto out = Json::array();
            for (auto i = std::size_t {}; i < events.size(); ++i) {
                const auto &event = events[i];
                out.push_back(Json {
                    {"index", i},
                    {"kind", rumble_request_kind_name(event.kind)},
                    {"channel", event.channel},
                    {"frame_index", event.frame_index},
                });
            }
            return out;
        }

        [[nodiscard]] Json sequence_request_events_json(std::span<const SequenceRequestEvent> events) {
            auto out = Json::array();
            for (auto i = std::size_t {}; i < events.size(); ++i) {
                const auto &event = events[i];
                out.push_back(Json {
                    {"index", i},
                    {"kind", sequence_request_kind_name(event.kind)},
                    {"frame_index", event.frame_index},
                });
            }
            return out;
        }

        [[nodiscard]] Json semantic_trace_event_json(const RuntimeContext::SemanticTraceEvent &event) {
            return Json {
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
            return Json {
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
            for (auto i = std::size_t {}; i < entries.size(); ++i) {
                const auto &entry = entries[i];
                out.push_back(Json {
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
            case RflOperationKind::Persist:
                return "persist";
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
            case RflOperationKind::SetExpression:
                return "set_expression";
            case RflOperationKind::MakeIcon:
                return "make_icon";
            case RflOperationKind::DrawModel:
                return "draw_model";
            case RflOperationKind::MiiSelectPage:
                return "mii_select_page";
            }
            return "unknown";
        }

        [[nodiscard]] Json rfl_trace_json(std::span<const RflOperationTrace> entries) {
            auto out = Json::array();
            for (auto i = std::size_t {}; i < entries.size(); ++i) {
                const auto &entry = entries[i];
                out.push_back(Json {
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
                    {"fallback_used", entry.fallback_used},
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
            return Json {
                {"nand_bound", status.nand_bound},
                {"db_present", status.db_present},
                {"fallback_used", status.fallback_used},
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

        [[nodiscard]] Json save_slot_json(const SaveDataService::SlotState &slot) {
            return Json {
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
            for (auto light_index = std::size_t {}; light_index < entries.size(); ++light_index) {
                const auto &light = entries[light_index];
                if (!light.loaded) {
                    continue;
                }
                out.push_back(Json {
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
            return Json {
                {"index", index},
                {"requested_path", entry.requested_path},
                {"resolved_path", entry.resolved_path},
                {"byte_count", entry.byte_count},
            };
        }

        [[nodiscard]] Json dvd_file_read_traces_json(std::span<const DvdFileReadTrace> entries) {
            auto out = Json::array();
            for (auto i = std::size_t {}; i < entries.size(); ++i) {
                out.push_back(dvd_file_read_trace_json(entries[i], i));
            }
            return out;
        }

        [[nodiscard]] Json dvd_archive_load_trace_json(const DvdArchiveLoadTrace &entry, std::size_t index) {
            return Json {
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
            for (auto i = std::size_t {}; i < entries.size(); ++i) {
                out.push_back(dvd_archive_load_trace_json(entries[i], i));
            }
            return out;
        }

        [[nodiscard]] Json runtime_services_json(const RuntimeContext &runtime) {
            return Json {
                {"dvd",
                 Json {
                     {"root", runtime.dvd().root().generic_string()},
                     {"cached_archive_count", runtime.dvd().cached_archive_count()},
                     {"file_reads", dvd_file_read_traces_json(runtime.dvd().file_read_trace())},
                     {"archive_loads", dvd_archive_load_traces_json(runtime.dvd().archive_load_trace())},
                 }},
                {"rfl",
                 Json {
                     {"initialized", runtime.rfl().is_initialized()},
                     {"error", runtime.rfl().has_error()},
                     {"db_status", rfl_db_status_json(runtime.rfl().db_status())},
                     {"valid_mii_count", runtime.rfl().valid_miis().size()},
                     {"valid_miis", rfl_miis_json(runtime.rfl().valid_miis())},
                     {"trace", rfl_trace_json(runtime.rfl().trace())},
                 }},
                {"save",
                 Json {
                     {"file_count", runtime.save_data().file_count()},
                     {"host_directory",
                      runtime.save_data().host_directory().has_value() ? Json(runtime.save_data().host_directory()->string()) : Json(nullptr)},
                     {"sys_config",
                      Json {
                          {"time_announced", runtime.save_data().sys_config_time_announced()},
                          {"time_sent", runtime.save_data().sys_config_time_sent()},
                          {"sent_bytes", runtime.save_data().sys_config_sent_bytes()},
                      }},
                     {"slot_count", runtime.save_data().slot_states().size()},
                     {"slots", save_slots_json(runtime.save_data().slot_states())},
                 }},
                {"scene_lights",
                 Json {
                     {"loaded_mask", runtime.scene_lights().loaded_mask()},
                     {"lights", scene_lights_json(runtime.scene_lights())},
                 }},
                {"wipe",
                 Json {
                     {"scene", wipe_service_json(runtime.scene_wipe())},
                     {"system", wipe_service_json(runtime.system_wipe())},
                 }},
                {"image_effects", image_effect_service_json(runtime.image_effects())},
                {"camera",
                 Json {
                     {"active_programmable_camera",
                      runtime.camera_system().active_programmable_camera_name().has_value() ? Json(std::string(*runtime.camera_system().active_programmable_camera_name())) : Json(nullptr)},
                     {"reset_camera_man_count", runtime.camera_system().reset_camera_man_count()},
                     {"normal_shake_request_count", runtime.camera_system().normal_shake_request_count()},
                     {"camera_director_pause_count", runtime.camera_system().camera_director_pause_count()},
                     {"programmable_camera_declare_count", runtime.camera_system().programmable_camera_declare_count()},
                     {"programmable_camera_start_count", runtime.camera_system().programmable_camera_start_count()},
                     {"programmable_camera_end_count", runtime.camera_system().programmable_camera_end_count()},
                     {"programmable_camera_param_count", runtime.camera_system().programmable_camera_param_count()},
                     {"programmable_camera_fovy_count", runtime.camera_system().programmable_camera_fovy_count()},
                     {"shake_events", camera_shake_request_events_json(runtime.camera_system().shake_request_events())},
                 }},
                {"rumble",
                 Json {
                     {"events", rumble_request_events_json(runtime.rumble().events())},
                 }},
                {"sequence_requests",
                 Json {
                     {"change_stage_in_game_after_loading_game_data",
                      runtime.sequence_requests().is_change_stage_in_game_after_loading_game_data_requested()},
                     {"events", sequence_request_events_json(runtime.sequence_requests().events())},
                 }},
            };
        }

        [[nodiscard]] Json gx_blend_json(const render::GxBlendMode2D &blend) {
            return Json {
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
            return Json {
                {"enabled", alpha_compare.enabled},
                {"comp0", alpha_compare.comp0},
                {"ref0", alpha_compare.ref0},
                {"op", alpha_compare.op},
                {"comp1", alpha_compare.comp1},
                {"ref1", alpha_compare.ref1},
            };
        }

        [[nodiscard]] Json gx_color_channel_control_json(const smgpc::render::GXColorChannelControlState &control) {
            return Json {
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

        [[nodiscard]] Json gx_color_channels_json(const smgpc::render::J3dRendererPacketState &state) {
            auto out = Json::array();
            for (auto channel = std::size_t {}; channel < state.color_channel_material_colors.size(); ++channel) {
                out.push_back(Json {
                    {"index", channel},
                    {"material_color", color_json(state.color_channel_material_colors[channel])},
                    {"ambient_color", color_json(state.color_channel_ambient_colors[channel])},
                    {"color_control", gx_color_channel_control_json(state.color_channel_controls[channel])},
                    {"alpha_control", gx_color_channel_control_json(state.alpha_channel_controls[channel])},
                });
            }
            return out;
        }

        [[nodiscard]] Json gx_lights_json(const smgpc::render::J3dRendererPacketState &state) {
            auto out = Json::array();
            for (auto light_index = std::size_t {}; light_index < state.lights.size(); ++light_index) {
                const auto &light = state.lights[light_index];
                if (!light.loaded) {
                    continue;
                }
                out.push_back(Json {
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

        [[nodiscard]] Json gx_texture_binding_json(const smgpc::render::J3dRendererTextureState &texture) {
            return Json {
                {"slot", texture.slot},
                {"texture_index", texture.texture_index},
                {"name", texture.name},
                {"has_source_texture", texture.has_source_texture},
                {"width", texture.width},
                {"height", texture.height},
                {"format", texture_format_name(texture.format)},
                {"format_raw", static_cast<std::uint32_t>(texture.format)},
                {"has_sampler_metadata", texture.has_sampler_metadata},
                {"transparency", texture.transparency},
                {"wrap_s", texture.wrap_s},
                {"wrap_t", texture.wrap_t},
                {"palette_format", texture.palette_format},
                {"palette_entry_count", texture.palette_entry_count},
                {"palette_data_offset", texture.palette_data_offset},
                {"mipmap", texture.mipmap},
                {"do_edge_lod", texture.do_edge_lod},
                {"bias_clamp", texture.bias_clamp},
                {"max_anisotropy", texture.max_anisotropy},
                {"min_filter", texture.min_filter},
                {"mag_filter", texture.mag_filter},
                {"min_lod_raw", texture.min_lod},
                {"max_lod_raw", texture.max_lod},
                {"min_lod", static_cast<float>(texture.min_lod) / 8.0F},
                {"max_lod", static_cast<float>(texture.max_lod) / 8.0F},
                {"image_count", texture.image_count},
                {"lod_bias_raw", texture.lod_bias},
                {"lod_bias", static_cast<float>(texture.lod_bias) / 100.0F},
                {"image_data_offset", texture.image_data_offset},
                {"host_texture_handle", texture.host_handle.is_valid() ? Json(static_cast<std::uint64_t>(texture.host_handle.debug_id())) : Json(nullptr)},
            };
        }

        [[nodiscard]] Json gx_texture_bindings_json(std::span<const smgpc::render::J3dRendererTextureState> textures) {
            auto out = Json::array();
            for (const auto &texture : textures) {
                out.push_back(gx_texture_binding_json(texture));
            }
            return out;
        }

        [[nodiscard]] Json render_texture_binding_json(const RuntimeContext::RenderTextureBindingTrace &texture) {
            return Json {
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

        [[nodiscard]] Json effect_texture_binding_json(const EffectTextureBindingTrace &texture) {
            return Json {
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

        [[nodiscard]] Json effect_texture_bindings_json(const EffectTextureBindingTrace &texture) {
            auto out = Json::array();
            out.push_back(effect_texture_binding_json(texture));
            return out;
        }

        [[nodiscard]] std::uint32_t used_textures_mask(std::span<const smgpc::render::J3dRendererTextureState> textures) {
            auto mask = std::uint32_t {};
            for (const auto &texture : textures) {
                if (texture.slot < 8U) {
                    mask |= 1U << texture.slot;
                }
            }
            return mask;
        }

        [[nodiscard]] std::uint32_t used_textures_mask(std::span<const RuntimeContext::RenderTextureBindingTrace> textures) {
            auto mask = std::uint32_t {};
            for (const auto &texture : textures) {
                if (texture.slot < 8U) {
                    mask |= 1U << texture.slot;
                }
            }
            return mask;
        }

        [[nodiscard]] std::uint32_t used_textures_mask(const EffectTextureBindingTrace &texture) {
            return texture.slot < 8U ? (1U << texture.slot) : 0U;
        }

        [[nodiscard]] Json used_texture_slots_json(std::span<const smgpc::render::J3dRendererTextureState> textures) {
            auto slots = std::array<bool, 8U>{};
            for (const auto &texture : textures) {
                if (texture.slot < slots.size()) {
                    slots[texture.slot] = true;
                }
            }

            auto out = Json::array();
            for (auto slot = std::size_t {}; slot < slots.size(); ++slot) {
                if (slots[slot]) {
                    out.push_back(slot);
                }
            }
            return out;
        }

        [[nodiscard]] Json used_texture_slots_json(std::span<const RuntimeContext::RenderTextureBindingTrace> textures) {
            auto slots = std::array<bool, 8U>{};
            for (const auto &texture : textures) {
                if (texture.slot < slots.size()) {
                    slots[texture.slot] = true;
                }
            }

            auto out = Json::array();
            for (auto slot = std::size_t {}; slot < slots.size(); ++slot) {
                if (slots[slot]) {
                    out.push_back(slot);
                }
            }
            return out;
        }

        [[nodiscard]] Json used_texture_slots_json(const EffectTextureBindingTrace &texture) {
            auto out = Json::array();
            if (texture.slot < 8U) {
                out.push_back(texture.slot);
            }
            return out;
        }

        [[nodiscard]] Json gx_tex_coord_gen_json(const smgpc::render::GXTexCoordGenState &gen) {
            return Json {
                {"slot", gen.slot},
                {"type", gen.type},
                {"source", gen.source},
                {"matrix", gen.matrix},
            };
        }

        [[nodiscard]] Json gx_tex_coord_gens_json(std::span<const smgpc::render::GXTexCoordGenState> gens) {
            auto out = Json::array();
            for (const auto &gen : gens) {
                out.push_back(gx_tex_coord_gen_json(gen));
            }
            return out;
        }

        [[nodiscard]] Json gx_tex_coord_scale_json(std::uint8_t slot, const smgpc::render::GXTexCoordScaleState &scale) {
            return Json {
                {"slot", slot},
                {"s_scale_minus_1", scale.s_scale_minus_1},
                {"t_scale_minus_1", scale.t_scale_minus_1},
                {"s_scale", static_cast<std::uint32_t>(scale.s_scale_minus_1) + 1U},
                {"t_scale", static_cast<std::uint32_t>(scale.t_scale_minus_1) + 1U},
                {"s_bias", scale.s_bias},
                {"t_bias", scale.t_bias},
                {"s_wrap", scale.s_wrap},
                {"t_wrap", scale.t_wrap},
                {"line_offset", scale.line_offset},
                {"point_offset", scale.point_offset},
                {"s_loaded", scale.s_loaded},
                {"t_loaded", scale.t_loaded},
                {"derived_from_texture", scale.derived_from_texture},
                {"raw_s", scale.raw_s},
                {"raw_t", scale.raw_t},
            };
        }

        [[nodiscard]] Json gx_tex_coord_scales_json(const std::array<smgpc::render::GXTexCoordScaleState, 8U> &scales) {
            auto out = Json::array();
            for (auto slot = 0U; slot < scales.size(); ++slot) {
                const auto &scale = scales[slot];
                if (!scale.s_loaded && !scale.t_loaded && !scale.derived_from_texture) {
                    continue;
                }

                out.push_back(gx_tex_coord_scale_json(static_cast<std::uint8_t>(slot), scale));
            }
            return out;
        }

        [[nodiscard]] Json gx_su_line_point_json(const smgpc::render::GXSULinePointState &state) {
            return Json {
                {"loaded", state.loaded},
                {"line_size", state.line_size},
                {"point_size", state.point_size},
                {"line_tex_offset", state.line_tex_offset},
                {"point_tex_offset", state.point_tex_offset},
                {"field_mode", state.field_mode},
                {"raw", state.raw},
            };
        }

        [[nodiscard]] Json gx_tex_matrix_json(const smgpc::render::GXTexMatrixState &matrix) {
            return Json {
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

        [[nodiscard]] Json gx_tex_matrices_json(std::span<const smgpc::render::GXTexMatrixState> matrices) {
            auto out = Json::array();
            for (const auto &matrix : matrices) {
                out.push_back(gx_tex_matrix_json(matrix));
            }
            return out;
        }

        [[nodiscard]] Json gx_tev_order_json(const smgpc::render::GXTevOrderState &order) {
            return Json {
                {"stage", order.stage},
                {"tex_coord", order.tex_coord},
                {"tex_map", order.tex_map},
                {"color_channel", order.color_channel},
            };
        }

        [[nodiscard]] Json gx_tev_orders_json(std::span<const smgpc::render::GXTevOrderState> orders) {
            auto out = Json::array();
            for (const auto &order : orders) {
                out.push_back(gx_tev_order_json(order));
            }
            return out;
        }

        [[nodiscard]] Json gx_tev_stage_json(const smgpc::render::GXTevStageState &stage) {
            return Json {
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

        [[nodiscard]] Json gx_tev_stages_json(std::span<const smgpc::render::GXTevStageState> stages) {
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

        [[nodiscard]] Json tev_k_colors_json(const std::array<smgpc::render::GXColorValue, 4U> &colors) {
            auto out = Json::array();
            for (const auto &color : colors) {
                out.push_back(color_json(color));
            }
            return out;
        }

        [[nodiscard]] Json gx_z_mode_json(const smgpc::render::GXZModeState &z_mode) {
            return Json {
                {"enabled", z_mode.enabled},
                {"compare_enable", z_mode.compare_enable},
                {"function", z_mode.function},
                {"update_enable", z_mode.update_enable},
            };
        }

        [[nodiscard]] Json gx_fog_json(const smgpc::render::GXFogState &fog) {
            return Json {
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

        [[nodiscard]] Json gx_indirect_texture_orders_json(std::span<const smgpc::render::GXIndirectTextureOrderState> orders) {
            auto out = Json::array();
            for (const auto &order : orders) {
                out.push_back(Json {
                    {"stage", order.stage},
                    {"tex_map", order.tex_map},
                    {"tex_coord", order.tex_coord},
                });
            }
            return out;
        }

        [[nodiscard]] Json gx_indirect_texture_matrices_json(std::span<const smgpc::render::GXIndirectTextureMatrixState> matrices) {
            auto out = Json::array();
            for (const auto &matrix : matrices) {
                out.push_back(Json {
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

        [[nodiscard]] Json gx_indirect_texture_coord_scales_json(std::span<const smgpc::render::GXIndirectTextureCoordScaleState> scales) {
            auto out = Json::array();
            for (const auto &scale : scales) {
                out.push_back(Json {
                    {"stage", scale.stage},
                    {"scale_s", scale.scale_s},
                    {"scale_t", scale.scale_t},
                });
            }
            return out;
        }

        [[nodiscard]] Json gx_indirect_tev_stages_json(std::span<const smgpc::render::GXIndirectTevStageState> stages) {
            auto out = Json::array();
            for (const auto &stage : stages) {
                out.push_back(Json {
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

        [[nodiscard]] Json gx_indirect_json(const smgpc::render::GXIndirectState &indirect) {
            return Json {
                {"stage_count", indirect.stage_count},
                {"texture_orders", gx_indirect_texture_orders_json(indirect.texture_orders)},
                {"texture_matrices", gx_indirect_texture_matrices_json(indirect.texture_matrices)},
                {"texture_coord_scales", gx_indirect_texture_coord_scales_json(indirect.texture_coord_scales)},
                {"tev_stages", gx_indirect_tev_stages_json(indirect.tev_stages)},
            };
        }

        [[nodiscard]] Json gx_register_load_json(const smgpc::render::GXRegisterLoadState &load, std::size_t index) {
            return Json {
                {"index", index},
                {"space", register_space_name(load.space)},
                {"byte_offset", load.byte_offset},
                {"address", load.address},
                {"count", load.count},
                {"value", load.value},
            };
        }

        [[nodiscard]] Json gx_register_loads_json(std::span<const smgpc::render::GXRegisterLoadState> loads) {
            auto out = Json::array();
            for (auto i = std::size_t {}; i < loads.size(); ++i) {
                out.push_back(gx_register_load_json(loads[i], i));
            }
            return out;
        }

        [[nodiscard]] Json j3d_packet_trace_json(const RuntimeContext::J3dRuntimePacketTrace &packet, std::size_t index) {
            const auto &state = packet.state;
            return Json {
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
                {"material_mode", state.material_mode},
                {"draw_buffer_opaque", state.draw_buffer_opaque},
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
                {"packet_mode_reason", state.packet_mode_reason},
                {"packet_mode_fallback", state.packet_mode_fallback},
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
                {"tex_coord_scales", gx_tex_coord_scales_json(state.tex_coord_scales)},
                {"su_line_point", gx_su_line_point_json(state.su_line_point)},
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

        [[nodiscard]] Json layout_packet_trace_json(const RuntimeContext::LayoutRuntimePacketTrace &packet, std::size_t index) {
            return Json {
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

        [[nodiscard]] Json effect_packet_trace_json(const EffectDrawPacketTrace &packet, std::size_t index) {
            const auto source_triangle_count = packet.primitive_type == "triangle_strip" ?
                                                   (packet.index_count >= 3U ? packet.index_count - 2U : 0U) :
                                                   packet.index_count / 3U;
            return Json {
                {"index", index},
                {"model_name", packet.actor_name},
                {"actor_name", packet.actor_name},
                {"effect_name", packet.effect_name},
                {"particle_name", packet.particle_name},
                {"user_index", packet.user_index},
                {"frame_index", packet.frame_index},
                {"draw_pass", "effect"},
                {"render_pass", "effect"},
                {"view_id", 0},
                {"material_name", packet.particle_name},
                {"draw_type", packet.draw_type},
                {"draw_order", packet.draw_order},
                {"packet_mode", "JpcBillboard2D"},
                {"primitive_type", packet.primitive_type},
                {"source_vertex_count", packet.vertex_count},
                {"source_triangle_count", source_triangle_count},
                {"num_indices", packet.index_count},
                {"particle_id", packet.particle_id},
                {"particle_age", packet.particle_age},
                {"particle_lifetime", packet.particle_lifetime},
                {"host_binding_found", packet.host_binding_found},
                {"host_binding_source", packet.host_binding_source},
                {"host_translation", Json::array({packet.host_translation[0U], packet.host_translation[1U], packet.host_translation[2U]})},
                {"particle_position", Json::array({packet.particle_x, packet.particle_y, packet.particle_z})},
                {"particle_scale", Json::array({packet.particle_scale_x, packet.particle_scale_y})},
                {"particle_alpha", packet.particle_alpha},
                {"live_particle_count", packet.live_particle_count},
                {"child_particle", packet.child_particle},
                {"texgen_count", 1U},
                {"color_channel_count", packet.color_channel_count},
                {"active_tev_stage_count", 1U},
                {"tev_stage_count", 1U},
                {"indirect_stage_count", 0U},
                {"cull_mode", "None"},
                {"used_textures_mask", used_textures_mask(packet.texture)},
                {"used_texture_slots", used_texture_slots_json(packet.texture)},
                {"texture_bindings", effect_texture_bindings_json(packet.texture)},
                {"alpha_compare_enabled", packet.alpha_compare_enabled},
                {"blend_enabled", packet.blend_enabled},
            };
        }

        [[nodiscard]] Json runtime_render_packets_json(const RuntimeContext &runtime) {
            auto out = Json::array();
            auto index = std::size_t {};
            for (const auto &packet : runtime.j3d_packet_trace()) {
                out.push_back(j3d_packet_trace_json(packet, index));
                ++index;
            }
            for (const auto &packet : runtime.layout_packet_trace()) {
                out.push_back(layout_packet_trace_json(packet, index));
                ++index;
            }
            for (const auto &packet : runtime.effects().draw_packets()) {
                out.push_back(effect_packet_trace_json(packet, index));
                ++index;
            }
            return out;
        }

    }  // namespace

    dump::Json runtime_parity_trace_json(const render::FrameContext &frame_context, const RuntimeContext &runtime) {
        auto trace = Json {
            {"schema", "smgpc-runtime-parity-trace-v1"},
            {"frame",
             Json {
                 {"index", frame_context.frame_index},
                 {"runtime_index", runtime.frame_index()},
                 {"time_seconds", frame_context.frame_time_seconds},
                 {"delta_seconds", frame_context.frame_delta_seconds},
                 {"framebuffer", Json {{"width", frame_context.framebuffer.width}, {"height", frame_context.framebuffer.height}}},
                 {"viewport", frame_viewport_json(frame_context)},
                 {"scissor", frame_scissor_json(frame_context)},
                 {"has_focus", frame_context.has_focus},
                 {"is_minimized", frame_context.is_minimized},
             }},
            {"camera_pose", runtime.last_camera_pose().has_value() ? camera_pose_json(*runtime.last_camera_pose()) : Json(nullptr)},
            {"host_input", host_input_json(runtime.host_input_trace(), frame_context)},
            {"wpad0", wpad_channel_json(runtime.wpad(), WPAD_CHAN0, frame_context)},
            {"audio",
             Json {
                 {"stage_bgm", runtime.current_stage_bgm_name()},
                 {"stage_bgm_active", !runtime.current_stage_bgm_name().empty()},
                 {"stage_bgm_prepared", runtime.is_stage_bgm_prepared()},
                 {"stage_bgm_unlocked", runtime.audio().is_stage_bgm_unlocked()},
                 {"stage_bgm_state", runtime.audio().stage_bgm_state()},
                 {"stage_bgm_state_change_frames", runtime.audio().stage_bgm_state_change_frames()},
                 {"events", audio_events_json(runtime.audio().events())},
             }},
            {"effects",
             Json {
                 {"registered_keepers", registered_effect_keepers_json(runtime.effects())},
                 {"active_effects", active_effect_instances_json(runtime.effects().active_effect_instances())},
                 {"events", effect_events_json(runtime.effects().events())},
             }},
            {"runtime_services", runtime_services_json(runtime)},
            {"scene_snapshot", scene_entries_json(runtime.scheduler().snapshot())},
            {"scene_trace", scene_entries_json(runtime.scheduler().last_execution_trace())},
            {"scene_messages", scene_messages_json(runtime.scheduler().message_trace())},
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
