#include "Game/Map/FileSelectSky.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/TitleFileSelectVisual.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>

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
        require(!error, "title visual proof requires a readable working directory");
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
            "title visual proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

#ifndef NDEBUG
    [[nodiscard]] const smgpc::runtime::SceneSchedulerEntryState &require_sky_entry(
        const std::vector<smgpc::runtime::SceneSchedulerEntryState> &entries) {
        const auto found = std::ranges::find_if(entries, [](const auto &entry) {
            return entry.name == "ファイルセレクト画面の空";
        });
        require(found != entries.end(),
                "production Title/File Select sky is absent from the scheduler");
        require(found->kind == smgpc::runtime::SceneEntryKind::LiveActorModel &&
                    found->movement_type == MR::MovementType_Sky &&
                    found->calc_anim_type == MR::CalcAnimType_MapObj &&
                    found->draw_buffer_type == MR::DrawBufferType_Sky &&
                    found->draw_type == -1,
                "Title/File Select sky bypassed the shared retail scheduler path");
        return *found;
    }
#endif

    void test_production_title_file_select_visual() {
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
            .title = "SMG PC production title sky proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
        runtime.set_current_stage_name("FileSelect");
        const auto renderer_context =
            smgpc::render::ScopedAuroraRendererContext(renderer);

        auto retained_visual =
            std::optional<smgpc::scene::TitleFileSelectVisualHandoff>{};
        auto packet_count = std::size_t{};
        {
            auto visual = smgpc::scene::TitleFileSelectVisual(runtime);
            const auto &camera = visual.title_camera();
            require_near(camera.eye.x, 0.0F, 0.0001F, "title camera eye X");
            require_near(camera.eye.y, 15800.0F, 0.0001F, "title camera eye Y");
            require_near(camera.eye.z, 15000.0F, 0.0001F, "title camera eye Z");
            require_near(camera.watch.x, 0.0F, 0.0001F, "title camera watch X");
            require_near(camera.watch.y, 15800.0F, 0.0001F, "title camera watch Y");
            require_near(camera.watch.z, 0.0F, 0.0001F, "title camera watch Z");
            require_near(camera.fovy_degrees, 60.0F, 0.0001F,
                         "title camera FOV");
            require(runtime.game_layout().is_game_scene_draw_3d_active(),
                    "Title/File Select composition did not activate 3D drawing");

            auto *sky = visual.sky();
            auto *model = smgpc::compat::actor_model(sky);
            require(sky != nullptr && model != nullptr && !sky->mFlag.mIsDead &&
                        model->model_arc_name() == "CometNearOrbitSky",
                    "composition did not own the exact live FileSelectSky model");
            require_near(sky->mScale.x, 0.8F, 0.0001F, "FileSelectSky scale X");
            require_near(sky->mScale.y, 0.8F, 0.0001F, "FileSelectSky scale Y");
            require_near(sky->mScale.z, 0.8F, 0.0001F, "FileSelectSky scale Z");
            model->requireLoaded();

#ifndef NDEBUG
            (void)require_sky_entry(runtime.scheduler().snapshot());
#endif

            auto frame = renderer.begin_frame();
            frame.frame_index = 121U;
            runtime.begin_frame(frame);
            runtime.set_scene_camera_pose(camera);
#ifndef NDEBUG
            runtime.set_j3d_packet_trace_frame(frame.frame_index);
#endif
            runtime.draw_3d_normal(camera);
            runtime.draw_2d_normal();

#ifndef NDEBUG
            const auto after_movement = runtime.scheduler().snapshot();
            const auto &entry = require_sky_entry(after_movement);
            require(entry.live_actor_bck_name == "CometNearOrbitSky" &&
                        entry.live_actor_btk_name == "CometNearOrbitSky",
                    "production FileSelectSky did not start exact BCK/BTK animation");
            packet_count = std::ranges::count_if(
                runtime.j3d_packet_trace(), [frame](const auto &packet) {
                    return packet.model_name == "CometNearOrbitSky" &&
                           packet.frame_index == frame.frame_index &&
                           packet.state.source_triangle_count != 0U &&
                           packet.state.bck_active && packet.state.btk_active &&
                           packet.state.btk_material_count != 0U;
                });
            require(packet_count != 0U,
                    "production title background submitted no real animated J3D packets");
#else
            throw std::runtime_error(
                "production title background proof requires a debug build");
#endif
            renderer.end_frame();

            auto *identity = visual.sky();
            retained_visual.emplace(visual.release_sky_for_file_select());
            require(retained_visual->sky() == identity && visual.sky() == nullptr,
                    "file-select transition did not transfer the sole sky owner");
        }

#ifndef NDEBUG
        require(runtime.game_layout().is_game_scene_draw_3d_active() &&
                    require_sky_entry(runtime.scheduler().snapshot()).live_actor_bck_name ==
                        "CometNearOrbitSky",
                "transferred sky did not preserve 3D activation and scheduler registration");
#endif
        retained_visual.reset();
#ifndef NDEBUG
        require(!runtime.game_layout().is_game_scene_draw_3d_active() &&
                    std::ranges::none_of(runtime.scheduler().snapshot(), [](const auto &entry) {
                    return entry.name == "ファイルセレクト画面の空";
                }),
                "destroying the transfer owner did not retire 3D and the sky registration");
#endif

        std::cout << "[proof] disc=" << disc_path.string()
                  << ";title_sky=CometNearOrbitSky;animated_packets=" << packet_count
                  << ";camera_eye=0,15800,15000;camera_watch=0,15800,0"
                  << ";transfer_identity=preserved" << '\n';
    }

}  // namespace

int main() {
    try {
        test_production_title_file_select_visual();
        std::cout << "[ok] production title and future File Select share exact sky ownership\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] production Title/File Select visual: " << error.what()
                  << '\n';
        return 1;
    }
}
