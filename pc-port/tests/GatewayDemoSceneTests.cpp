#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/AreaObj/LightArea.hpp"
#include "Game/AreaObj/LightAreaHolder.hpp"
#include "Game/Gravity/GravityInfo.hpp"
#include "Game/Gravity/PointGravity.hpp"
#include "Game/Map/PlanetMap.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/MapObj/BrightObj.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/LensFlare.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/CollisionPartsCompat.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/StageSessionState.hpp"
#include "resource/BcsvTable.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/AuthoredPlacementInstantiator.hpp"
#include "scene/GatewayDemoScene.hpp"
#include "scene/StageHostScene.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

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
#include <memory>
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
        require(!error, "the Gateway demo proof requires a readable working directory");
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
            "the Gateway demo proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

    [[nodiscard]] float dot(const TVec3f &left, const TVec3f &right) {
        return left.x * right.x + left.y * right.y + left.z * right.z;
    }

    void test_real_gateway_spawn_planet_collision_and_gravity() {
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
            .title = "SMG PC exact Gateway scene proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
        runtime.set_current_stage_name("HeavensDoorGalaxy");
        auto frame = renderer.begin_frame();
        const auto renderer_context =
            smgpc::render::ScopedAuroraRendererContext(renderer);
        runtime.begin_frame(frame);
        auto &dvd = runtime.dvd();
        auto outer_stage_session = smgpc::compat::StageSessionState(
            "FileSelect", "OuterSessionGalaxy", 2, JMapIdInfo(9, 4));
        auto outer_stage_session_binding =
            smgpc::compat::StageSessionBinding(outer_stage_session);
        (void)outer_stage_session_binding;
        auto *const previous_stage_session =
            smgpc::compat::try_active_stage_session();
        require(previous_stage_session == &outer_stage_session,
                "Gateway session-shadow proof did not install a real outer owner");
        auto scene_owner =
            std::make_unique<smgpc::scene::GatewayDemoScene>(dvd);
        auto &scene = *scene_owner;
        const auto &stage_session = scene.stage_session();
        require(smgpc::compat::try_active_stage_session() == &stage_session &&
                    stage_session.scene_name() == "Game" &&
                    stage_session.stage_name() == "HeavensDoorGalaxy" &&
                    stage_session.scenario_no() == 1 &&
                    stage_session.initial_start_id()._0 == 0 &&
                    stage_session.initial_start_id().mZoneID == 0 &&
                    stage_session.restart_id()._0 == 0 &&
                    stage_session.restart_id().mZoneID == 0,
                "Gateway must bind one exact scenario/start session above all authored placements");
        const auto &placement_report = scene.authored_placement_report();
        require(placement_report.mode ==
                        smgpc::scene::AuthoredPlacementMode::
                            SupportedSubsetForDevelopment &&
                    placement_report.preflight_accepted &&
                    placement_report.state ==
                        smgpc::scene::AuthoredPlacementRuntimeState::
                            InitializedAfterPlacement &&
                    placement_report.entries.size() == scene.placements().size() &&
                    placement_report.created_count ==
                        placement_report.ready_count &&
                    placement_report.initialized_after_placement_count ==
                        placement_report.created_count,
                "Gateway must explicitly report its complete development subset after one placement pass");
        auto prior_pass =
            smgpc::scene::AuthoredPlacementRetailPass::CommonHighPriority;
        auto shaped_row_count = std::size_t{};
        auto ready_shaped_row_count = std::size_t{};
        for (const auto &entry : placement_report.entries) {
            require(entry.retail_pass >= prior_pass,
                    "Gateway placement report lost retail five-pass order");
            prior_pass = entry.retail_pass;
            if (entry.placement != nullptr &&
                entry.placement->shape_model_no != -1) {
                ++shaped_row_count;
                if (entry.support.kind ==
                    smgpc::scene::AuthoredPlacementSupportKind::Ready) {
                    ++ready_shaped_row_count;
                }
            }
            if (entry.support.kind ==
                smgpc::scene::AuthoredPlacementSupportKind::Ready) {
                require(entry.actor != nullptr &&
                            entry.outcome ==
                                smgpc::scene::AuthoredPlacementOutcome::
                                    InitializedAfterPlacement,
                        "a ready Gateway placement bypassed the shared lifecycle");
            } else if (entry.support.kind ==
                       smgpc::scene::AuthoredPlacementSupportKind::Blocked) {
                require(entry.actor == nullptr && !entry.support.reason.empty(),
                        "a blocked Gateway row was not retained as explicit evidence");
            }
        }
        require(ready_shaped_row_count == 0U,
                "Gateway allowed a ShapeModelNo row to borrow an ordinary creator");
        const auto planet_report = std::ranges::find_if(
            placement_report.entries, [&scene](const auto &entry) {
                return entry.placement == &scene.planet_placement();
            });
        require(planet_report != placement_report.entries.end() &&
                    planet_report->support.kind ==
                        smgpc::scene::AuthoredPlacementSupportKind::Ready &&
                    planet_report->actor == scene.planet(),
                "the active planet catalog must classify and construct the exact authored planet row");
        auto light_area_placements = std::size_t{};
        const smgpc::scene::StagePlacementObject *child_light_placement = nullptr;
        for (const auto &placement : scene.placements()) {
            if (placement.object_name == "LightCtrlCube") {
                ++light_area_placements;
                if (placement.zone_id == 5 && placement.table_path ==
                                                  "jmp/placement/layera/areaobjinfo") {
                    child_light_placement = &placement;
                }
            }
        }
        require(light_area_placements == 8U && child_light_placement != nullptr &&
                    child_light_placement->jmap_entry_index == 0 &&
                    child_light_placement->switch_appear_id == 90 &&
                    child_light_placement->object_args[0] == 0 &&
                    child_light_placement->object_args[1] == 10,
                "Gateway must retain all eight authored LightCtrl placements and the exact child-zone switch row");
        auto *area_container = dynamic_cast<AreaObjContainer *>(
            scene.scene_obj_holder().getObj(SceneObj_AreaObjContainer));
        auto *light_area_holder = area_container != nullptr
                                      ? dynamic_cast<LightAreaHolder *>(
                                            area_container->getManager("LightArea"))
                                      : nullptr;
        require(light_area_holder != nullptr && light_area_holder->mArray.size() == 8,
                "Gateway must construct every complete LightCtrl through the shared AreaObj manager");
        LightArea *child_light_area = nullptr;
        for (auto index = 0; index < light_area_holder->mArray.size(); ++index) {
            auto *candidate = dynamic_cast<LightArea *>(light_area_holder->getAreaObj(index));
            if (candidate != nullptr && candidate->mPlacedZoneID == 5 &&
                candidate->mObjArg0 == 0) {
                child_light_area = candidate;
                break;
            }
        }
        require(child_light_area != nullptr && child_light_area->mObjArg1 == 10 &&
                    child_light_area->mSwitchCtrl != nullptr &&
                    child_light_area->mSwitchCtrl->isValidSwitchAppear() &&
                    !child_light_area->mSwitchCtrl->isOnSwitchAppear() &&
                    !child_light_area->isValid(),
                "the child LightCtrl must honor authored SW_APPEAR 90's real initial off state rather than bypassing it");
        const auto &demo = scene.demo_runtime();
        const auto guide_index = demo.find_definition(5, 0);
        require(guide_index.has_value(),
                "the Gateway spin checkpoint requires the zone-5/link-0 guide demo");
        const auto* guide = demo.definition(*guide_index);
        require(guide != nullptr && guide->time_sheet_name == "TicoGuideDemo" &&
                    guide->source_row == 0 &&
                    guide->source_table_path == "jmp/placement/layera/demoobjinfo",
                "the guide DemoGroup must select the retail TicoGuideDemo sheet");
        constexpr auto spin_parts = std::array{
            std::pair{std::string_view{"スピンゲット[デモ1]"}, 420},
            std::pair{std::string_view{"スピンゲット[会話1]"}, 120},
            std::pair{std::string_view{"スピンゲット[デモ2]"}, 150},
            std::pair{std::string_view{"スピンゲット[会話2]"}, 120},
            std::pair{std::string_view{"スピンゲット[デモ3]"}, 240},
            std::pair{std::string_view{"スピンゲット[会話3]"}, 120},
            std::pair{std::string_view{"スピンゲット[デモ4]"}, 500},
            std::pair{std::string_view{"スピンゲット[デモ5]"}, 60},
        };
        require(guide->sheet.time_rows().size() == 27U,
                "the guide checkpoint requires the complete retail Time sheet");
        auto pre_prompt_frames = 0;
        for (auto offset = std::size_t{}; offset < spin_parts.size(); ++offset) {
            const auto& row = guide->sheet.time_rows()[15U + offset];
            require(row.part_name == spin_parts[offset].first &&
                        row.total_step == spin_parts[offset].second && !row.suspend,
                    "the spin checkpoint Time rows must retain their retail order and duration");
            if (offset + 1U < spin_parts.size()) {
                pre_prompt_frames += row.total_step;
            }
        }
        require(pre_prompt_frames == 1670,
                "the retail guide must reach the spin prompt after exactly 1670 demo frames");
        require(guide->sheet.sound_rows().empty() &&
                    std::ranges::none_of(guide->sheet.camera_rows(), [&](const auto& row) {
                        return std::ranges::any_of(spin_parts, [&](const auto& part) {
                            return row.part_name == part.first;
                        });
                    }),
                "the spin checkpoint must not invent sound or camera work absent from parts 15-22");
        require(std::ranges::any_of(guide->sheet.player_rows(), [](const auto& row) {
                    return row.part_name == "スピンゲット[デモ1]" &&
                           row.position_name == "MarioDemoPos4";
                }),
                "part 15 must retain the retail MarioDemoPos4 Player row");

        const auto require_position = [&](std::string_view name,
                                          const std::array<float, 3U>& local) {
            const auto found = std::ranges::find_if(scene.general_positions(),
                                                    [&](const auto& position) {
                                                        return position.name == name;
                                                    });
            require(found != scene.general_positions().end(),
                    "the spin checkpoint GeneralPos row is absent");
            for (auto axis = std::size_t{}; axis < local.size(); ++axis) {
                require_near(found->local_position[axis], local[axis], 0.001F,
                             "spin checkpoint GeneralPos local position");
            }
        };
        require_position("MarioDemoPos2", {-1490.0F, 1303.19043F, 720.0F});
        require_position("MarioDemoPos4", {-140.0F, 2500.0F, -700.0F});

        const auto require_cast = [&](std::string_view name, int row, int cast_id) {
            const auto found = std::ranges::find_if(scene.placements(),
                                                    [&](const auto& placement) {
                                                        return placement.object_name == name &&
                                                               placement.zone_id == 5 &&
                                                               placement.table_path ==
                                                                   "jmp/placement/layera/objinfo";
                                                    });
            require(found != scene.placements().end() &&
                        found->jmap_entry_index == row && found->cast_id == cast_id,
                    "the spin checkpoint cast row differs from RMGK01");
        };
        require_cast("Rosetta", 12, -1);
        require_cast("TicoBaby", 13, 0);

        const auto &start = scene.start_info();
        require(start.object_name == "Mario" && start.start_id == 0 && start.zone_id == 0 &&
                    start.camera_id == 78 && start.layer_name == "layera" &&
                    start.table_path == "jmp/start/layera/startinfo",
                "the development scene must retain the exact scenario-1 StartInfo identity");
        require_near(start.world_position[0], 14459.978515625F, 0.0005F,
                     "scenario-1 StartInfo X");
        require_near(start.world_position[1], -12791.11328125F, 0.0005F,
                     "scenario-1 StartInfo Y");
        require_near(start.world_position[2], 6059.91162109375F, 0.0005F,
                     "scenario-1 StartInfo Z");
        require_near(start.local_rotation[0], 124.76948547363281F, 0.0005F,
                     "scenario-1 StartInfo rotation X");
        require_near(start.local_rotation[1], -114.13859558105469F, 0.0005F,
                     "scenario-1 StartInfo rotation Y");
        require_near(start.local_rotation[2], 47.654739379882812F, 0.0005F,
                     "scenario-1 StartInfo rotation Z");

        const auto start_iter = scene.player_start_iter();
        const char *start_name = nullptr;
        auto start_position = TVec3f{};
        require(start_iter.isValid() && start_iter.getValue("name", &start_name) &&
                    start_name != nullptr && std::string_view(start_name) == "Mario" &&
                    start_iter.getValue("pos_x", &start_position.x) &&
                    start_iter.getValue("pos_y", &start_position.y) &&
                    start_iter.getValue("pos_z", &start_position.z) &&
                    start_position.epsilonEquals(
                        TVec3f{14459.978515625F, -12791.11328125F, 6059.91162109375F},
                        0.0005F),
                "the forthcoming real MarioActor must receive the retained exact JMap row");

        auto *planet = scene.planet();
        auto *planet_model = smgpc::compat::actor_model(planet);
        require(planet != nullptr && planet_model != nullptr &&
                    planet_model->model_arc_name() == "HeavensDoorMysteriousPlanet",
                "Gateway must expose the production-owned ordinary PlanetMap model");
        planet_model->requireLoaded();
        const auto planet_resources =
            smgpc::compat::actor_collision_parts_resources(planet);
        require(planet_model->isLoaded() && planet_model->has_indirect_texture() &&
                    planet_resources.size() == 2U &&
                    planet_resources[0].resource_name ==
                        "HeavensDoorMysteriousPlanet" &&
                    planet_resources[0].kcl_size == 632430U &&
                    planet_resources[0].attributes_size == 31232U &&
                    planet_resources[0].kcl_source.ends_with(
                        "HeavensDoorMysteriousPlanet.arc:/heavensdoormysteriousplanet.kcl") &&
                    planet_resources[0].attributes_source.ends_with(
                        "HeavensDoorMysteriousPlanet.arc:/heavensdoormysteriousplanet.pa") &&
                    planet_resources[1].resource_name == "MoveLimit" &&
                    planet_resources[1].kcl_size == 25868U &&
                    planet_resources[1].attributes_size == 1152U &&
                    planet_resources[1].kcl_source.ends_with(
                        "HeavensDoorMysteriousPlanet.arc:/movelimit.kcl") &&
                    planet_resources[1].attributes_source.ends_with(
                        "HeavensDoorMysteriousPlanet.arc:/movelimit.pa"),
                "ordinary PlanetMap must own exact main and MoveLimit model/collision resources");
        const auto ordinary_planet_count = std::ranges::count_if(
            scene.visuals(), [](const auto &visual) {
                return dynamic_cast<PlanetMap *>(visual.actor) != nullptr;
            });
        require(scene.visuals().size() == 7U && ordinary_planet_count == 4 &&
                    scene.collision().stats().mesh_count == 6U &&
                    scene.collision().stats().triangle_count != 0U,
                "Gateway must retain Sky, Air, BrightSun, four ordinary planets, and their six shared collision meshes");

        const auto bright_placement = std::ranges::find_if(
            scene.placements(), [](const auto& placement) {
                return placement.object_name == "BrightSun" &&
                       placement.zone_name == "HeavensDoorGalaxy" &&
                       placement.table_path == "jmp/placement/common/objinfo";
            });
        require(bright_placement != scene.placements().end() &&
                    bright_placement->jmap_entry_index == 1U &&
                    bright_placement->l_id == 10 && bright_placement->zone_id == 0 &&
                    bright_placement->factory_supported,
                "Gateway did not retain the exact root-common BrightSun placement");
        require_near(bright_placement->translation[0], 26110.0F, 0.001F,
                     "BrightSun placement X");
        require_near(bright_placement->translation[1], 0.0F, 0.001F,
                     "BrightSun placement Y");
        require_near(bright_placement->translation[2], -35950.0F, 0.001F,
                     "BrightSun placement Z");

        const auto bright_visual = std::ranges::find_if(
            scene.visuals(), [&](const auto& visual) {
                return visual.placement == &*bright_placement;
            });
        require(bright_visual != scene.visuals().end() &&
                    dynamic_cast<BrightSun*>(bright_visual->actor) != nullptr &&
                    MR::isExistSceneObj(SceneObj_LensFlareDirector),
                "the generic visual lifecycle did not create exact BrightSun and LensFlareDirector actors");
        const auto bright_report = std::ranges::find_if(
            placement_report.entries, [&](const auto& entry) {
                return entry.placement == &*bright_placement;
            });
        require(bright_report != placement_report.entries.end() &&
                    bright_report->actor == bright_visual->actor &&
                    bright_report->actor_name == "レンズフレア用太陽" &&
                    bright_visual->actor->getName() != nullptr &&
                    std::string_view(bright_visual->actor->getName()) ==
                        *bright_report->actor_name,
                "BrightSun did not retain its exact ObjNameTable actor identity");
        const auto bright_runtime_name = *bright_report->actor_name;

        const auto bright_factory =
            smgpc::scene::nameobj::describe_name_obj_factory(runtime.dvd(), "BrightSun");
        const auto has_archive = [&](std::string_view name) {
            return std::ranges::any_of(bright_factory.archives, [&](const auto& archive) {
                return archive.archive_name == name && archive.loaded;
            });
        };
        require(bright_factory.creator_supported &&
                    bright_factory.archives.size() == 4U &&
                    has_archive("LensFlare") && has_archive("GlareGlow") &&
                    has_archive("GlareLine") && has_archive("Sun"),
                "BrightSun did not preload its exact shared flare and Sun archives");

#ifndef NDEBUG
        const auto scheduler_entries = runtime.scheduler().snapshot();
        const auto has_scheduler_entry = [&](std::string_view name, s32 movement,
                                             s32 calc_anim, s32 draw_buffer,
                                             s32 draw_type) {
            return std::ranges::any_of(scheduler_entries, [&](const auto& entry) {
                return entry.name == name && entry.movement_type == movement &&
                       entry.calc_anim_type == calc_anim &&
                       entry.draw_buffer_type == draw_buffer &&
                       entry.draw_type == draw_type;
            });
        };
        require(has_scheduler_entry(bright_runtime_name,
                                    MR::MovementType_Environment, -1, -1,
                                    MR::DrawType_BrightSun) &&
                    has_scheduler_entry("太陽", MR::MovementType_Sky,
                                        MR::CalcAnimType_MapObj,
                                        MR::DrawBufferType_Sun, -1) &&
                    has_scheduler_entry("レンズフレアリング",
                                        MR::MovementType_Layout,
                                        MR::CalcAnimType_Layout,
                                        MR::DrawBufferType_Model3DFor2D, -1) &&
                    has_scheduler_entry("グレア（円形）",
                                        MR::MovementType_Layout,
                                        MR::CalcAnimType_Layout,
                                        MR::DrawBufferType_Model3DFor2D, -1) &&
                    has_scheduler_entry("グレア（ライン）",
                                        MR::MovementType_Layout,
                                        MR::CalcAnimType_Layout,
                                        MR::DrawBufferType_Model3DFor2D, -1),
                "BrightSun, Sun, or lens-flare children bypassed their retail scheduler categories");
#endif

        const auto *point_gravity = dynamic_cast<const PointGravity *>(&scene.gravity());
        require(point_gravity != nullptr,
                "the child-zone gravity must be the exact Game PointGravity implementation");
        require_near(point_gravity->mTranslation.x, 14760.0F, 0.001F,
                     "child-zone point-gravity center X");
        require_near(point_gravity->mTranslation.y, -10676.2255859375F, 0.001F,
                     "child-zone point-gravity center Y");
        require_near(point_gravity->mTranslation.z, 6770.0F, 0.001F,
                     "child-zone point-gravity center Z");
        require_near(point_gravity->mRange, 8200.0F, 0.001F,
                     "child-zone point-gravity range");

        auto requester = NameObj{"Gateway Mario acceptance requester"};
        auto info = GravityInfo{};
        auto resolved_gravity = TVec3f{};
        require(scene.resolve_gravity(requester, start_position, &resolved_gravity, &info) &&
                    info.mGravityInstance == &scene.gravity(),
                "the exact child-zone point gravity must win at Mario's start position");
        auto expected_gravity =
            point_gravity->mTranslation - start_position;
        expected_gravity.scale(1.0F / expected_gravity.length());
        require(resolved_gravity.epsilonEquals(expected_gravity, 0.0001F),
                "resolved Gateway gravity must point toward the exact child-zone center");

        const auto contact = scene.prove_start_contact(requester);
        const auto attributes =
            smgpc::resource::BcsvTable::from_bytes(contact.surface.attributes);
        require(contact.surface.source_name.ends_with(
                        "HeavensDoorMysteriousPlanet.arc:/heavensdoormysteriousplanet.kcl") &&
                    contact.surface.attributes.size() ==
                        planet_resources[0].attributes_size &&
                    contact.surface.sensor != nullptr &&
                    contact.collision.attribute < attributes.entry_count(),
                "the start contact must retain exact KCL, PA, and body-sensor provenance");
        require(contact.separation < 0.1F,
                "scenario-1 StartInfo must rest on the real planet KCL face");
        require(dot(contact.collision.normal, contact.gravity) < -0.95F,
                "the contacted planet face must oppose the resolved inward gravity");

        require(scene.planet_placement().factory_supported &&
                    smgpc::scene::nameobj::can_create_name_obj(
                        "HeavensDoorMysteriousPlanet"),
                "the exact ordinary PlanetMap must be supported by the production factory");
        smgpc::scene::preflight_stage_placements_or_throw(
            "HeavensDoorGalaxy", 1,
            std::span<const smgpc::scene::StagePlacementObject>{
                &scene.planet_placement(), 1U});

        std::cout << "[proof] disc=" << disc_path.string()
                  << "; start=(" << start_position.x << ',' << start_position.y << ','
                  << start_position.z << ")"
                  << "; planet_model=" << planet_model->model_arc_name()
                  << "; visual_count=" << scene.visuals().size()
                  << "; ordinary_planets=" << ordinary_planet_count
                  << "; main_kcl_bytes=" << planet_resources[0].kcl_size
                  << "; move_limit_kcl_bytes=" << planet_resources[1].kcl_size
                  << "; kcl_triangles=" << scene.collision().stats().triangle_count
                  << "; collision_meshes=" << scene.collision().stats().mesh_count
                  << "; gravity_center=(" << point_gravity->mTranslation.x << ','
                  << point_gravity->mTranslation.y << ','
                  << point_gravity->mTranslation.z << ')'
                  << "; start_surface_separation=" << contact.separation << '\n';
        std::cout << "[proof] gateway_shaped_rows=" << shaped_row_count
                  << ";gateway_ready_shaped_rows="
                  << ready_shaped_row_count << '\n';
        scene_owner.reset();
        require(smgpc::compat::try_active_stage_session() ==
                        previous_stage_session &&
                    previous_stage_session == &outer_stage_session &&
                    outer_stage_session.scene_name() == "FileSelect" &&
                    outer_stage_session.stage_name() ==
                        "OuterSessionGalaxy" &&
                    outer_stage_session.scenario_no() == 2 &&
                    outer_stage_session.initial_start_id()._0 == 9 &&
                    outer_stage_session.initial_start_id().mZoneID == 4,
                "Gateway teardown did not restore the exact outer stage-session identity");
        renderer.end_frame();
    }

}  // namespace

int main() {
    try {
        test_real_gateway_spawn_planet_collision_and_gravity();
        std::cout << "[ok] exact Gateway development scene is ready for real MarioActor\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] Gateway development scene: " << error.what() << '\n';
        return 1;
    }
}
