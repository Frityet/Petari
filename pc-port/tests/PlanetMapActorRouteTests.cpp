#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/PlanetMap.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Logger.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/CollisionPartsCompat.hpp"
#include "render/RendererService.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/GatewayDemoScene.hpp"
#include "scene/nameobj/PlanetMapCatalog.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
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
        require(!error, "the PlanetMap proof requires a readable working directory");
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
            "the PlanetMap proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

    [[nodiscard]] bool ends_with(std::string_view value, std::string_view suffix) {
        return value.ends_with(suffix);
    }

    struct ExpectedGatewayPlanet {
        std::string_view name;
        std::string_view runtime_name;
        std::size_t collision_count;
        s32 draw_buffer_type;
        bool has_indirect_texture;
    };

    constexpr std::array cExpectedGatewayPlanets = {
        ExpectedGatewayPlanet{
            "HeavensDoorMysteriousPlanet", "ヘブンズドアミステリアス惑星",
            2U, MR::DrawBufferType_IndirectPlanet, true},
        ExpectedGatewayPlanet{
            "HeavensDoorSmallPlanet", "ヘブンズドア惑星（小）", 1U,
            MR::DrawBufferType_IndirectPlanet, true},
        ExpectedGatewayPlanet{
            "HeavensDoorMiddlePlanet", "ヘブンズドア惑星（中）", 2U,
            MR::DrawBufferType_IndirectPlanet, true},
        ExpectedGatewayPlanet{
            "HeavensDoorBlackHolePlanet", "ヘブンズドアブラックホール惑星",
            1U, MR::DrawBufferType_Planet, false},
    };

    void test_gateway_ordinary_planet_actor() {
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
            .title = "SMG PC ordinary PlanetMap actor proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
        runtime.set_current_stage_name("HeavensDoorGalaxy");

        const auto camera = smgpc::camera::CameraPose{
            .eye = {14459.9785F, -12791.1133F, 7059.9116F},
            .watch = {14459.9785F, -12791.1133F, 6059.9116F},
            .up = {0.0F, 1.0F, 0.0F},
            .near_clip = 1.0F,
        };

        auto retired_planets = std::array<const PlanetMap *, cExpectedGatewayPlanets.size()>{};
        auto retired_planet_runtime_names =
            std::array<std::string, cExpectedGatewayPlanets.size()>{};
        {
            auto frame = renderer.begin_frame();
            frame.frame_index = 91U;
            const auto renderer_context =
                smgpc::render::ScopedAuroraRendererContext(renderer);
            runtime.begin_frame(frame);
            runtime.set_scene_camera_pose(camera);
#ifndef NDEBUG
            runtime.set_j3d_packet_trace_frame(frame.frame_index);
#endif
            auto gateway_scheduler_player =
                LiveActor{"Planet route scheduler player"};
            gateway_scheduler_player.mPosition.set(1000000.0F, 1000000.0F,
                                                   1000000.0F);
            gateway_scheduler_player.calcAndSetBaseMtx();
            {
                auto scene = smgpc::scene::GatewayDemoScene(runtime.dvd());
                auto *planet = scene.planet();
                auto *model = smgpc::compat::actor_model(planet);
                const auto ordinary_planet_count = std::ranges::count_if(
                    scene.visuals(), [](const auto &visual) {
                        return dynamic_cast<PlanetMap *>(visual.actor) != nullptr;
                    });
                require(planet != nullptr && model != nullptr &&
                            model->model_arc_name() == "HeavensDoorMysteriousPlanet" &&
                            !planet->mFlag.mIsDead && planet->mLODCtrl != nullptr &&
                            planet->mBloomModel == nullptr && planet->mWaterModel == nullptr &&
                            planet->mIndirectModel == nullptr &&
                            scene.visuals().size() == 7U && ordinary_planet_count == 4,
                        "Gateway did not construct all four ordinary zero-optional PlanetMap actors through the generic visual path");
                model->requireLoaded();
                require(model->isLoaded() && planet->mActorLightCtrl == nullptr,
                        "PlanetMap must retain its real model without inventing an ActorLightCtrl");

#ifndef NDEBUG
                const auto entries = runtime.scheduler().snapshot();
#endif
                for (std::size_t i = 0U; i < cExpectedGatewayPlanets.size(); ++i) {
                    const auto &expected = cExpectedGatewayPlanets[i];
                    const auto visual = std::ranges::find_if(
                        scene.visuals(), [&expected](const auto &candidate) {
                            return candidate.placement != nullptr &&
                                   candidate.placement->object_name == expected.name &&
                                   dynamic_cast<PlanetMap *>(candidate.actor) != nullptr;
                        });
                    require(visual != scene.visuals().end(),
                            "Gateway omitted an authored ordinary PlanetMap identity");

                    auto *expected_planet = dynamic_cast<PlanetMap *>(visual->actor);
                    retired_planets[i] = expected_planet;
                    require(expected_planet != nullptr &&
                                expected_planet->getName() != nullptr,
                            "Gateway ordinary PlanetMap has no runtime actor identity");
                    retired_planet_runtime_names[i] = expected_planet->getName();
                    auto *expected_model = smgpc::compat::actor_model(expected_planet);
                    const auto expected_resources =
                        smgpc::compat::actor_collision_parts_resources(expected_planet);
                    require(retired_planet_runtime_names[i] ==
                                    expected.runtime_name &&
                                expected_model != nullptr &&
                                expected_model->model_arc_name() == expected.name &&
                                expected_model->has_indirect_texture() ==
                                    expected.has_indirect_texture &&
                                expected_resources.size() == expected.collision_count &&
                                !expected_resources.empty() &&
                                expected_resources[0].resource_name == expected.name &&
                                (expected.collision_count == 1U ||
                                 expected_resources[1].resource_name == "MoveLimit"),
                            "Gateway ordinary PlanetMap identity has the wrong model or CollisionParts closure");
#ifndef NDEBUG
                    const auto scheduled = std::ranges::find_if(
                        entries, [&retired_planet_runtime_names,
                                  i](const auto &entry) {
                            return entry.name ==
                                       retired_planet_runtime_names[i] &&
                                   entry.kind ==
                                       smgpc::runtime::SceneEntryKind::LiveActorModel;
                        });
                    require(scheduled != entries.end() &&
                                scheduled->movement_type == MR::MovementType_Planet &&
                                scheduled->calc_anim_type == MR::CalcAnimType_Planet &&
                                scheduled->draw_buffer_type == expected.draw_buffer_type,
                            "Gateway ordinary PlanetMap identity has the wrong retail scheduler categories");
#endif
                }
                require(retired_planets[0] == planet,
                        "Gateway player-start surface is not the authored mysterious planet");

                const auto resources =
                    smgpc::compat::actor_collision_parts_resources(planet);
                require(resources.size() == 2U &&
                            resources[0].resource_name ==
                                "HeavensDoorMysteriousPlanet" &&
                            resources[0].kcl_size == 632430U &&
                            resources[0].attributes_size == 31232U &&
                            ends_with(resources[0].kcl_source,
                                      "HeavensDoorMysteriousPlanet.arc:/heavensdoormysteriousplanet.kcl") &&
                            ends_with(resources[0].attributes_source,
                                      "HeavensDoorMysteriousPlanet.arc:/heavensdoormysteriousplanet.pa") &&
                            resources[1].resource_name == "MoveLimit" &&
                            resources[1].kcl_size == 25868U &&
                            resources[1].attributes_size == 1152U &&
                            ends_with(resources[1].kcl_source,
                                      "HeavensDoorMysteriousPlanet.arc:/movelimit.kcl") &&
                            ends_with(resources[1].attributes_source,
                                      "HeavensDoorMysteriousPlanet.arc:/movelimit.pa"),
                        "PlanetMap did not retain exact main and MoveLimit KCL/PA provenance");

#ifndef NDEBUG
                const auto scheduled = std::ranges::find_if(entries, [&](const auto &entry) {
                    return entry.name == retired_planet_runtime_names.front() &&
                           entry.kind == smgpc::runtime::SceneEntryKind::LiveActorModel;
                });
                require(model->has_indirect_texture(),
                        "the real Mysterious Planet model must retain its indirect material");
                require(scheduled != entries.end() &&
                            scheduled->movement_type == MR::MovementType_Planet &&
                            scheduled->calc_anim_type == MR::CalcAnimType_Planet &&
                            scheduled->draw_buffer_type == MR::DrawBufferType_IndirectPlanet,
                        "PlanetMap did not use the retail planet scheduler categories");
#endif

                runtime.player_system().attach_actor(gateway_scheduler_player);
                runtime.scheduler().execute_movement();
                runtime.scheduler().execute_calc_anim();
                runtime.scheduler().execute_calc_view_and_entry();
                runtime.draw_3d_normal(camera);
                const auto *area_light = LightFunction::getAreaLightInfo(ZoneLightID{});
                const auto &ambient = runtime.scene_lights().actor_ambient();
                const auto *light0 = runtime.scene_lights().light(0U);
                const auto *light1 = runtime.scene_lights().light(1U);
                require(area_light != nullptr && ambient.has_value() &&
                            *ambient == smgpc::render::GXColorValue{
                                            area_light->mPlanetLight.mColor.r,
                                            area_light->mPlanetLight.mColor.g,
                                            area_light->mPlanetLight.mColor.b,
                                            area_light->mPlanetLight.mColor.a} &&
                            light0 != nullptr && light1 != nullptr &&
                            light0->coordinate_space ==
                                (area_light->mPlanetLight.mInfo0.mIsFollowCamera
                                     ? smgpc::render::GXLightCoordinateSpace::View
                                     : smgpc::render::GXLightCoordinateSpace::World) &&
                            light1->coordinate_space ==
                                (area_light->mPlanetLight.mInfo1.mIsFollowCamera
                                     ? smgpc::render::GXLightCoordinateSpace::View
                                     : smgpc::render::GXLightCoordinateSpace::World),
                        "IndirectPlanet draw did not load authored Planet ambient and light spaces");
#ifndef NDEBUG
                require(std::ranges::any_of(runtime.j3d_packet_trace(),
                                            [frame_index = frame.frame_index](const auto &packet) {
                                                return packet.model_name ==
                                                           "HeavensDoorMysteriousPlanet" &&
                                                       packet.frame_index == frame_index &&
                                                       packet.state.source_triangle_count != 0U &&
                                                       packet.state.parsed_display_list_bytes != 0U;
                                            }),
                        "ordinary PlanetMap submitted no parsed J3D packets through the scheduler");
#endif
                // Aurora resolves deferred GX display-list array pointers while
                // draining the frame FIFO, so scene-owned models must outlive
                // end_frame just as they do in the production scene loop.
                renderer.end_frame();
                runtime.player_system().detach_actor(&gateway_scheduler_player);
            }

            require(smgpc::scene::nameobj::PlanetMapCatalog::active() == nullptr,
                    "destroying Gateway left a stale PlanetMap catalog");
            require(std::ranges::all_of(retired_planets, [](const auto *retired_planet) {
                        return retired_planet != nullptr &&
                               smgpc::compat::actor_collision_parts_count(retired_planet) == 0U;
                    }),
                    "destroying Gateway left stale ordinary PlanetMap CollisionParts state");
#ifndef NDEBUG
            const auto retired_entries = runtime.scheduler().snapshot();
            require(std::ranges::all_of(retired_planet_runtime_names,
                                        [&retired_entries](const auto &runtime_name) {
                                            return std::ranges::none_of(
                                                retired_entries, [&runtime_name](const auto &entry) {
                                                    return entry.name == runtime_name;
                                                });
                                        }),
                    "destroying Gateway left a stale ordinary PlanetMap scheduler entry");
#endif
        }

        std::cout << "[ok] ordinary PlanetMap owns Gateway model, draw-buffer planet light, scheduler, main KCL/PA, and MoveLimit KCL/PA\n";
    }

}  // namespace

int main() {
    try {
        test_gateway_ordinary_planet_actor();
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] ordinary PlanetMap actor: " << error.what() << '\n';
        return 1;
    }
}
