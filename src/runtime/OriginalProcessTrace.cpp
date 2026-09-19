#include "runtime/OriginalProcessTrace.hpp"

#ifndef NDEBUG
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Gravity/GlobalGravityObj.hpp"
#include "Game/Gravity/PlanetGravity.hpp"
#include "Game/Gravity/GravityInfo.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadHolder.hpp"
#include "Game/System/WPadStick.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "layout/LayoutHost.hpp"
#include "resource/TextEncoding.hpp"
#include "scene/SceneNameObjRegistry.hpp"
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

        Json vector(const TVec3f& v) { return {v.x, v.y, v.z}; }

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
                        {"nerve", spine(value.mSpine)}};
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
            if (const auto* player = dynamic_cast<const MarioActor*>(&value); player && player->mMario) {
                const auto& mario = *player->mMario;
                result["player"] = {{"status", mario.getCurrentStatus()},
                    {"position", vector(mario.mPosition)}, {"velocity", vector(mario.mVelocity)},
                    {"velocity_after", vector(mario.mVelocityAfter)}, {"stick_position", vector(mario.mStickPos)},
                    {"world_pad_direction", vector(mario.mWorldPadDir)}, {"front", vector(mario.mFrontVec)},
                    {"up", vector(mario.mHeadVec)}, {"air_gravity", vector(mario.mAirGravityVec)},
                    {"movement_up", vector(mario._398)}, {"camera_position", vector(player->mCamPos)},
                    {"camera_x", vector(player->mCamDirX)}, {"camera_y", vector(player->mCamDirY)},
                    {"camera_z", vector(player->mCamDirZ)},
                    {"movement_low_word", mario.mMovementStates_LOW_WORD},
                    {"movement_high_word", mario.mMovementStates_HIGH_WORD}, {"draw_word", mario.mDrawStates_WORD}};
                result["player"]["ground_triangle"] = triangle(mario.mGroundPolygon);
                result["player"]["ground_position"] = vector(mario.mGroundPos);
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
        std::string layout_path;
        std::uint64_t layout_frame = 0;

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
        const auto layout_path = environment("SMGPC_DEBUG_LAYOUT_DUMP_PATH");
        if (path.empty() && layout_path.empty()) return;
        _state = std::make_unique<State>();
        _state->interval = number("SMGPC_DEBUG_ACTOR_TRACE_INTERVAL", 60, false);
        _state->layout_path = layout_path;
        _state->layout_frame = number("SMGPC_DEBUG_LAYOUT_DUMP_FRAME", 0, true);
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
        if (!_state->layout_path.empty() && frame_index == _state->layout_frame)
            layout::debug_dump_layout_text(_state->layout_path.c_str());
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
                const auto objects = scene::SceneNameObjRegistry::snapshot_holder(*controller->mObjHolder);
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
