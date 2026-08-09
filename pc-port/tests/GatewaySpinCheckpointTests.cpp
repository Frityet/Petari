#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "camera/StageStartCamera.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/AudioFacadeCompat.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/GameDataHolderCompat.hpp"
#include "compat/InformationMessageCompat.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/GatewayDemoScene.hpp"
#include "scene/GatewaySpinCheckpoint.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_near(float actual, float expected, float tolerance,
                      std::string_view message) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > tolerance) {
            throw std::runtime_error(std::string(message) + ";actual=" +
                                     std::to_string(actual) + ";expected=" +
                                     std::to_string(expected));
        }
    }

    class ScopedEnvironmentVariable final {
    public:
        ScopedEnvironmentVariable(std::string name, std::string value)
            : _name(std::move(name)) {
            if (const auto *previous = std::getenv(_name.c_str());
                previous != nullptr) {
                _previous = std::string(previous);
            }
            require(::setenv(_name.c_str(), value.c_str(), 1) == 0,
                    "could not install the deterministic input script");
        }

        ~ScopedEnvironmentVariable() {
            if (_previous.has_value()) {
                (void)::setenv(_name.c_str(), _previous->c_str(), 1);
            } else {
                (void)::unsetenv(_name.c_str());
            }
        }

        ScopedEnvironmentVariable(const ScopedEnvironmentVariable &) = delete;
        ScopedEnvironmentVariable &operator=(const ScopedEnvironmentVariable &) =
            delete;

    private:
        std::string _name;
        std::optional<std::string> _previous;
    };

    [[nodiscard]] std::filesystem::path require_real_disc() {
        if (const auto *configured = std::getenv("SMGPC_REAL_DISC");
            configured != nullptr && configured[0] != '\0') {
            const auto path = std::filesystem::path(configured);
            require(std::filesystem::is_regular_file(path),
                    "SMGPC_REAL_DISC must name the real RMGK01 image");
            return path;
        }

        auto error = std::error_code{};
        auto directory = std::filesystem::current_path(error);
        require(!error, "spin checkpoint proof requires a readable directory");
        while (true) {
            for (const auto name : {"RMGK01.iso", "RMGK01.wbfs"}) {
                const auto candidate = directory / name;
                if (std::filesystem::is_regular_file(candidate, error) && !error) {
                    return candidate;
                }
                error.clear();
            }
            const auto parent = directory.parent_path();
            if (parent.empty() || parent == directory) {
                break;
            }
            directory = parent;
        }
        throw std::runtime_error(
            "spin checkpoint proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

    void set_mario_swing_permission(LiveActor &actor, bool permitted) {
        auto *mario = dynamic_cast<MarioActor *>(&actor);
        require(mario != nullptr,
                "spin entitlement bridge requires the real MarioActor");
        mario->_EEB = permitted;
    }

    [[nodiscard]] const smgpc::scene::StagePlacementObject &find_placement(
        std::span<const smgpc::scene::StagePlacementObject> placements,
        std::string_view name, int row) {
        const auto found = std::ranges::find_if(placements, [&](const auto &entry) {
            return entry.object_name == name && entry.zone_id == 5 &&
                   entry.table_path == "jmp/placement/layera/objinfo" &&
                   entry.jmap_entry_index == row;
        });
        require(found != placements.end(), "required exact placement is absent");
        return *found;
    }

    [[nodiscard]] const smgpc::scene::StageGeneralPos &find_position(
        std::span<const smgpc::scene::StageGeneralPos> positions,
        std::string_view name) {
        const auto found = std::ranges::find_if(positions, [&](const auto &entry) {
            return entry.name == name && entry.zone_id == 5;
        });
        require(found != positions.end(), "required exact GeneralPos is absent");
        return *found;
    }

    void test_real_gateway_spin_unlock_checkpoint() {
        constexpr auto cPromptFrame = std::uint64_t{1762U};
        constexpr auto cAcceptFrame = std::uint64_t{1792U};
        const auto scripted_input = ScopedEnvironmentVariable{
            "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT",
            std::to_string(cAcceptFrame) + ":A"};
        const auto disc_path = require_real_disc();

        aurora_dvd_close();
        const auto disc_string = disc_path.string();
        require(aurora_dvd_open(disc_string.c_str()),
                "the selected RMGK01 image must open through Aurora DVD");
        struct DiscCloseGuard final {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } disc_close_guard;
        DVDInit();

        auto logger = smgpc::logging::create_default_logger();
        auto window = smgpc::render::AuroraWindow({
            .width = 640,
            .height = 456,
            .title = "SMG PC Gateway spin checkpoint proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
        runtime.set_current_stage_name("HeavensDoorGalaxy");
        auto logical_audio = smgpc::runtime::AudioEventService{};
        const auto audio_binding =
            smgpc::compat::ScopedAudioEventServiceOverride{logical_audio};

        auto scene = smgpc::scene::GatewayDemoScene(runtime.dvd());
        const auto camera = smgpc::camera::resolve_stage_start_camera(
            runtime.dvd(), scene.start_info());
        require(camera.status ==
                        smgpc::camera::StageStartCameraResolveStatus::Resolved &&
                    camera.camera.has_value(),
                "checkpoint must resolve exact Gateway StartInfo camera 78");
        runtime.camera_system().set_game_camera_pose(camera.camera->calculation.pose);
        runtime.set_scene_camera_pose(camera.camera->calculation.pose);
        auto created = std::unique_ptr<NameObj>{
            createNameObj<MarioActor>("MarioActor")};
        auto *mario = dynamic_cast<MarioActor *>(created.get());
        require(mario != nullptr, "typed creator must construct real MarioActor");
        runtime.player_system().attach_actor(
            *mario, smgpc::runtime::PlayerActorEntitlementBridge{
                        .set_swing_permission = &set_mario_swing_permission,
                    });

        auto checkpoint = std::unique_ptr<smgpc::scene::GatewaySpinCheckpoint>{};
        {
            const auto renderer_context =
                smgpc::render::ScopedAuroraRendererContext(renderer);
            mario->init(scene.player_start_iter());
            mario->initAfterPlacement();
            checkpoint =
                std::make_unique<smgpc::scene::GatewaySpinCheckpoint>(
                    runtime.dvd(), scene.placements(), scene.general_positions(),
                    runtime.player_system(), runtime.scene_wipe(), *mario);
        }

        require(smgpc::compat::current_information_message() != nullptr &&
                    MR::isExistSceneObj(SceneObj_InformationObserver),
                "checkpoint must eagerly construct InformationMessage and InformationObserver before frame one");
        require(runtime.player_system().attached_actor() == mario &&
                    !runtime.player_system().is_swing_permitted() && !mario->_EEB,
                "real Mario must start attached with spin entitlement locked");
        require(GameDataFunction::isPassedStoryEvent("チコガイドデモ終了") &&
                    !GameDataFunction::isPassedStoryEvent("スピン権利") &&
                    smgpc::compat::game_data::holder_story_progress(
                        checkpoint->checkpoint_game_data()) == 10U,
                "checkpoint must seed exact post-high-tower story progress 10");

        const auto &mario_pos2 =
            find_position(scene.general_positions(), "MarioDemoPos2");
        require_near(mario->mPosition.x, mario_pos2.world_position[0U], 0.001F,
                     "checkpoint MarioDemoPos2 X");
        require_near(mario->mPosition.y, mario_pos2.world_position[1U], 0.001F,
                     "checkpoint MarioDemoPos2 Y");
        require_near(mario->mPosition.z, mario_pos2.world_position[2U], 0.001F,
                     "checkpoint MarioDemoPos2 Z");

        const auto guide_index = checkpoint->demo_runtime().find_definition(5, 0);
        require(guide_index.has_value() &&
                    std::string_view(checkpoint->tico_cast().getName()) == "チコ" &&
                    checkpoint->demo_runtime().membership_count(
                        &checkpoint->tico_cast()) == 1U &&
                    checkpoint->demo_runtime().cast_id(
                        &checkpoint->tico_cast(), *guide_index) == 0,
                "typed チコ cast must retain TicoBaby row-13 membership and CastId 0");

        const auto &rosetta = find_placement(scene.placements(), "Rosetta", 12);
        mario->mPosition.set(rosetta.translation[0U], rosetta.translation[1U],
                            rosetta.translation[2U]);
        mario->mVelocity.zero();
        runtime.player_system().synchronize_attached_actor();

        const auto run_frame = [&](std::uint64_t frame_index) {
            logical_audio.begin_frame(frame_index);
            const auto renderer_context =
                smgpc::render::ScopedAuroraRendererContext(renderer);
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = frame_index,
                .frame_time_seconds = static_cast<double>(frame_index) / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
            });
        };

        run_frame(1U);
        require(checkpoint->state() ==
                        smgpc::scene::GatewaySpinCheckpointState::FadeHandoff &&
                    !runtime.player_system().is_control_enabled() &&
                    checkpoint->evidence().fade_handoff_frames == 0U &&
                    !runtime.scene_wipe().events().empty() &&
                    runtime.scene_wipe().events().back().frame_count == 60,
                "500-unit Rosetta trigger must begin the retail fade/control handoff");

        for (auto frame = std::uint64_t{2U}; frame <= 90U; ++frame) {
            run_frame(frame);
        }
        require(checkpoint->state() ==
                        smgpc::scene::GatewaySpinCheckpointState::FadeHandoff &&
                    checkpoint->evidence().fade_handoff_frames == 89U &&
                    !checkpoint->demo_runtime().is_active(),
                "the checkpoint must not start the demo before fade frame 90");

        run_frame(91U);
        require(checkpoint->state() ==
                        smgpc::scene::GatewaySpinCheckpointState::SpinDemo &&
                    checkpoint->evidence().fade_handoff_frames == 90U &&
                    checkpoint->demo_runtime().is_active("チコガイドデモ") &&
                    checkpoint->demo_runtime().part_step(
                        "スピンゲット[デモ1]") == -1 &&
                    !runtime.player_system().is_control_enabled() &&
                    runtime.player_system().consume_reset_condition_request(),
                "fade frame 90 must hand control through exact part-15 MarioPuppetable start");

        for (auto frame = std::uint64_t{92U}; frame < cPromptFrame; ++frame) {
            run_frame(frame);
        }
        require(checkpoint->state() ==
                        smgpc::scene::GatewaySpinCheckpointState::SpinDemo &&
                    checkpoint->evidence().pre_prompt_demo_ticks == 1670U &&
                    checkpoint->evidence().prompt_delegate_calls == 0U &&
                    checkpoint->evidence()
                        .player_row_dispatched_to_mario_demo_pos4 &&
                    checkpoint->demo_runtime().part_step(
                        "スピンゲット[デモ4]") == 499,
                "real Player/Wipe keepers must complete exactly 1670 pre-prompt demo ticks");
        require(std::ranges::count_if(
                    runtime.scene_wipe().events(), [](const auto &event) {
                        return event.kind == smgpc::runtime::WipeEventKind::Open &&
                               event.name == "フェードワイプ" &&
                               event.frame_count == -1 && event.frame_index == 92U;
                    }) == 1,
                "part-15 first step must dispatch the exact authored fade-open Wipe row");

        run_frame(cPromptFrame);
        const auto *guide_definition =
            checkpoint->demo_runtime().definition(*guide_index);
        require(checkpoint->state() ==
                        smgpc::scene::GatewaySpinCheckpointState::PromptDelegated &&
                    checkpoint->evidence().prompt_delegate_calls == 1U &&
                    checkpoint->demo_runtime().is_part_active(
                        "スピンゲット[デモ5]") &&
                    checkpoint->demo_runtime().part_step(
                        "スピンゲット[デモ5]") == 0 &&
                    guide_definition != nullptr &&
                    guide_definition->sheet.is_paused() &&
                    !GameDataFunction::isPassedStoryEvent("スピン権利") &&
                    !runtime.player_system().is_swing_permitted() && !mario->_EEB,
                "part-22 first step must invoke exact spin explanation once and pause before granting entitlement");

        for (auto frame = cPromptFrame + 1U; frame < cAcceptFrame; ++frame) {
            run_frame(frame);
        }
        require(guide_definition->sheet.is_paused() &&
                    !GameDataFunction::isPassedStoryEvent("スピン権利") &&
                    checkpoint->evidence().prompt_delegate_calls == 1U,
                "InformationObserver must retain its 30-frame guard without duplicate explanation");

        run_frame(cAcceptFrame);
        require(!guide_definition->sheet.is_paused() &&
                    checkpoint->demo_runtime().is_time_keep_active() &&
                    GameDataFunction::isPassedStoryEvent("スピン権利") &&
                    smgpc::compat::game_data::holder_story_progress(
                        checkpoint->checkpoint_game_data()) == 15U &&
                    runtime.player_system().is_swing_permitted() && mario->_EEB &&
                    checkpoint->evidence().prompt_delegate_calls == 1U,
                "fresh A after the guard must let exact InformationObserver resume and grant spin access");
        require(std::ranges::count_if(
                    logical_audio.events(), [](const auto &event) {
                        return event.kind ==
                                   smgpc::runtime::AudioEventKind::SystemSoundStart &&
                               event.name == "SE_SY_TALK_OK";
                    }) == 1,
                "prompt dismissal must retain one logical SE_SY_TALK_OK request");

        run_frame(cAcceptFrame + 1U);
        require(checkpoint->evidence().prompt_delegate_calls == 1U &&
                    std::ranges::none_of(
                        smgpc::compat::snapshot_name_obj_runtime_objects(),
                        [](const auto *object) {
                            const auto name = object != nullptr &&
                                                      object->getName() != nullptr
                                                  ? std::string_view(object->getName())
                                                  : std::string_view{};
                            return name.find("Rabbit") != std::string_view::npos ||
                                   name.find("ウサギ") != std::string_view::npos;
                        }),
                "checkpoint must stop making route claims at spin access without rabbit actors");

        std::cout << "[proof] disc=" << disc_path.string()
                  << ";checkpoint=progress10"
                  << ";trigger_radius=500;fade_handoff=90"
                  << ";demo_start_row=15;pre_prompt_ticks=1670"
                  << ";prompt_row=22;information_guard=30"
                  << ";spin_progress=15;rabbit_route=absent\n";

        checkpoint.reset();
        runtime.player_system().detach_actor(mario);
        runtime.unregister_live_actor_model(*mario);
        MR::getMarioHolder()->setMarioActor(nullptr);
        created.reset();
    }

}  // namespace

int main() {
    try {
        test_real_gateway_spin_unlock_checkpoint();
        std::cout << "[ok] real Gateway spin-unlock checkpoint proof\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] Gateway spin-unlock checkpoint proof: "
                  << error.what() << '\n';
        return 1;
    }
}
