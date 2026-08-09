#include "Game/Map/FileSelectSky.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "render/light/LightData.hpp"
#include "resource/Yaz0.hpp"
#include "runtime/JAudioPlaybackService.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/FileSelectFarVisual.hpp"
#include "scene/GatewayDemoScene.hpp"
#include "scene/TitleFileSelectRoute.hpp"

#include <aurora/dvd.h>
#include <aurora/audio.hpp>
#include <aurora/j_audio_sound_archive.hpp>
#include <dolphin/dvd.h>
#include <revolution/wpad.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

    class ScopedEnvironmentVariable final {
    public:
        ScopedEnvironmentVariable(std::string name, std::string value)
            : _name(std::move(name)) {
            if (const auto *previous = std::getenv(_name.c_str());
                previous != nullptr) {
                _previous = std::string(previous);
            }
            if (::setenv(_name.c_str(), value.c_str(), 1) != 0) {
                throw std::runtime_error(
                    "could not install the explicit test audio driver");
            }
        }

        ~ScopedEnvironmentVariable() {
            if (_previous.has_value()) {
                (void)::setenv(_name.c_str(), _previous->c_str(), 1);
            } else {
                (void)::unsetenv(_name.c_str());
            }
        }

        ScopedEnvironmentVariable(const ScopedEnvironmentVariable &) = delete;
        ScopedEnvironmentVariable &operator=(
            const ScopedEnvironmentVariable &) = delete;

    private:
        std::string _name;
        std::optional<std::string> _previous{};
    };

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

    void require_far_pose(const smgpc::camera::CameraPose &pose) {
        require_near(pose.eye.x, 0.0F, 0.001F, "far camera eye X");
        require_near(pose.eye.y, 0.0F, 0.001F, "far camera eye Y");
        require_near(pose.eye.z, 15000.0F, 0.001F, "far camera eye Z");
        require_near(pose.watch.x, 0.0F, 0.001F, "far camera watch X");
        require_near(pose.watch.y, 800.0F, 0.001F, "far camera watch Y");
        require_near(pose.watch.z, 0.0F, 0.001F, "far camera watch Z");
        require_near(pose.up.x, 0.0F, 0.001F, "far camera up X");
        require_near(pose.up.y, 1.0F, 0.001F, "far camera up Y");
        require_near(pose.up.z, 0.0F, 0.001F, "far camera up Z");
        require_near(pose.fovy_degrees, 40.0F, 0.001F, "far camera FOV");
    }

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
        require(!error,
                "Title/File Select route proof requires a readable working directory");
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
            "Title/File Select route proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

    [[nodiscard]] std::unique_ptr<smgpc::runtime::JAudioPlaybackService>
    make_explicit_test_audio_playback(
        smgpc::runtime::DvdFileSystemService &dvd) {
        const auto smr_path = dvd.find_first({
            std::filesystem::path("KrKorean") / "AudioRes" / "SMR.szs",
            std::filesystem::path("AudioRes") / "SMR.szs",
        });
        require(smr_path.has_value(),
                "route proof requires the retail AudioRes/SMR.szs archive");

        auto archive_factory = [&dvd, smr_path = *smr_path] {
            const auto compressed =
                dvd.read_file(smr_path.generic_string());
            auto baa = smgpc::resource::decompress_yaz0(compressed);
            const auto localized_waves_path =
                smr_path.parent_path() / "Waves";
            return std::make_unique<aurora::audio::JAudioSoundArchive>(
                baa,
                [&dvd, localized_waves_path](std::string_view archive_name) {
                    const auto name_path =
                        std::filesystem::path(archive_name);
                    if (name_path.empty() || name_path.is_absolute() ||
                        name_path.filename() != name_path) {
                        throw std::runtime_error(
                            "WSYS wave archive name is not a plain filename");
                    }
                    const auto retail_path = dvd.find_first({
                        localized_waves_path / name_path,
                        std::filesystem::path("AudioRes") / "Waves" /
                            name_path,
                    });
                    if (!retail_path.has_value()) {
                        throw std::runtime_error(
                            "route proof could not resolve retail JAudio waves " +
                            std::string(archive_name));
                    }
                    return dvd.read_file(retail_path->generic_string());
                });
        };

        auto stream_loader = [&dvd](std::string_view path) {
            auto stream_path = std::filesystem::path(path);
            if (stream_path.empty()) {
                throw std::invalid_argument(
                    "route proof received an empty retail JAudio stream path");
            }
            if (stream_path.is_absolute()) {
                stream_path = stream_path.relative_path();
            }
            if (std::ranges::any_of(
                    stream_path, [](const auto &component) {
                        return component == "..";
                    })) {
                throw std::runtime_error(
                    "route proof JAudio stream path escapes the disc root");
            }
            return dvd.read_file(stream_path.generic_string());
        };

        return std::make_unique<smgpc::runtime::JAudioPlaybackService>(
            std::move(archive_factory), std::move(stream_loader),
            std::make_unique<aurora::audio::PcmAudioMixer>(
                48000U,
                aurora::audio::PlaybackDevicePolicy::AllowExplicitTestSink));
    }

    void run_route_frame(smgpc::runtime::RuntimeContext &runtime,
                         smgpc::render::AuroraRenderer &renderer,
                         smgpc::scene::TitleFileSelectRoute &route,
                         std::uint64_t frame_index, std::uint32_t button_mask) {
        auto frame = renderer.begin_frame();
        frame.frame_index = frame_index;
        runtime.begin_frame(frame);

        // RuntimeContext has already sampled the host window for this frame.
        // Override that sampled channel at the same WpadService boundary so
        // this proof can choose holds and fresh edges deterministically.
        auto &wpad = runtime.wpad();
        wpad.set_connected(WPAD_CHAN0, true);
        wpad.set_button_mask(WPAD_CHAN0, button_mask);

        route.update();
        runtime.draw_3d_normal();
        runtime.draw_2d_normal();
        // Match the application frame boundary so the proof covers the
        // authored render mode's ordinary vertical display-copy filter too.
        const auto *disable_filter =
            std::getenv("SMGPC_FILE_SELECT_DISABLE_VFILTER");
        renderer.end_frame(runtime.wii_video().render_mode(),
                           disable_filter == nullptr ||
                               disable_filter[0] == '\0');
    }

    void capture_screen_if_requested(
        smgpc::render::AuroraRenderer &renderer, const char *environment_name,
        std::string_view proof_name) {
        const auto *configured = std::getenv(environment_name);
        if (configured == nullptr || configured[0] == '\0') {
            return;
        }

        const auto path = std::filesystem::path(configured);
        renderer.request_screenshot_png(path);
        require(std::filesystem::is_regular_file(path) &&
                    std::filesystem::file_size(path) > 64U,
                "the requested title/File Select PNG was not written");
        std::cout << "[info] " << proof_name << '=' << path.string() << '\n';
    }

#ifndef NDEBUG
    [[nodiscard]] std::size_t scheduler_name_count(
        const smgpc::runtime::RuntimeContext &runtime, std::string_view name) {
        return std::ranges::count_if(
            runtime.scheduler().snapshot(), [name](const auto &entry) {
                return entry.name == name;
            });
    }

    [[nodiscard]] bool title_prompt_visible(
        const smgpc::runtime::RuntimeContext &runtime) {
        const auto layouts =
            runtime.scheduler().debug_layout_runtime_snapshot();
        const auto has_wait_animation = [&layouts](std::string_view name,
                                                    std::string_view layout) {
            return std::ranges::any_of(layouts, [&](const auto &entry) {
                return entry.name == name && entry.layout_name == layout &&
                       !entry.dead &&
                       std::ranges::any_of(entry.animations,
                                           [](const auto &animation) {
                                               return animation.layer_index == 0U &&
                                                      animation.active &&
                                                      animation.name == "Wait";
                                           });
            });
        };
        return has_wait_animation("ロゴ", "TitleLogo") &&
               has_wait_animation("Press[A][B]", "PressStart");
    }
#endif

    void test_title_to_blank_file_select_route() {
        constexpr auto cTitleFrameLimit = std::uint64_t{1200U};
        constexpr auto cExpectedFileNumbers =
            std::array<s32, 6U>{1, 2, 4, 6, 5, 3};

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

        const auto test_audio_driver =
            ScopedEnvironmentVariable{"SDL_AUDIODRIVER", "dummy"};
        auto audio_dvd = smgpc::runtime::DvdFileSystemService{
            std::filesystem::path{"/"}};
        auto playback = make_explicit_test_audio_playback(audio_dvd);

        auto logger = smgpc::logging::create_default_logger();
        auto window = smgpc::render::AuroraWindow({
            .width = 640,
            .height = 456,
            .title = "SMG PC Title/File Select route proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto runtime = smgpc::runtime::RuntimeContext(
            *logger, window, std::move(playback));
        const auto renderer_context =
            smgpc::render::ScopedAuroraRendererContext(renderer);

        const auto baseline_name_objs =
            smgpc::compat::name_obj_runtime_state_count();
        const auto baseline_actors =
            smgpc::compat::actor_runtime_state_count();
        const auto baseline_scheduler = runtime.scheduler().snapshot().size();
        const auto camera_end_baseline =
            runtime.camera_system().programmable_camera_end_count();

        auto route =
            std::make_unique<smgpc::scene::TitleFileSelectRoute>(runtime);
        auto *sky_identity = route->sky();
        require(route->state() ==
                        smgpc::scene::TitleFileSelectRouteState::Title &&
                    route->far_visual() == nullptr &&
                    !route->selected_slot().has_value() &&
                    !route->launch_request().has_value() &&
                    sky_identity != nullptr &&
                    runtime.current_stage_name() == "FileSelect" &&
                    runtime.game_layout().is_game_scene_draw_3d_active() &&
                    smgpc::compat::name_obj_runtime_state_count() ==
                        baseline_name_objs + 4U &&
                    smgpc::compat::actor_runtime_state_count() ==
                        baseline_actors + 1U &&
                    runtime.scheduler().snapshot().size() ==
                        baseline_scheduler + 4U,
                "route did not begin as the ordinary FileSelect title owner");
#ifndef NDEBUG
        require(scheduler_name_count(runtime, "ファイルセレクト画面の空") == 1U &&
                    scheduler_name_count(runtime, "ロゴ") == 1U &&
                    scheduler_name_count(runtime, "Press[A][B]") == 1U &&
                    scheduler_name_count(runtime, "PAL60案内") == 1U,
                "title route did not schedule its shared sky and three captured retail layouts exactly once");
#else
        throw std::runtime_error(
            "production Title/File Select route proof requires a debug build");
#endif

        // TitleSequenceProduct decides from simultaneous A+B levels, not a
        // synthetic route shortcut. Holding both from frame one lets its real
        // layout/BGM nerves reach that decision whenever the prompt is ready.
        auto frame_index = std::uint64_t{};
#ifndef NDEBUG
        if (const auto *title_screenshot =
                std::getenv("SMGPC_TITLE_SCREENSHOT");
            title_screenshot != nullptr && title_screenshot[0] != '\0') {
            while (!title_prompt_visible(runtime) &&
                   frame_index < cTitleFrameLimit) {
                ++frame_index;
                run_route_frame(runtime, renderer, *route, frame_index, 0U);
            }
            require(route->state() ==
                            smgpc::scene::TitleFileSelectRouteState::Title &&
                        title_prompt_visible(runtime),
                    "title proof did not reach the exact Logo/PressStart Wait state");

            // Both layouts are now in their authored Wait animations. Sample
            // a later prompt frame so fade-in and first-frame handoffs cannot
            // be mistaken for the settled title composition.
            for (auto settle_frame = 0U; settle_frame < 60U; ++settle_frame) {
                ++frame_index;
                run_route_frame(runtime, renderer, *route, frame_index, 0U);
            }
            require(route->state() ==
                            smgpc::scene::TitleFileSelectRouteState::Title &&
                        title_prompt_visible(runtime),
                    "settled title prompt left its authored Wait state");
            capture_screen_if_requested(renderer, "SMGPC_TITLE_SCREENSHOT",
                                        "title_screenshot");
        }
#else
        if (const auto *title_screenshot =
                std::getenv("SMGPC_TITLE_SCREENSHOT");
            title_screenshot != nullptr && title_screenshot[0] != '\0') {
            throw std::runtime_error(
                "title screenshot proof requires a debug build");
        }
#endif
        while (route->state() ==
                   smgpc::scene::TitleFileSelectRouteState::Title &&
               frame_index < cTitleFrameLimit) {
            ++frame_index;
            run_route_frame(runtime, renderer, *route, frame_index,
                            WPAD_BUTTON_A | WPAD_BUTTON_B);
        }
        require(route->state() ==
                        smgpc::scene::TitleFileSelectRouteState::MoveToFar &&
                    route->far_visual() != nullptr &&
                    route->sky() == sky_identity &&
                    route->far_visual()->sky() == sky_identity,
                "real title completion did not transfer the same sky to Far");
#ifndef NDEBUG
        require(scheduler_name_count(runtime, "ファイルセレクト画面の空") == 1U,
                "title-to-far handoff duplicated or disconnected the shared sky");
#endif
        const auto title_frames = frame_index;

        const auto slots = route->far_visual()->slots();
        require(slots.size() == cExpectedFileNumbers.size(),
                "blank File Select route did not expose all six slots");
        for (auto index = std::size_t{}; index < slots.size(); ++index) {
            require(slots[index].file_number == cExpectedFileNumbers[index] &&
                        slots[index].planet != nullptr &&
                        slots[index].number != nullptr,
                    "blank File Select route changed retail slot order or children");
        }

        // Preserve A continuously after title completion. The camera and
        // number handoff must finish, but that held level must never become a
        // blank-slot decision because the route consumes a fresh A edge.
        auto far_movement_frames = std::uint64_t{};
        while (route->state() ==
                   smgpc::scene::TitleFileSelectRouteState::MoveToFar &&
               far_movement_frames < 100U) {
            ++frame_index;
            ++far_movement_frames;
            run_route_frame(runtime, renderer, *route, frame_index,
                            WPAD_BUTTON_A);
        }
        require(far_movement_frames == 63U &&
                    route->state() ==
                        smgpc::scene::TitleFileSelectRouteState::SelectBlank &&
                    route->far_visual()->is_at_far_point() &&
                    route->far_visual()->numbers_visible() &&
                    route->far_visual()->highlighted_slot() == 0U &&
                    route->selected_slot() ==
                        smgpc::scene::TitleFileSelectRouteSelection{
                            .visual_index = 0U,
                            .file_number = 1,
                        } &&
                    !route->launch_request().has_value(),
                "route selected early or missed the exact 63-frame far/number handoff");
        const auto far_pose =
            runtime.camera_system().active_programmable_camera_pose();
        require(far_pose.has_value(),
                "File Select programmable camera disappeared at Far");
        require_far_pose(*far_pose);

        for (auto held_frame = 0U; held_frame < 4U; ++held_frame) {
            ++frame_index;
            run_route_frame(runtime, renderer, *route, frame_index,
                            WPAD_BUTTON_A);
        }
        require(route->state() ==
                        smgpc::scene::TitleFileSelectRouteState::SelectBlank &&
                    !route->launch_request().has_value(),
                "a held title A level was reused as a blank-slot edge");

        // Release the title hold and let the exact step-0..30 selection scale
        // recurrence settle before capturing a stable far composition. The
        // screenshot hook is test-only and reads the already filtered display
        // copy; it does not change renderer or production audio policy.
        for (auto settle_frame = 0U; settle_frame < 32U; ++settle_frame) {
            ++frame_index;
            run_route_frame(runtime, renderer, *route, frame_index, 0U);
        }
        capture_screen_if_requested(renderer, "SMGPC_FILE_SELECT_SCREENSHOT",
                                    "file_select_far_screenshot");

        // Release A, then hold Right for several frames. Only the first frame
        // has a direction edge, so selection advances once from file 1 to 2.
        ++frame_index;
        run_route_frame(runtime, renderer, *route, frame_index, 0U);
        for (auto held_frame = 0U; held_frame < 4U; ++held_frame) {
            ++frame_index;
            run_route_frame(runtime, renderer, *route, frame_index,
                            WPAD_BUTTON_RIGHT);
        }
        require(route->selected_slot() ==
                        smgpc::scene::TitleFileSelectRouteSelection{
                            .visual_index = 1U,
                            .file_number = 2,
                        } &&
                    route->far_visual()->highlighted_slot() == 1U &&
                    !route->launch_request().has_value(),
                "held Right moved more than one retail-order blank slot");

        // A neutral frame arms a genuinely fresh A edge. The route publishes
        // only the selected blank file identity; it performs no save/RFL work.
        ++frame_index;
        run_route_frame(runtime, renderer, *route, frame_index, 0U);
        ++frame_index;
        run_route_frame(runtime, renderer, *route, frame_index, WPAD_BUTTON_A);
        require(route->state() ==
                        smgpc::scene::TitleFileSelectRouteState::LaunchRequested &&
                    route->launch_request() ==
                        smgpc::scene::TitleFileSelectRouteSelection{
                            .visual_index = 1U,
                            .file_number = 2,
                        },
                "fresh A did not publish the selected blank-slot launch request");

        route.reset();
        require(!runtime.game_layout().is_game_scene_draw_3d_active() &&
                    smgpc::compat::name_obj_runtime_state_count() ==
                        baseline_name_objs &&
                    smgpc::compat::actor_runtime_state_count() == baseline_actors &&
                    runtime.scheduler().snapshot().size() == baseline_scheduler &&
                    smgpc::render::light::StageLightData::instance()
                        .stage_zones()
                        .empty() &&
                    !runtime.camera_system()
                         .active_programmable_camera_name()
                         .has_value() &&
                    runtime.camera_system().programmable_camera_end_count() ==
                        camera_end_baseline + 1U,
                "route teardown left sky, far actors, StageLight, or camera state");
#ifndef NDEBUG
        for (const auto name : {
                 "ロゴ",
                 "Press[A][B]",
                 "PAL60案内",
                 "ファイルセレクト画面の空",
                 "ファイルセレクトカメラ制御",
                 "ニューフェイス",
                 "ファイル番号",
                 "ファイルセレクト遠景表示制御",
             }) {
            require(scheduler_name_count(runtime, name) == 0U,
                    "route teardown left a far visual scheduler child");
        }
#endif

        // Prove the title/file-select composition can hand the same live
        // RuntimeContext to an unrelated authored scene and that both scene
        // lifetimes return every registry to the original runtime baseline.
        runtime.set_current_stage_name("HeavensDoorGalaxy");
        {
            auto gateway =
                std::make_unique<smgpc::scene::GatewayDemoScene>(runtime.dvd());
            require(gateway->sky() != nullptr && gateway->planet() != nullptr,
                    "a fresh Gateway scene could not start after File Select teardown");
        }
        require(smgpc::compat::name_obj_runtime_state_count() ==
                        baseline_name_objs &&
                    smgpc::compat::actor_runtime_state_count() == baseline_actors &&
                    runtime.scheduler().snapshot().size() == baseline_scheduler &&
                    smgpc::render::light::StageLightData::instance()
                        .stage_zones()
                        .empty(),
                "fresh Gateway teardown did not restore the pre-title RuntimeContext registries");

        std::cout << "[proof] disc=" << disc_path.string()
                  << ";title_frames=" << title_frames
                  << ";sky_identity=retained;far_frames=" << far_movement_frames
                  << ";slots=1,2,4,6,5,3;held_a=rejected"
                  << ";held_right=single_edge;fresh_a=file2"
                  << ";title_children=3;fresh_gateway=clean;teardown=clean"
                  << '\n';
    }

}  // namespace

int main() {
    try {
        test_title_to_blank_file_select_route();
        std::cout << "[ok] exact title-to-blank-file-select route contract passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] Title/File Select route: " << error.what() << '\n';
        return 1;
    }
}
