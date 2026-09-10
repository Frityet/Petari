#include "resource/TextEncoding.hpp"
#include "Game/Gravity/PointGravity.hpp"
#include "Game/Animation/XanimeCore.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/Map/CollisionCode.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Map/PlanetMap.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/NameObj/NameObjListExecutor.hpp"
#include "Game/System/DrawBuffer.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadAcceleration.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/System/WPadHolder.hpp"
#include <JSystem/J3DGraphBase/J3DShape.hpp>
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Player/MarioMapCode.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Logger.hpp"
#include "MarioWalkParameterTests.hpp"
#include "MarioCameraTargetTests.hpp"
#include "OriginalMarioStateTests.hpp"
#include "OriginalPlayerUtilTests.hpp"
#include "RendererService.hpp"
#include "camera/StageStartCamera.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/CollisionPartsCompat.hpp"
#include "compat/CollisionDirectorOwnership.hpp"
#include "compat/GameDataSession.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/MarioCameraTarget.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/GatewayDemoScene.hpp"
#include "scene/AuthoredPlacementInstantiator.hpp"
#include "scene/NameObjChildOwner.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/SceneInitializationState.hpp"
#include "scene/SceneExecutionBinding.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <aurora/dvd.h>
#include <aurora/wpad.hpp>
#include <dolphin/dvd.h>
#include <dolphin/mtx.h>
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
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    constexpr auto cPlanetCollisionSource =
        std::string_view{"HeavensDoorMysteriousPlanet.arc:/heavensdoormysteriousplanet.kcl"};

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    // Same raw-child and heap lifetime boundary used by Showcase's player
    // owner, including failure unwinding and same-scene player replacement.
    class MarioFixtureOwner final {
    public:
        explicit MarioFixtureOwner(smgpc::runtime::RuntimeContext& runtime)
            : _runtime(runtime), _domain(smgpc::scene::current_scene_allocation_domain()) {
            require(_domain != nullptr, "Mario fixture construction requires the actual scene heap");
            create();
        }

        ~MarioFixtureOwner() { reset(); }

        void create() {
            require(_actor == nullptr, "Mario fixture must retire its old actor before replacement");
            const auto phase = smgpc::scene::SceneInitializationScope(SceneInitializeState_PlacementPlayer);
            _actor = static_cast<MarioActor*>(_objects.capture_construction_children([&] {
                const auto game = smgpc::compat::JkrAllocationScope(_domain);
                return createNameObj<MarioActor>("MarioActor");
            }));
            require(_actor != nullptr, "Mario fixture creator returned no actor");
        }

        void initialize(const JMapInfoIter& placement) {
            const auto phase = smgpc::scene::SceneInitializationScope(SceneInitializeState_PlacementPlayer);
            _objects.capture_construction_children([&] {
                const auto game = smgpc::compat::JkrAllocationScope(_domain);
                _actor->init(placement);
            });
        }

        MarioActor* get() const { return _actor; }

        void reset() noexcept {
            const auto host = smgpc::compat::JkrHostAllocationScope{};
            if (_actor != nullptr && _runtime.player_system().attached_actor() == _actor) {
                _runtime.player_system().detach_actor(_actor);
            }
            if (auto* holder = MR::getMarioHolder(); holder != nullptr && holder->getMarioActor() == _actor) {
                holder->setMarioActor(nullptr);
            }
            _objects.clear();
            _actor = nullptr;
        }

    private:
        smgpc::runtime::RuntimeContext& _runtime;
        std::shared_ptr<smgpc::compat::JkrAllocationDomain> _domain;
        smgpc::scene::NameObjChildOwner _objects;
        MarioActor* _actor = nullptr;
    };

    void require_near(float actual, float expected, float tolerance,
                      std::string_view message) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > tolerance) {
            throw std::runtime_error(std::string(message) + ";actual=" +
                                     std::to_string(actual) + ";expected=" +
                                     std::to_string(expected));
        }
    }

    void verify_mario_camera_target_accessors(MarioActor& actor) {
        const auto saved_shadow = actor.mMario->mShadowPos;
        const auto saved_ground = actor.mMario->mGroundPos;
        const auto saved_up = actor.mUpVec;
        const auto saved_front = actor.mMario->mFrontVec;
        const auto saved_side = actor.mMario->mSideVec;
        const auto saved_forced_matrix_enabled = actor._EA5;
        const auto saved_forced_matrix = actor._EA8;
        const auto saved_bound = actor._934;
        auto* const saved_bound_sensor = actor._924;
        actor.mMario->mShadowPos.set(11.0F, 22.0F, 33.0F);
        actor.mMario->mGroundPos.set(44.0F, 55.0F, 66.0F);
        actor.mUpVec.set(0.0F, 2.0F, 0.0F);
        actor.mMario->mFrontVec.set(3.0F, 0.0F, 0.0F);
        actor.mMario->mSideVec.set(0.0F, 0.0F, -4.0F);
        actor._934 = false;
        const auto target = smgpc::compat::create_mario_camera_target(actor);
        target->movement();

        TVec3f player_up;
        TVec3f player_front;
        TVec3f player_side;
        MR::getPlayerUpVec(&player_up);
        MR::getPlayerFrontVec(&player_front);
        MR::getPlayerSideVec(&player_side);

        // A real rush host and forced base matrix make the bound target's
        // orientation distinct from the raw MarioActor vector getters.
        auto bound_host = LiveActor("Player vector accessor bound host");
        bound_host.initBinder(10.0F, 0.0F, 0U);
        auto bound_sensor = HitSensor(0U, 0U, 10.0F, &bound_host);
        actor._924 = &bound_sensor;
        actor._934 = true;
        actor._EA5 = true;
        actor._EA8.identity();
        actor._EA8[0][0] = 0.0F;
        actor._EA8[0][2] = 1.0F;
        actor._EA8[1][0] = 1.0F;
        actor._EA8[1][1] = 0.0F;
        actor._EA8[2][1] = 1.0F;
        actor._EA8[2][2] = 0.0F;
        const auto bound_target = smgpc::compat::create_mario_camera_target(actor);
        bound_target->movement();
        TVec3f bound_player_up;
        TVec3f bound_player_front;
        TVec3f bound_player_side;
        MR::getPlayerUpVec(&bound_player_up);
        MR::getPlayerFrontVec(&bound_player_front);
        MR::getPlayerSideVec(&bound_player_side);

        actor.mMario->mShadowPos = saved_shadow;
        actor.mMario->mGroundPos = saved_ground;
        actor.mUpVec = saved_up;
        actor.mMario->mFrontVec = saved_front;
        actor.mMario->mSideVec = saved_side;
        actor._EA5 = saved_forced_matrix_enabled;
        actor._EA8 = saved_forced_matrix;
        actor._934 = saved_bound;
        actor._924 = saved_bound_sensor;
        // Retail PSVECNormalize's frsqrte/Newton sequence produces 0x3F7FFFFF
        // for these axis inputs. Mathematical division previously gave 1.0.
        constexpr auto normalized_axis = 0x1.fffffep-1F;
        const auto up_input = Vec{0.0F, 2.0F, 0.0F};
        auto sdk_up = Vec{};
        PSVECNormalize(&up_input, &sdk_up);
        const auto bound_up_input = Vec{0.0F, 0.0F, 1.0F};
        auto sdk_bound_up = Vec{};
        PSVECNormalize(&bound_up_input, &sdk_bound_up);
        require(sdk_up.y == normalized_axis && sdk_bound_up.z == normalized_axis,
                "original SDK axis normalization must preserve the retail 0x3F7FFFFF result");
        require(target->getGroundPos().x == 11.0F &&
                    target->getGroundPos().y == 22.0F &&
                    target->getGroundPos().z == 33.0F &&
                    target->getUpVec().x == 0.0F && target->getUpVec().y == sdk_up.y && target->getUpVec().z == 0.0F &&
                    target->getSideVec().z == -4.0F,
                "the player camera bridge must use shadow position, normalized camera up, and Mario's side getter");
        const auto raw_vectors_match = [](const TVec3f& up, const TVec3f& front, const TVec3f& side) {
            return up.x == 0.0F && up.y == 2.0F && up.z == 0.0F &&
                   front.x == 3.0F && front.y == 0.0F && front.z == 0.0F &&
                   side.x == 0.0F && side.y == 0.0F && side.z == -4.0F;
        };
        require(raw_vectors_match(player_up, player_front, player_side) &&
                    raw_vectors_match(bound_player_up, bound_player_front, bound_player_side),
                "global player orientation must preserve all three raw MarioActor getters in normal and bound states");
        require(bound_target->getUpVec().x == 0.0F && bound_target->getUpVec().y == 0.0F &&
                    bound_target->getUpVec().z == sdk_bound_up.z &&
                    bound_target->getFrontVec().x == 1.0F && bound_target->getFrontVec().y == 0.0F &&
                    bound_target->getFrontVec().z == 0.0F &&
                    bound_target->getSideVec().x == 0.0F && bound_target->getSideVec().y == 1.0F &&
                    bound_target->getSideVec().z == 0.0F,
                "bound camera target axes must still follow the forced base matrix independently of global player vectors");
    }

    void set_host_key(smgpc::render::AuroraWindow& window, SDL_Keycode key, bool held) {
        auto event = SDL_Event{};
        event.type = held ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        event.key.key = key;
        require(SDL_PushEvent(&event),
                "the deterministic proof must inject a real SDL key event");
        require(window.poll_events(),
                "the Gateway window must remain open while sampling host input");
    }

    void set_host_swing_key(smgpc::render::AuroraWindow& window, bool held) {
        set_host_key(window, SDLK_X, held);
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

    std::size_t original_player_submitted_shapes(const MarioActor& actor) {
        auto* holder = smgpc::scene::current_scene_name_obj_list_executor().mBufferHolder;
        require(holder != nullptr, "original scene must own its allocated DrawBufferHolder");
        const auto* model = actor.mModelManager->getJ3DModel();
        auto submitted = std::size_t{};
        for (const auto& group : holder->mBufferGroups) {
            for (const auto* executor : group.mActiveExecutors) {
                if (std::find(executor->mActors, executor->mActors + executor->mNumActors, &actor) ==
                    executor->mActors + executor->mNumActors) continue;
                require(executor->mDrawBuffer != nullptr && executor->mDrawBuffer->mNumActors > 0,
                        "active original Mario draw executor must own its actor list and draw buffer");
                for (auto drawer_index = 0; drawer_index < executor->mDrawBuffer->mNumShapeDrawers; ++drawer_index) {
                    const auto* drawer = executor->mDrawBuffer->getShapeDrawerByIndex(drawer_index);
                    for (auto packet_index = 0; packet_index < drawer->mNumPackets; ++packet_index) {
                        const auto* packet = drawer->getShapePacket(packet_index);
                        if (packet->getModel() != model || (packet->mpShape->mFlags & 1U) != 0U) continue;
                        require(packet == model->getShapePacket(packet->mpShape->mIndex) &&
                                    packet->mpMtxBuffer == model->mMtxBuffer &&
                                    drawer->mMatPacket->mpDisplayListObj != nullptr &&
                                    drawer->mMatPacket->mpDisplayListObj->mSize != 0U,
                                "submitted original Mario shape must retain the model's actual packet, matrix buffer and material display list");
                        ++submitted;
                    }
                }
            }
        }
        return submitted;
    }

    struct DrawProof {
        std::size_t packet_count = 0U;
        std::size_t source_triangles = 0U;
        std::size_t parsed_display_list_bytes = 0U;
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
        }
        return proof;
    }

    void require_mario_packet_lighting(
        const smgpc::runtime::RuntimeContext& runtime,
        std::uint64_t frame_index, const ActorLightInfo& actor_light) {
        const auto expected_ambient = smgpc::render::GXColorValue{
            actor_light.mColor.r, actor_light.mColor.g,
            actor_light.mColor.b, actor_light.mColor.a};
        const auto expected_space = [](const LightInfo& light) {
            return light.mIsFollowCamera
                       ? smgpc::render::GXLightCoordinateSpace::View
                       : smgpc::render::GXLightCoordinateSpace::World;
        };
        auto material_packet_count = std::size_t{};
        for (const auto& packet : runtime.j3d_packet_trace()) {
            if (packet.model_name != "Mario" ||
                packet.frame_index != frame_index ||
                packet.state.material_index == 0xffffU ||
                packet.state.color_channel_count == 0U) {
                continue;
            }
            ++material_packet_count;
            require(packet.state.color_channel_ambient_colors[0U] ==
                            expected_ambient &&
                        (packet.state.scene_loaded_light_mask & 0x03U) == 0x03U &&
                        packet.state.lights[0U].loaded &&
                        packet.state.lights[1U].loaded &&
                        packet.state.lights[0U].coordinate_space ==
                            expected_space(actor_light.mInfo0) &&
                        packet.state.lights[1U].coordinate_space ==
                            expected_space(actor_light.mInfo1),
                    "a material-bearing Mario packet lost its authored player ambient or light-space policy");
        }
        require(material_packet_count != 0U,
                "the final neutral frame contains no material-bearing Mario packet lighting proof");
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
        std::string animation_name{};
        std::string dominant_bck{};
        float animation_frame = 0.0F;
        s16 animation_frame_max = 0;
        float moving_track_weight = 0.0F;
        std::size_t animated_joint_count = 0U;
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

    void test_real_gateway_mario_stand_and_walk(bool focused_player_util) {
        const auto disc_path = require_real_disc();
        const auto scripted_input = ScopedEnvironmentVariable{
            "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", ""};

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
        auto resource_runtime = smgpc::resource::GameResourceRuntime{};
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window, resource_runtime);
        runtime.initialize_scenario_catalog(resource_runtime);
        runtime.initialize_particle_resources(resource_runtime);
        runtime.set_current_stage_name("HeavensDoorGalaxy");

        const auto scene_renderer_context =
            smgpc::render::ScopedAuroraRendererContext(renderer);
        auto game_data_session = smgpc::compat::GameDataSession{1U, resource_runtime, runtime.retain_scenario_catalog()};
        game_data_session.holder().followStoryEventByName(smgpc::resource::encode_cp932("ピーチ城浮上後").c_str());
        game_data_session.store_scene_start();
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
        const auto camera_result =
            smgpc::camera::resolve_stage_start_camera(runtime.dvd(), start);
        require(camera_result.status ==
                        smgpc::camera::StageStartCameraResolveStatus::Resolved &&
                    camera_result.camera.has_value() &&
                    camera_result.camera->camera_key == "s:004e" &&
                    camera_result.camera->camera_param.camera_type == "CAM_TYPE_XZ_PARA",
                "the proof must resolve exact Gateway StartInfo camera 78");
        auto camera = camera_result.camera->calculation.pose;
        const auto authored_camera = camera;
        const auto camera_owner = runtime.camera_system().set_authored_game_camera(
            *camera_result.camera);
        runtime.camera_system().set_game_camera_target_player(camera_owner, runtime.player_system());
        runtime.set_freecam_enabled(false);

        require(NameObjFactory::getCreator("Mario") == nullptr &&
                    NameObjFactory::getCreator("MarioActor") == nullptr &&
                    !smgpc::scene::nameobj::can_create_name_obj("Mario") &&
                    !smgpc::scene::nameobj::can_create_name_obj("MarioActor"),
                "the production Mario factory must remain absent in the development slice");

        auto created = MarioFixtureOwner(runtime);
        auto* actor = dynamic_cast<MarioActor*>(created.get());
        require(actor != nullptr,
                "the typed development creator must construct the real MarioActor");
        const auto entitlement_bridge =
            smgpc::runtime::PlayerActorBridge{
                .read_element_mode = +[](const LiveActor& value) -> s32 {
                    return static_cast<const MarioActor&>(value).mPlayerMode;
                },
                .read_base_matrix = +[](const LiveActor& value) {
                    return smgpc::compat::mario_camera_base_matrix(
                        static_cast<const MarioActor&>(value));
                },
                .read_up_vector = +[](const LiveActor& value, TVec3f* out) {
                    static_cast<const MarioActor&>(value).getUpVec(out);
                },
                .read_front_vector = +[](const LiveActor& value, TVec3f* out) {
                    static_cast<const MarioActor&>(value).getFrontVec(out);
                },
                .read_side_vector = +[](const LiveActor& value, TVec3f* out) {
                    static_cast<const MarioActor&>(value).getSideVec(out);
                },
            };
        runtime.player_system().attach_actor(*actor, entitlement_bridge);
        require(runtime.player_system().attached_actor() == actor && actor->_EEB,
                "attaching Mario must preserve the original constructor state until actor init");

        auto placement_lease =
            smgpc::scene::GatewayDemoScene::PlacementLease{};
        auto wait_frame_max = std::int16_t{};
        {
            auto frame = renderer.begin_frame();
            frame.frame_index = 100U;
            {
                const auto renderer_context =
                    smgpc::render::ScopedAuroraRendererContext(renderer);
                // Scene finalization allocates the original execution lists.
                // As in Showcase, initialization has a renderer frame but no
                // scene tick until those lists and Mario are ready.
                created.initialize(scene.player_start_iter());
                placement_lease = scene.finalize_placements(*actor);
                smgpc::tests::verify_original_mario_walk_parameters(*actor);
                verify_mario_camera_target_accessors(*actor);
                smgpc::tests::verify_original_mario_camera_target(*actor, runtime.dvd(), scene.demo_runtime());
                runtime.player_system().set_camera_target(
                    smgpc::compat::create_mario_camera_target(*actor));
                wait_frame_max =
                    MR::getBckFrameMax(actor, "Wait");
                runtime.game_layout().activate_game_scene_draw_3d();
            }
            renderer.end_frame();
        }

        auto *planet = scene.planet();
        require(planet != nullptr && planet->mModelManager != nullptr &&
                    planet->mModelManager->getJ3DModel() != nullptr,
                "the Mario proof must use the production-owned ordinary PlanetMap model");
        auto* planet_model = planet->mModelManager->getJ3DModelData();
        auto* planet_resources_owner = planet->mModelManager->getModelResourceHolder();
        require(planet_model != nullptr && planet_resources_owner != nullptr &&
                    planet_resources_owner->mModelResTable->getRes("HeavensDoorMysteriousPlanet") == planet_model,
                "the ordinary PlanetMap J3D model must borrow the actual authored ResourceHolder model");
        const auto planet_resources =
            smgpc::compat::actor_collision_parts_resources(planet);
        require(planet_model->getJointNum() != 0U && planet_resources.size() == 2U &&
                    planet_resources[0].resource_name ==
                        "HeavensDoorMysteriousPlanet" &&
                    planet_resources[0].kcl_size == 632430U &&
                    planet_resources[0].attributes_size == 31232U &&
                    planet_resources[0].kcl_source.ends_with(
                        cPlanetCollisionSource) &&
                    planet_resources[1].resource_name == "MoveLimit" &&
                    planet_resources[1].kcl_size == 25868U &&
                    planet_resources[1].attributes_size == 1152U,
                "the proof must retain the exact actor-owned main and MoveLimit KCL/PA resources");
        const auto ordinary_planet_count = std::ranges::count_if(
            scene.visuals(), [](const auto &visual) {
                return dynamic_cast<PlanetMap *>(visual.actor) != nullptr;
            });
        require(scene.visuals().size() == 7U && ordinary_planet_count == 4,
                "the Mario proof must retain Sky, Air, BrightSun, and four ordinary planets through the generic visual path");

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
        // Original tryCreateCollisionMoveLimit registers category 3. The
        // active map service counts only the four category-0 planet meshes.
        require(smgpc::scene::StageCollisionService::active() == &walk_collision &&
                    walk_collision.stats().mesh_count == 4U &&
                    walk_collision.stats().triangle_count == 14207U,
                "Mario and the executable must share the four authored map meshes and 14207 category-0 triangles");
        auto* collision_owner = smgpc::scene::current_collision_director_ownership();
        require(collision_owner != nullptr && collision_owner->category_service(3).stats().mesh_count == 2U,
                "both authored MoveLimit meshes must remain in the original separate collision category");
        auto collision_sources = std::vector<std::pair<std::string, std::size_t>>{};
        for (const auto& entry : scene.authored_placement_report().entries) {
            if (const auto* placed_actor = dynamic_cast<const LiveActor*>(entry.actor)) {
                for (const auto& resource : smgpc::compat::actor_collision_parts_resources(placed_actor)) {
                    collision_sources.emplace_back(resource.kcl_source, resource.kcl_size);
                }
            }
        }
        auto expected_collision_sources = std::vector<std::pair<std::string, std::size_t>>{
            {"/ObjectData/HeavensDoorMysteriousPlanet.arc:/heavensdoormysteriousplanet.kcl", 632430U},
            {"/ObjectData/HeavensDoorMysteriousPlanet.arc:/movelimit.kcl", 25868U},
            {"/ObjectData/HeavensDoorMiddlePlanet.arc:/heavensdoormiddleplanet.kcl", 67598U},
            {"/ObjectData/HeavensDoorMiddlePlanet.arc:/movelimit.kcl", 2446U},
            {"/ObjectData/HeavensDoorSmallPlanet.arc:/heavensdoorsmallplanet.kcl", 69376U},
            {"/ObjectData/HeavensDoorBlackHolePlanet.arc:/heavensdoorblackholeplanet.kcl", 365324U},
        };
        std::ranges::sort(collision_sources);
        std::ranges::sort(expected_collision_sources);
        require(collision_sources == expected_collision_sources,
                "all six exact authored planet/MoveLimit KCL resources must retain their original owners");
        const auto walk_triangle_count = walk_collision.stats().triangle_count;

        require(MR::getMarioHolder()->getMarioActor() == actor,
                "MarioActor init must register the real actor with MarioHolder");
        require(actor->mGravityRatio == 1.0F,
                "MarioActor init2 must establish the retail airborne gravity multiplier");
        smgpc::tests::verify_original_mario_state_lifecycle(*actor);
        require(actor->mActorLightCtrl != nullptr &&
                    actor->mActorLightCtrl->_4 == MR::LightType_Player &&
                    runtime.scene_lights().player_light_ctrl() ==
                        actor->mActorLightCtrl,
                "MarioActor PC init must install and register the exact player-light controller after scene connection");
        require(actor->mModelManager != nullptr &&
                    actor->mModelManager->getJ3DModel() != nullptr &&
                    actor->mModelManager->getJ3DModelData() != nullptr &&
                    actor->mModelManager->getJ3DModelData()->getJointNum() != 0U,
                "MarioActor init must load the real Mario model and joints");
        auto* initial_animation = actor->mModelManager->mXanimePlayer;
        require(initial_animation != nullptr && initial_animation->isRun(smgpc::resource::encode_cp932("ステージインA").c_str()) &&
                    std::string_view(initial_animation->getCurrentBckName()) == "StageStartGround" &&
                    initial_animation->mCurrentAnimation->_20[0] ==
                        actor->mModelManager->getResourceHolder()->mMotionResTable->getRes("StageStartGround") &&
                    MR::getBckFrameMax(actor, "StageStartGround") == 64 && wait_frame_max == 180,
                "the authored initial-animation branch must select original StageStartGround while retaining the 180-frame Wait resource");

        auto expected_gravity = point_gravity->mTranslation - actor->mPosition;
        expected_gravity.scale(1.0F / expected_gravity.length());
        // Original MarioActor::initAfterPlacement explicitly updates _240
        // and Mario's gravity. It does not enable LiveActor's gravity phase.
        require(MR::getPlayerGravity() == actor->mMario->getGravityVec() &&
                    MR::getPlayerGravity() == &actor->getGravityVec() &&
                    actor->getGravityVec().epsilonEquals(expected_gravity, 0.0001F) &&
                    actor->_240.epsilonEquals(expected_gravity, 0.0001F) &&
                    actor->mMario->mAirGravityVec.epsilonEquals(expected_gravity, 0.0001F),
                "original Mario post-placement must retain selected player gravity from the authored point-gravity owner");

#ifndef NDEBUG
        require(std::ranges::count_if(runtime.scheduler().snapshot(), [](const auto& entry) {
                    return entry.name == "MarioActor";
                }) == 1,
                "RuntimeContext must own exactly one Mario registration");
#endif

        auto screenshot_frame = std::optional<std::uint64_t>{};
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
                const auto prior_movement_timer = actor->_378;
                runtime.begin_frame(frame);
                const auto* target = dynamic_cast<const CameraTargetPlayer*>(
                    runtime.player_system().camera_target());
                require(target != nullptr && target->mPlayerMovementTimer == prior_movement_timer,
                        "the original camera target must update before Mario movement");
                runtime.player_system().advance_camera_target(frame_index);
                require(target->mPlayerMovementTimer == prior_movement_timer,
                        "repeated camera-phase requests must not advance the player target twice");
                proof.position_after = actor->mPosition;
                proof.last_move = actor->getLastMove();
                proof.grounded = MR::isOnGroundPlayer();
                auto& animation = *actor->mModelManager->mXanimePlayer;
                auto& core = *animation.mCore;
                require(animation.mCurrentAnimation != nullptr && core.mTrackCount != 0U,
                        "scheduled Mario must retain its actual current Xanime group and tracks");
                proof.animation_name = smgpc::resource::decode_cp932(animation.getCurrentAnimationName());
                proof.animated_joint_count = core.mJointCount;
                auto dominant_track = 0U;
                for (auto index = 0U; index < animation.mCurrentAnimation->mBckTableVariant; ++index) {
                    require(core.mTrackList[index]._0 == animation.mCurrentAnimation->_20[index] &&
                                std::isfinite(core.mTrackList[index].mWeight),
                            "XanimeCore tracks must borrow the actual current-group BCK resources with finite weights");
                    if (core.mTrackList[index].mWeight > core.mTrackList[dominant_track].mWeight) {
                        dominant_track = index;
                    }
                    if (proof.animation_name == "基本" && index < 3U) {
                        proof.moving_track_weight += core.mTrackList[index].mWeight;
                    }
                }
                const auto* motion = core.mTrackList[dominant_track]._0;
                require(motion != nullptr, "the dominant original animation track must have a BCK resource");
                const auto* motion_name = actor->mModelManager->getResourceHolder()->mMotionResTable->findResName(motion);
                require(motion_name != nullptr, "the dominant original animation resource must belong to Mario's ResourceHolder");
                proof.dominant_bck = motion_name;
                proof.animation_frame = motion->getFrame();
                proof.animation_frame_max = motion->getFrameMax();
                std::copy_n(&actor->getBaseMtx()[0][0], proof.base_matrix.size(), proof.base_matrix.begin());

                require(runtime.scene_camera_pose().has_value(),
                        "the authored camera must remain active while Mario moves");
                camera = *runtime.scene_camera_pose();
                runtime.draw_3d_normal(camera);
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
            if (screenshot_frame == frame_index) {
                if (const auto* path = std::getenv("SMGPC_TEST_SCREENSHOT_PATH");
                    path != nullptr && path[0] != '\0') {
                    renderer.request_screenshot_png(path);
                    require(std::filesystem::is_regular_file(path),
                            "the requested walk frame must produce a display-copy screenshot");
                }
            }

            const auto observed_move = proof.position_after - proof.position_before;
            require(observed_move.epsilonEquals(proof.last_move, 0.0001F),
                    "Mario must integrate exactly once in each RuntimeContext frame");
            return proof;
        };

        if (focused_player_util) {
            auto frame_index = std::uint64_t{101U};
            const auto wait_until_ready = [&] {
                auto ready_frames = 0U;
                auto submitted_shapes = std::size_t{};
                for (auto attempt = 0U; attempt < 240U; ++attempt) {
                    auto frame = renderer.begin_frame();
                    frame.frame_index = frame_index++;
                    {
                        const auto renderer_context = smgpc::render::ScopedAuroraRendererContext(renderer);
#ifndef NDEBUG
                        runtime.set_j3d_packet_trace_frame(frame.frame_index);
#endif
                        runtime.begin_frame(frame);
                        require(runtime.scene_camera_pose().has_value(),
                                "focused original player fixture requires its live camera owner");
                        runtime.draw_3d_normal(*runtime.scene_camera_pose());
                        auto* model = actor->mModelManager->getJ3DModel();
                        require(model != nullptr && model->mModelData->getJointNum() != 0U,
                                "focused player fixture must retain the real animated J3D model");
                        for (auto joint = 0U; joint < model->mModelData->getJointNum(); ++joint) {
                            const auto* matrix = &model->getAnmMtx(joint)[0][0];
                            require(std::all_of(matrix, matrix + 12U, [](float value) { return std::isfinite(value); }),
                                    "original Mario joints must remain finite during focused entry progression");
                        }
                        submitted_shapes = std::max(submitted_shapes, original_player_submitted_shapes(*actor));
                    }
                    renderer.end_frame();
                    if (!actor->mMario->isInputDisable() && !actor->mMario->mDrawStates._7 &&
                        actor->mModelManager->mXanimePlayer->isRun(smgpc::resource::encode_cp932("基本").c_str()) && MR::isOnGroundPlayer()) {
                        if (++ready_frames >= 3U) {
                            require(submitted_shapes != 0U,
                                    "focused original Mario must submit its own material/shape packets through the active draw buffer");
                            return;
                        }
                    } else {
                        ready_frames = 0U;
                    }
                }
                throw std::runtime_error("focused original player fixture did not finish entry and reach controllable ground within 240 ticks");
            };
            wait_until_ready();
            smgpc::tests::verify_original_player_util(*actor);
            runtime.camera_system().clear_stage_start_camera(camera_owner);
            placement_lease.reset();
            require(scene.state() == smgpc::scene::GatewayDemoSceneState::Retired &&
                        walk_collision.empty() && smgpc::scene::StageCollisionService::active() == nullptr,
                    "focused original player fixture must retire authored collision before its player");
            created.reset();
            actor = nullptr;
            require(runtime.player_system().attached_actor() == nullptr &&
                        MR::getMarioHolder()->getMarioActor() == nullptr &&
                        runtime.scene_lights().player_light_ctrl() == nullptr,
                    "focused player retirement must clear every borrowed player owner");
            std::cout << "[proof] focused actual PlayerUtil: original entry, finite joints, original draw buffers, utility checks and scene teardown passed\n";
            return;
        }

        auto wait_frame = FrameProof{};
        auto saw_ground = false;
        auto entry_complete = false;
        auto stable_entry_frames = 0U;
        auto last_settle_frame = std::uint64_t{100U};
        for (auto frame_index = std::uint64_t{101U}; frame_index < 341U; ++frame_index) {
            wait_frame = run_frame(frame_index);
            require(!wait_frame.debug_button_script_applied,
                    "neutral settle frames must use only real host input");
            const auto input_ready = !actor->mMario->isInputDisable() && !actor->mMario->mDrawStates._7;
            if (input_ready && saw_ground && !wait_frame.grounded) {
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
            saw_ground = saw_ground || (input_ready && wait_frame.grounded);
            last_settle_frame = frame_index;
            if (input_ready && wait_frame.grounded && wait_frame.animation_name == "基本" &&
                wait_frame.dominant_bck == "Wait" && wait_frame.last_move.length() < 0.01F) {
                if (++stable_entry_frames >= 3U) {
                    entry_complete = true;
                    break;
                }
            } else {
                stable_entry_frames = 0U;
            }
        }
        require(entry_complete && saw_ground,
                "original stage entry must finish, unlock input, and settle on authored KCL within 240 ticks;animation=" +
                    wait_frame.animation_name + ";track=" + wait_frame.dominant_bck);
        std::cout << "[entry] original entry completed at frame " << last_settle_frame << '\n';
        const auto* actor_light = actor->mActorLightCtrl->getActorLight();
        require(actor_light != nullptr,
                "Mario's final neutral frame must retain its exact ActorLightInfo");
#ifndef NDEBUG
        require_mario_packet_lighting(runtime, last_settle_frame, *actor_light);
#else
        throw std::runtime_error(
            "the Mario packet-boundary lighting proof requires a debug build");
#endif

        const auto* area_light = LightFunction::getAreaLightInfo(ZoneLightID{});
        const auto& post_frame_ambient = runtime.scene_lights().actor_ambient();
        const auto* post_frame_light0 = runtime.scene_lights().light(0U);
        const auto* post_frame_light1 = runtime.scene_lights().light(1U);
        require(area_light != nullptr && post_frame_ambient.has_value() &&
                    *post_frame_ambient == smgpc::render::GXColorValue{
                        area_light->mPlanetLight.mColor.r,
                        area_light->mPlanetLight.mColor.g,
                        area_light->mPlanetLight.mColor.b,
                        area_light->mPlanetLight.mColor.a} &&
                    post_frame_light0 != nullptr && post_frame_light1 != nullptr &&
                    post_frame_light0->color == smgpc::render::GXColorValue{
                        area_light->mPlanetLight.mInfo0.mColor.r,
                        area_light->mPlanetLight.mInfo0.mColor.g,
                        area_light->mPlanetLight.mInfo0.mColor.b,
                        area_light->mPlanetLight.mInfo0.mColor.a} &&
                    post_frame_light0->position == std::array<float, 3U>{
                        area_light->mPlanetLight.mInfo0.mPos.x,
                        area_light->mPlanetLight.mInfo0.mPos.y,
                        area_light->mPlanetLight.mInfo0.mPos.z} &&
                    post_frame_light0->coordinate_space ==
                        (area_light->mPlanetLight.mInfo0.mIsFollowCamera
                             ? smgpc::render::GXLightCoordinateSpace::View
                             : smgpc::render::GXLightCoordinateSpace::World) &&
                    post_frame_light1->color == smgpc::render::GXColorValue{
                        area_light->mPlanetLight.mInfo1.mColor.r,
                        area_light->mPlanetLight.mInfo1.mColor.g,
                        area_light->mPlanetLight.mInfo1.mColor.b,
                        area_light->mPlanetLight.mInfo1.mColor.a} &&
                    post_frame_light1->position == std::array<float, 3U>{
                        area_light->mPlanetLight.mInfo1.mPos.x,
                        area_light->mPlanetLight.mInfo1.mPos.y,
                        area_light->mPlanetLight.mInfo1.mPos.z} &&
                    post_frame_light1->coordinate_space ==
                        (area_light->mPlanetLight.mInfo1.mIsFollowCamera
                             ? smgpc::render::GXLightCoordinateSpace::View
                             : smgpc::render::GXLightCoordinateSpace::World),
                "the final after-indirect Planet buffer must leave authored Planet lighting in the scene service");
        const auto assert_follow_camera_round_trip = [&](const LightInfo& info) {
            if (!info.mIsFollowCamera) {
                return;
            }
            auto world_position = TVec3f{};
            LightFunction::calcLightWorldPos(&world_position, info);
            const auto view_position = smgpc::camera::transform_world_to_camera(
                camera, {world_position.x, world_position.y, world_position.z});
            const auto delta_x = view_position.x - info.mPos.x;
            const auto delta_y = view_position.y - info.mPos.y;
            const auto delta_z = view_position.z + info.mPos.z;
            auto authored_scale = std::max(
                {std::fabs(info.mPos.x), std::fabs(info.mPos.y),
                 std::fabs(info.mPos.z)});
            if (authored_scale == 0.0F) {
                authored_scale = 0.001F;
            }
            // This composes a float view inverse and forward transform at
            // Gateway-scale world coordinates. Bound accumulated rounding by
            // four representable steps at the authored vector's largest
            // component instead of using a route-specific absolute epsilon.
            const auto authored_ulp =
                std::nextafter(authored_scale,
                               std::numeric_limits<float>::infinity()) -
                authored_scale;
            const auto round_trip_tolerance = 4.0F * authored_ulp;
            if (std::fabs(delta_x) <= round_trip_tolerance &&
                std::fabs(delta_y) <= round_trip_tolerance &&
                std::fabs(delta_z) <= round_trip_tolerance) {
                return;
            }
            const auto pose_text = [](const std::optional<smgpc::camera::CameraPose>& pose) {
                if (!pose.has_value()) {
                    return std::string{"none"};
                }
                return std::to_string(pose->eye.x) + "," +
                       std::to_string(pose->eye.y) + "," +
                       std::to_string(pose->eye.z) + "->" +
                       std::to_string(pose->watch.x) + "," +
                       std::to_string(pose->watch.y) + "," +
                       std::to_string(pose->watch.z);
            };
            throw std::runtime_error(
                "follow-camera LightData round-trip mismatch;info=" +
                std::to_string(info.mPos.x) + "," + std::to_string(info.mPos.y) +
                "," + std::to_string(info.mPos.z) + ";world=" +
                std::to_string(world_position.x) + "," +
                std::to_string(world_position.y) + "," +
                std::to_string(world_position.z) + ";view=" +
                std::to_string(view_position.x) + "," +
                std::to_string(view_position.y) + "," +
                std::to_string(view_position.z) + ";delta=" +
                std::to_string(delta_x) + "," + std::to_string(delta_y) + "," +
                std::to_string(delta_z) + ";tolerance=" +
                std::to_string(round_trip_tolerance) + ";assert_camera=" +
                pose_text(std::optional<smgpc::camera::CameraPose>{camera}) +
                ";last_camera=" + pose_text(runtime.last_camera_pose()) +
                ";effective_camera=" +
                pose_text(runtime.camera_system().effective_camera_pose()) +
                ";scene_camera=" + pose_text(runtime.scene_camera_pose()));
        };
        assert_follow_camera_round_trip(actor_light->mInfo0);
        assert_follow_camera_round_trip(actor_light->mInfo1);
        require(wait_frame.dominant_bck == "Wait" && wait_frame.draw.packet_count != 0U &&
                    wait_frame.draw.source_triangles != 0U &&
                    wait_frame.draw.parsed_display_list_bytes != 0U &&
                    wait_frame.animated_joint_count != 0U &&
                    wait_frame.animation_frame_max == 180,
                "the stable stand frame must draw real Mario packets with Wait.bck");

        const auto& stand_triangle = actor->mBinder->mGroundInfo.mParentTriangle;
        const auto stand_surface = walk_collision.surface(stand_triangle.mIdx);
        require(stand_surface.has_value() &&
                    stand_surface->source_name.ends_with(cPlanetCollisionSource) &&
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
        set_host_key(window, SDLK_W, true);
        const auto walk_begin_frame = last_settle_frame + 1U;
        const auto walk_end_frame = walk_begin_frame + 45U;
        screenshot_frame = walk_end_frame - 1U;
        auto last_walk_frame = FrameProof{};
        for (auto frame_index = walk_begin_frame; frame_index < walk_end_frame; ++frame_index) {
            const auto walk_frame = run_frame(frame_index);
            const auto stick = aurora::wpad_service().sub_stick(WPAD_CHAN0);
            require(!walk_frame.debug_button_script_applied &&
                        (walk_frame.effective_hold_mask & WPAD_BUTTON_UP) == 0U &&
                        stick.x == 0.0F && stick.y == 1.0F,
                    "W must reach the Nunchuk stick without also pressing the camera D-pad");
            walked_every_frame_on_ground =
                walked_every_frame_on_ground && walk_frame.grounded;
            saw_run = saw_run || walk_frame.dominant_bck == "Run";
            if (walk_frame.dominant_bck == "Run") {
                if (previous_run_frame.has_value() &&
                    std::fabs(walk_frame.animation_frame - *previous_run_frame) > 0.001F) {
                    animation_frame_advanced = true;
                }
                previous_run_frame = walk_frame.animation_frame;
            }
            run_draw = walk_frame.draw;
            last_walk_frame = walk_frame;
        }
        set_host_key(window, SDLK_W, false);

        const auto walk_displacement = actor->mPosition - stand_position;
        const auto walk_distance = walk_displacement.length();
        const auto walk_speed = actor->mMario->mWalkSpeed;
        require(std::fabs(actor->mMario->mStickPos.x) < 0.01F &&
                    actor->mMario->mStickPos.y > 0.9F &&
                    actor->mMario->mStickPos.z > 0.9F && walk_speed > 0.0F &&
                    actor->mMario->mWorldPadDir.length() > 0.9F,
                "the host W path must retain retail forward-stick orientation, direction, and speed");
        require(walked_every_frame_on_ground,
                "stick-driven Mario must remain grounded across the real planet KCL seam");
        require(saw_run && last_walk_frame.dominant_bck == "Run" &&
                    run_draw.packet_count != 0U && run_draw.source_triangles != 0U &&
                    run_draw.parsed_display_list_bytes != 0U &&
                    last_walk_frame.animated_joint_count != 0U &&
                    last_walk_frame.animation_frame_max > 0 && animation_frame_advanced,
                "stick input must select, advance, and draw the real Run.bck model;saw_run=" +
                    std::to_string(saw_run) + ";current_bck=" +
                    last_walk_frame.dominant_bck +
                    ";packets=" + std::to_string(run_draw.packet_count) +
                    ";triangles=" + std::to_string(run_draw.source_triangles) +
                    ";display_list_bytes=" + std::to_string(run_draw.parsed_display_list_bytes) +
                    ";original_animated_joints=" + std::to_string(last_walk_frame.animated_joint_count) +
                    ";bck_frame=" + std::to_string(last_walk_frame.animation_frame) +
                    ";bck_frame_max=" + std::to_string(last_walk_frame.animation_frame_max) +
                    ";animation_advanced=" + std::to_string(animation_frame_advanced));
        require(walk_distance > 5.0F &&
                    std::fabs(dot(walk_displacement, stand_gravity)) <
                        walk_distance * 0.35F,
                "stick input must move Mario materially along the planet tangent");

        const auto camera_displacement = TVec3f{
            camera.watch.x - authored_camera.watch.x,
            camera.watch.y - authored_camera.watch.y,
            camera.watch.z - authored_camera.watch.z};
        require(camera_displacement.length() > 5.0F,
                "the authored camera must track the walking player instead of retaining the start pose");
        require_near(camera.fovy_degrees, authored_camera.fovy_degrees, 0.0001F,
                     "live tracking must preserve the authored field of view");
        const auto authored_eye_offset = TVec3f{
            authored_camera.eye.x - authored_camera.watch.x,
            authored_camera.eye.y - authored_camera.watch.y,
            authored_camera.eye.z - authored_camera.watch.z};
        const auto raw_camera = *runtime.camera_system().game_camera_pose();
        const auto tracked_eye_offset = TVec3f{
            raw_camera.eye.x - raw_camera.watch.x,
            raw_camera.eye.y - raw_camera.watch.y,
            raw_camera.eye.z - raw_camera.watch.z};
        require(tracked_eye_offset.epsilonEquals(authored_eye_offset, 0.01F),
                "the original Parallel manager must preserve its authored orbit and distance before view interpolation");
        const auto rendered_eye = TVec3f{camera.eye.x, camera.eye.y, camera.eye.z};
        const auto raw_eye = TVec3f{raw_camera.eye.x, raw_camera.eye.y, raw_camera.eye.z};
        require((rendered_eye - raw_eye).length() > 0.01F,
                "walking must render the original interpolator's persistent view rather than the raw camera manager pose");
        std::cout << "[camera] tracked watch moved " << camera_displacement.length()
                  << " units; raw authored orbit and FOV retained; rendered view offset="
                  << (rendered_eye - raw_eye).length() << "\n";

        const auto& walk_triangle = actor->mBinder->mGroundInfo.mParentTriangle;
        require(FloorCode{}.getCode(&walk_triangle) == CollisionFloorCode_NoSlip &&
                    MR::getSoundCodeIndex(walk_triangle.getAttributes()) ==
                        CollisionSoundCode_Lawn,
                "the walked-to real planet surface must remain NoSlip and Lawn");
        auto walk_matrix_proof = FrameProof{};
        walk_matrix_proof.position_after = actor->mPosition;
        std::copy_n(&actor->getBaseMtx()[0][0], walk_matrix_proof.base_matrix.size(),
                    walk_matrix_proof.base_matrix.begin());
        require_grounded_base_matrix(walk_matrix_proof, *walk_triangle.getNormal(0));

        auto release_frame = FrameProof{};
        auto release_end_frame = walk_end_frame;
        auto saw_release_inertia = false;
        auto reached_wait_at_rest = false;
        auto stable_rest_frames = std::size_t{};
        auto last_release_tangent_move = 0.0F;
        auto last_release_normal_move = 0.0F;
        for (; release_end_frame < walk_end_frame + 202U; ++release_end_frame) {
            release_frame = run_frame(release_end_frame);
            require(!release_frame.debug_button_script_applied,
                    "release frames must use only real host input");
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
                    std::to_string(actor->mMario->mWalkSpeed) + ";band=" +
                    std::to_string(actor->mMario->mTargetWalkSpeedIndex) + ";position=" +
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

            const auto speed = actor->mMario->mWalkSpeed;
            if (actor->mMario->mTargetWalkSpeedIndex == 0 && speed >= 0.2F) {
                saw_release_inertia = true;
                require(release_frame.animation_name == "基本" && release_frame.moving_track_weight > 0.0F,
                        "release inertia must retain the original locomotion group and its fading movement-track weights");
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
            if (actor->mMario->mTargetWalkSpeedIndex == 0 && speed == 0.0F &&
                actor->mVelocity.length() < 0.001F &&
                release_frame.dominant_bck == "Wait" &&
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
              actor->mMario->mWalkSpeed < walk_speed &&
              actor->mMario->mStickPos.z < 0.01F)) {
            throw std::runtime_error(
                "stick release did not reach stable Wait/rest;frame=" +
                std::to_string(release_end_frame) + ";speed=" +
                std::to_string(actor->mMario->mWalkSpeed) + ";stick=" +
                std::to_string(actor->mMario->mStickPos.z) + ";band=" +
                std::to_string(actor->mMario->mTargetWalkSpeedIndex) + ";bck=" +
                release_frame.dominant_bck + ";last_move=" +
                std::to_string(release_frame.last_move.length()) +
                ";tangent_move=" +
                std::to_string(last_release_tangent_move) +
                ";normal_move=" + std::to_string(last_release_normal_move) +
                ";stable_frames=" + std::to_string(stable_rest_frames) +
                ";saw_inertia=" + std::to_string(saw_release_inertia));
        }
        require(release_frame.draw.packet_count != 0U &&
                    release_frame.animated_joint_count != 0U &&
                    release_frame.animation_frame_max == 180,
                "the released actor must remain visible with the real Wait.bck");
        require_grounded_base_matrix(
            release_frame, *actor->mBinder->mGroundInfo.mParentTriangle.getNormal(0));

        constexpr auto cIdleProofFrames = std::uint64_t{60U};
        for (auto idle_index = std::uint64_t{}; idle_index < cIdleProofFrames;
             ++idle_index) {
            ++release_end_frame;
            release_frame = run_frame(release_end_frame);
            require(!release_frame.debug_button_script_applied &&
                        release_frame.grounded && actor->mMario->mWalkSpeed == 0.0F &&
                        actor->mMario->mTargetWalkSpeedIndex == 0 &&
                        actor->mMario->mStickPos.z < 0.01F &&
                        actor->mVelocity.length() < 0.001F &&
                        release_frame.dominant_bck == "Wait" &&
                        release_frame.draw.packet_count != 0U &&
                        release_frame.animated_joint_count != 0U,
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

        set_host_key(window, SDLK_RIGHT, true);
        for (auto index = 0U; index < 12U; ++index) {
            const auto camera_frame = run_frame(++release_end_frame);
            const auto stick = aurora::wpad_service().sub_stick(WPAD_CHAN0);
            require((camera_frame.effective_hold_mask & WPAD_BUTTON_RIGHT) != 0U &&
                        stick.x == 0.0F && stick.y == 0.0F &&
                        actor->mMario->mWalkSpeed == 0.0F &&
                        camera_frame.dominant_bck == "Wait",
                    "the host right arrow must reach the camera D-pad without moving Mario");
        }
        set_host_key(window, SDLK_RIGHT, false);
        (void)run_frame(++release_end_frame);
        set_host_key(window, SDLK_C, true);
        (void)run_frame(++release_end_frame);
        set_host_key(window, SDLK_C, false);
        for (auto index = 0U; index < 12U; ++index) {
            (void)run_frame(++release_end_frame);
        }

        // The complete GameSequenceProgress::startScene owns this story
        // entitlement in retail. This focused fixture selects the locked
        // precondition explicitly through the original utility.
        MR::setPlayerSwingPermission(false);
        const auto entitlement_magic = actor->mMario->mMagic;
        const auto entitlement_action = actor->_1E1;
        const auto entitlement_cooldown = actor->_946;
        require(entitlement_magic == nullptr && !entitlement_action &&
                    entitlement_cooldown == 0U,
                "the walk slice must begin without fabricated spin Magic/action state");

        require(MR::getWPad(WPAD_CHAN0)->mCorePadAccel->_628 > 20,
                "the walking frames must warm the original twenty-frame acceleration history");
        set_host_swing_key(window, true);
        const auto locked_swing_frame = run_frame(++release_end_frame);
        if (!(MR::isCorePadSwing(WPAD_CHAN0) &&
              MR::isCorePadSwingTrigger(WPAD_CHAN0) &&
              actor->_F00 && !actor->_EEB && !actor->isRequestRush() &&
              locked_swing_frame.dominant_bck == "Wait")) {
            std::cerr << "[swing diagnostic] held=" << MR::isCorePadSwing(WPAD_CHAN0)
                      << " triggered=" << MR::isCorePadSwingTrigger(WPAD_CHAN0)
                      << " actor_edge=" << static_cast<int>(actor->_F00)
                      << " permitted=" << static_cast<int>(actor->_EEB)
                      << " rush=" << actor->isRequestRush()
                      << " bck=" << locked_swing_frame.dominant_bck << '\n';
        }
        require(MR::isCorePadSwing(WPAD_CHAN0) &&
                    MR::isCorePadSwingTrigger(WPAD_CHAN0) &&
                    actor->_F00 && !actor->_EEB && !actor->isRequestRush() &&
                    locked_swing_frame.dominant_bck == "Wait",
                "host key acceleration must reach the original detector but be denied before entitlement");

        const auto locked_held_frame = run_frame(++release_end_frame);
        require(MR::isCorePadSwing(WPAD_CHAN0) &&
                    !MR::isCorePadSwingTrigger(WPAD_CHAN0) &&
                    !actor->_F00 && !actor->isRequestRush() &&
                    locked_held_frame.dominant_bck == "Wait",
                "the next physical sample must preserve the original retained swing without another trigger");

        set_host_swing_key(window, false);
        // Releasing the key does not erase a physical gesture or the original
        // delayed acceleration comparison. Let the complete history settle.
        for (int sample = 0; sample < 64; ++sample) {
            (void)run_frame(++release_end_frame);
            require(!actor->isRequestRush(), "every physical gesture response remains denied while entitlement is locked");
        }
        const auto settled_acceleration = aurora::wpad_service().core_acceleration(WPAD_CHAN0);
        require(settled_acceleration.x == 0 && settled_acceleration.y == 0 && settled_acceleration.z == 1 &&
                    !MR::isCorePadSwing(WPAD_CHAN0) && !MR::isCorePadSwingTrigger(WPAD_CHAN0) &&
                    !actor->_F20 && !actor->_F00 && !actor->isRequestRush(),
                "completed physical motion and settled original history rearm Mario's retail edge detector");

        MR::setPlayerSwingPermission(true);
        require(actor->_EEB &&
                    actor->mMario->mMagic == entitlement_magic &&
                    actor->_1E1 == entitlement_action &&
                    actor->_946 == entitlement_cooldown,
                "swing entitlement must mirror only MarioActor::_EEB without fabricating action state");

        set_host_swing_key(window, true);
        const auto unlocked_swing_frame = run_frame(++release_end_frame);
        require(MR::isCorePadSwingTrigger(WPAD_CHAN0) &&
                    actor->_F00 && actor->isRequestRush() &&
                    actor->mMario->mMagic == entitlement_magic &&
                    actor->_1E1 == entitlement_action &&
                    actor->_946 == entitlement_cooldown &&
                    unlocked_swing_frame.dominant_bck == "Wait",
                "a fresh physical gesture must request Rush after entitlement without fabricating spin action state");

        const auto unlocked_held_frame = run_frame(++release_end_frame);
        require(MR::isCorePadSwing(WPAD_CHAN0) &&
                    !MR::isCorePadSwingTrigger(WPAD_CHAN0) &&
                    !actor->_F00 && !actor->isRequestRush() &&
                    actor->mMario->mMagic == entitlement_magic &&
                    actor->_1E1 == entitlement_action &&
                    actor->_946 == entitlement_cooldown &&
                    unlocked_held_frame.dominant_bck == "Wait",
                "the next entitled acceleration sample must remain debounced without fabricating spin action state");

        set_host_swing_key(window, false);
        for (int sample = 0; sample < 64; ++sample) (void)run_frame(++release_end_frame);
        require(!MR::isCorePadSwing(WPAD_CHAN0) && !MR::isCorePadSwingTrigger(WPAD_CHAN0) &&
                    !actor->_F20 && !actor->_F00 && !actor->isRequestRush(),
                "the completed pulse stops requesting Rush after the original history settles");

        runtime.player_system().detach_actor(actor);
        require(!runtime.player_system().camera_target_state().has_value(),
                "player detachment must retire its original camera target");
        require(runtime.player_system().attached_actor() == nullptr &&
                    actor->_EEB,
                "Gateway owner detach must retire host references without changing original entitlement state");
        runtime.unregister_live_actor_model(*actor);
        MR::getMarioHolder()->setMarioActor(nullptr);
        created.reset();
        actor = nullptr;
        require(MR::getMarioHolder()->getMarioActor() == nullptr &&
                    runtime.scene_lights().player_light_ctrl() == nullptr,
                "MarioHolder and the non-owning player-light binding must clear before the actor owner is destroyed");
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
                created.create();
                actor = dynamic_cast<MarioActor*>(created.get());
                require(actor != nullptr,
                        "Mario must be constructible again while the Gateway placement lease remains active");
                runtime.player_system().attach_actor(*actor, entitlement_bridge);
                require(actor->_EEB,
                        "Mario replacement must preserve its own original constructor entitlement");
                // Authored SwitchArea movement has the retail non-null player
                // contract, so publish the replacement before advancing the
                // next scene frame, then initialize it at that frame boundary.
                runtime.begin_frame(frame);
                created.initialize(scene.player_start_iter());
                // Gateway finalization owns the one retail player postpass at
                // scene start. A later same-scene replacement crosses its own
                // ordinary actor postpass while the placement lease is active.
                actor->initAfterPlacement();
                runtime.player_system().set_camera_target(
                    smgpc::compat::create_mario_camera_target(*actor));
            }
            renderer.end_frame();
        }
        require(MR::getMarioHolder()->getMarioActor() == actor &&
                    actor->mModelManager != nullptr &&
                    actor->mModelManager->getJ3DModel() != nullptr &&
                    actor->mModelManager->getJ3DModelData() != nullptr &&
                    actor->mModelManager->getJ3DModelData()->getJointNum() != 0U &&
                    runtime.scene_lights().player_light_ctrl() ==
                        actor->mActorLightCtrl,
                "recreated Mario must own the real model and replace the holder and player-light bindings");
        const auto recreated_frame = run_frame(release_end_frame + 2U);
        require(recreated_frame.draw.packet_count != 0U,
                "recreated Mario must update and draw through RuntimeContext");

        smgpc::tests::verify_original_player_util(*actor);

        runtime.camera_system().clear_stage_start_camera(camera_owner);
        placement_lease.reset();
        require(scene.state() == smgpc::scene::GatewayDemoSceneState::Retired &&
                    walk_collision.empty() &&
                    smgpc::scene::StageCollisionService::active() == nullptr,
                "Gateway placement collision must retire before the live replacement Mario");
        auto retired_scene_rejected_retry = false;
        try {
            auto invalid_retry = scene.finalize_placements(*actor);
            (void)invalid_retry;
        } catch (const std::logic_error&) {
            retired_scene_rejected_retry = true;
        }
        require(retired_scene_rejected_retry,
                "a retired Gateway scene must reject placement finalization retry");

        runtime.player_system().detach_actor(actor);
        require(runtime.player_system().attached_actor() == nullptr &&
                    actor->_EEB,
                "recreated Mario detachment must preserve its original constructor entitlement");
        runtime.unregister_live_actor_model(*actor);
        MR::getMarioHolder()->setMarioActor(nullptr);
        created.reset();
        actor = nullptr;
        require(runtime.scene_lights().player_light_ctrl() == nullptr,
                "recreated Mario destruction must not leave a stale player-light controller");
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
                  << ";kcl_triangles=" << walk_triangle_count
                  << ";stand_prism=" << stand_surface->prism_index
                  << ";floor=NoSlip;sound=Lawn"
                  << ";walk_distance=" << walk_distance
                  << ";bck=Wait->Run->Wait"
                  << ";release_frame=" << release_end_frame
                  << ";run_packets=" << run_draw.packet_count
                  << ";recreated=1;lease_retired_before_mario=1;retry=rejected\n";
    }
}  // namespace

int main(int argc, char** argv) {
    try {
        const auto focused_player_util = argc == 2 && std::string_view(argv[1]) == "--player-util";
        require(argc == 1 || focused_player_util, "usage: smg-pc-mario-gateway-walk-tests [--player-util]");
        // The process OS allocator initializes once. Repeat this focused
        // lifecycle in separate processes, each with a fresh original scene.
        test_real_gateway_mario_stand_and_walk(focused_player_util);
        std::cout << (focused_player_util ? "[ok] real original PlayerUtil ownership proof\n"
                                        : "[ok] real Gateway Mario stand/walk/release proof\n");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] real Gateway Mario stand/walk proof: " << error.what()
                  << '\n';
        return 1;
    }
}
