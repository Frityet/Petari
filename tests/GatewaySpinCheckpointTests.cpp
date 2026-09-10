#include "compat/MarioCameraTarget.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/NPC/DemoRabbit.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "camera/StageStartCamera.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/AudioFacadeCompat.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/GameDataHolderCompat.hpp"
#include "compat/GameDataSession.hpp"
#include "compat/InformationMessageCompat.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/AuthoredPlacementInstantiator.hpp"
#include "scene/GatewayDemoScene.hpp"
#include "scene/GatewaySpinCheckpoint.hpp"
#include "scene/NameObjChildOwner.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
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
#include <vector>

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

    struct DemoRabbitExpectation final {
        s32 source_row = -1;
        s32 cast_id = -1;
        s32 message_id = -1;
        s32 common_path_id = -1;
    };

    constexpr auto cDemoRabbitExpectations = std::array{
        DemoRabbitExpectation{.source_row = 8, .cast_id = 0, .message_id = 0,
                              .common_path_id = 0},
        DemoRabbitExpectation{.source_row = 9, .cast_id = 1, .message_id = -1,
                              .common_path_id = -1},
        DemoRabbitExpectation{.source_row = 10, .cast_id = 2, .message_id = -1,
                              .common_path_id = -1},
    };

    [[nodiscard]] std::vector<const DemoRabbit *> require_authored_demo_rabbits(
        const smgpc::scene::GatewayDemoScene &scene) {
        const auto &entries = scene.authored_placement_report().entries;
        require(std::ranges::count_if(entries, [](const auto &entry) {
                    return entry.placement != nullptr &&
                           entry.placement->object_name == "DemoRabbit";
                }) == cDemoRabbitExpectations.size(),
                "Gateway must retain exactly three authored DemoRabbit placements");

        auto identities = std::vector<const DemoRabbit *>{};
        identities.reserve(cDemoRabbitExpectations.size());
        for (const auto &expected : cDemoRabbitExpectations) {
            const auto found = std::ranges::find_if(entries, [&](const auto &entry) {
                const auto *placement = entry.placement;
                return placement != nullptr &&
                       placement->object_name == "DemoRabbit" &&
                       placement->zone_name == "HeavensDoorMysteriousZone" &&
                       placement->zone_id == 5 &&
                       placement->layer_name == "layera" &&
                       placement->table_path == "jmp/placement/layera/objinfo" &&
                       placement->jmap_entry_index == expected.source_row &&
                       placement->demo_group_id == 0 &&
                       placement->cast_id == expected.cast_id &&
                       placement->message_id == expected.message_id &&
                       placement->common_path_id == expected.common_path_id;
            });
            auto *rabbit = found != entries.end()
                               ? dynamic_cast<DemoRabbit *>(found->actor)
                               : nullptr;
            require(rabbit != nullptr &&
                        found->outcome == smgpc::scene::AuthoredPlacementOutcome::
                                              InitializedAfterPlacement &&
                        found->actor_name == std::optional<std::string>{"デモウサギ"} &&
                        rabbit->getName() != nullptr &&
                        std::string_view(rabbit->getName()) == "デモウサギ",
                    "an exact placement-owned DemoRabbit identity is absent");
            identities.push_back(rabbit);
        }
        return identities;
    }

    [[nodiscard]] std::vector<const DemoRabbit *> snapshot_demo_rabbits() {
        auto identities = std::vector<const DemoRabbit *>{};
        for (const auto *object : smgpc::compat::snapshot_name_obj_runtime_objects()) {
            if (const auto *rabbit =
                    dynamic_cast<const DemoRabbit *>(object);
                rabbit != nullptr) {
                identities.push_back(rabbit);
            }
        }
        return identities;
    }

    void require_same_rabbit_identities(
        const std::vector<const DemoRabbit *> &actual,
        const std::vector<const DemoRabbit *> &expected,
        std::string_view message) {
        require(actual.size() == expected.size() &&
                    std::ranges::is_permutation(actual, expected),
                message);
    }

    class CheckpointMarioOwner final {
    public:
        explicit CheckpointMarioOwner(
            smgpc::runtime::PlayerSystemService& player_system)
            : _player_system(&player_system),
              _domain(smgpc::scene::current_scene_allocation_domain()) {
            if (!_domain) {
                throw std::logic_error("Mario construction requires the actual scene allocation domain");
            }
            try {
                _actor = dynamic_cast<MarioActor*>(_objects.capture_construction_children([&] {
                    const smgpc::compat::JkrAllocationScope game(_domain);
                    return createNameObj<MarioActor>("MarioActor");
                }));
                if (_actor == nullptr) {
                    throw std::runtime_error("the typed Gateway MarioActor creator returned the wrong object");
                }
                _player_system->attach_actor(
                    *_actor,
                    smgpc::runtime::PlayerActorBridge{
                        .read_element_mode = &CheckpointMarioOwner::read_element_mode,
                        .read_base_matrix = &CheckpointMarioOwner::read_base_matrix,
                        .read_up_vector = &CheckpointMarioOwner::read_up_vector,
                        .read_front_vector = &CheckpointMarioOwner::read_front_vector,
                        .read_side_vector = &CheckpointMarioOwner::read_side_vector,
                        .read_nerve_change_enabled = [](const LiveActor& actor) {
                            return static_cast<const MarioActor&>(actor).isEnableNerveChange();
                        },
                        .read_center_position = [](LiveActor& actor) {
                            return &static_cast<MarioActor&>(actor)._2A0;
                        },
                    });
            } catch (...) {
                retire();
                throw;
            }
        }

        CheckpointMarioOwner(const CheckpointMarioOwner&) = delete;
        CheckpointMarioOwner& operator=(const CheckpointMarioOwner&) = delete;

        ~CheckpointMarioOwner() {
            retire();
        }

        void initialize(const JMapInfoIter& placement) {
            _objects.capture_construction_children([&] {
                const smgpc::compat::JkrAllocationScope game(_domain);
                _actor->init(placement);
            });
        }

        [[nodiscard]] MarioActor& actor() const {
            return *_actor;
        }

        void retire() noexcept {
            const smgpc::compat::JkrHostAllocationScope host;
            if (_actor != nullptr && _player_system != nullptr &&
                _player_system->attached_actor() == _actor) {
                _player_system->detach_actor(_actor);
            }
            if (auto* holder = MR::getMarioHolder();
                holder != nullptr && holder->getMarioActor() == _actor) {
                holder->setMarioActor(nullptr);
            }
            _objects.clear();
            _actor = nullptr;
        }
    private:
        static s32 read_element_mode(const LiveActor& actor) {
            const auto* mario = dynamic_cast<const MarioActor*>(&actor);
            if (mario == nullptr) {
                throw std::logic_error(
                    "Gateway player element-mode bridge requires MarioActor");
            }
            return mario->mPlayerMode;
        }

        static MtxPtr read_base_matrix(const LiveActor& actor) {
            return smgpc::compat::mario_camera_base_matrix(static_cast<const MarioActor&>(actor));
        }

        static void read_up_vector(const LiveActor& actor, TVec3f* out) {
            static_cast<const MarioActor&>(actor).getUpVec(out);
        }

        static void read_front_vector(const LiveActor& actor, TVec3f* out) {
            static_cast<const MarioActor&>(actor).getFrontVec(out);
        }

        static void read_side_vector(const LiveActor& actor, TVec3f* out) {
            static_cast<const MarioActor&>(actor).getSideVec(out);
        }

        smgpc::runtime::PlayerSystemService* _player_system = nullptr;
        std::shared_ptr<smgpc::compat::JkrAllocationDomain> _domain;
        smgpc::scene::NameObjChildOwner _objects;
        MarioActor* _actor = nullptr;
    };

    void test_real_gateway_spin_unlock_checkpoint() {
        constexpr auto cPromptFrame = std::uint64_t{1762U};
        constexpr auto cLastGuardedFrame = cPromptFrame + 30U;
        constexpr auto cAcceptFrame = cLastGuardedFrame + 2U;
        const auto scripted_input = ScopedEnvironmentVariable{
            "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT",
            std::to_string(cLastGuardedFrame) + ":A;" +
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
        auto resource_runtime = smgpc::resource::GameResourceRuntime{};
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window, resource_runtime);
        runtime.initialize_scenario_catalog(resource_runtime);
        runtime.initialize_particle_resources(resource_runtime);
        runtime.set_current_stage_name("HeavensDoorGalaxy");
        const auto scene_renderer_context =
            smgpc::render::ScopedAuroraRendererContext(renderer);
        auto logical_audio = smgpc::runtime::AudioEventService{};
        const auto audio_binding =
            smgpc::compat::ScopedAudioEventServiceOverride{logical_audio};

        auto game_data_session = smgpc::compat::GameDataSession{1U};
        require(smgpc::compat::game_data::holder_story_progress(
                    game_data_session.holder()) == 5U &&
                    GameDataFunction::getCurrentGameDataHolder() ==
                        &game_data_session.holder() &&
                    GameDataFunction::getSceneStartGameDataHolder() ==
                        &game_data_session.holder(),
                "the spin proof must begin with one active selected-file progress-5 holder");
        auto scene = smgpc::scene::GatewayDemoScene(runtime.dvd());
        const auto camera = smgpc::camera::resolve_stage_start_camera(
            runtime.dvd(), scene.start_info());
        require(camera.status ==
                        smgpc::camera::StageStartCameraResolveStatus::Resolved &&
                    camera.camera.has_value(),
                "checkpoint must resolve exact Gateway StartInfo camera 78");
        runtime.set_freecam_enabled(false);
        runtime.refresh_scene_camera_pose();
        auto mario_owner = [&] {
            const auto phase = smgpc::scene::SceneInitializationScope(
                SceneInitializeState_PlacementPlayer);
            return CheckpointMarioOwner{runtime.player_system()};
        }();
        auto* mario = &mario_owner.actor();

        auto placement_lease =
            smgpc::scene::GatewayDemoScene::PlacementLease{};
        auto checkpoint = std::unique_ptr<smgpc::scene::GatewaySpinCheckpoint>{};
        auto placement_demo_rabbits = std::vector<const DemoRabbit *>{};
        auto placement_lod_baseline = std::size_t{};
        auto placement_lod_count = std::size_t{};
        // Resource uploads need a renderer frame, but original scene execution
        // starts only after the complete placement postpass allocates its lists.
        (void)renderer.begin_frame();
        {
            const auto renderer_context =
                smgpc::render::ScopedAuroraRendererContext(renderer);
            {
                const auto phase = smgpc::scene::SceneInitializationScope(
                    SceneInitializeState_PlacementPlayer);
                mario_owner.initialize(scene.player_start_iter());
            }
            placement_lod_baseline =
                smgpc::compat::actor_lod_ctrl_runtime_state_count();
            placement_lease = scene.finalize_placements(*mario);
            // Establish this fixture's locked precondition through the original
            // API. Retail GameSequenceProgress::startScene does this before spin
            // entitlement; this bounded scene does not yet own that sequence.
            require(!GameDataFunction::isPassedStoryEvent("スピン権利"),
                    "the spin fixture precondition requires an unearned entitlement");
            MR::setPlayerSwingPermission(false);
            placement_demo_rabbits = require_authored_demo_rabbits(scene);
            placement_lod_count =
                smgpc::compat::actor_lod_ctrl_runtime_state_count();
            require(placement_lod_count >=
                        placement_lod_baseline +
                            placement_demo_rabbits.size(),
                    "the three placement-owned DemoRabbit actors must each add an owned LOD controller");
            require_same_rabbit_identities(
                snapshot_demo_rabbits(), placement_demo_rabbits,
                "Gateway placement finalization must publish exactly its three authored DemoRabbit actors");
            const auto mario_position_before_checkpoint = mario->mPosition;
            GameDataFunction::followStoryEventByName("チコガイドデモ終了");
            require(smgpc::compat::game_data::holder_story_progress(
                        game_data_session.holder()) == 10U,
                    "the dev spin caller must advance its selected-file holder from progress 5 to 10");
            checkpoint =
                std::make_unique<smgpc::scene::GatewaySpinCheckpoint>(
                    runtime.dvd(), scene.placements(), scene.general_positions(),
                    game_data_session.holder(), runtime.player_system(),
                    runtime.scene_wipe(), *mario);
            require(mario->mPosition.epsilonEquals(
                        mario_position_before_checkpoint, 0.001F),
                    "checkpoint construction must not teleport the external Mario actor");
            require_same_rabbit_identities(
                snapshot_demo_rabbits(), placement_demo_rabbits,
                "checkpoint construction must borrow the three placement rabbits without creating extras");
            require(smgpc::compat::actor_lod_ctrl_runtime_state_count() ==
                        placement_lod_count,
                    "checkpoint construction must not duplicate placement-owned rabbit LOD controllers");
        }

        renderer.end_frame(runtime.wii_video().render_mode());

        require(&checkpoint->demo_runtime() == &scene.demo_runtime(),
                "Gateway spin checkpoint installed a nested DemoDirector instead of borrowing the scene owner");

        require(smgpc::compat::current_information_message() != nullptr &&
                    MR::isExistSceneObj(SceneObj_InformationObserver),
                "checkpoint must eagerly construct InformationMessage and InformationObserver before frame one");
        require(runtime.player_system().attached_actor() == mario &&
                    !mario->_EEB,
                "real Mario must start attached with spin entitlement locked");
        require(&checkpoint->checkpoint_game_data() ==
                        &game_data_session.holder() &&
                    GameDataFunction::getCurrentGameDataHolder() ==
                        &game_data_session.holder() &&
                    GameDataFunction::getSceneStartGameDataHolder() ==
                        &game_data_session.holder() &&
                    GameDataFunction::isPassedStoryEvent("チコガイドデモ終了") &&
                    !GameDataFunction::isPassedStoryEvent("スピン権利") &&
                    smgpc::compat::game_data::holder_story_progress(
                        checkpoint->checkpoint_game_data()) == 10U,
                "checkpoint must borrow the caller's exact post-high-tower story progress 10");

        const auto &start = scene.start_info();
        require_near(mario->mPosition.x, start.world_position[0U], 0.001F,
                     "checkpoint-preserved StartInfo X");
        require_near(mario->mPosition.y, start.world_position[1U], 0.001F,
                     "checkpoint-preserved StartInfo Y");
        require_near(mario->mPosition.z, start.world_position[2U], 0.001F,
                     "checkpoint-preserved StartInfo Z");

        const auto guide_index = checkpoint->demo_runtime().find_definition(5, 0);
        require(guide_index.has_value() &&
                    std::string_view(checkpoint->tico_cast().getName()) == "チコ" &&
                    checkpoint->demo_runtime().membership_count(
                        &checkpoint->tico_cast()) == 1U &&
                    checkpoint->demo_runtime().cast_id(
                        &checkpoint->tico_cast(), *guide_index) == 0,
                    "typed チコ cast must retain TicoBaby row-13 membership and CastId 0");

        // Test-only stimulus for the state machine: production leaves Mario at
        // StartInfo and requires ordinary movement into this authored radius.
        const auto &rosetta = find_placement(scene.placements(), "Rosetta", 12);
        MR::setPlayerPos(TVec3f{rosetta.translation[0U], rosetta.translation[1U],
                               rosetta.translation[2U]});
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
                    MR::isOffPlayerControl() &&
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
                    MR::isOffPlayerControl(),
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
                    !mario->_EEB,
                "part-22 first step must invoke exact spin explanation once and pause before granting entitlement");

        for (auto frame = cPromptFrame + 1U; frame < cLastGuardedFrame; ++frame) {
            run_frame(frame);
        }
        require(guide_definition->sheet.is_paused() &&
                    !GameDataFunction::isPassedStoryEvent("スピン権利") &&
                    checkpoint->evidence().prompt_delegate_calls == 1U,
                "InformationObserver must retain its 30-frame guard without duplicate explanation");

        // The checkpoint appears the observer after the original executor's
        // connection phase, so its first display execution is the next frame.
        // Execution 30 reaches mDisplayFrame == 0 and rejects a fresh A edge.
        run_frame(cLastGuardedFrame);
        require(MR::testCorePadTriggerA(WPAD_CHAN0) &&
                    guide_definition->sheet.is_paused() &&
                    !GameDataFunction::isPassedStoryEvent("スピン権利") &&
                    !mario->_EEB &&
                    checkpoint->evidence().prompt_delegate_calls == 1U,
                "the 30th display execution must reject fresh A at the final guarded frame");
        run_frame(cLastGuardedFrame + 1U);
        require(!MR::testCorePadTriggerA(WPAD_CHAN0) &&
                    guide_definition->sheet.is_paused() &&
                    !GameDataFunction::isPassedStoryEvent("スピン権利") &&
                    !mario->_EEB,
                "guard expiry without a fresh edge must keep the prompt and entitlement locked");

        run_frame(cAcceptFrame);
        require(MR::testCorePadTriggerA(WPAD_CHAN0) &&
                    !guide_definition->sheet.is_paused() &&
                    checkpoint->demo_runtime().is_time_keep_active() &&
                    GameDataFunction::isPassedStoryEvent("スピン権利") &&
                    smgpc::compat::game_data::holder_story_progress(
                        checkpoint->checkpoint_game_data()) == 15U &&
                    mario->_EEB &&
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
                    smgpc::compat::game_data::holder_story_progress(
                        game_data_session.holder()) == 15U,
                "the prompt proof must retain the same selected-file holder after granting spin");
        require_same_rabbit_identities(
            snapshot_demo_rabbits(), placement_demo_rabbits,
            "the three placement-owned DemoRabbit actors must persist through the complete checkpoint");

        std::cout << "[proof] disc=" << disc_path.string()
                  << ";selected_file_progress=5->10->15"
                  << ";checkpoint=borrowed_progress10"
                  << ";trigger_radius=500;fade_handoff=90"
                  << ";demo_start_row=15;pre_prompt_ticks=1670"
                  << ";prompt_row=22;information_guard=30"
                  << ";spin_progress=15;placement_demo_rabbits=3\n";

        checkpoint.reset();
        require_same_rabbit_identities(
            snapshot_demo_rabbits(), placement_demo_rabbits,
            "destroying the borrowed checkpoint must not retire placement-owned rabbits");
        require(smgpc::compat::actor_lod_ctrl_runtime_state_count() ==
                    placement_lod_count,
                "destroying the borrowed checkpoint must preserve placement-owned rabbit LOD controllers");
        placement_lease.reset();
        const auto &retired_report = scene.authored_placement_report();
        require(scene.state() == smgpc::scene::GatewayDemoSceneState::Retired &&
                    snapshot_demo_rabbits().empty() &&
                    smgpc::compat::actor_lod_ctrl_runtime_state_count() ==
                        placement_lod_baseline &&
                    std::ranges::count_if(
                        retired_report.entries, [](const auto &entry) {
                            return entry.placement != nullptr &&
                                   entry.placement->object_name == "DemoRabbit" &&
                                   entry.actor == nullptr &&
                                   entry.outcome ==
                                       smgpc::scene::AuthoredPlacementOutcome::Destroyed;
                        }) == cDemoRabbitExpectations.size(),
                "Gateway lease retirement must destroy all three placement rabbits before the external Mario owner");
        require(GameDataFunction::getCurrentGameDataHolder() ==
                        &game_data_session.holder() &&
                    GameDataFunction::getSceneStartGameDataHolder() ==
                        &game_data_session.holder() &&
                    smgpc::compat::game_data::holder_story_progress(
                        game_data_session.holder()) == 15U,
                "placement retirement must leave the caller-owned selected-file holder active at progress 15");
        mario_owner.retire();
        require(runtime.player_system().attached_actor() == nullptr &&
                    MR::getMarioHolder()->getMarioActor() == nullptr,
                "the actual Mario owner must detach the player and clear its holder before scene teardown");
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
