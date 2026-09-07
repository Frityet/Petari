#include "Game/AreaObj/AreaForm.hpp"
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/MapObj/BrightObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/LensFlare.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/GameDataSession.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/GatewayDemoScene.hpp"
#include "GatewayDemoSceneTestSupport.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
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

    void require_angle_near(float actual, float expected, float tolerance,
                            std::string_view message) {
        if (!std::isfinite(actual) ||
            std::fabs(std::remainder(actual - expected, 360.0F)) > tolerance) {
            throw std::runtime_error(std::string(message) + ";actual=" +
                                     std::to_string(actual) + ";expected=" +
                                     std::to_string(expected));
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
                "the BrightSun route proof requires a readable working directory");
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
            "the BrightSun route proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

    [[nodiscard]] float dot(const TVec3f& left, const TVec3f& right) {
        return left.x * right.x + left.y * right.y + left.z * right.z;
    }

    [[nodiscard]] smgpc::camera::CameraPose camera_along(
        const TVec3f& eye, const TVec3f& direction) {
        require(direction.length() > 0.001F,
                "the authored BrightSun direction must be non-degenerate");
        return smgpc::camera::CameraPose{
            .eye = {eye.x, eye.y, eye.z},
            .watch = {eye.x + direction.x, eye.y + direction.y,
                      eye.z + direction.z},
            .up = {0.0F, 1.0F, 0.0F},
            .fovy_degrees = 45.0F,
            .aspect_ratio = 608.0F / 456.0F,
            .near_clip = 100.0F,
            .far_clip = 800000.0F,
        };
    }

#ifndef NDEBUG
    struct FramePacketProof {
        std::uint64_t frame_index = 0U;
        std::size_t sun_packets = 0U;
        std::size_t ring_packets = 0U;
        std::size_t glow_packets = 0U;
        std::size_t line_packets = 0U;

        [[nodiscard]] bool has_all_flares() const {
            return ring_packets != 0U && glow_packets != 0U &&
                   line_packets != 0U;
        }

        [[nodiscard]] bool has_any_flare() const {
            return ring_packets != 0U || glow_packets != 0U ||
                   line_packets != 0U;
        }
    };

    [[nodiscard]] std::size_t packet_count(
        const smgpc::runtime::RuntimeContext& runtime, std::string_view model_name,
        std::uint64_t frame_index) {
        return std::ranges::count_if(
            runtime.j3d_packet_trace(), [&](const auto& packet) {
                return packet.frame_index == frame_index &&
                       packet.model_name == model_name &&
                       packet.state.source_triangle_count != 0U;
            });
    }

    [[nodiscard]] FramePacketProof packet_proof(
        const smgpc::runtime::RuntimeContext& runtime,
        std::uint64_t frame_index) {
        return FramePacketProof{
            .frame_index = frame_index,
            .sun_packets = packet_count(runtime, "Sun", frame_index),
            .ring_packets = packet_count(runtime, "LensFlare", frame_index),
            .glow_packets = packet_count(runtime, "GlareGlow", frame_index),
            .line_packets = packet_count(runtime, "GlareLine", frame_index),
        };
    }

    void require_brk_packet(
        const smgpc::runtime::RuntimeContext& runtime,
        std::uint64_t frame_index, std::string_view model_name,
        std::string_view material_name, std::int16_t frame_max,
        std::uint16_t track_count, std::uint8_t maximum_alpha) {
        const auto packet = std::ranges::find_if(
            runtime.j3d_packet_trace(), [&](const auto& candidate) {
                return candidate.frame_index == frame_index &&
                       candidate.model_name == model_name &&
                       candidate.state.material_name == material_name;
            });
        require(packet != runtime.j3d_packet_trace().end(),
                "a real flare BRK target material did not submit a packet");
        const auto& state = packet->state;
        require(state.packet_mode ==
                        smgpc::render::J3dRendererPacketMode::ShaderGxTev &&
                    state.brk_active && state.brk_frame_max == frame_max &&
                    state.brk_color_track_count == 0U &&
                    state.brk_konst_track_count == track_count &&
                    state.gx_initial_tev_k_colors[0U][3U] > 0U &&
                    state.gx_initial_tev_k_colors[0U][3U] <= maximum_alpha,
                "a real flare packet did not carry its evaluated TRK1 konst-alpha state");
    }

    void require_exact_flare_brk_packets(
        const smgpc::runtime::RuntimeContext& runtime,
        std::uint64_t frame_index) {
        require_brk_packet(runtime, frame_index, "LensFlare", "LensFleareMat",
                           100, 1U, 40U);
        require_brk_packet(runtime, frame_index, "GlareGlow",
                           "pasted__heatwave", 119, 2U, 125U);
        require_brk_packet(runtime, frame_index, "GlareGlow",
                           "pasted__heatwave(2)", 119, 2U, 50U);
        constexpr auto line_materials =
            std::array<std::string_view, 3U>{
                "glare_b_03", "glare_b_03(2)", "glare_b_03(3)"};
        for (const auto material : line_materials) {
            require_brk_packet(runtime, frame_index, "GlareLine", material,
                               120, 3U, 145U);
        }
    }

    void require_exact_scheduler_route(
        const smgpc::runtime::RuntimeContext& runtime,
        std::string_view bright_runtime_name) {
        const auto entries = runtime.scheduler().snapshot();
        const auto has_entry = [&](std::string_view name,
                                   smgpc::runtime::SceneEntryKind kind,
                                   s32 movement, s32 calc_anim, s32 draw_buffer,
                                   s32 draw_type) {
            return std::ranges::any_of(entries, [&](const auto& entry) {
                return entry.name == name && entry.kind == kind &&
                       entry.movement_type == movement &&
                       entry.calc_anim_type == calc_anim &&
                       entry.draw_buffer_type == draw_buffer &&
                       entry.draw_type == draw_type;
            });
        };

        require(
            has_entry(bright_runtime_name,
                      smgpc::runtime::SceneEntryKind::NameObj,
                      MR::MovementType_Environment, -1, -1,
                      MR::DrawType_BrightSun) &&
                has_entry("太陽",
                          smgpc::runtime::SceneEntryKind::LiveActorModel,
                          MR::MovementType_Sky, MR::CalcAnimType_MapObj,
                          MR::DrawBufferType_Sun, -1) &&
                has_entry("レンズフレアリング",
                          smgpc::runtime::SceneEntryKind::LiveActorModel,
                          MR::MovementType_Layout, MR::CalcAnimType_Layout,
                          MR::DrawBufferType_Model3DFor2D, -1) &&
                has_entry("グレア（円形）",
                          smgpc::runtime::SceneEntryKind::LiveActorModel,
                          MR::MovementType_Layout, MR::CalcAnimType_Layout,
                          MR::DrawBufferType_Model3DFor2D, -1) &&
                has_entry("グレア（ライン）",
                          smgpc::runtime::SceneEntryKind::LiveActorModel,
                          MR::MovementType_Layout, MR::CalcAnimType_Layout,
                          MR::DrawBufferType_Model3DFor2D, -1),
            "BrightSun, Sun, or a flare child bypassed its exact retail scheduler category");
    }
#endif

    void test_real_gateway_bright_sun_route() {
#ifdef NDEBUG
        throw std::runtime_error(
            "the BrightSun packet and scheduler route proof requires a debug build");
#else
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
            .title = "SMG PC exact Gateway BrightSun route proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        renderer.set_copy_clear({
            .color = {0U, 0U, 0U, 255U},
            .depth = GX_MAX_Z24,
        });
        auto resource_runtime = smgpc::resource::GameResourceRuntime{};
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window, resource_runtime);
        runtime.set_current_stage_name("HeavensDoorGalaxy");
        const auto renderer_context =
            smgpc::render::ScopedAuroraRendererContext(renderer);

        // Prime Aurora's display-copy/depth pipeline before asking the exact
        // BrightSun pass for tagged snapshots.
        for (auto index = 0U; index < 2U; ++index) {
            static_cast<void>(renderer.begin_frame());
            renderer.end_frame();
        }

        const auto seed_eye = TVec3f{13480.0F, -21226.259766F, -6110.0F};
        const auto seed_camera = camera_along(seed_eye, TVec3f{0.0F, 0.0F, 1.0F});
        runtime.camera_system().set_game_camera_pose(seed_camera);
        auto initial_frame = renderer.begin_frame();
        runtime.begin_frame(initial_frame);
        runtime.set_scene_camera_pose(seed_camera);

        auto game_data_session = smgpc::compat::GameDataSession{1U};
        {
            auto scene = smgpc::scene::GatewayDemoScene(runtime.dvd());
            auto player = smgpc::test::GatewayPlayerSentinel{runtime, scene};
            auto placement_lease = scene.finalize_placements(player);

            const auto bright_placement = std::ranges::find_if(
                scene.placements(), [](const auto& placement) {
                    return placement.object_name == "BrightSun" &&
                           placement.zone_name == "HeavensDoorGalaxy" &&
                           placement.table_path ==
                               "jmp/placement/common/objinfo";
                });
            require(bright_placement != scene.placements().end() &&
                        bright_placement->jmap_entry_index == 1 &&
                        bright_placement->l_id == 10 &&
                        bright_placement->zone_id == 0 &&
                        bright_placement->layer_name == "common" &&
                        bright_placement->factory_supported,
                    "Gateway did not select the exact root-common BrightSun row");
            require_near(bright_placement->translation[0], 26110.0F, 0.001F,
                         "BrightSun authored X");
            require_near(bright_placement->translation[1], 0.0F, 0.001F,
                         "BrightSun authored Y");
            require_near(bright_placement->translation[2], -35950.0F, 0.001F,
                         "BrightSun authored Z");
            require_angle_near(bright_placement->rotation[0], 180.791F, 0.001F,
                               "BrightSun authored rotation X");
            require_angle_near(bright_placement->rotation[1], 360.0F, 0.001F,
                               "BrightSun authored rotation Y");
            require_angle_near(bright_placement->rotation[2], 186.921005F,
                               0.001F, "BrightSun authored rotation Z");

            const auto bright_visual = std::ranges::find_if(
                scene.visuals(), [&](const auto& visual) {
                    return visual.placement == &*bright_placement;
                });
            auto* bright =
                bright_visual != scene.visuals().end()
                    ? dynamic_cast<BrightSun*>(bright_visual->actor)
                    : nullptr;
            auto* director = dynamic_cast<LensFlareDirector*>(
                scene.scene_obj_holder().getObj(SceneObj_LensFlareDirector));
            require(bright != nullptr && director != nullptr &&
                        director->mBrightObjArray.size() == 1U &&
                        director->mBrightObjArray[0] == bright &&
                        director->mRing != nullptr && director->mGlow != nullptr &&
                        director->mLine != nullptr,
                    "the authored BrightSun did not create its exact director and three children");
            require(bright->getName() != nullptr &&
                        std::string_view(bright->getName()) ==
                            "レンズフレア用太陽",
                    "BrightSun did not retain its exact ObjNameTable actor identity");
            const auto bright_runtime_name = std::string(bright->getName());

            const auto require_model = [](LiveActor* actor,
                                          std::string_view actor_name,
                                          std::string_view archive_name) {
                auto* model = smgpc::compat::actor_model(actor);
                require(actor != nullptr && actor->getName() == actor_name &&
                            model != nullptr &&
                            model->model_arc_name() == archive_name,
                        "a LensFlareDirector child has the wrong actor or model identity");
                model->requireLoaded();
                require(model->isLoaded(),
                        "a real LensFlareDirector child model did not load");
            };
            require_model(director->mRing, "レンズフレアリング", "LensFlare");
            require_model(director->mGlow, "グレア（円形）", "GlareGlow");
            require_model(director->mLine, "グレア（ライン）", "GlareLine");
            require_exact_scheduler_route(runtime, bright_runtime_name);

            const auto flare_placement = std::ranges::find_if(
                scene.placements(), [](const auto& placement) {
                    return placement.object_name == "LensFlareArea" &&
                           placement.zone_name == "HeavensDoorGalaxy" &&
                           placement.table_path ==
                               "jmp/placement/common/areaobjinfo";
                });
            require(flare_placement != scene.placements().end() &&
                        flare_placement->jmap_entry_index == 4 &&
                        flare_placement->l_id == 6 &&
                        flare_placement->zone_id == 0 &&
                        flare_placement->layer_name == "common" &&
                        flare_placement->factory_supported &&
                        flare_placement->object_args[0] == -1,
                    "Gateway did not select the exact all-components LensFlareArea row");
            require_near(flare_placement->translation[0], 13480.0F, 0.001F,
                         "LensFlareArea authored X");
            require_near(flare_placement->translation[1], -21226.259766F, 0.001F,
                         "LensFlareArea authored Y");
            require_near(flare_placement->translation[2], -6110.0F, 0.001F,
                         "LensFlareArea authored Z");
            for (const auto scale : flare_placement->scale) {
                require_near(scale, 53.11F, 0.0001F,
                             "LensFlareArea authored scale");
            }

            auto* area_container = dynamic_cast<AreaObjContainer*>(
                scene.scene_obj_holder().getObj(SceneObj_AreaObjContainer));
            auto* area_manager = area_container != nullptr
                                     ? area_container->getManager("LensFlareArea")
                                     : nullptr;
            auto* flare_area =
                area_manager != nullptr && area_manager->mArray.size() == 1U
                    ? area_manager->getAreaObj(0)
                    : nullptr;
            auto* flare_cube =
                flare_area != nullptr
                    ? dynamic_cast<AreaFormCube*>(flare_area->mForm)
                    : nullptr;
            require(flare_area != nullptr && flare_cube != nullptr &&
                        flare_area->mFormType == AreaForm::Type_Cube2 &&
                        flare_area->mObjArg0 == -1 && flare_area->isValid(),
                    "the exact LensFlareArea row did not enter its shared Cube2 manager");

            TPos3f flare_area_matrix;
            flare_cube->calcWorldMtx(&flare_area_matrix);
            const auto local_inside = TVec3f{
                (flare_cube->mBounding.i.x + flare_cube->mBounding.f.x) * 0.5F,
                (flare_cube->mBounding.i.y + flare_cube->mBounding.f.y) * 0.5F,
                (flare_cube->mBounding.i.z + flare_cube->mBounding.f.z) * 0.5F,
            };
            auto inside = TVec3f{};
            flare_area_matrix.mult(local_inside, inside);
            const auto local_outside = TVec3f{
                flare_cube->mBounding.f.x + 1000.0F,
                local_inside.y,
                local_inside.z,
            };
            auto outside = TVec3f{};
            flare_area_matrix.mult(local_outside, outside);
            require(area_manager->find_in(inside) == flare_area &&
                        area_manager->find_in(outside) == nullptr,
                    "the real LensFlareArea volume did not accept its center and reject an exterior point");

            player.mPosition.set(inside);
            player.calcAndSetBaseMtx();
            runtime.player_system().synchronize_attached_actor();

            // Let the authored rotation produce the actual camera-relative Sun
            // position once, then point exactly opposite it for the initial
            // off-screen proof. This observes actor behavior instead of
            // reproducing BrightSun's rotation math in the test.
            runtime.scheduler().execute_movement();
            const auto sun_direction = bright->mPosition - seed_eye;
            require_near(sun_direction.length(), 100000.0F, 0.5F,
                         "BrightSun camera-relative distance");
            const auto away_direction = TVec3f{-sun_direction.x,
                                               -sun_direction.y,
                                               -sun_direction.z};
            const auto away_camera = camera_along(seed_eye, away_direction);
            const auto toward_camera = camera_along(seed_eye, sun_direction);
            require(dot(sun_direction, away_direction) < 0.0F,
                    "the initial camera must face away from the authored Sun");

            runtime.set_scene_camera_pose(away_camera);
            runtime.scheduler().execute_movement();
            runtime.scheduler().execute_calc_anim();
            runtime.scheduler().execute_calc_view_and_entry();
            runtime.set_j3d_packet_trace_frame(initial_frame.frame_index);
            runtime.draw_3d_normal(away_camera);
            runtime.draw_2d_normal();
            const auto initial_packets =
                packet_proof(runtime, initial_frame.frame_index);
            require(initial_packets.sun_packets != 0U &&
                        !initial_packets.has_any_flare() &&
                        director->mRing->mFlag.mIsHiddenModel &&
                        director->mGlow->mFlag.mIsHiddenModel &&
                        director->mLine->mFlag.mIsHiddenModel,
                    "an off-screen authored Sun must draw its Sun model but no flare child");
            renderer.end_frame();

            const auto run_frame = [&](const smgpc::camera::CameraPose& camera) {
                runtime.camera_system().set_game_camera_pose(camera);
                const auto frame = renderer.begin_frame();
                runtime.begin_frame(frame);
                runtime.set_scene_camera_pose(camera);
                runtime.set_j3d_packet_trace_frame(frame.frame_index);
                runtime.draw_3d_normal(camera);
                runtime.draw_2d_normal();
                const auto result = packet_proof(runtime, frame.frame_index);
                renderer.end_frame();
                return result;
            };

            auto visible_packets = FramePacketProof{};
            constexpr auto cVisibilityFrameBudget = 16U;
            for (auto index = 0U; index < cVisibilityFrameBudget; ++index) {
                const auto packets = run_frame(toward_camera);
                require(packets.sun_packets != 0U,
                        "the scheduler omitted the real Sun while awaiting tagged depth");
                if (packets.has_all_flares()) {
                    visible_packets = packets;
                    break;
                }
                require(!packets.has_any_flare(),
                        "the three exact flare children must become visible atomically");
            }
            require(visible_packets.has_all_flares() && director->mBright > 0.0F &&
                        !director->mRing->mFlag.mIsHiddenModel &&
                        !director->mGlow->mFlag.mIsHiddenModel &&
                        !director->mLine->mFlag.mIsHiddenModel,
                    "the real tagged-depth result never exposed all three Model3DFor2D flare children");
            require_exact_flare_brk_packets(runtime,
                                            visible_packets.frame_index);

            // Leaving the exact authored area while looking away exercises the
            // retail 0.05 fade. The children must keep submitting during all
            // twenty fade steps, then the Kill nerve hides and retires them on
            // the following movement.
            player.mPosition.set(outside);
            player.calcAndSetBaseMtx();
            runtime.player_system().synchronize_attached_actor();
            constexpr auto cFadeSteps = 20U;
            for (auto step = 0U; step < cFadeSteps; ++step) {
                const auto packets = run_frame(away_camera);
                require(packets.sun_packets != 0U && packets.has_all_flares() &&
                            !director->mRing->mFlag.mIsDead &&
                            !director->mGlow->mFlag.mIsDead &&
                            !director->mLine->mFlag.mIsDead,
                        "a 0.05 LensFlare fade retired a child before twenty steps");
            }
            const auto hidden_packets = run_frame(away_camera);
            require(hidden_packets.sun_packets != 0U &&
                        !hidden_packets.has_any_flare() &&
                        director->mRing->mFlag.mIsDead &&
                        director->mGlow->mFlag.mIsDead &&
                        director->mLine->mFlag.mIsDead &&
                        director->mRing->mFlag.mIsHiddenModel &&
                        director->mGlow->mFlag.mIsHiddenModel &&
                        director->mLine->mFlag.mIsHiddenModel,
                    "looking away and leaving LensFlareArea did not complete the exact fade/hide path");

            std::cout << "[proof] disc=" << disc_path.string()
                      << ";visible_frame=" << visible_packets.frame_index
                      << ";sun_packets=" << visible_packets.sun_packets
                      << ";ring_packets=" << visible_packets.ring_packets
                      << ";glow_packets=" << visible_packets.glow_packets
                      << ";line_packets=" << visible_packets.line_packets
                      << ";fade_steps=" << cFadeSteps << '\n';
        }
#endif
    }

}  // namespace

int main() {
    try {
        test_real_gateway_bright_sun_route();
        std::cout << "[ok] exact Gateway BrightSun tagged-depth and LensFlare route\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] Gateway BrightSun route: " << error.what() << '\n';
        return 1;
    }
}
