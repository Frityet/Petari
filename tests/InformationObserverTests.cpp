#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/IconAButton.hpp"
#include "Game/Screen/InformationMessage.hpp"
#include "Game/Screen/InformationObserver.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "camera/StageStartCamera.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/AudioFacadeCompat.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/GameDataHolderCompat.hpp"
#include "compat/GameDataSession.hpp"
#include "compat/InformationMessageCompat.hpp"
#include "layout/LayoutHost.hpp"
#include "layout/LayoutRuntime.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/GatewayDemoScene.hpp"
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
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    constexpr auto cPromptPart = "スピンゲット[デモ5]";

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
        SpinPromptTrigger() : LiveActor("チコ") {
            makeActorAppeared();
            MR::connectToSceneNpcMovement(this);
        }

        void movement() override {
            if (!_fired && MR::isDemoPartFirstStep(cPromptPart)) {
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

    class SwingEntitlementProbe final : public LiveActor {
    public:
        SwingEntitlementProbe()
            : LiveActor("InformationObserver swing-entitlement probe") {}

        static void set_swing_permission(LiveActor& actor, bool permitted) {
            static_cast<SwingEntitlementProbe&>(actor)._permitted = permitted;
        }

        void init(const JMapInfoIter& iter) override {
            if (!iter.isValid() || !iter.getValue("pos_x", &mPosition.x) ||
                !iter.getValue("pos_y", &mPosition.y) ||
                !iter.getValue("pos_z", &mPosition.z)) {
                throw std::logic_error(
                    "the entitlement probe requires Gateway's retained StartInfo");
            }
            calcAndSetBaseMtx();
            MR::connectToScene(
                this, MR::MovementType_Player, MR::CalcAnimType_Player,
                MR::DrawBufferType_Player, MR::DrawType_Player);
            _initialized = true;
        }

        void initAfterPlacement() override {
            if (!_initialized) {
                throw std::logic_error(
                    "the entitlement probe postpass preceded player init");
            }
            ++_post_placement_count;
        }

        [[nodiscard]] bool is_permitted() const {
            return _permitted;
        }

        [[nodiscard]] std::size_t post_placement_count() const {
            return _post_placement_count;
        }

    private:
        bool _permitted = false;
        bool _initialized = false;
        std::size_t _post_placement_count = 0U;
    };

    class PlayerDetachGuard final {
    public:
        PlayerDetachGuard(smgpc::runtime::PlayerSystemService& service,
                          const LiveActor& actor)
            : _service(service), _actor(actor) {}

        ~PlayerDetachGuard() {
            _service.detach_actor(&_actor);
            if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
                runtime != nullptr) {
                runtime->unregister_live_actor_model(
                    const_cast<LiveActor&>(_actor));
            }
        }

        PlayerDetachGuard(const PlayerDetachGuard&) = delete;
        PlayerDetachGuard& operator=(const PlayerDetachGuard&) = delete;

    private:
        smgpc::runtime::PlayerSystemService& _service;
        const LiveActor& _actor;
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
        runtime.set_current_stage_name("HeavensDoorGalaxy");
        const auto renderer_context =
            smgpc::render::ScopedAuroraRendererContext(renderer);
        runtime.player_system().clear_stage_state();

        auto logical_audio = smgpc::runtime::AudioEventService{};
        const auto audio_binding =
            smgpc::compat::ScopedAudioEventServiceOverride{logical_audio};
        auto game_data_session = smgpc::compat::GameDataSession{1U};
        auto player = SwingEntitlementProbe{};

        const auto baseline_name_objects =
            smgpc::compat::name_obj_runtime_state_count();
#ifndef NDEBUG
        const auto baseline_scheduler_entries = runtime.scheduler().snapshot().size();
#endif
        {
            auto scene = smgpc::scene::GatewayDemoScene{runtime.dvd()};
            const auto camera = smgpc::camera::resolve_stage_start_camera(
                runtime.dvd(), scene.start_info());
            require(camera.status ==
                            smgpc::camera::StageStartCameraResolveStatus::Resolved &&
                        camera.camera.has_value(),
                    "the InformationObserver proof could not resolve Gateway's exact start camera");
            runtime.camera_system().set_game_camera_pose(
                camera.camera->calculation.pose);
            runtime.set_scene_camera_pose(camera.camera->calculation.pose);
            runtime.player_system().attach_actor(
                player,
                {.set_swing_permission =
                     &SwingEntitlementProbe::set_swing_permission});
            player.init(scene.player_start_iter());
            const auto player_detach_guard =
                PlayerDetachGuard{runtime.player_system(), player};
            auto placement_lease = scene.finalize_placements(player);
            require(runtime.player_system().attached_actor() == &player &&
                        !player.is_permitted() &&
                        player.post_placement_count() == 1U,
                    "the prompt proof must finalize one initially-locked external player");
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
            require(smgpc::compat::game_data::holder_story_progress(
                        game_data_session.holder()) == 5U &&
                        GameDataFunction::getCurrentGameDataHolder() ==
                            &game_data_session.holder() &&
                        GameDataFunction::getSceneStartGameDataHolder() ==
                            &game_data_session.holder(),
                    "the explicit prompt proof must retain its selected-file progress-5 holder");
            GameDataFunction::followStoryEventByName("チコガイドデモ終了");
            require(smgpc::compat::game_data::holder_story_progress(
                        game_data_session.holder()) == 10U,
                    "the explicit part-22 proof must advance the same selected-file holder from progress 5 to 10");
            MR::startTimeKeepDemo(&trigger, "チコガイドデモ", cPromptPart);

            const auto guide_index = demo.find_definition("チコガイドデモ");
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

            // Carry a fresh A edge into the same frame that part 22 reaches
            // step 0. The exact observer must show the prompt and consume no
            // entitlement while its 30-frame guard is active.
            run_frame(true);
            require(trigger.fired() && !observer->mFlag.mIsDead &&
                        !information_message.message().mFlag.mIsDead &&
                        guide->sheet.current_part_step() == 0 &&
                        guide->sheet.is_paused() &&
                        !GameDataFunction::isPassedStoryEvent("スピン権利") &&
                        !runtime.player_system().is_swing_permitted() &&
                        !player.is_permitted(),
                    "part 22 step 0 must pause the real timekeeper and reject a carried A edge");
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

            // Retail DemoTimeKeeper performs its one paused 0->1 correction.
            run_frame(false);
            require(guide->sheet.current_part_step() == 1 &&
                        guide->sheet.is_paused(),
                    "a paused first-step timekeeper must correct once to step 1 and then freeze");

            // A second fresh edge well inside the guard is still rejected.
            run_frame(true);
            require(!observer->mFlag.mIsDead &&
                        !GameDataFunction::isPassedStoryEvent("スピン権利"),
                    "an early fresh A edge must not dismiss the spin prompt");
            run_frame(false);

            // Executions 5..29 remain released. Execution 30 receives a
            // fresh edge while mDisplayFrame becomes 0 and must still reject.
            for (auto execution = 5; execution <= 29; ++execution) {
                run_frame(false);
            }
            run_frame(true);
            require(!observer->mFlag.mIsDead && guide->sheet.is_paused() &&
                        !GameDataFunction::isPassedStoryEvent("スピン権利"),
                    "the 30th display execution must remain guarded at frame zero");

            // Execution 31 expires the guard while released. Only a new edge
            // on execution 32 may resume, persist the flag, and grant swing.
            run_frame(false);
            require(!observer->mFlag.mIsDead && guide->sheet.is_paused(),
                    "guard expiry without a trigger must leave the prompt active");
            run_frame(true);
            require(observer->mFlag.mIsDead && !guide->sheet.is_paused() &&
                        guide->sheet.current_part_step() == 1 &&
                        GameDataFunction::isPassedStoryEvent("スピン権利") &&
                        smgpc::compat::game_data::holder_story_progress(
                            game_data_session.holder()) == 15U &&
                        runtime.player_system().is_swing_permitted() &&
                        player.is_permitted(),
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

        require(smgpc::compat::current_information_message() == nullptr &&
                    smgpc::compat::name_obj_runtime_state_count() ==
                        baseline_name_objects,
                "scene teardown must release the observer, message, and raw-new IconAButton child");
#ifndef NDEBUG
        require(runtime.scheduler().snapshot().size() ==
                    baseline_scheduler_entries,
                "scene teardown must leave no scheduler entry pointing at released prompt objects");
#endif
        require(!player.is_permitted(),
                "detaching the prompt proof player must revoke its entitlement bit");
    }

}  // namespace

int main() {
    try {
        test_exact_timekeep_prompt_guard_and_fresh_a();
        std::cout << "InformationObserver tests passed: exact timekeep/prompt/guard/lifecycle\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "InformationObserver test failure: " << error.what()
                  << '\n';
        return 1;
    }
}
