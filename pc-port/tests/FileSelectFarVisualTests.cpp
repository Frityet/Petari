#include "Game/LiveActor/PartsModel.hpp"
#include "Game/Map/FileSelectCameraController.hpp"
#include "Game/Map/FileSelectSky.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Screen/FileSelectNumber.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "render/light/LightData.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/FileSelectFarVisual.hpp"
#include "scene/TitleFileSelectVisual.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

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

    void require_pose(const smgpc::camera::CameraPose &pose,
                      const std::array<float, 3U> &eye,
                      const std::array<float, 3U> &watch, float fovy,
                      std::string_view context) {
        require_near(pose.eye.x, eye[0], 0.001F,
                     std::string(context) + " eye X");
        require_near(pose.eye.y, eye[1], 0.001F,
                     std::string(context) + " eye Y");
        require_near(pose.eye.z, eye[2], 0.001F,
                     std::string(context) + " eye Z");
        require_near(pose.watch.x, watch[0], 0.001F,
                     std::string(context) + " watch X");
        require_near(pose.watch.y, watch[1], 0.001F,
                     std::string(context) + " watch Y");
        require_near(pose.watch.z, watch[2], 0.001F,
                     std::string(context) + " watch Z");
        require_near(pose.up.x, 0.0F, 0.001F,
                     std::string(context) + " up X");
        require_near(pose.up.y, 1.0F, 0.001F,
                     std::string(context) + " up Y");
        require_near(pose.up.z, 0.0F, 0.001F,
                     std::string(context) + " up Z");
        require_near(pose.fovy_degrees, fovy, 0.001F,
                     std::string(context) + " FOV");
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
                "File Select far proof requires a readable working directory");
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
            "File Select far proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

    void render_frame(smgpc::runtime::RuntimeContext &runtime,
                      smgpc::render::AuroraRenderer &renderer,
                      std::uint64_t frame_index, bool trace_packets = false) {
        auto frame = renderer.begin_frame();
        frame.frame_index = frame_index;
        runtime.begin_frame(frame);
#ifndef NDEBUG
        runtime.set_j3d_packet_trace_frame(
            trace_packets ? std::optional<std::uint64_t>{frame_index}
                          : std::nullopt);
#else
        (void)trace_packets;
#endif
        runtime.draw_3d_normal();
        runtime.draw_2d_normal();
        renderer.end_frame();
    }

    constexpr auto cExpectedSlots =
        std::array<std::array<float, 3U>, 6U>{
            std::array<float, 3U>{-1711.50049F, -1606.41858F, 4414.76904F},
            std::array<float, 3U>{1709.70166F, -1606.64246F, 4415.38477F},
            std::array<float, 3U>{5000.0F, 0.0F, 0.0F},
            std::array<float, 3U>{2501.10718F, 1480.42542F, -4068.51392F},
            std::array<float, 3U>{-2499.44678F, 1480.75317F, -4069.41455F},
            std::array<float, 3U>{-5000.0F, -0.000149467F, 0.000410765F},
        };
    constexpr auto cExpectedFileNumbers = std::array<s32, 6U>{1, 2, 4, 6, 5, 3};

    void require_authored_slots(const smgpc::scene::FileSelectFarVisual &visual) {
        const auto slots = visual.slots();
        require(slots.size() == cExpectedSlots.size(),
                "far File Select did not create all six authored slots");

        for (auto index = std::size_t{}; index < slots.size(); ++index) {
            const auto &slot = slots[index];
            const auto &expected = cExpectedSlots[index];
            require(slot.file_number == cExpectedFileNumbers[index] &&
                        slot.planet != nullptr && slot.number != nullptr &&
                        slot.planet_matrix != nullptr,
                    "far File Select slot lost its retail ID or exact child");
            require_near(slot.base_position.x, expected[0], 0.002F,
                         "authored File Select slot X");
            require_near(slot.base_position.y, expected[1], 0.002F,
                         "authored File Select slot Y");
            require_near(slot.base_position.z, expected[2], 0.002F,
                         "authored File Select slot Z");

            const auto &matrix = *slot.planet_matrix;
            require_near(matrix.mMtx[0][0], 1.0F, 0.0001F,
                         "planet matrix XX");
            require_near(matrix.mMtx[1][1], 1.0F, 0.0001F,
                         "planet matrix YY");
            require_near(matrix.mMtx[2][2], 1.0F, 0.0001F,
                         "planet matrix ZZ");
            require_near(matrix.mMtx[0][1], 0.0F, 0.0001F,
                         "planet matrix XY");
            require_near(matrix.mMtx[0][2], 0.0F, 0.0001F,
                         "planet matrix XZ");
            require_near(matrix.mMtx[1][0], 0.0F, 0.0001F,
                         "planet matrix YX");
            require_near(matrix.mMtx[1][2], 0.0F, 0.0001F,
                         "planet matrix YZ");
            require_near(matrix.mMtx[2][0], 0.0F, 0.0001F,
                         "planet matrix ZX");
            require_near(matrix.mMtx[2][1], 0.0F, 0.0001F,
                         "planet matrix ZY");
            require_near(matrix.mMtx[0][3], expected[0], 0.002F,
                         "planet matrix authored X");
            require_near(matrix.mMtx[1][3], expected[1] + 900.0F, 0.002F,
                         "planet matrix authored +900 Y");
            require_near(matrix.mMtx[2][3], expected[2], 0.002F,
                         "planet matrix authored Z");

            auto *model = smgpc::compat::actor_model(slot.planet);
            require(model != nullptr &&
                        model->model_arc_name() == "FileSelectDataPlanet" &&
                        !slot.planet->mFlag.mIsDead &&
                        slot.number->mFlag.mIsDead,
                    "slot did not use the real planet or initially-dead number layout");
            require_near(slot.planet->mScale.x, 30.0F, 0.0001F,
                         "planet scale X");
            require_near(slot.planet->mScale.y, 30.0F, 0.0001F,
                         "planet scale Y");
            require_near(slot.planet->mScale.z, 30.0F, 0.0001F,
                         "planet scale Z");
            model->requireLoaded();
        }
    }

#ifndef NDEBUG
    void require_scheduler_composition(
        const smgpc::runtime::RuntimeContext &runtime) {
        const auto entries = runtime.scheduler().snapshot();
        const auto count = [&entries](std::string_view name) {
            return std::ranges::count_if(entries, [name](const auto &entry) {
                return entry.name == name;
            });
        };
        require(count("ファイルセレクト画面の空") == 1 &&
                    count("ファイルセレクトカメラ制御") == 1 &&
                    count("ニューフェイス") == 6 &&
                    count("ファイル番号") == 6 &&
                    count("ファイルセレクト遠景表示制御") == 1,
                "far composition does not uniquely schedule its exact children");

        require(std::ranges::count_if(entries, [](const auto &entry) {
                    return entry.name == "ニューフェイス" &&
                           entry.kind ==
                               smgpc::runtime::SceneEntryKind::LiveActorModel &&
                           entry.movement_type ==
                               MR::MovementType_EnemyDecoration &&
                           entry.calc_anim_type ==
                               MR::CalcAnimType_MapObjDecoration &&
                           entry.draw_buffer_type == MR::DrawBufferType_MapObj;
                }) == 6,
                "planet PartsModels bypassed their retail scheduler categories");
        require(std::ranges::count_if(entries, [](const auto &entry) {
                    return entry.name == "ファイル番号" &&
                           entry.kind ==
                               smgpc::runtime::SceneEntryKind::LayoutActor &&
                           entry.movement_type == MR::MovementType_Layout &&
                           entry.calc_anim_type == MR::CalcAnimType_Layout &&
                           entry.draw_type == MR::DrawType_Layout;
                }) == 6,
                "FileSelectNumber bypassed the ordinary layout scheduler");
    }

    void require_number_selection_animation(
        const smgpc::runtime::RuntimeContext &runtime,
        std::size_t selected_slot) {
        const auto layouts = runtime.scheduler().debug_layout_runtime_snapshot();
        auto numbers = std::vector<const smgpc::runtime::SceneLayoutRuntimeDebugState *>{};
        for (const auto &layout : layouts) {
            if (layout.name == "ファイル番号" &&
                layout.layout_name == "FileNumber") {
                numbers.push_back(&layout);
            }
        }
        require(numbers.size() == cExpectedSlots.size(),
                "selection proof did not find the six exact FileNumber layouts");
        for (auto index = std::size_t{}; index < numbers.size(); ++index) {
            const auto &animations = numbers[index]->animations;
            require(animations.size() > 1U && animations[1].active &&
                        animations[1].name ==
                            (index == selected_slot ? "SelectIn" : "SelectOut"),
                    "highlight did not centralize exact FileNumber SelectIn/Out");
        }
    }
#endif

    void require_planet_scales(
        const smgpc::scene::FileSelectFarVisual &visual,
        const std::array<float, 6U> &expected,
        std::string_view context) {
        const auto slots = visual.slots();
        for (auto index = std::size_t{}; index < slots.size(); ++index) {
            require_near(slots[index].planet->mScale.x, expected[index], 0.002F,
                         std::string(context) + " scale X");
            require_near(slots[index].planet->mScale.y, expected[index], 0.002F,
                         std::string(context) + " scale Y");
            require_near(slots[index].planet->mScale.z, expected[index], 0.002F,
                         std::string(context) + " scale Z");
        }
    }

    void require_stage_light_loaded() {
        auto &light_data =
            smgpc::render::light::StageLightData::instance();
        require(std::ranges::any_of(light_data.stage_zones(), [](const auto &zone) {
                    return zone.zone_id == 0 && zone.zone_name == "FileSelect";
                }),
                "far File Select did not bind the authored root StageLight zone");
        const auto *default_name = light_data.default_area_light_name();
        require(default_name != nullptr && default_name[0] != '\0' &&
                    LightFunction::getAreaLightInfo(ZoneLightID{}) != nullptr,
                "far File Select did not load authored AreaLight data");
    }

    std::unique_ptr<smgpc::scene::FileSelectFarVisual> create_far_visual(
        smgpc::runtime::RuntimeContext &runtime, FileSelectSky **identity_out) {
        auto title =
            std::make_unique<smgpc::scene::TitleFileSelectVisual>(runtime);
        auto *identity = title->sky();
        auto handoff = title->release_sky_for_file_select();
        require(handoff.sky() == identity,
                "title handoff did not retain the exact sky identity");
        title.reset();

        auto far = std::make_unique<smgpc::scene::FileSelectFarVisual>(
            runtime, std::move(handoff));
        require(far->sky() == identity,
                "far composition replaced the transferred FileSelectSky");
        if (identity_out != nullptr) {
            *identity_out = identity;
        }
        return far;
    }

    void test_production_file_select_far_visual() {
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
            .title = "SMG PC production File Select far proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
        runtime.set_current_stage_name("FileSelect");
        const auto renderer_context =
            smgpc::render::ScopedAuroraRendererContext(renderer);

        const auto baseline_name_objs =
            smgpc::compat::name_obj_runtime_state_count();
        const auto baseline_actors = smgpc::compat::actor_runtime_state_count();
        const auto baseline_scheduler = runtime.scheduler().snapshot().size();
        const auto camera_end_baseline =
            runtime.camera_system().programmable_camera_end_count();

        auto *first_sky_identity = static_cast<FileSelectSky *>(nullptr);
        {
            auto far = create_far_visual(runtime, &first_sky_identity);
            require(first_sky_identity != nullptr &&
                        runtime.game_layout().is_game_scene_draw_3d_active() &&
                        far->camera_controller() != nullptr &&
                        !far->is_at_far_point() && !far->numbers_visible() &&
                        !far->highlighted_slot().has_value(),
                    "far composition did not begin from the retained title state");
            require(smgpc::compat::name_obj_runtime_state_count() ==
                            baseline_name_objs + 21U &&
                        smgpc::compat::actor_runtime_state_count() ==
                            baseline_actors + 14U &&
                        runtime.scheduler().snapshot().size() ==
                            baseline_scheduler + 15U,
                    "far composition did not establish unique scene ownership");
            require_authored_slots(*far);
            require_stage_light_loaded();
#ifndef NDEBUG
            require_scheduler_composition(runtime);
#else
            throw std::runtime_error(
                "production File Select far proof requires a debug build");
#endif

            const auto initial_pose =
                runtime.camera_system().active_programmable_camera_pose();
            require(initial_pose.has_value(),
                    "exact File Select controller did not own the camera");
            require_pose(*initial_pose, {0.0F, 15800.0F, 15000.0F},
                         {0.0F, 15800.0F, 0.0F}, 60.0F,
                         "seeded title camera");

            // goToFarPoint installs the Move nerve at step -1. The first
            // scheduler update executes retail step 0, so 60 updates end at
            // step 60 and the 61st executes the exact setNerveAtStep(..., 60)
            // boundary. This is the original 60-frame interpolation contract,
            // including its step-zero sample.
            for (auto frame_index = std::uint64_t{1U}; frame_index <= 60U;
                 ++frame_index) {
                render_frame(runtime, renderer, frame_index);
            }
            require(!far->is_at_far_point() && !far->numbers_visible(),
                    "File Select camera reached Far before retail step 60");

            render_frame(runtime, renderer, 61U);
            require(far->is_at_far_point() && !far->numbers_visible(),
                    "File Select camera missed retail step 60 or exposed indices early");
            render_frame(runtime, renderer, 62U);
            require(!far->numbers_visible(),
                    "FileSelectNumber skipped FileSelector's two-frame nerve handoff");

            const auto far_pose =
                runtime.camera_system().active_programmable_camera_pose();
            require(far_pose.has_value(),
                    "far programmable camera pose disappeared");
            require_pose(*far_pose, {0.0F, 0.0F, 15000.0F},
                         {0.0F, 800.0F, 0.0F}, 40.0F, "far camera");

            render_frame(runtime, renderer, 63U, true);
            require(far->numbers_visible() &&
                        std::ranges::all_of(far->slots(), [](const auto &slot) {
                            const auto trans = slot.number->getTrans();
                            return !slot.number->mFlag.mIsDead &&
                                   std::isfinite(trans.x) && std::isfinite(trans.y);
                        }),
                    "six projected FileSelectNumber layouts did not appear at Far");

#ifndef NDEBUG
            const auto planet_packets = std::ranges::count_if(
                runtime.j3d_packet_trace(), [](const auto &packet) {
                    return packet.model_name == "FileSelectDataPlanet" &&
                           packet.frame_index == 63U &&
                           packet.state.source_triangle_count != 0U &&
                           packet.state.parsed_display_list_bytes != 0U;
                });
            const auto number_packets = std::ranges::count_if(
                runtime.layout_packet_trace(), [](const auto &packet) {
                    return packet.layout_name == "FileNumber" &&
                           packet.frame_index == 63U &&
                           packet.vertex_count != 0U &&
                           packet.index_count != 0U;
                });
            require(planet_packets >= 6U,
                    "six real FileSelectDataPlanet actors submitted no J3D packets");
            require(number_packets >= 6U,
                    "six real FileNumber layouts submitted no layout packets");
#endif

            const auto *area_light =
                LightFunction::getAreaLightInfo(ZoneLightID{});
            const auto &ambient = runtime.scene_lights().actor_ambient();
            require(area_light != nullptr && ambient.has_value() &&
                        *ambient == smgpc::render::GXColorValue{
                                        area_light->mPlanetLight.mColor.r,
                                        area_light->mPlanetLight.mColor.g,
                                        area_light->mPlanetLight.mColor.b,
                                        area_light->mPlanetLight.mColor.a},
                    "MapObj planet draw did not consume authored StageLight");

            auto rejected_invalid_slot = false;
            try {
                far->set_highlighted_slot(6U);
            } catch (const std::out_of_range &) {
                rejected_invalid_slot = true;
            }
            require(rejected_invalid_slot &&
                        !far->highlighted_slot().has_value(),
                    "highlight API accepted a slot outside the authored six");

            constexpr auto cAllNormal =
                std::array<float, 6U>{30.0F, 30.0F, 30.0F,
                                     30.0F, 30.0F, 30.0F};
            require_planet_scales(*far, cAllNormal, "initial blank planet");
            far->set_highlighted_slot(2U);
            require(far->highlighted_slot() == 2U,
                    "highlight API did not retain the selected authored slot");
            require_planet_scales(*far, cAllNormal,
                                  "selection request before movement");

            // ScaleController begins at nerve step zero, so its first update
            // deliberately retains scale 30 before converging toward 36.
            render_frame(runtime, renderer, 64U);
            require_planet_scales(*far, cAllNormal,
                                  "selection transition step zero");
#ifndef NDEBUG
            require_number_selection_animation(runtime, 2U);
#endif
            for (auto frame_index = std::uint64_t{65U}; frame_index <= 74U;
                 ++frame_index) {
                render_frame(runtime, renderer, frame_index);
            }
            require_planet_scales(
                *far,
                {30.0F, 30.0F, 35.261448F, 30.0F, 30.0F, 30.0F},
                "selection transition source step ten");
            for (auto frame_index = std::uint64_t{75U}; frame_index <= 94U;
                 ++frame_index) {
                render_frame(runtime, renderer, frame_index);
            }
            require_planet_scales(
                *far, {30.0F, 30.0F, 36.0F, 30.0F, 30.0F, 30.0F},
                "selection transition source step thirty");

            // A new selection sends the prior exact number SelectOut and
            // reverses its live scale through the same source recurrence.
            far->set_highlighted_slot(4U);
            require(far->highlighted_slot() == 4U,
                    "highlight switch did not retain the new authored slot");
            render_frame(runtime, renderer, 95U);
#ifndef NDEBUG
            require_number_selection_animation(runtime, 4U);
#endif
            for (auto frame_index = std::uint64_t{96U}; frame_index <= 105U;
                 ++frame_index) {
                render_frame(runtime, renderer, frame_index);
            }
            require_planet_scales(
                *far,
                {30.0F, 30.0F, 30.738556F, 30.0F, 35.261448F, 30.0F},
                "highlight switch source step ten");
            for (auto frame_index = std::uint64_t{106U}; frame_index <= 125U;
                 ++frame_index) {
                render_frame(runtime, renderer, frame_index);
            }
            require_planet_scales(
                *far, {30.0F, 30.0F, 30.0F, 30.0F, 36.0F, 30.0F},
                "highlight switch source step thirty");
        }

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
                "destroying far File Select left child, light, camera, or sky state");

        // Recreate the same production composition in one RuntimeContext. This
        // proves the explicit child owner and StageLight singleton both retire
        // cleanly rather than merely surviving until process shutdown.
        auto *second_sky_identity = static_cast<FileSelectSky *>(nullptr);
        {
            auto far = create_far_visual(runtime, &second_sky_identity);
            require(second_sky_identity != nullptr &&
                        far->sky() == second_sky_identity,
                    "recreated far composition did not own a fresh exact sky");
            require_authored_slots(*far);
            require_stage_light_loaded();
            render_frame(runtime, renderer, 201U);
        }
        require(smgpc::compat::name_obj_runtime_state_count() ==
                        baseline_name_objs &&
                    smgpc::compat::actor_runtime_state_count() == baseline_actors &&
                    runtime.scheduler().snapshot().size() == baseline_scheduler &&
                    smgpc::render::light::StageLightData::instance()
                        .stage_zones()
                        .empty() &&
                    runtime.camera_system().programmable_camera_end_count() ==
                        camera_end_baseline + 2U,
                "recreated far composition did not tear down cleanly");

        std::cout << "[proof] disc=" << disc_path.string()
                  << ";sky_identity=retained;camera_transition=step60"
                  << ";slots=1,2,4,6,5,3;planets=6;numbers=6"
                  << ";stage_light=authored;recreate=clean" << '\n';
    }

}  // namespace

int main() {
    try {
        test_production_file_select_far_visual();
        std::cout << "[ok] production File Select far composition owns exact shared visuals\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] production File Select far visual: " << error.what()
                  << '\n';
        return 1;
    }
}
