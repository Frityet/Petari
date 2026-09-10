#include "resource/TextEncoding.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/IconAButton.hpp"
#include "Game/Screen/InformationMessage.hpp"
#include "Game/Screen/InformationObserver.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "camera/StageStartCamera.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/AudioFacadeCompat.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/GameDataOwnership.hpp"
#include "compat/GameDataSession.hpp"
#include "compat/InformationMessageCompat.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/MarioCameraTarget.hpp"
#include "compat/StarPointerDepthOwnership.hpp"
#include "layout/LayoutHost.hpp"
#include "layout/LayoutRuntime.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/GatewayDemoScene.hpp"
#include "scene/NameObjChildOwner.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <aurora/dvd.h>
#include <aurora/wpad.hpp>
#include <dolphin/dvd.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    const auto cPromptPart = smgpc::resource::encode_cp932("スピンゲット[デモ5]");
    const auto cTriggerName = smgpc::resource::encode_cp932("チコ");

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    [[nodiscard]] std::filesystem::path require_real_disc() {
        if (const auto* configured = std::getenv("SMGPC_REAL_DISC");
            configured != nullptr && configured[0] != '\0') {
            const auto path = std::filesystem::path(configured);
            require(std::filesystem::is_regular_file(path),
                    "SMGPC_REAL_DISC must name the real RMGK01 image");
            return path;
        }

        auto error = std::error_code{};
        auto directory = std::filesystem::current_path(error);
        require(!error,
                "the InformationObserver proof requires a readable working directory");
        while (true) {
            for (const auto name : {"RMGK01.iso", "RMGK01.wbfs"}) {
                const auto candidate = directory / name;
                if (std::filesystem::is_regular_file(candidate, error) && !error) {
                    return candidate;
                }
                error.clear();
            }
            const auto parent = directory.parent_path();
            if (parent == directory || parent.empty()) {
                break;
            }
            directory = parent;
        }
        throw std::runtime_error(
            "the InformationObserver proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

    class SpinPromptTrigger final : public LiveActor {
    public:
        SpinPromptTrigger() : LiveActor(cTriggerName.c_str()) {
            MR::connectToSceneNpcMovement(this);
            // This nonvisual stimulus must run independently of the camera,
            // like the original InformationObserver it requests.
            MR::invalidateClipping(this);
            makeActorAppeared();
        }

        void movement() override {
            if (!_fired && MR::isDemoPartFirstStep(cPromptPart.c_str())) {
                _fired = true;
                MR::explainEnableToSpin(this);
            }
        }

        [[nodiscard]] bool fired() const {
            return _fired;
        }

    private:
        bool _fired = false;
    };

    class PromptMarioOwner final {
    public:
        explicit PromptMarioOwner(smgpc::runtime::PlayerSystemService& player_system)
            : _player_system(player_system),
              _domain(smgpc::scene::current_scene_allocation_domain()) {
            require(_domain != nullptr,
                    "the prompt's original Mario requires the active scene heap");
            try {
                _actor = static_cast<MarioActor*>(_objects.capture_construction_children([&] {
                    const auto game = smgpc::compat::JkrAllocationScope{_domain};
                    return createNameObj<MarioActor>("MarioActor");
                }));
                _player_system.attach_actor(*_actor, {
                    .read_element_mode = [](const LiveActor& actor) -> s32 {
                        return static_cast<const MarioActor&>(actor).mPlayerMode;
                    },
                    .read_base_matrix = [](const LiveActor& actor) {
                        return smgpc::compat::mario_camera_base_matrix(
                            static_cast<const MarioActor&>(actor));
                    },
                    .read_up_vector = [](const LiveActor& actor, TVec3f* out) {
                        static_cast<const MarioActor&>(actor).getUpVec(out);
                    },
                    .read_front_vector = [](const LiveActor& actor, TVec3f* out) {
                        static_cast<const MarioActor&>(actor).getFrontVec(out);
                    },
                    .read_side_vector = [](const LiveActor& actor, TVec3f* out) {
                        static_cast<const MarioActor&>(actor).getSideVec(out);
                    },
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

        ~PromptMarioOwner() {
            retire();
        }

        void initialize(const JMapInfoIter& placement) {
            _objects.capture_construction_children([&] {
                const auto game = smgpc::compat::JkrAllocationScope{_domain};
                _actor->init(placement);
            });
        }

        [[nodiscard]] MarioActor& actor() const {
            return *_actor;
        }

    private:
        void retire() noexcept {
            const auto host = smgpc::compat::JkrHostAllocationScope{};
            if (_actor != nullptr) {
                _player_system.detach_actor(_actor);
                if (auto* scene_holder = smgpc::scene::current_scene_obj_holder();
                    scene_holder != nullptr) {
                    auto* holder = static_cast<MarioHolder*>(
                        scene_holder->getObj(SceneObj_MarioHolder));
                    if (holder != nullptr && holder->getMarioActor() == _actor) {
                        holder->setMarioActor(nullptr);
                    }
                }
            }
            _objects.clear();
            _actor = nullptr;
        }

        smgpc::runtime::PlayerSystemService& _player_system;
        std::shared_ptr<smgpc::compat::JkrAllocationDomain> _domain;
        smgpc::scene::NameObjChildOwner _objects;
        MarioActor* _actor = nullptr;
    };

    void test_exact_timekeep_prompt_guard_and_fresh_a() {
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
            .title = "SMG PC InformationObserver proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto resource_runtime = smgpc::resource::GameResourceRuntime{};
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window, resource_runtime);
        runtime.initialize_scenario_catalog(resource_runtime);
        runtime.initialize_particle_resources(resource_runtime);
        runtime.set_current_stage_name("HeavensDoorGalaxy");
        const auto renderer_context =
            smgpc::render::ScopedAuroraRendererContext(renderer);
        runtime.player_system().clear_stage_state();

        auto logical_audio = smgpc::runtime::AudioEventService{};
        const auto audio_binding =
            smgpc::compat::ScopedAudioEventServiceOverride{logical_audio};
        auto game_data_session = smgpc::compat::GameDataSession{1U, resource_runtime, runtime.retain_scenario_catalog()};
        game_data_session.holder().followStoryEventByName(smgpc::resource::encode_cp932("ピーチ城浮上後").c_str());
        game_data_session.store_scene_start();

        const auto baseline_objects =
            smgpc::compat::snapshot_name_obj_runtime_objects();
        auto expected_runtime_objects = baseline_objects;
        const auto* pointer_owner = smgpc::compat::try_star_pointer_depth();
        require(pointer_owner != nullptr,
                "the prompt fixture requires the actual runtime star-pointer owner");
#ifndef NDEBUG
        const auto baseline_scheduler_entries = runtime.scheduler().snapshot().size();
#endif
        {
            auto scene = smgpc::scene::GatewayDemoScene{runtime.dvd()};
            // First-scene startup lazily creates pointer layouts retained by
            // RuntimeContext. Capture only that owner's existing identities,
            // before constructing any prompt or player fixture children.
            require(pointer_owner->guidance() != nullptr,
                    "Gateway scene startup must initialize the original pointer layouts");
            for (auto* object : smgpc::compat::snapshot_name_obj_runtime_objects()) {
                if (smgpc::compat::name_obj_runtime_owner(object) == pointer_owner &&
                    std::ranges::find(expected_runtime_objects, object) == expected_runtime_objects.end()) {
                    expected_runtime_objects.push_back(object);
                }
            }
            const auto camera = smgpc::camera::resolve_stage_start_camera(
                runtime.dvd(), scene.start_info());
            require(camera.status ==
                            smgpc::camera::StageStartCameraResolveStatus::Resolved &&
                        camera.camera.has_value(),
                    "the InformationObserver proof could not resolve Gateway's exact start camera");
            runtime.camera_system().set_game_camera_pose(
                camera.camera->calculation.pose);
            runtime.set_scene_camera_pose(camera.camera->calculation.pose);
            auto player_owner = PromptMarioOwner{runtime.player_system()};
            auto& player = player_owner.actor();
            auto placement_lease = smgpc::scene::GatewayDemoScene::PlacementLease{};
            // Register every participant and request its initial connection
            // before finalization allocates the original execution lists.
            (void)renderer.begin_frame();
            player_owner.initialize(scene.player_start_iter());
            const auto information_registration_marker =
                smgpc::compat::mark_name_obj_runtime_registrations();
            auto information_message =
                smgpc::compat::InformationMessageBinding{};
            const auto information_registrations =
                smgpc::compat::snapshot_name_obj_runtime_objects_since(
                    information_registration_marker);
            require(information_registrations.size() == 2U &&
                        information_registrations.front() ==
                            &information_message.message() &&
                        dynamic_cast<InformationMessage*>(
                            information_registrations.front()) != nullptr &&
                        dynamic_cast<IconAButton*>(
                            information_registrations.back()) != nullptr &&
                        smgpc::compat::name_obj_runtime_owner(
                            information_registrations.front()) != nullptr &&
                        smgpc::compat::name_obj_runtime_owner(
                            information_registrations.front()) ==
                            smgpc::compat::name_obj_runtime_owner(
                                information_registrations.back()) &&
                        !smgpc::scene::
                            current_scene_obj_holder_binding_owns(
                                information_registrations.back()),
                    "InformationMessage and its raw-new IconAButton must share one explicit non-SceneObj owner");
            auto* observer = dynamic_cast<InformationObserver*>(
                MR::createSceneObj(SceneObj_InformationObserver));
            require(observer != nullptr && observer->mFlag.mIsDead,
                    "SceneObj_InformationObserver must create the exact initially-dead actor");
            require(MR::getLayoutMessageDirect("InformationObserverSpin") != nullptr,
                    "the spin prompt must resolve the real layout message");

            auto &demo = scene.demo_runtime();
            auto trigger = SpinPromptTrigger{};
            const auto tico = std::ranges::find_if(
                scene.placements(), [](const auto& placement) {
                    return placement.object_name == "Tico" &&
                           placement.zone_id == 5 &&
                           placement.demo_group_id == 0 &&
                           placement.cast_id == 0 &&
                           placement.table_path ==
                               "jmp/placement/layera/objinfo";
                });
            require(tico != scene.placements().end(),
                    "the exact Gateway group-0 Tico cast row must exist");
            require(MR::tryRegisterDemoCast(
                        &trigger,
                        JMapInfoIter(&tico->jmap_info, tico->jmap_entry_index)),
                    "the prompt trigger must be a real active-executor cast");
            placement_lease = scene.finalize_placements(player);
            renderer.end_frame();
            require(!GameDataFunction::isPassedStoryEvent(smgpc::resource::encode_cp932("スピン権利").c_str()),
                    "the prompt fixture must begin before the saved spin entitlement");
            // GameSequenceProgress::startScene applies this original API when
            // the story flag is absent. This fixture has no full sequence
            // startup owner, so establish its locked-spin precondition here.
            MR::setPlayerSwingPermission(false);
            require(runtime.player_system().attached_actor() == &player &&
                        MR::getMarioHolder()->getMarioActor() == &player &&
                        MR::getJ3DModel(&player) != nullptr &&
                        MR::getJ3DModelData(&player)->getJointNum() != 0U &&
                        !player._EEB && player._1C0 &&
                        player.mMario->mAirGravityVec.epsilonEquals(player._240, 0.0001F) &&
                        scene.state() == smgpc::scene::GatewayDemoSceneState::Active,
                    "the prompt proof must retain original Mario and its model/gravity postpass with the explicit swing-lock precondition");
            require(smgpc::compat::game_data::holder_story_progress(
                        game_data_session.holder()) == 5U &&
                        GameDataFunction::getCurrentGameDataHolder() ==
                            &game_data_session.holder() &&
                        GameDataFunction::getSceneStartGameDataHolder() ==
                            &game_data_session.scene_start_holder(),
                    "the explicit prompt proof must retain its selected-file progress-5 holder");
            GameDataFunction::followStoryEventByName(smgpc::resource::encode_cp932("チコガイドデモ終了").c_str());
            require(smgpc::compat::game_data::holder_story_progress(
                        game_data_session.holder()) == 10U,
                    "the explicit part-22 proof must advance the same selected-file holder from progress 5 to 10");
            MR::startTimeKeepDemo(&trigger, smgpc::resource::encode_cp932("チコガイドデモ").c_str(), cPromptPart.c_str());

            const auto guide_index = demo.find_definition(smgpc::resource::encode_cp932("チコガイドデモ"));
            require(guide_index.has_value(),
                    "the real TicoGuideDemo executor must exist");
            const auto* guide = demo.definition(*guide_index);
            require(guide != nullptr && guide->sheet.current_part_step() == -1,
                    "starting at part 22 must retain the pre-first-step sentinel");

            auto frame_index = std::uint64_t{};
            const auto registrations_before_prompt =
                runtime.scheduler().registration_marker();
            auto run_frame = [&](bool hold_a) {
                logical_audio.begin_frame(frame_index++);
                auto& wpad = runtime.wpad();
                wpad.begin_frame();
                wpad.set_button_mask(WPAD_CHAN0,
                                     hold_a ? WPAD_BUTTON_A : 0U);
                runtime.scheduler().execute_movement();
            };

            const auto require_prompt = [&](bool condition, std::string_view message) {
                if (!condition) {
                    std::cerr << "spin-prompt state: frame=" << frame_index
                              << ";trigger_fired=" << trigger.fired()
                              << ";trigger_clipped=" << static_cast<bool>(trigger.mFlag.mIsClipped)
                              << ";trigger_invalid_clipping=" << static_cast<bool>(trigger.mFlag.mIsInvalidClipping)
                              << ";trigger_movement_off=" << smgpc::compat::name_obj_is_suspended(&trigger)
                              << ";trigger_executor_index=" << trigger.mExecutorIdx
                              << ";observer_dead=" << static_cast<bool>(observer->mFlag.mIsDead)
                              << ";observer_nerve_step=" << observer->getNerveStep()
                              << ";message_dead=" << static_cast<bool>(information_message.message().mFlag.mIsDead)
                              << ";demo_step=" << guide->sheet.current_part_step().value_or(-999)
                              << ";demo_paused=" << guide->sheet.is_paused()
                              << ";trigger_a=" << runtime.wpad().is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A)
                              << ";spin_story=" << GameDataFunction::isPassedStoryEvent(smgpc::resource::encode_cp932("スピン権利").c_str())
                              << ";mario_swing=" << static_cast<bool>(player._EEB) << '\n';
                }
                require(condition, message);
            };

            // The NPC requests the observer's connection after this frame's
            // connect-requirement phase. Entry pauses the clock immediately;
            // its pending Disp nerve does not execute until the next frame.
            run_frame(true);
            require_prompt(trigger.fired() && !observer->mFlag.mIsDead &&
                        information_message.message().mFlag.mIsDead &&
                        observer->getNerveStep() == -1 &&
                        guide->sheet.current_part_step() == 0 &&
                        guide->sheet.is_paused() &&
                        !GameDataFunction::isPassedStoryEvent(smgpc::resource::encode_cp932("スピン権利").c_str()) &&
                        !player._EEB,
                    "part 22 step 0 must pause and request the prompt without executing its pending display nerve");

            // Display execution 1 starts the 30-frame guard. Carry the held A
            // through this frame; the timekeeper performs its paused 0->1
            // correction before the now-connected observer displays text.
            run_frame(true);
            require_prompt(!observer->mFlag.mIsDead &&
                        !information_message.message().mFlag.mIsDead &&
                        observer->getNerveStep() == 1 &&
                        guide->sheet.current_part_step() == 1 &&
                        guide->sheet.is_paused() &&
                        !GameDataFunction::isPassedStoryEvent(smgpc::resource::encode_cp932("スピン権利").c_str()) &&
                        !player._EEB,
                    "the first scheduled display must show text while keeping the paused clock and swing lock");
            require(runtime.scheduler().registration_marker() ==
                        registrations_before_prompt,
                    "showing the pre-created prompt must not register objects during movement");
#ifndef NDEBUG
            const auto& prompt_layout =
                smgpc::layout::require_layout_runtime(
                    &information_message.message(),
                    "Inspecting the spin prompt text");
            const auto text_rasters = prompt_layout.debugTextRasters({});
            const auto has_visible_text = std::ranges::any_of(
                text_rasters, [](const auto& raster) {
                    return raster.nontransparent_pixel_count > 0U;
                });
            if (!has_visible_text) {
                std::cerr << "spin-prompt text diagnostics: boxes="
                          << prompt_layout.debugTextBoxCount()
                          << ";fonts=" << prompt_layout.debugFontCount()
                          << ";rasters=" << text_rasters.size() << '\n';
                for (const auto& raster : text_rasters) {
                    std::cerr << "  raster=" << raster.text_box_name
                              << ";size=" << raster.width << 'x'
                              << raster.height << ";opaque="
                              << raster.nontransparent_pixel_count << '\n';
                }
                for (const auto& pane : prompt_layout.debugPanes()) {
                    for (const auto& content : pane.contents) {
                        if (content.kind == "text_box") {
                            std::cerr << "  pane=" << pane.name
                                      << ";text_box=" << content.name
                                      << ";font=" << content.font_name << '\n';
                        }
                    }
                }
            }
            require(has_visible_text,
                    "the real spin prompt must rasterize visible message text");
#endif

            // Display execution 2 releases A while the corrected clock stays
            // frozen. Counting starts at exeDisp, not the earlier entry call.
            run_frame(false);
            require_prompt(observer->getNerveStep() == 2 &&
                        guide->sheet.current_part_step() == 1 &&
                        guide->sheet.is_paused(),
                    "the paused timekeeper must remain at step 1 during the second display execution");

            // A second fresh edge well inside the guard is still rejected.
            run_frame(true);
            require_prompt(!observer->mFlag.mIsDead && observer->getNerveStep() == 3 &&
                        !GameDataFunction::isPassedStoryEvent(smgpc::resource::encode_cp932("スピン権利").c_str()) && !player._EEB,
                    "an early fresh A edge must not dismiss the spin prompt");
            run_frame(false);

            // Executions 5..29 remain released. Execution 30 receives a
            // fresh edge while mDisplayFrame becomes 0 and must still reject.
            for (auto execution = 5; execution <= 29; ++execution) {
                run_frame(false);
            }
            run_frame(true);
            require_prompt(!observer->mFlag.mIsDead && observer->getNerveStep() == 30 &&
                        guide->sheet.is_paused() &&
                        !GameDataFunction::isPassedStoryEvent(smgpc::resource::encode_cp932("スピン権利").c_str()) && !player._EEB,
                    "the 30th display execution must remain guarded at frame zero");

            // Execution 31 expires the guard while released. Only a new edge
            // on execution 32 may resume, persist the flag, and grant swing.
            run_frame(false);
            require_prompt(!observer->mFlag.mIsDead && observer->getNerveStep() == 31 &&
                        guide->sheet.is_paused() && !player._EEB,
                    "guard expiry without a trigger must leave the prompt active");
            run_frame(true);
            require_prompt(observer->mFlag.mIsDead && observer->getNerveStep() == 32 &&
                        !guide->sheet.is_paused() &&
                        guide->sheet.current_part_step() == 1 &&
                        GameDataFunction::isPassedStoryEvent(smgpc::resource::encode_cp932("スピン権利").c_str()) &&
                        smgpc::compat::game_data::holder_story_progress(
                            game_data_session.holder()) == 15U &&
                        player._EEB,
                    "the first post-guard fresh A edge must dismiss, resume, and grant spin entitlement");
            require(runtime.scheduler().registration_marker() ==
                        registrations_before_prompt,
                    "dismissal must not mutate scheduler registration from inside movement");

            const auto talk_ok_events = std::ranges::count_if(
                logical_audio.events(), [](const auto& event) {
                    return event.kind ==
                               smgpc::runtime::AudioEventKind::SystemSoundStart &&
                           event.name == "SE_SY_TALK_OK";
                });
            require(talk_ok_events == 1,
                    "dismissal must retain the exact SE_SY_TALK_OK request on the logical audio path");

            run_frame(false);
            require(guide->sheet.current_part_step() == 2 &&
                        !guide->sheet.is_paused(),
                    "the resumed timekeeper must advance on the next DemoDirector frame");
        }

        const auto retained_objects = smgpc::compat::snapshot_name_obj_runtime_objects();
        if (retained_objects != expected_runtime_objects) {
            std::cerr << "spin-prompt retirement: before=" << baseline_objects.size()
                      << ";expected_runtime=" << expected_runtime_objects.size()
                      << ";after=" << retained_objects.size() << '\n';
            for (const auto* object : retained_objects) {
                if (std::ranges::find(expected_runtime_objects, object) == expected_runtime_objects.end()) {
                    std::cerr << "  unexpected=" << object->getName()
                              << ";object=" << object
                              << ";owner=" << smgpc::compat::name_obj_runtime_owner(object) << '\n';
                }
            }
            for (const auto* object : expected_runtime_objects) {
                if (std::ranges::find(retained_objects, object) == retained_objects.end()) {
                    std::cerr << "  missing_runtime_object=" << object << '\n';
                }
            }
        }
        require(smgpc::compat::current_information_message() == nullptr &&
                    retained_objects == expected_runtime_objects,
                "scene teardown must release all prompt/player/scene children and preserve exactly the runtime-owned pointer layouts");
#ifndef NDEBUG
        require(runtime.scheduler().snapshot().size() ==
                    baseline_scheduler_entries,
                "scene teardown must leave no scheduler entry pointing at released prompt objects");
#endif
        require(runtime.player_system().attached_actor() == nullptr &&
                    runtime.player_system().actor_center_position() == nullptr,
                "scene teardown must detach the original Mario and release its host bridge");
    }

}  // namespace

int main() {
    try {
        const auto process_baseline = smgpc::compat::snapshot_name_obj_runtime_objects();
        test_exact_timekeep_prompt_guard_and_fresh_a();
        require(smgpc::compat::snapshot_name_obj_runtime_objects() == process_baseline,
                "RuntimeContext teardown must also release every retained star-pointer layout identity");
        std::cout << "InformationObserver tests passed: exact timekeep/prompt/guard/lifecycle\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "InformationObserver test failure: " << error.what()
                  << '\n';
        return 1;
    }
}
