#include "Game/Gravity/PointGravity.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/Map/CollisionCode.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Player/MarioMapCode.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "camera/StageStartCamera.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/GatewayDemoScene.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <aurora/dvd.h>
#include <aurora/wpad.hpp>
#include <dolphin/dvd.h>
#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    constexpr auto cPlanetCollisionSource =
        std::string_view{"HeavensDoorMysteriousPlanet.arc/heavensdoormysteriousplanet.kcl"};

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

    void set_mario_swing_permission(LiveActor& actor, bool permitted) {
        auto* mario = dynamic_cast<MarioActor*>(&actor);
        if (mario == nullptr) {
            throw std::logic_error(
                "the test player entitlement bridge requires MarioActor");
        }
        mario->_EEB = permitted;
    }

    void set_host_swing_key(smgpc::render::AuroraWindow& window, bool held) {
        auto event = SDL_Event{};
        event.type = held ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        event.key.key = SDLK_X;
        require(SDL_PushEvent(&event),
                "the deterministic proof must inject a real SDL swing-key event");
        require(window.poll_events(),
                "the Gateway window must remain open while sampling the swing key");
    }

    class ScopedEnvironmentVariable final {
    public:
        ScopedEnvironmentVariable(std::string name, std::string value)
            : _name(std::move(name)) {
            if (const auto* previous = std::getenv(_name.c_str()); previous != nullptr) {
                _previous = std::string(previous);
            }
            if (::setenv(_name.c_str(), value.c_str(), 1) != 0) {
                throw std::runtime_error("could not install the deterministic WPAD script");
            }
        }

        ScopedEnvironmentVariable(const ScopedEnvironmentVariable&) = delete;
        ScopedEnvironmentVariable& operator=(const ScopedEnvironmentVariable&) = delete;

        ~ScopedEnvironmentVariable() {
            if (_previous.has_value()) {
                (void)::setenv(_name.c_str(), _previous->c_str(), 1);
            } else {
                (void)::unsetenv(_name.c_str());
            }
        }

    private:
        std::string _name;
        std::optional<std::string> _previous{};
    };

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
                "the Gateway Mario walk proof requires a readable working directory");
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
            "the Gateway Mario walk proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

    [[nodiscard]] float dot(const TVec3f& left, const TVec3f& right) {
        return left.x * right.x + left.y * right.y + left.z * right.z;
    }

    [[nodiscard]] TVec3f cross(const TVec3f& left, const TVec3f& right) {
        return TVec3f{left.y * right.z - left.z * right.y,
                      left.z * right.x - left.x * right.z,
                      left.x * right.y - left.y * right.x};
    }

    [[nodiscard]] std::size_t model_packet_count(
        const smgpc::render::J3dModelGeometry& geometry) {
        auto count = std::size_t{};
        for (const auto& shape : geometry.shapes) {
            count += shape.draw_packets.size();
        }
        return count;
    }

    struct DrawProof {
        std::size_t packet_count = 0U;
        std::size_t source_triangles = 0U;
        std::size_t parsed_display_list_bytes = 0U;
        std::size_t animated_joint_packets = 0U;
        float bck_frame = 0.0F;
        std::int16_t bck_frame_max = 0;
    };

#ifndef NDEBUG
    [[nodiscard]] DrawProof collect_mario_draw_proof(
        const smgpc::runtime::RuntimeContext& runtime, std::uint64_t frame_index) {
        auto proof = DrawProof{};
        for (const auto& packet : runtime.j3d_packet_trace()) {
            if (packet.model_name != "Mario" || packet.frame_index != frame_index) {
                continue;
            }
            ++proof.packet_count;
            proof.source_triangles += packet.state.source_triangle_count;
            proof.parsed_display_list_bytes += packet.state.parsed_display_list_bytes;
            proof.animated_joint_packets +=
                packet.state.bck_active && packet.state.bck_joint_count != 0U;
            if (packet.state.bck_active) {
                proof.bck_frame = packet.state.bck_frame;
                proof.bck_frame_max = packet.state.bck_frame_max;
            }
        }
        return proof;
    }

    void require_single_phase(const smgpc::runtime::SceneScheduler& scheduler,
                              smgpc::runtime::SceneSchedulerPhase phase) {
        const auto trace = scheduler.last_execution_trace();
        const auto count = std::ranges::count_if(trace, [phase](const auto& entry) {
            return entry.name == "MarioActor" && entry.phase == phase;
        });
        if (count != 1) {
            throw std::runtime_error(
                "RuntimeContext Mario phase count mismatch;phase=" +
                std::to_string(static_cast<int>(phase)) + ";count=" +
                std::to_string(count));
        }
    }
#endif

    struct FrameProof {
        TVec3f position_before{};
        TVec3f position_after{};
        TVec3f last_move{};
        std::array<float, 12U> base_matrix{};
        DrawProof draw{};
        std::uint32_t effective_hold_mask = 0U;
        bool grounded = false;
        bool debug_button_script_applied = false;
        std::string bck_name{};
    };

    void require_grounded_base_matrix(const FrameProof& proof,
                                      const TVec3f& ground_normal) {
        require(std::ranges::all_of(proof.base_matrix, [](float value) {
                    return std::isfinite(value);
                }),
                "Mario's rendered base matrix must remain finite");
        require_near(proof.base_matrix[3U], proof.position_after.x, 0.001F,
                     "Mario base-matrix translation X");
        require_near(proof.base_matrix[7U], proof.position_after.y, 0.001F,
                     "Mario base-matrix translation Y");
        require_near(proof.base_matrix[11U], proof.position_after.z, 0.001F,
                     "Mario base-matrix translation Z");

        const auto side = TVec3f{
            proof.base_matrix[0U], proof.base_matrix[4U], proof.base_matrix[8U]};
        const auto up = TVec3f{
            proof.base_matrix[1U], proof.base_matrix[5U], proof.base_matrix[9U]};
        const auto front = TVec3f{
            proof.base_matrix[2U], proof.base_matrix[6U], proof.base_matrix[10U]};
        require_near(side.length(), 1.0F, 0.001F, "Mario base side must be unit length");
        require_near(up.length(), 1.0F, 0.001F, "Mario base up must be unit length");
        require_near(front.length(), 1.0F, 0.001F, "Mario base front must be unit length");
        require(std::fabs(dot(side, up)) < 0.001F &&
                    std::fabs(dot(side, front)) < 0.001F &&
                    std::fabs(dot(up, front)) < 0.001F &&
                    dot(cross(side, up), front) > 0.999F,
                "Mario's rendered base matrix must be right-handed and orthonormal");
        require(dot(up, ground_normal) > 0.999F,
                "grounded Mario's rendered up axis must follow the fresh Binder ground normal");
    }

    void test_real_gateway_mario_stand_and_walk() {
        const auto disc_path = require_real_disc();
        const auto scripted_input = ScopedEnvironmentVariable{
            "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "113-157:UP"};

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
            .title = "SMG PC Gateway Mario stand/walk proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
        runtime.set_current_stage_name("HeavensDoorGalaxy");

        auto scene = smgpc::scene::GatewayDemoScene(runtime.dvd());
        const auto& start = scene.start_info();
        require(start.object_name == "Mario" && start.start_id == 0 && start.zone_id == 0 &&
                    start.camera_id == 78 && start.layer_name == "layera" &&
                    start.table_path == "jmp/start/layera/startinfo",
                "the Mario walk proof must use exact Gateway scenario-1 StartInfo");
        require(scene.planet_placement().zone_name == "HeavensDoorMysteriousZone" &&
                    scene.planet_placement().jmap_entry_index == 24 &&
                    scene.planet_placement().object_name == "HeavensDoorMysteriousPlanet",
                "the Mario walk proof must use exact child-zone mysterious-planet placement");
        require(scene.planet_bdl().size() == 1269376U &&
                    scene.planet_kcl().size() == 632430U &&
                    scene.planet_pa().size() == 31232U &&
                    model_packet_count(scene.planet_geometry()) != 0U,
                "the proof must retain the exact retail planet model/KCL/PA resources");

        const auto* point_gravity = dynamic_cast<const PointGravity*>(&scene.gravity());
        require(point_gravity != nullptr,
                "the Mario walk proof must use the real child-zone PointGravity");
        require_near(point_gravity->mTranslation.x, 14760.0F, 0.001F,
                     "Gateway point-gravity center X");
        require_near(point_gravity->mTranslation.y, -10676.2255859375F, 0.001F,
                     "Gateway point-gravity center Y");
        require_near(point_gravity->mTranslation.z, 6770.0F, 0.001F,
                     "Gateway point-gravity center Z");
        require(MR::getMarioHolder() != nullptr,
                "GatewayDemoScene must provide the retail MarioHolder SceneObj");

        auto& walk_collision = scene.collision();
        require(smgpc::scene::StageCollisionService::active() == &walk_collision &&
                    walk_collision.stats().mesh_count == 1U &&
                    walk_collision.stats().triangle_count == 7789U,
                "Mario and the executable must share GatewayDemoScene's exact active KCL service");

        const auto camera_result =
            smgpc::camera::resolve_stage_start_camera(runtime.dvd(), start);
        require(camera_result.status ==
                        smgpc::camera::StageStartCameraResolveStatus::Resolved &&
                    camera_result.camera.has_value() &&
                    camera_result.camera->camera_key == "s:004e" &&
                    camera_result.camera->camera_param.camera_type == "CAM_TYPE_XZ_PARA",
                "the proof must resolve exact Gateway StartInfo camera 78");
        const auto camera = camera_result.camera->calculation.pose;
        runtime.camera_system().set_game_camera_pose(camera);
        runtime.set_freecam_enabled(false);

        require(NameObjFactory::getCreator("Mario") == nullptr &&
                    NameObjFactory::getCreator("MarioActor") == nullptr &&
                    !smgpc::scene::nameobj::can_create_name_obj("Mario") &&
                    !smgpc::scene::nameobj::can_create_name_obj("MarioActor"),
                "the production Mario factory must remain absent in the development slice");

        auto created = std::unique_ptr<NameObj>{createNameObj<MarioActor>("MarioActor")};
        auto* actor = dynamic_cast<MarioActor*>(created.get());
        require(actor != nullptr,
                "the typed development creator must construct the real MarioActor");
        const auto entitlement_bridge =
            smgpc::runtime::PlayerActorEntitlementBridge{
                .set_swing_permission = &set_mario_swing_permission,
            };
        runtime.player_system().attach_actor(*actor, entitlement_bridge);
        require(runtime.player_system().attached_actor() == actor &&
                    !runtime.player_system().is_swing_permitted() && !actor->_EEB,
                "the real Gateway player owner must attach Mario with spin entitlement initially locked");

        auto wait_frame_max = std::int16_t{};
        {
            auto frame = renderer.begin_frame();
            frame.frame_index = 100U;
            {
                const auto renderer_context =
                    smgpc::render::ScopedAuroraRendererContext(renderer);
                runtime.begin_frame(frame);
                actor->init(scene.player_start_iter());
                actor->initAfterPlacement();
                wait_frame_max =
                    smgpc::compat::require_actor_bck(actor, "Wait", nullptr);
                runtime.game_layout().activate_game_scene_draw_3d();
            }
            renderer.end_frame();
        }

        require(MR::getMarioHolder()->getMarioActor() == actor,
                "MarioActor init must register the real actor with MarioHolder");
        require(smgpc::compat::actor_model(actor) != nullptr &&
                    smgpc::compat::actor_model(actor)->isLoaded() &&
                    smgpc::compat::actor_model_joint_count(actor) != 0U,
                "MarioActor init must load the real Mario model and joints");
        require(smgpc::compat::actor_current_bck_name(actor) == "Wait" &&
                    wait_frame_max == 180,
                "MarioActor init must bind the real 180-frame Wait.bck");

        auto expected_gravity = point_gravity->mTranslation - actor->mPosition;
        expected_gravity.scale(1.0F / expected_gravity.length());
        require(actor->mFlag.mIsCalcGravity &&
                    actor->mGravity.epsilonEquals(expected_gravity, 0.0001F) &&
                    actor->_240.epsilonEquals(expected_gravity, 0.0001F) &&
                    actor->mMario->mAirGravityVec.epsilonEquals(expected_gravity, 0.0001F),
                "MarioActor init must enable and retain exact Gateway point gravity");

#ifndef NDEBUG
        require(std::ranges::count_if(runtime.scheduler().snapshot(), [](const auto& entry) {
                    return entry.name == "MarioActor";
                }) == 1,
                "RuntimeContext must own exactly one Mario registration");
#endif

        const auto run_frame = [&](std::uint64_t frame_index) {
            auto proof = FrameProof{};
            auto frame = renderer.begin_frame();
            frame.frame_index = frame_index;
            {
                const auto renderer_context =
                    smgpc::render::ScopedAuroraRendererContext(renderer);
#ifndef NDEBUG
                runtime.set_j3d_packet_trace_frame(frame.frame_index);
#endif
                proof.position_before = actor->mPosition;
                runtime.begin_frame(frame);
                proof.position_after = actor->mPosition;
                proof.last_move = actor->getLastMove();
                proof.grounded =
                    actor->mBinder != nullptr && actor->mBinder->isBindedGround();
                proof.bck_name =
                    std::string(smgpc::compat::actor_current_bck_name(actor));
                proof.base_matrix = smgpc::compat::actor_base_matrix(actor).m;

                const auto active_camera =
                    runtime.scene_camera_pose().value_or(camera);
                runtime.draw_3d_normal(active_camera);
#ifndef NDEBUG
                proof.draw = collect_mario_draw_proof(runtime, frame.frame_index);
                proof.effective_hold_mask =
                    runtime.host_input_trace().effective_hold_mask;
                proof.debug_button_script_applied =
                    runtime.host_input_trace().debug_button_script_applied;
                require_single_phase(runtime.scheduler(),
                                     smgpc::runtime::SceneSchedulerPhase::Movement);
                require_single_phase(runtime.scheduler(),
                                     smgpc::runtime::SceneSchedulerPhase::CalcAnim);
                require_single_phase(runtime.scheduler(),
                                     smgpc::runtime::SceneSchedulerPhase::CalcViewAndEntry);
                require_single_phase(runtime.scheduler(),
                                     smgpc::runtime::SceneSchedulerPhase::DrawType);
#else
                throw std::runtime_error(
                    "the Gateway Mario proof requires debug input and packet traces");
#endif
            }
            renderer.end_frame();

            const auto observed_move = proof.position_after - proof.position_before;
            require(observed_move.epsilonEquals(proof.last_move, 0.0001F),
                    "Mario must integrate exactly once in each RuntimeContext frame");
            return proof;
        };

        auto wait_frame = FrameProof{};
        auto saw_ground = false;
        for (auto frame_index = std::uint64_t{101U}; frame_index < 113U;
             ++frame_index) {
            wait_frame = run_frame(frame_index);
            require(!wait_frame.debug_button_script_applied,
                    "neutral settle frames must precede the scripted input window");
            if (saw_ground && !wait_frame.grounded) {
                const auto binder_center = actor->mPosition + actor->_2C4;
                const auto support = walk_collision.move_sphere(
                    binder_center, TVec3f{}, actor->mBinder->mRadius, 32U, true);
                throw std::runtime_error(
                    "Mario lost neutral ground;frame=" + std::to_string(frame_index) +
                    ";position=" + std::to_string(actor->mPosition.x) + "," +
                    std::to_string(actor->mPosition.y) + "," +
                    std::to_string(actor->mPosition.z) + ";last_move=" +
                    std::to_string(wait_frame.last_move.x) + "," +
                    std::to_string(wait_frame.last_move.y) + "," +
                    std::to_string(wait_frame.last_move.z) + ";velocity=" +
                    std::to_string(actor->mVelocity.x) + "," +
                    std::to_string(actor->mVelocity.y) + "," +
                    std::to_string(actor->mVelocity.z) + ";planes=" +
                    std::to_string(actor->mBinder->mPlaneNum) +
                    ";support_contacts=" + std::to_string(support.contacts.size()) +
                    ";support_move=" + std::to_string(support.displacement.length()));
            }
            saw_ground = saw_ground || wait_frame.grounded;
        }
        require(saw_ground && wait_frame.grounded && wait_frame.last_move.length() < 0.01F,
                "neutral Mario must settle to a stable stand on the real planet KCL");
        require(wait_frame.bck_name == "Wait" && wait_frame.draw.packet_count != 0U &&
                    wait_frame.draw.source_triangles != 0U &&
                    wait_frame.draw.parsed_display_list_bytes != 0U &&
                    wait_frame.draw.animated_joint_packets != 0U &&
                    wait_frame.draw.bck_frame_max == 180,
                "the stable stand frame must draw real Mario packets with Wait.bck");

        const auto& stand_triangle = actor->mBinder->mGroundInfo.mParentTriangle;
        const auto stand_surface = walk_collision.surface(stand_triangle.mIdx);
        require(stand_surface.has_value() &&
                    stand_surface->source_name == cPlanetCollisionSource &&
                    stand_surface->prism_index == 4642U &&
                    stand_surface->attribute == 4642U &&
                    stand_surface->sensor != nullptr &&
                    stand_triangle.mSensor == stand_surface->sensor,
                "Gateway stand contact must be exact prism/PA row 4642 with scene-owned body-sensor provenance");
        require(FloorCode{}.getCode(&stand_triangle) == CollisionFloorCode_NoSlip &&
                    MR::getSoundCodeIndex(stand_triangle.getAttributes()) ==
                        CollisionSoundCode_Lawn,
                "Gateway stand contact must decode retail NoSlip and Lawn attributes");
        require_grounded_base_matrix(wait_frame, *stand_triangle.getNormal(0));

        const auto stand_position = actor->mPosition;
        auto stand_gravity = actor->mGravity;
        stand_gravity.scale(1.0F / stand_gravity.length());
        auto run_draw = DrawProof{};
        auto saw_run = false;
        auto walked_every_frame_on_ground = true;
        auto animation_frame_advanced = false;
        auto previous_run_frame = std::optional<float>{};
        for (auto frame_index = std::uint64_t{113U}; frame_index < 158U;
             ++frame_index) {
            const auto walk_frame = run_frame(frame_index);
            require(walk_frame.debug_button_script_applied &&
                        (walk_frame.effective_hold_mask & WPAD_BUTTON_UP) != 0U,
                    "the walk must come from RuntimeContext's pre-scheduler WPAD path");
            walked_every_frame_on_ground =
                walked_every_frame_on_ground && walk_frame.grounded;
            saw_run = saw_run || walk_frame.bck_name == "Run";
            if (walk_frame.bck_name == "Run") {
                if (previous_run_frame.has_value() &&
                    std::fabs(walk_frame.draw.bck_frame - *previous_run_frame) > 0.001F) {
                    animation_frame_advanced = true;
                }
                previous_run_frame = walk_frame.draw.bck_frame;
            }
            run_draw = walk_frame.draw;
        }

        const auto walk_displacement = actor->mPosition - stand_position;
        const auto walk_distance = walk_displacement.length();
        const auto walk_speed = actor->mMario->_278;
        require(std::fabs(actor->mMario->mStickPos.x) < 0.01F &&
                    actor->mMario->mStickPos.y > 0.9F &&
                    actor->mMario->mStickPos.z > 0.9F && walk_speed > 0.0F &&
                    actor->mMario->mWorldPadDir.length() > 0.9F,
                "the runtime UP path must retain retail forward-stick orientation, direction, and speed");
        require(walked_every_frame_on_ground,
                "stick-driven Mario must remain grounded across the real planet KCL seam");
        require(saw_run && smgpc::compat::actor_current_bck_name(actor) == "Run" &&
                    run_draw.packet_count != 0U && run_draw.source_triangles != 0U &&
                    run_draw.parsed_display_list_bytes != 0U &&
                    run_draw.animated_joint_packets != 0U &&
                    run_draw.bck_frame_max > 0 && animation_frame_advanced,
                "stick input must select, advance, and draw the real Run.bck model");
        require(walk_distance > 5.0F &&
                    std::fabs(dot(walk_displacement, stand_gravity)) <
                        walk_distance * 0.35F,
                "stick input must move Mario materially along the planet tangent");

        const auto& walk_triangle = actor->mBinder->mGroundInfo.mParentTriangle;
        require(FloorCode{}.getCode(&walk_triangle) == CollisionFloorCode_NoSlip &&
                    MR::getSoundCodeIndex(walk_triangle.getAttributes()) ==
                        CollisionSoundCode_Lawn,
                "the walked-to real planet surface must remain NoSlip and Lawn");
        auto walk_matrix_proof = FrameProof{};
        walk_matrix_proof.position_after = actor->mPosition;
        walk_matrix_proof.base_matrix = smgpc::compat::actor_base_matrix(actor).m;
        require_grounded_base_matrix(walk_matrix_proof, *walk_triangle.getNormal(0));

        auto release_frame = FrameProof{};
        auto release_end_frame = std::uint64_t{158U};
        auto saw_release_inertia = false;
        auto reached_wait_at_rest = false;
        auto stable_rest_frames = std::size_t{};
        auto last_release_tangent_move = 0.0F;
        auto last_release_normal_move = 0.0F;
        for (; release_end_frame < 360U; ++release_end_frame) {
            release_frame = run_frame(release_end_frame);
            require(!release_frame.debug_button_script_applied,
                    "release frames must leave the deterministic input window");
            if (!release_frame.grounded) {
                const auto binder_center = actor->mPosition + actor->_2C4;
                const auto support = walk_collision.move_sphere(
                    binder_center, TVec3f{}, actor->mBinder->mRadius, 32U, true);
                std::string expanded_counts;
                for (const auto extra : {1.2F, 1.21F, 1.25F, 1.3F, 1.5F, 2.0F}) {
                    expanded_counts += "," + std::to_string(extra) + ":" +
                        std::to_string(walk_collision.sphere_contacts(
                            binder_center, actor->mBinder->mRadius + extra, 32U).size());
                }
                throw std::runtime_error(
                    "Mario lost release ground;frame=" +
                    std::to_string(release_end_frame) + ";speed=" +
                    std::to_string(actor->mMario->_278) + ";band=" +
                    std::to_string(actor->mMario->_71C) + ";position=" +
                    std::to_string(actor->mPosition.x) + "," +
                    std::to_string(actor->mPosition.y) + "," +
                    std::to_string(actor->mPosition.z) + ";last_move=" +
                    std::to_string(release_frame.last_move.x) + "," +
                    std::to_string(release_frame.last_move.y) + "," +
                    std::to_string(release_frame.last_move.z) + ";velocity=" +
                    std::to_string(actor->mVelocity.x) + "," +
                    std::to_string(actor->mVelocity.y) + "," +
                    std::to_string(actor->mVelocity.z) + ";planes=" +
                    std::to_string(actor->mBinder->mPlaneNum) +
                    ";support_contacts=" + std::to_string(support.contacts.size()) +
                    ";expanded_counts=" + expanded_counts);
            }

            const auto speed = actor->mMario->_278;
            if (actor->mMario->_71C == 0 && speed >= 0.2F) {
                saw_release_inertia = true;
                require(release_frame.bck_name == "Run",
                        "Mario must keep Run while release inertia still moves him");
            }
            auto release_ground_normal =
                *actor->mBinder->mGroundInfo.mParentTriangle.getNormal(0);
            release_ground_normal.scale(1.0F / release_ground_normal.length());
            last_release_normal_move =
                dot(release_frame.last_move, release_ground_normal);
            const auto release_normal_move =
                release_ground_normal * last_release_normal_move;
            const auto release_tangent_move =
                release_frame.last_move - release_normal_move;
            last_release_tangent_move = release_tangent_move.length();
            if (actor->mMario->_71C == 0 && speed == 0.0F &&
                actor->mVelocity.length() < 0.001F &&
                release_frame.bck_name == "Wait" &&
                last_release_tangent_move < 0.01F &&
                std::fabs(last_release_normal_move) <= 1.2001F) {
                ++stable_rest_frames;
                if (stable_rest_frames >= 3U) {
                    reached_wait_at_rest = true;
                    break;
                }
            } else {
                stable_rest_frames = 0U;
            }
        }
        if (!(saw_release_inertia && reached_wait_at_rest &&
              actor->mMario->_278 < walk_speed &&
              actor->mMario->mStickPos.z < 0.01F)) {
            throw std::runtime_error(
                "stick release did not reach stable Wait/rest;frame=" +
                std::to_string(release_end_frame) + ";speed=" +
                std::to_string(actor->mMario->_278) + ";stick=" +
                std::to_string(actor->mMario->mStickPos.z) + ";band=" +
                std::to_string(actor->mMario->_71C) + ";bck=" +
                release_frame.bck_name + ";last_move=" +
                std::to_string(release_frame.last_move.length()) +
                ";tangent_move=" +
                std::to_string(last_release_tangent_move) +
                ";normal_move=" + std::to_string(last_release_normal_move) +
                ";stable_frames=" + std::to_string(stable_rest_frames) +
                ";saw_inertia=" + std::to_string(saw_release_inertia));
        }
        require(release_frame.draw.packet_count != 0U &&
                    release_frame.draw.animated_joint_packets != 0U &&
                    release_frame.draw.bck_frame_max == 180,
                "the released actor must remain visible with the real Wait.bck");
        require_grounded_base_matrix(
            release_frame, *actor->mBinder->mGroundInfo.mParentTriangle.getNormal(0));

        constexpr auto cIdleProofFrames = std::uint64_t{60U};
        for (auto idle_index = std::uint64_t{}; idle_index < cIdleProofFrames;
             ++idle_index) {
            ++release_end_frame;
            release_frame = run_frame(release_end_frame);
            require(!release_frame.debug_button_script_applied &&
                        release_frame.grounded && actor->mMario->_278 == 0.0F &&
                        actor->mMario->_71C == 0 &&
                        actor->mMario->mStickPos.z < 0.01F &&
                        actor->mVelocity.length() < 0.001F &&
                        release_frame.bck_name == "Wait" &&
                        release_frame.draw.packet_count != 0U &&
                        release_frame.draw.animated_joint_packets != 0U,
                    "zero-input Mario must remain grounded, still, animated, and visible throughout the idle proof");
            auto idle_ground_normal =
                *actor->mBinder->mGroundInfo.mParentTriangle.getNormal(0);
            idle_ground_normal.scale(1.0F / idle_ground_normal.length());
            const auto idle_normal_move =
                dot(release_frame.last_move, idle_ground_normal);
            const auto idle_tangent_move =
                release_frame.last_move - idle_ground_normal * idle_normal_move;
            require(idle_tangent_move.length() < 0.01F &&
                        std::fabs(idle_normal_move) <= 1.2001F,
                    "zero-input idle contact must not hide tangent drift or an excessive normal correction");
        }
        require_grounded_base_matrix(
            release_frame, *actor->mBinder->mGroundInfo.mParentTriangle.getNormal(0));

        const auto entitlement_magic = actor->mMario->mMagic;
        const auto entitlement_action = actor->_1E1;
        const auto entitlement_cooldown = actor->_946;
        require(entitlement_magic == nullptr && !entitlement_action &&
                    entitlement_cooldown == 0U,
                "the walk slice must begin without fabricated spin Magic/action state");

        set_host_swing_key(window, true);
        const auto locked_swing_frame = run_frame(++release_end_frame);
        require(aurora::wpad_service().is_core_swing(WPAD_CHAN0) &&
                    aurora::wpad_service().is_core_swing_triggered(WPAD_CHAN0) &&
                    actor->_F00 && !actor->_EEB && !actor->isRequestRush() &&
                    locked_swing_frame.bck_name == "Wait",
                "a real host swing edge must be sampled but denied before entitlement");

        const auto locked_held_frame = run_frame(++release_end_frame);
        require(aurora::wpad_service().is_core_swing(WPAD_CHAN0) &&
                    !aurora::wpad_service().is_core_swing_triggered(WPAD_CHAN0) &&
                    !actor->_F00 && !actor->isRequestRush() &&
                    locked_held_frame.bck_name == "Wait",
                "holding the locked swing key must not synthesize another controller edge");

        set_host_swing_key(window, false);
        const auto rearm_swing_frame = run_frame(++release_end_frame);
        require(!aurora::wpad_service().is_core_swing(WPAD_CHAN0) &&
                    !actor->_F20 && !actor->_F00 && !actor->isRequestRush() &&
                    rearm_swing_frame.bck_name == "Wait",
                "releasing the host swing key must rearm Mario's retail edge detector");

        MR::setPlayerSwingPermission(true);
        require(runtime.player_system().is_swing_permitted() && actor->_EEB &&
                    actor->mMario->mMagic == entitlement_magic &&
                    actor->_1E1 == entitlement_action &&
                    actor->_946 == entitlement_cooldown,
                "swing entitlement must mirror only MarioActor::_EEB without fabricating action state");

        set_host_swing_key(window, true);
        const auto unlocked_swing_frame = run_frame(++release_end_frame);
        require(aurora::wpad_service().is_core_swing_triggered(WPAD_CHAN0) &&
                    actor->_F00 && actor->isRequestRush() &&
                    actor->mMario->mMagic == entitlement_magic &&
                    actor->_1E1 == entitlement_action &&
                    actor->_946 == entitlement_cooldown &&
                    unlocked_swing_frame.bck_name == "Wait",
                "a fresh real host swing edge must request Rush after entitlement without starting the spin action");

        const auto unlocked_held_frame = run_frame(++release_end_frame);
        require(aurora::wpad_service().is_core_swing(WPAD_CHAN0) &&
                    !aurora::wpad_service().is_core_swing_triggered(WPAD_CHAN0) &&
                    !actor->_F00 && !actor->isRequestRush() &&
                    actor->mMario->mMagic == entitlement_magic &&
                    actor->_1E1 == entitlement_action &&
                    actor->_946 == entitlement_cooldown &&
                    unlocked_held_frame.bck_name == "Wait",
                "holding an entitled swing must remain debounced without entering spin action state");

        set_host_swing_key(window, false);
        const auto released_swing_frame = run_frame(++release_end_frame);
        require(!actor->_F00 && !actor->isRequestRush() &&
                    released_swing_frame.bck_name == "Wait",
                "Rush permission must remain edge-triggered after the host key is released");

        runtime.player_system().detach_actor(actor);
        require(runtime.player_system().attached_actor() == nullptr &&
                    runtime.player_system().is_swing_permitted() && !actor->_EEB,
                "Gateway owner detach must revoke the outgoing actor bit while retaining same-stage entitlement");
        runtime.unregister_live_actor_model(*actor);
        MR::getMarioHolder()->setMarioActor(nullptr);
        created.reset();
        actor = nullptr;
        require(MR::getMarioHolder()->getMarioActor() == nullptr,
                "MarioHolder must be cleared before the actor owner is destroyed");
#ifndef NDEBUG
        require(std::ranges::none_of(runtime.scheduler().snapshot(), [](const auto& entry) {
                    return entry.name == "MarioActor";
                }),
                "Mario teardown must remove the RuntimeContext scheduler entry");
#endif

        {
            auto frame = renderer.begin_frame();
            frame.frame_index = release_end_frame + 1U;
            {
                const auto renderer_context =
                    smgpc::render::ScopedAuroraRendererContext(renderer);
                runtime.begin_frame(frame);
                created.reset(createNameObj<MarioActor>("MarioActor"));
                actor = dynamic_cast<MarioActor*>(created.get());
                require(actor != nullptr,
                        "Mario must be constructible again after ordered teardown");
                runtime.player_system().attach_actor(*actor, entitlement_bridge);
                require(actor->_EEB,
                        "same-stage Mario replacement must inherit the retained spin entitlement");
                actor->init(scene.player_start_iter());
                actor->initAfterPlacement();
            }
            renderer.end_frame();
        }
        require(MR::getMarioHolder()->getMarioActor() == actor &&
                    smgpc::compat::actor_model(actor) != nullptr &&
                    smgpc::compat::actor_model(actor)->isLoaded(),
                "recreated Mario must own the real model and replace the holder binding");
        const auto recreated_frame = run_frame(release_end_frame + 2U);
        require(recreated_frame.draw.packet_count != 0U,
                "recreated Mario must update and draw through RuntimeContext");

        runtime.player_system().detach_actor(actor);
        require(runtime.player_system().attached_actor() == nullptr &&
                    !actor->_EEB,
                "recreated Mario must detach and revoke its entitlement bit before destruction");
        runtime.unregister_live_actor_model(*actor);
        MR::getMarioHolder()->setMarioActor(nullptr);
        created.reset();
        actor = nullptr;
#ifndef NDEBUG
        require(std::ranges::none_of(runtime.scheduler().snapshot(), [](const auto& entry) {
                    return entry.name == "MarioActor";
                }),
                "recreated Mario must also tear down without a stale scheduler entry");
#endif
        require(NameObjFactory::getCreator("MarioActor") == nullptr &&
                    !smgpc::scene::nameobj::can_create_name_obj("MarioActor"),
                "the passing development route must not enable the global Mario factory");

        std::cout << "[proof] disc=" << disc_path.string()
                  << ";start=(" << start.world_position[0] << ','
                  << start.world_position[1] << ',' << start.world_position[2] << ')'
                  << ";kcl_triangles=" << walk_collision.stats().triangle_count
                  << ";stand_prism=" << stand_surface->prism_index
                  << ";floor=NoSlip;sound=Lawn"
                  << ";walk_distance=" << walk_distance
                  << ";bck=Wait->Run->Wait"
                  << ";release_frame=" << release_end_frame
                  << ";run_packets=" << run_draw.packet_count
                  << ";recreated=1\n";
    }
}  // namespace

int main() {
    try {
        test_real_gateway_mario_stand_and_walk();
        std::cout << "[ok] real Gateway Mario stand/walk/release proof\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] real Gateway Mario stand/walk proof: " << error.what()
                  << '\n';
        return 1;
    }
}
