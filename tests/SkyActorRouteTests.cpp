#include "Game/Map/FileSelectSky.hpp"
#include "Game/Map/Sky.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Logger.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/GameDataSession.hpp"
#include "render/RendererService.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/GatewayDemoScene.hpp"
#include "scene/nameobj/NameObjFactory.hpp"
#include "GatewayDemoSceneTestSupport.hpp"

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
        require(!error, "the sky actor proof requires a readable working directory");
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
            "the sky actor proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

#ifndef NDEBUG
    [[nodiscard]] const smgpc::runtime::SceneSchedulerEntryState &require_sky_entry(
        const std::vector<smgpc::runtime::SceneSchedulerEntryState> &entries,
        std::string_view name) {
        const auto found = std::ranges::find_if(entries, [name](const auto &entry) {
            return entry.name == name;
        });
        require(found != entries.end(), "exact sky actor is absent from the scheduler");
        require(found->kind == smgpc::runtime::SceneEntryKind::LiveActorModel &&
                    found->movement_type == MR::MovementType_Sky &&
                    found->calc_anim_type == MR::CalcAnimType_MapObj &&
                    found->draw_buffer_type == MR::DrawBufferType_Sky &&
                    found->draw_type == -1,
                "sky actor did not use the shared retail scheduler categories");
        return *found;
    }

    struct PacketProof {
        std::size_t packet_count = 0U;
        std::size_t triangle_count = 0U;
        std::size_t btk_packet_count = 0U;
    };

    [[nodiscard]] PacketProof collect_packet_proof(
        const smgpc::runtime::RuntimeContext &runtime, std::string_view model_name,
        std::uint64_t frame_index) {
        auto proof = PacketProof{};
        for (const auto &packet : runtime.j3d_packet_trace()) {
            if (packet.model_name != model_name || packet.frame_index != frame_index) {
                continue;
            }
            ++proof.packet_count;
            proof.triangle_count += packet.state.source_triangle_count;
            proof.btk_packet_count += packet.state.btk_active &&
                                      packet.state.btk_material_count != 0U;
        }
        return proof;
    }
#endif

    void test_gateway_and_file_select_share_exact_sky_path() {
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
            .title = "SMG PC exact shared sky actor proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto resource_runtime = smgpc::resource::GameResourceRuntime{};
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window, resource_runtime);
        runtime.set_current_stage_name("HeavensDoorGalaxy");

        require(smgpc::scene::nameobj::can_create_name_obj("VROrbit") &&
                    smgpc::scene::nameobj::can_create_name_obj("VRDarkSpace") &&
                    smgpc::scene::nameobj::can_create_name_obj("CloudSky") &&
                    smgpc::scene::nameobj::can_create_name_obj("AstroDomeSkyA") &&
                    !smgpc::scene::nameobj::can_create_name_obj("SummerSky") &&
                    !smgpc::scene::nameobj::can_create_name_obj("AstroDomeSky"),
                "the bounded retail sky factory table is incomplete or overclaims unavailable actors");

        const auto camera = smgpc::camera::CameraPose{
            .eye = {1234.0F, -5678.0F, 9012.0F},
            .watch = {1234.0F, -5678.0F, 8012.0F},
            .up = {0.0F, 1.0F, 0.0F},
            .near_clip = 1.0F,
        };

#ifndef NDEBUG
        auto gateway_packets = PacketProof{};
        auto file_select_packets = PacketProof{};
#endif
        {
            auto frame = renderer.begin_frame();
            frame.frame_index = 70U;
            const auto renderer_context =
                smgpc::render::ScopedAuroraRendererContext(renderer);
            runtime.begin_frame(frame);
            runtime.set_scene_camera_pose(camera);
#ifndef NDEBUG
            runtime.set_j3d_packet_trace_frame(frame.frame_index);
#endif
            auto gateway_sky_runtime_name = std::string{};
            auto game_data_session = smgpc::compat::GameDataSession{1U};
            {
                auto scene = smgpc::scene::GatewayDemoScene{runtime.dvd()};
                const auto &placement = scene.sky_placement();
                require(placement.object_name == "VROrbit" &&
                            placement.zone_id == 0 && placement.l_id == 0 &&
                            placement.jmap_entry_index == 0 &&
                            placement.layer_name == "common" &&
                            placement.table_path == "jmp/placement/common/objinfo" &&
                            placement.factory_supported,
                        "Gateway scenario 1 did not select the exact authored VROrbit row");
                require_near(placement.translation[0], 0.0F, 0.0001F,
                             "VROrbit placement X");
                require_near(placement.translation[1], -3532.498046875F, 0.0005F,
                             "VROrbit placement Y");
                require_near(placement.translation[2], -1040.0F, 0.0001F,
                             "VROrbit placement Z");

                auto gateway_player =
                    smgpc::test::GatewayPlayerSentinel{runtime, scene};
                gateway_player.mPosition.set(1000000.0F, 1000000.0F,
                                             1000000.0F);
                runtime.player_system().synchronize_attached_actor();
                auto placement_lease =
                    scene.finalize_placements(gateway_player);

                auto *sky = scene.sky();
                auto *model = smgpc::compat::actor_model(sky);
                require(sky != nullptr && model != nullptr &&
                            model->model_arc_name() == "VROrbit" &&
                            !sky->mFlag.mIsDead,
                        "Gateway did not construct the exact live ProjectionMapSky model");
                require(sky->getName() != nullptr &&
                            std::string_view(sky->getName()) == "VR軌道",
                        "Gateway sky did not retain its exact ObjNameTable actor identity");
                gateway_sky_runtime_name = sky->getName();
                model->requireLoaded();
                require(model->isLoaded(), "VROrbit retail BDL did not load");

                // GameScene creates DemoDirector before placement. Gateway's
                // exact Sky initialization must therefore join the same
                // already-active scene owner, never a deferred compatibility
                // list waiting for a later checkpoint-specific director.
                auto &demo = scene.demo_runtime();
                require(smgpc::compat::active_demo_scene_runtime() == &demo &&
                            demo.simple_cast_registration_count(sky) == 1U,
                        "Gateway Sky did not register with its scene-owned DemoDirector during init");
                const NameObj *retired_identity = nullptr;
                {
                    auto retired = LiveActor{"Retired scene simple cast"};
                    retired_identity = &retired;
                    MR::registerDemoSimpleCastAll(&retired);
                }
                require(demo.simple_cast_registration_count(sky) == 1U &&
                            demo.simple_cast_registration_count(retired_identity) == 0U,
                        "scene-owned simple-cast cleanup retained a destroyed actor");

#ifndef NDEBUG
                const auto before = runtime.scheduler().snapshot();
                const auto &entry =
                    require_sky_entry(before, gateway_sky_runtime_name);
                require(entry.live_actor_bck_name.empty() &&
                            entry.live_actor_brk_name.empty() &&
                            entry.live_actor_btk_name == "VROrbit",
                        "VROrbit did not start only its exact same-name BTK");
#endif
                runtime.scheduler().execute_movement();
                runtime.scheduler().execute_calc_anim();
                runtime.scheduler().execute_calc_view_and_entry();
                require_near(sky->mPosition.x, camera.eye.x, 0.0001F,
                             "ProjectionMapSky camera-follow X");
                require_near(sky->mPosition.y, camera.eye.y, 0.0001F,
                             "ProjectionMapSky camera-follow Y");
                require_near(sky->mPosition.z, camera.eye.z, 0.0001F,
                             "ProjectionMapSky camera-follow Z");
                runtime.scheduler().execute_draw_buffer_opa(camera, MR::DrawBufferType_Sky);
                runtime.scheduler().execute_draw_buffer_xlu(camera, MR::DrawBufferType_Sky);
#ifndef NDEBUG
                gateway_packets = collect_packet_proof(runtime, "VROrbit", frame.frame_index);
#endif
            }
#ifndef NDEBUG
            require(std::ranges::none_of(
                        runtime.scheduler().snapshot(),
                        [&gateway_sky_runtime_name](const auto &entry) {
                            return entry.name == gateway_sky_runtime_name;
                        }),
                    "destroying GatewayDemoScene left a stale VROrbit scheduler entry");
#endif
            renderer.end_frame();
        }

        runtime.set_current_stage_name("FileSelect");
        {
            auto frame = renderer.begin_frame();
            frame.frame_index = 71U;
            const auto renderer_context =
                smgpc::render::ScopedAuroraRendererContext(renderer);
            runtime.begin_frame(frame);
            runtime.set_scene_camera_pose(camera);
#ifndef NDEBUG
            runtime.set_j3d_packet_trace_frame(frame.frame_index);
#endif
            {
                auto sky = FileSelectSky{"FileSelectSky"};
                sky.initWithoutIter();
                sky.appear();
                auto *model = smgpc::compat::actor_model(&sky);
                require(model != nullptr &&
                            model->model_arc_name() == "CometNearOrbitSky" &&
                            !sky.mFlag.mIsDead,
                        "FileSelectSky did not retain its exact CometNearOrbitSky model");
                model->requireLoaded();
                require(model->isLoaded(), "CometNearOrbitSky retail BDL did not load");

#ifndef NDEBUG
                (void)require_sky_entry(runtime.scheduler().snapshot(), "FileSelectSky");
#endif
                runtime.scheduler().execute_movement();
                runtime.scheduler().execute_calc_anim();
                runtime.scheduler().execute_calc_view_and_entry();
#ifndef NDEBUG
                const auto after_movement = runtime.scheduler().snapshot();
                const auto &entry = require_sky_entry(after_movement, "FileSelectSky");
                require(entry.live_actor_bck_name == "CometNearOrbitSky" &&
                            entry.live_actor_btk_name == "CometNearOrbitSky",
                        "FileSelectSky did not start its exact BCK and BTK through the shared actor registry");
#endif
                runtime.scheduler().execute_draw_buffer_opa(camera, MR::DrawBufferType_Sky);
                runtime.scheduler().execute_draw_buffer_xlu(camera, MR::DrawBufferType_Sky);
#ifndef NDEBUG
                file_select_packets =
                    collect_packet_proof(runtime, "CometNearOrbitSky", frame.frame_index);
#endif
            }
#ifndef NDEBUG
            require(std::ranges::none_of(runtime.scheduler().snapshot(), [](const auto &entry) {
                        return entry.name == "FileSelectSky";
                    }),
                    "destroying FileSelectSky left a stale scheduler entry");
#endif
            renderer.end_frame();
        }

#ifndef NDEBUG
        require(gateway_packets.packet_count != 0U &&
                    gateway_packets.triangle_count != 0U &&
                    gateway_packets.btk_packet_count != 0U,
                "VROrbit did not submit real BTK-animated J3D packets through DrawBufferType_Sky");
        require(file_select_packets.packet_count != 0U &&
                    file_select_packets.triangle_count != 0U &&
                    file_select_packets.btk_packet_count != 0U,
                "FileSelectSky did not submit real BTK-animated J3D packets through DrawBufferType_Sky");
        std::cout << "[proof] disc=" << disc_path.string()
                  << "; gateway_model=VROrbit;gateway_packets="
                  << gateway_packets.packet_count
                  << "; file_select_model=CometNearOrbitSky;file_select_packets="
                  << file_select_packets.packet_count << '\n';
#else
        throw std::runtime_error("the shared sky scheduler proof requires a debug build");
#endif
    }

}  // namespace

int main() {
    try {
        test_gateway_and_file_select_share_exact_sky_path();
        std::cout << "[ok] Gateway and FileSelect exact skies share LiveActorModel and the retail scheduler\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] exact shared sky actor route: " << error.what() << '\n';
        return 1;
    }
}
