#include "runtime/OriginalProcessTrace.hpp"

#ifndef NDEBUG
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/Enemy/WalkerStateRunaway.hpp"
#include "Game/Gravity/GlobalGravityObj.hpp"
#include "Game/Gravity/PlanetGravity.hpp"
#include "Game/Gravity/GravityInfo.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioState.hpp"
#include "Game/Player/MarioRecovery.hpp"
#include "Game/Player/MarioWarp.hpp"
#include "Game/Player/J3DModelX.hpp"
#include "Game/NPC/DemoRabbit.hpp"
#include "Game/NPC/RunawayRabbit.hpp"
#include "JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadHolder.hpp"
#include "Game/System/WPadStick.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "resource/TextEncoding.hpp"
#include "Game/NameObj/NameObjHolder.hpp"
#include <aurora/allocation.hpp>
#include <revolution/kpad.h>
#include <nlohmann/json.hpp>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <vector>

namespace smgpc::runtime {
    namespace {
        using Json = nlohmann::json;
        using GravityOwners = std::map<const PlanetGravity*, std::uint64_t>;

        std::string environment(const char* name) {
            const auto* value = std::getenv(name);
            return value ? value : "";
        }

        std::uint64_t number(const char* name, std::uint64_t fallback, bool allow_zero) {
            const auto text = environment(name);
            if (text.empty()) return fallback;
            std::uint64_t value = 0;
            const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
            if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || (!allow_zero && value == 0))
                throw std::invalid_argument(std::string(name) + " requires an unsigned integer in range");
            return value;
        }

        template <class Vector>
        Json vector(const Vector& v) { return {v.x, v.y, v.z}; }

        Json quaternion(const TQuat4f& q) { return {q.x, q.y, q.z, q.w}; }

        Json matrix(const float (*value)[4]) {
            if (!value) return nullptr;
            return {{value[0][0], value[0][1], value[0][2], value[0][3]},
                    {value[1][0], value[1][1], value[1][2], value[1][3]},
                    {value[2][0], value[2][1], value[2][2], value[2][3]}};
        }

        Json model(const J3DModel* value) {
            if (!value) return nullptr;
            Json result{{"base_matrix", matrix(value->mBaseTransformMtx)},
                        {"base_scale", vector(value->mBaseScale)}, {"flags", value->mFlags},
                        {"internal_view", matrix(value->mInternalView)}};
            const auto* buffer = value->mMtxBuffer;
            if (value->mModelData && value->mModelData->getJointNum() != 0 && buffer && buffer->mpAnmMtx) {
                result["joint0_animation_matrix"] = matrix(buffer->mpAnmMtx[0]);
            } else {
                result["joint0_animation_matrix"] = nullptr;
            }
            return result;
        }

        Json spine(const Spine* state) {
            if (!state) return nullptr;
            const auto* nerve = state->getCurrentNerve();
            return {{"type", nerve ? typeid(*nerve).name() : ""}, {"step", state->mStep}};
        }

        Json triangle(const Triangle* value) {
            if (!value || !value->mParts || value->mIdx == 0xFFFFFFFFU) return nullptr;
            return {{"prism_index", value->mIdx},
                    {"host_id", value->mSensor && value->mSensor->mHost
                        ? compat::name_obj_runtime_generation(value->mSensor->mHost) : 0},
                    {"normal", vector(value->mNormals[0])},
                    {"vertices", {vector(value->mPos[0]), vector(value->mPos[1]), vector(value->mPos[2])}}};
        }

        Json contact(const HitInfo& value, float distance) {
            if (distance < 0.0f) return nullptr;
            return {{"distance", distance}, {"feature", value._88},
                    {"hit_position", vector(value.mHitPos)},
                    {"unknown_70", vector(value._70)}, {"moving_reaction", vector(value._7C)},
                    {"triangle", triangle(&value.mParentTriangle)}};
        }

        Json actor(const LiveActor& value, const GravityOwners& gravity_owners) {
            Json result{{"id", compat::name_obj_runtime_generation(&value)},
                        {"type", typeid(value).name()}, {"position", vector(value.mPosition)},
                        {"rotation", vector(value.mRotation)}, {"velocity", vector(value.mVelocity)},
                        {"gravity", vector(value.mGravity)}, {"dead", value.mFlag.mIsDead},
                        {"hidden", value.mFlag.mIsHiddenModel}, {"clipped", value.mFlag.mIsClipped},
                        {"invalid_clipping", value.mFlag.mIsInvalidClipping},
                        {"no_calc_anim", value.mFlag.mIsNoCalcAnim},
                        {"no_calc_view", value.mFlag.mIsNoCalcView},
                        {"nerve", spine(value.mSpine)}};
            // These are borrowed stored matrices; tracing must not invoke
            // animation, model calculation, or matrix-generation functions.
            result["model"] = model(value.mModelManager ? value.mModelManager->getJ3DModel() : nullptr);
            if (const auto* binder = value.mBinder) {
                result["binder"] = {{"radius", binder->mRadius}, {"offset_y", binder->mOffsetY},
                    {"plane_count", binder->mPlaneNum}, {"fix_reaction", vector(binder->mFixReactionVector)},
                    {"ground", contact(binder->mGroundInfo, binder->_C8)},
                    {"wall", contact(binder->mWallInfo, binder->_158)},
                    {"roof", contact(binder->mRoofInfo, binder->_1E8)}};
            }
            if (const auto* owner = dynamic_cast<const GlobalGravityObj*>(&value);
                owner && owner->mGravityCreator && owner->mGravityCreator->getGravity()) {
                const auto& field = *owner->mGravityCreator->getGravity();
                result["gravity_field"] = {{"type", typeid(field).name()},
                    {"priority", field.mPriority}, {"gravity_id", field.mGravityId},
                    {"range", field.mRange}, {"distant", field.mDistant},
                    {"gravity_type", field.mGravityType}, {"power", field.mGravityPower},
                    {"activated", field.mActivated}, {"inverse", field.mIsInverse},
                    {"valid_follower", field.mValidFollower}, {"registered", field.mIsRegistered},
                    {"appeared", field.mAppeared}};
            }
            if (value.mName) {
                try { result["name"] = resource::decode_cp932(value.mName); }
                catch (const std::exception& error) { result["name_decode_error"] = error.what(); }
            }
            if (const auto* npc = dynamic_cast<const NPCActor*>(&value)) {
                result["npc_pose"] = {{"quaternion_a0", quaternion(npc->_A0)},
                                      {"quaternion_b0", quaternion(npc->_B0)},
                                      {"cached_euler_cc", vector(npc->_CC)}};
            }
            if (const auto* rabbit = dynamic_cast<const DemoRabbit*>(&value)) {
                result["demo_rabbit"] = {{"front", vector(rabbit->mFrontVec)},
                    {"actor_base_matrix", matrix(rabbit->getBaseMtx())},
                    {"no_ground_timer", rabbit->mNoGroundTimer}};
            }
            if (const auto* rabbit = dynamic_cast<const RunawayRabbit*>(&value)) {
                result["runaway_rabbit"] = {{"pose_quaternion_a4", quaternion(rabbit->_A4)},
                    {"front_b4", vector(rabbit->_B4)}, {"player_bind_quaternion_c0", quaternion(rabbit->_C0)},
                    {"actor_base_matrix", matrix(rabbit->getBaseMtx())},
                    {"player_bind_translation_d0", vector(rabbit->_D0)}};
                if (const auto* walker = rabbit->mStateRunaway) {
                    result["runaway_rabbit"]["walker"] = {{"nerve", spine(walker->mSpine)},
                        {"dead", walker->mIsDead}, {"direction", walker->mDirection ? vector(*walker->mDirection) : Json(nullptr)},
                        {"runaway_speed", walker->mRunawaySpeed}, {"counter_18", walker->_18}};
                }
            }
            if (const auto* player = dynamic_cast<const MarioActor*>(&value); player && player->mMario) {
                const auto& mario = *player->mMario;
                result["player"] = {{"status", mario.getCurrentStatus()},
                    {"state_type", mario._97C ? typeid(*mario._97C).name() : ""},
                    {"position", vector(mario.mPosition)}, {"velocity", vector(mario.mVelocity)},
                    {"velocity_after", vector(mario.mVelocityAfter)}, {"stick_position", vector(mario.mStickPos)},
                    {"world_pad_direction", vector(mario.mWorldPadDir)}, {"front", vector(mario.mFrontVec)},
                    {"up", vector(mario.mHeadVec)}, {"air_gravity", vector(mario.mAirGravityVec)},
                    {"movement_up", vector(mario._398)}, {"camera_position", vector(player->mCamPos)},
                    {"camera_x", vector(player->mCamDirX)}, {"camera_y", vector(player->mCamDirY)},
                    {"camera_z", vector(player->mCamDirZ)},
                    {"camera_up_actor", vector(player->mUpVec)},
                    {"camera_up_target_300", vector(player->_300)}, {"camera_up_timer_330", player->_330},
                    {"side", vector(mario.mSideVec)}, {"direction_up_1fc", vector(mario._1FC)},
                    {"posture_matrix_c4", matrix(mario._C4.mMtx)},
                    {"posture_matrix_f4", matrix(mario._F4.mMtx)},
                    {"yaw_angle_offset", mario.mYAngleOffset},
                    {"active_model_index", player->mCurrModel}, {"player_mode", player->mPlayerMode},
                    {"model_update_requested_1c0", player->_1C0}, {"model_update_skipped_1c1", player->_1C1},
                    {"bound_actor_934", player->_934}, {"fixed_matrix_ea4", player->_EA4},
                    {"fixed_matrix_ea5", player->_EA5}, {"fixed_matrix_ea6", player->_EA6},
                    {"movement_low_word", mario.mMovementStates_LOW_WORD},
                    {"movement_high_word", mario.mMovementStates_HIGH_WORD}, {"draw_word", mario.mDrawStates_WORD}};
                result["player"]["active_model"] = player->mCurrModel < 6 ? model(player->getJ3DModel()) : Json(nullptr);
                result["player"]["model0"] = model(player->mModels[0]);
                result["player"]["actor_base_matrix"] = matrix(player->getBaseMtx());
                result["player"]["ground_triangle"] = triangle(mario.mGroundPolygon);
                result["player"]["ground_position"] = vector(mario.mGroundPos);
                // getLastSafetyTrans can recalculate triangle normals. Capture
                // both saved candidates directly instead of invoking it.
                result["player"]["safety"] = {{"latest_position_7d4", vector(mario._7D4)},
                    {"latest_triangle_7e0", triangle(mario._7E0)},
                    {"latest_saved_matrix_7e4", matrix(mario._7E4.mMtx)},
                    {"previous_position_814", vector(mario._814)},
                    {"previous_triangle_820", triangle(mario._820)},
                    {"previous_saved_matrix_824", matrix(mario._824.mMtx)},
                    {"flags_1c", mario._1C_WORD}, {"not_safety_timer_96a", mario._96A},
                    {"recovery_jump_path_12", mario.mRecovery ? Json(mario.mRecovery->_12) : Json(nullptr)}};
                if (const auto* warp = dynamic_cast<const MarioWarp*>(mario._97C)) {
                    result["player"]["warp"] = {{"destination_14", vector(warp->_14)},
                        {"start_position_20", vector(warp->_20)}, {"direction_38", vector(warp->_38)},
                        {"mode_45", warp->_45}, {"timer_52", warp->_52}, {"timer_54", warp->_54}};
                }
                if (const auto* recovery = dynamic_cast<const MarioRecovery*>(mario._97C)) {
                    result["player"]["recovery"] = {{"saved_position_34", vector(recovery->_34)},
                        {"timer_14", recovery->_14}, {"timer_16", recovery->_16},
                        {"duration_18", recovery->_18}, {"phase_1a", recovery->_1A}};
                }
                if (const auto* info = player->mGravityInfo) {
                    const auto owner = gravity_owners.find(info->mGravityInstance);
                    result["player"]["gravity_info"] = {{"vector", vector(info->mGravityVector)},
                        {"largest_priority", info->mLargestPriority},
                        {"field_actor_id", owner != gravity_owners.end() ? Json(owner->second) : Json(nullptr)},
                        {"field_type", info->mGravityInstance ? typeid(*info->mGravityInstance).name() : ""},
                        {"field_gravity_id", info->mGravityInstance ? Json(info->mGravityInstance->mGravityId) : Json(nullptr)}};
                }
            }
            return result;
        }
    }

    struct OriginalProcessTrace::State {
        FILE* output = nullptr;
        std::uint64_t interval = 60;
        std::vector<std::string> types;

        ~State() { if (output) std::fclose(output); }

        bool includes(std::string_view type) const {
            if (types.empty()) return true;
            for (const auto& filter : types) if (type.find(filter) != std::string_view::npos) return true;
            return false;
        }
    };

    OriginalProcessTrace::OriginalProcessTrace() {
        const aurora::allocation::HostAllocationScope host;
        const auto path = environment("SMGPC_DEBUG_ACTOR_TRACE_PATH");
        if (path.empty()) return;
        _state = std::make_unique<State>();
        _state->interval = number("SMGPC_DEBUG_ACTOR_TRACE_INTERVAL", 60, false);
        auto filters = environment("SMGPC_DEBUG_ACTOR_TRACE_TYPES");
        std::string_view remaining(filters);
        while (!remaining.empty()) {
            const auto end = remaining.find(',');
            const auto part = remaining.substr(0, end);
            if (part.empty()) throw std::invalid_argument("Actor trace type filters cannot contain empty entries");
            _state->types.emplace_back(part);
            if (end == std::string_view::npos) break;
            remaining.remove_prefix(end + 1);
        }
        if (!path.empty()) {
            _state->output = std::fopen(path.c_str(), "wbx");
            if (!_state->output) throw std::runtime_error("Cannot create a new original actor trace: " + path);
        }
    }

    OriginalProcessTrace::~OriginalProcessTrace() = default;

    void OriginalProcessTrace::capture(const GameSystem& system, std::uint64_t frame_index) {
        if (!_state) return;
        const aurora::allocation::HostAllocationScope host;
        if (!_state->output || frame_index % _state->interval) return;
        Json record{{"frame_index", frame_index}, {"system_nerve", spine(system.mSpine)},
                    {"actors", Json::array()}};
        if (const auto* controller = system.mSceneController) {
            record["scene"] = controller->mCurrSceneControlInfo.mScene;
            record["stage"] = controller->mCurrSceneControlInfo.mStage;
            record["scenario"] = controller->mCurrSceneControlInfo.mScenarioNo;
            record["scene_initialization_state"] = controller->mSceneInitializeState;
            record["scene_nerve"] = spine(controller->mSpine);
            // The controller's executable scene is the initialized current
            // owner. Never inspect actors of an asynchronously loading scene.
            if (controller->mObjHolder && controller->mScene &&
                controller->getCurrentSceneForExecute() == controller->mScene) {
                const auto objects = controller->mObjHolder->snapshotNativeObjects();
                GravityOwners gravity_owners;
                for (const auto* object : objects) {
                    if (const auto* owner = dynamic_cast<const GlobalGravityObj*>(object);
                        owner && owner->mGravityCreator && owner->mGravityCreator->getGravity())
                        gravity_owners.emplace(owner->mGravityCreator->getGravity(), compat::name_obj_runtime_generation(owner));
                }
                for (const auto* object : objects) {
                    if (const auto* live = dynamic_cast<const LiveActor*>(object); live && _state->includes(typeid(*live).name()))
                        record["actors"].push_back(actor(*live, gravity_owners));
                }
            }
        }
        if (system.mObjHolder && system.mObjHolder->mWPadHolder) {
            const auto* pad = system.mObjHolder->mWPadHolder->mPad[0];
            if (pad && pad->getValidStatusCount() > 0) {
                const auto& status = *pad->getKPadStatus(0);
                record["controller"] = {{"hold", status.hold}, {"trigger", status.trig}, {"release", status.release}};
                if (pad->mStick) record["controller"]["stick"] = {pad->mStick->mStick.x, pad->mStick->mStick.y};
            }
        }
        const auto line = record.dump() + '\n';
        if (std::fwrite(line.data(), 1, line.size(), _state->output) != line.size() || std::fflush(_state->output))
            throw std::runtime_error("Cannot write original actor trace");
    }
}
#endif
