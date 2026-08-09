#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/Air.hpp"
#include "Game/Map/PlanetMap.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Logger.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "render/RendererService.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/GatewayDemoScene.hpp"
#include "scene/NameObjLifecycleService.hpp"
#include "scene/SceneExecutionService.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <cmath>
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
        require(!error, "the Air actor proof requires a readable working directory");
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
            "the Air actor proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

    [[nodiscard]] smgpc::scene::NameObjPlacementContext placement_context(
        const smgpc::scene::StagePlacementObject &placement) {
        return smgpc::scene::NameObjPlacementContext{
            .iter = JMapInfoIter(&placement.jmap_info, placement.jmap_entry_index),
            .source = smgpc::scene::NameObjPlacementSource::StagePlacement,
            .stage_name = placement.stage_name,
            .zone_name = placement.zone_name,
            .table_path = placement.table_path,
            .row = placement.jmap_entry_index,
            .local_id = placement.l_id,
        };
    }

    void require_factory_mapping(smgpc::runtime::DvdFileSystemService &dvd,
                                 std::string_view object_name) {
        const auto description =
            smgpc::scene::nameobj::describe_name_obj_factory(dvd, object_name);
        require(description.creator_supported &&
                    description.creator_support.kind ==
                        smgpc::scene::nameobj::NameObjCreatorSupportKind::Supported &&
                    smgpc::scene::nameobj::scene_visual_kind(object_name) ==
                        smgpc::scene::nameobj::NameObjSceneVisualKind::Air &&
                    description.archives.size() == 1U &&
                    description.archives.front().archive_name == object_name &&
                    description.archives.front().kind ==
                        smgpc::scene::nameobj::NameObjArchiveKind::Object,
                "retail Air factory mapping or same-name archive is unavailable");
    }

#ifndef NDEBUG
    [[nodiscard]] std::size_t require_trace_index(
        std::span<const smgpc::runtime::SceneSchedulerEntryState> trace,
        std::string_view name, smgpc::runtime::SceneSchedulerPhase phase) {
        const auto found = std::ranges::find_if(trace, [&](const auto &entry) {
            return entry.name == name && entry.phase == phase;
        });
        require(found != trace.end(), "required scheduler pass is absent from the trace");
        return static_cast<std::size_t>(std::distance(trace.begin(), found));
    }

    [[nodiscard]] std::size_t packet_count(
        const smgpc::runtime::RuntimeContext &runtime, std::string_view model_name,
        std::uint64_t frame_index) {
        return std::ranges::count_if(runtime.j3d_packet_trace(), [&](const auto &packet) {
            return packet.model_name == model_name && packet.frame_index == frame_index &&
                   packet.state.source_triangle_count != 0U;
        });
    }
#endif

    void test_exact_air_and_prior_draw_route() {
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
            .title = "SMG PC exact Air/PriorDrawAir proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
        runtime.set_current_stage_name("HeavensDoorGalaxy");

        for (const auto name : {"HomeAir", "SphereAir", "SunsetAir", "FineAir",
                                "DimensionAir", "DarknessRoomAir", "TwilightAir"}) {
            require_factory_mapping(runtime.dvd(), name);
        }

        auto frame = renderer.begin_frame();
        frame.frame_index = 90U;
        const auto renderer_context =
            smgpc::render::ScopedAuroraRendererContext(renderer);
        runtime.begin_frame(frame);
#ifndef NDEBUG
        runtime.set_j3d_packet_trace_frame(frame.frame_index);
#endif

        {
            auto scene = smgpc::scene::GatewayDemoScene{runtime.dvd()};
            const auto found = std::ranges::find_if(
                scene.placements(), [](const auto &placement) {
                    return placement.object_name == "SphereAir" &&
                           placement.zone_name == "HeavensDoorMysteriousZone" &&
                           placement.table_path == "jmp/placement/common/objinfo";
                });
            require(found != scene.placements().end() &&
                        found->jmap_entry_index == 25 && found->l_id == 70 &&
                        found->zone_id == 5 && found->layer_name == "common" &&
                        found->factory_supported && found->clipping_group_id == -1 &&
                        found->demo_group_id == -1 && found->object_args[0] == -1 &&
                        found->object_args[1] == -1 &&
                        found->object_archive_path.ends_with("/ObjectData/SphereAir.arc"),
                    "Gateway did not retain the exact supported SphereAir row");
            require_near(found->translation[0], 14760.0F, 0.001F,
                         "SphereAir world X");
            require_near(found->translation[1], -10676.2255859375F, 0.001F,
                         "SphereAir world Y");
            require_near(found->translation[2], 6770.0F, 0.001F,
                         "SphereAir world Z");
            require_near(found->scale[0], 0.9F, 0.0001F, "SphereAir scale X");
            require_near(found->scale[1], 0.9F, 0.0001F, "SphereAir scale Y");
            require_near(found->scale[2], 0.9F, 0.0001F, "SphereAir scale Z");

            const auto context = placement_context(*found);
            const auto requests =
                smgpc::scene::nameobj::collect_name_obj_archive_requests(
                    runtime.dvd(), "SphereAir", &context.iter);
            require(requests.size() == 1U &&
                        requests.front().archive_name == "SphereAir" &&
                        requests.front().loaded,
                    "the exact SphereAir archive was not accepted by the retail lifecycle");

            const auto ordinary_planet_count = std::ranges::count_if(
                scene.visuals(), [](const auto &entry) {
                    return dynamic_cast<PlanetMap *>(entry.actor) != nullptr;
                });
            require(scene.visuals().size() == 7U && ordinary_planet_count == 4,
                    "Gateway scenario 1 must select Sky, Air, BrightSun, and four ordinary planets through one visual path");
            const auto visual = std::ranges::find_if(
                scene.visuals(), [&](const auto &entry) {
                    return entry.placement == &*found;
                });
            require(visual != scene.visuals().end(),
                    "SphereAir is absent from Gateway's generic authored visual list");
            auto *air = dynamic_cast<PriorDrawAir *>(visual->actor);
            auto *air_model = smgpc::compat::actor_model(air);
            require(air != nullptr && air_model != nullptr &&
                        air_model->model_arc_name() == "SphereAir" &&
                        MR::isExistSceneObj(SceneObj_PriorDrawAirHolder),
                    "SphereAir did not construct exact PriorDrawAir and its scene holder");
            air_model->requireLoaded();

            // The scene owns MarioHolder before the real Mario actor is attached.
            // Exact Air must remain alive but hidden rather than dereferencing a
            // null player position or inventing a Gateway-specific deferred ctor.
            require(runtime.player_system().attached_actor() == nullptr &&
                        !air->isDrawing() && !MR::isExistPriorDrawAir() &&
                        smgpc::compat::actor_current_brk_name(air) == "Disappear",
                    "unattached-player Air initialization was not the exact hidden Out state");

            auto planet = LiveActor{"Planet pass sentinel"};
            // This actor is intentionally a scheduler-only sentinel. SphereAir
            // remains the real archived/modelled draw proof below; loading the
            // full Gateway planet here would duplicate an unrelated multi-MiB
            // geometry submission merely to observe pass ordering.
            MR::connectToScene(&planet, -1, MR::CalcAnimType_MapObj,
                               MR::DrawBufferType_Planet, -1);
            planet.makeActorAppeared();

            const auto camera = smgpc::camera::CameraPose{
                .eye = {found->translation[0], found->translation[1],
                        found->translation[2] + 2251.0F},
                .watch = {found->translation[0], found->translation[1],
                          found->translation[2]},
                .up = {0.0F, 1.0F, 0.0F},
                .near_clip = 1.0F,
            };
            runtime.set_scene_camera_pose(camera);
            auto execution = smgpc::scene::SceneExecutionService{runtime};
            execution.execute_movement();
            execution.execute_calc_anim_and_view();
            execution.draw_3d_normal(camera);

#ifndef NDEBUG
            {
                const auto trace = runtime.scheduler().last_execution_trace();
                const auto planet_opa = require_trace_index(
                    trace, "Planet pass sentinel",
                    smgpc::runtime::SceneSchedulerPhase::DrawBufferOpa);
                const auto air_opa = require_trace_index(
                    trace, "SphereAir",
                    smgpc::runtime::SceneSchedulerPhase::DrawBufferOpa);
                require(planet_opa < air_opa,
                        "ordinary Air ordering must follow Planet while no PriorDrawAir is visible");
            }
#endif

            auto player = LiveActor{"Air lifecycle player"};
            player.mPosition.set(air->mPosition);
            player.calcAndSetBaseMtx();
            runtime.player_system().attach_actor(player);

            renderer.end_frame();
            frame = renderer.begin_frame();
            frame.frame_index = 91U;
            runtime.camera_system().set_game_camera_pose(camera);
            runtime.begin_frame(frame);
            runtime.set_scene_camera_pose(camera);
#ifndef NDEBUG
            runtime.set_j3d_packet_trace_frame(frame.frame_index);
#endif
            require(air->isDrawing() && MR::isExistPriorDrawAir() &&
                        smgpc::compat::actor_current_brk_name(air) == "Appear",
                    "SphereAir did not naturally enter after real player attachment");
            execution.draw_3d_normal(camera);

#ifndef NDEBUG
            const auto trace = runtime.scheduler().last_execution_trace();
            const auto air_opa = require_trace_index(
                trace, "SphereAir",
                smgpc::runtime::SceneSchedulerPhase::DrawBufferOpa);
            const auto air_xlu = require_trace_index(
                trace, "SphereAir",
                smgpc::runtime::SceneSchedulerPhase::DrawBufferXlu);
            const auto planet_opa = require_trace_index(
                trace, "Planet pass sentinel",
                smgpc::runtime::SceneSchedulerPhase::DrawBufferOpa);
            const auto packets = packet_count(runtime, "SphereAir", frame.frame_index);
            const auto air_packet = std::ranges::find_if(
                runtime.j3d_packet_trace(), [&](const auto& packet) {
                    return packet.model_name == "SphereAir" &&
                           packet.frame_index == frame.frame_index &&
                           packet.state.material_name == "AirMat";
                });
            require(air_opa < planet_opa && air_xlu < planet_opa,
                    "visible PriorDrawAir did not move Sky/Air passes ahead of Planet");
            require(packets != 0U &&
                        air_packet != runtime.j3d_packet_trace().end() &&
                        air_packet->state.packet_mode ==
                            smgpc::render::J3dRendererPacketMode::ShaderGxTev &&
                        air_packet->state.brk_active &&
                        air_packet->state.brk_frame_max == 59 &&
                        air_packet->state.brk_color_track_count == 1U &&
                        air_packet->state.brk_konst_track_count == 0U &&
                        air_packet->state.gx_initial_tev_registers[1U][3U] ==
                            -100,
                    "exact SphereAir packet did not carry frame-zero Appear BRK state through DrawBufferType_Air");
            std::cout << "[proof] disc=" << disc_path.string()
                      << ";air_model=SphereAir;air_packets=" << packets
                      << ";pre_attach_prior_air=false;post_attach_prior_air=true"
                      << ";planet_order=after_sky_air" << '\n';
#else
            throw std::runtime_error("the exact prior-air scheduler proof requires a debug build");
#endif

            runtime.player_system().detach_actor(&player);
            // Drain deferred GX work while every scene-owned model referenced
            // by the FIFO remains alive.
            renderer.end_frame();
        }
    }

}  // namespace

int main() {
    try {
        test_exact_air_and_prior_draw_route();
        std::cout << "[ok] exact Air actors drive the shared PriorDrawAir scheduler branch\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] exact Air/PriorDrawAir route: " << error.what() << '\n';
        return 1;
    }
}
