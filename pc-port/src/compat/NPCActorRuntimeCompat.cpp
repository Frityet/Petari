#include "Game/Enemy/AnimScaleController.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/PartsModel.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/NPC/NPCActorItem.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/JointController.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/NPCUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/TalkUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/ActorShadowCsvCompat.hpp"
#include "render/J3dMaterialRuntime.hpp"

#include "runtime/RuntimeContext.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string>

namespace {
    [[noreturn]] void throwNPCBehaviorUnavailable() {
        throw std::logic_error("NPC behavior utilities are unavailable without their real NPCUtil implementation.");
    }

    TVec3f normalized(const TVec3f& value, const char* context) {
        auto result = value;
        const auto length = result.length();
        if (!(length > 1.0e-6F) || !std::isfinite(length)) {
            throw std::logic_error(std::string(context) + " requires a finite, non-degenerate vector.");
        }
        result.scale(1.0F / length);
        return result;
    }

    TQuat4f multiplyQuat(const TQuat4f& lhs, const TQuat4f& rhs) {
        return TQuat4f{
            lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x,
            lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w,
            lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z,
        };
    }

    TQuat4f axisAngleQuat(const TVec3f& axis, float radians) {
        const auto unit = normalized(axis, "Quaternion rotation");
        const auto half = radians * 0.5F;
        const auto sine = std::sin(half);
        return TQuat4f{unit.x * sine, unit.y * sine, unit.z * sine, std::cos(half)};
    }

    TVec3f orthogonalAxis(const TVec3f& vector) {
        auto candidate = std::abs(vector.x) < std::abs(vector.y) ? TVec3f{1.0F, 0.0F, 0.0F}
                                                                 : TVec3f{0.0F, 1.0F, 0.0F};
        auto axis = vector.cross(candidate);
        if (axis.length() <= 1.0e-6F) {
            candidate.set(0.0F, 0.0F, 1.0F);
            axis = vector.cross(candidate);
        }
        return normalized(axis, "Opposite-vector rotation");
    }

    bool turnQuatAxis(TQuat4f* destination, const TQuat4f& source, const TVec3f& currentAxis,
                      const TVec3f& targetAxis, float maximumRadians) {
        if (destination == nullptr || !std::isfinite(maximumRadians) || maximumRadians < 0.0F) {
            throw std::invalid_argument("Quaternion turning requires an output and a non-negative finite angle.");
        }
        const auto current = normalized(currentAxis, "Quaternion current axis");
        const auto target = normalized(targetAxis, "Quaternion target axis");
        const auto cosine = std::clamp(current.dot(target), -1.0F, 1.0F);
        const auto angle = std::acos(cosine);
        if (angle <= 1.0e-6F) {
            *destination = source;
            destination->normalize();
            return true;
        }

        auto axis = current.cross(target);
        if (axis.length() <= 1.0e-6F) {
            axis = orthogonalAxis(current);
        }
        const auto applied = std::min(angle, maximumRadians);
        *destination = multiplyQuat(axisAngleQuat(axis, applied), source);
        destination->normalize();
        return angle <= maximumRadians + 1.0e-6F;
    }

    PartsModel* createNPCGoodsImpl(LiveActor* host, const char* modelName, const char* jointName, int drawBufferType) {
        if (host == nullptr) {
            throw std::invalid_argument("NPC goods require a host actor.");
        }
        if (modelName == nullptr || *modelName == '\0') {
            return nullptr;
        }
        if (!MR::isNPCItemFileExist(modelName)) {
            return nullptr;
        }

        auto* jointMatrix = static_cast<MtxPtr>(nullptr);
        if (jointName != nullptr && *jointName != '\0') {
            jointMatrix = MR::getJointMtx(host, jointName);
            if (jointMatrix == nullptr) {
                throw std::logic_error("NPC goods require a real named model joint.");
            }
        }

        auto* goods = new PartsModel(host, modelName, modelName, jointMatrix, drawBufferType, false);
        goods->initWithoutIter();
        goods->_99 = true;
        return goods;
    }
}  // namespace

void JointController::registerCallBack() {
    throw std::logic_error("J3D joint callbacks are unavailable without the real joint-controller pipeline.");
}

const void* smgpcNPCActorModelPresence(const LiveActor* actor) {
    return actor != nullptr ? actor->mModelManager : nullptr;
}

const void* smgpcNPCActorStarPointerPresence(const LiveActor* actor) {
    const auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
    return runtime != nullptr && actor != nullptr && runtime->star_pointer().has_target(*actor) ? actor : nullptr;
}

namespace MR {
    JointControlDelegator<NPCActor>* createNPCActorJointDelegator(NPCActor*, const char*) {
        throw std::logic_error("NPC joint controllers are unavailable without the real J3D joint-controller pipeline.");
    }

    void makeQuatRotateRadian(TQuat4f* destination, const TVec3f& rotation) {
        if (destination == nullptr) {
            throw std::invalid_argument("Quaternion rotation requires an output.");
        }
        const auto hx = rotation.x * 0.5F;
        const auto hy = rotation.y * 0.5F;
        const auto hz = rotation.z * 0.5F;
        const auto sx = std::sin(hx);
        const auto cx = std::cos(hx);
        const auto sy = std::sin(hy);
        const auto cy = std::cos(hy);
        const auto sz = std::sin(hz);
        const auto cz = std::cos(hz);
        destination->set((sx * cy * cz) - (cx * sy * sz), (cx * sy * cz) + (sx * cy * sz),
                         (cx * cy * sz) - (sx * sy * cz), (cx * cy * cz) + (sx * sy * sz));
        destination->normalize();
    }

    void makeQuatRotateDegree(TQuat4f* destination, const TVec3f& rotation) {
        constexpr auto degreesToRadians = std::numbers::pi_v<float> / 180.0F;
        makeQuatRotateRadian(destination, rotation * degreesToRadians);
    }

    void extractMtxTrans(MtxPtr matrix, TVec3f* destination) {
        if (matrix == nullptr || destination == nullptr) {
            throw std::invalid_argument("Matrix translation extraction requires real input and output storage.");
        }
        destination->set(matrix[0][3], matrix[1][3], matrix[2][3]);
    }

    void makeAxisFrontUp(TVec3f* side, TVec3f* up, const TVec3f& front, const TVec3f& supportUp) {
        if (side == nullptr || up == nullptr) {
            throw std::invalid_argument("Axis construction requires two outputs.");
        }
        side->cross(supportUp, front);
        *side = normalized(*side, "Front/up side axis");
        up->cross(front, *side);
    }

    bool isSameDirection(const TVec3f& lhs, const TVec3f& rhs, f32 tolerance) {
        const auto cross = lhs.cross(rhs);
        return std::abs(cross.x) <= tolerance && std::abs(cross.y) <= tolerance && std::abs(cross.z) <= tolerance;
    }

    bool isOppositeDirection(const TVec3f& lhs, const TVec3f& rhs, f32 tolerance) {
        return lhs.dot(rhs) < 0.0F && isSameDirection(lhs, rhs, tolerance);
    }

    void clampVecAngleDeg(TVec3f* vector, const TVec3f& reference, f32 maximumDegrees) {
        if (vector == nullptr || !std::isfinite(maximumDegrees) || maximumDegrees < 0.0F) {
            throw std::invalid_argument("Vector angle clamping requires an output and a finite non-negative angle.");
        }
        const auto length = vector->length();
        const auto source = normalized(reference, "Vector angle reference");
        const auto target = normalized(*vector, "Vector angle target");
        const auto angle = std::acos(std::clamp(source.dot(target), -1.0F, 1.0F));
        const auto maximum = maximumDegrees * (std::numbers::pi_v<float> / 180.0F);
        if (angle <= maximum) {
            return;
        }
        auto axis = source.cross(target);
        if (axis.length() <= 1.0e-6F) {
            axis = orthogonalAxis(source);
        }
        axis = normalized(axis, "Vector angle axis");
        const auto cosine = std::cos(maximum);
        const auto sine = std::sin(maximum);
        auto clamped = source * cosine + axis.cross(source) * sine + axis * (axis.dot(source) * (1.0F - cosine));
        clamped.scale(length);
        vector->set(clamped);
    }

    bool turnQuatYDirRad(TQuat4f* destination, const TQuat4f& source, const TVec3f& target, f32 maximumRadians) {
        auto current = TVec3f{};
        source.getYDir(current);
        return turnQuatAxis(destination, source, current, target, maximumRadians);
    }

    bool faceToVector(TQuat4f* quaternion, TVec3f target, f32 maximumDegrees) {
        if (quaternion == nullptr) {
            throw std::invalid_argument("Facing requires a quaternion.");
        }
        auto up = TVec3f{};
        quaternion->getYDir(up);
        target = normalized(target, "Facing target");
        if (vecKillElement(target, up, &target) > 0.95F) {
            return false;
        }
        auto current = TVec3f{};
        quaternion->getZDir(current);
        return turnQuatAxis(quaternion, *quaternion, current, target,
                            maximumDegrees * (std::numbers::pi_v<float> / 180.0F));
    }

    bool checkPlayerSwingTrigger() {
        throw std::logic_error("Player swing state is unavailable without the real MarioActor.");
    }

    void calcGravity(LiveActor* actor) {
        if (actor == nullptr) {
            throw std::invalid_argument("Gravity calculation requires a LiveActor.");
        }
        auto gravity = TVec3f{};
        if (calcGravityVector(actor, &gravity, nullptr, 0U) && !isNearZero(gravity)) {
            gravity = normalized(gravity, "Actor gravity");
            actor->mGravity.set(gravity);
        }
    }

    void setBckFrameAtRandom(const LiveActor* actor) {
        auto* controller = getBckCtrl(actor);
        const auto randomFrame = static_cast<s32>(
            static_cast<f32>(controller->getEnd()) * getRandom());
        setBckFrame(actor, static_cast<f32>(randomFrame));
    }

    void initStarPointerTargetAtJoint(LiveActor*, const char*, f32, const TVec3f&) {
        throw std::logic_error("Joint-bound StarPointer targets are unavailable without real joint-matrix binding.");
    }

    bool isStarPointerPointing2POnPressButton(const LiveActor*, const char*, bool, bool) {
        throw std::logic_error("Second-player StarPointer input is unavailable in the keyboard/mouse runtime.");
    }

    bool getNPCItemData(NPCActorItem*, s32) {
        throw std::logic_error("NPC item-table data is unavailable without the real NPC item parameter table.");
    }

    bool isNPCItemFileExist(const char* name) {
        if (name == nullptr || *name == '\0') {
            return false;
        }
        const auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime != nullptr && runtime->find_object_archive(name).has_value();
    }

    PartsModel* createNPCGoods(LiveActor* host, const char* modelName, const char* jointName) {
        return createNPCGoodsImpl(host, modelName, jointName, DrawBufferType_NPC);
    }

    PartsModel* createIndirectNPCGoods(LiveActor* host, const char* modelName, const char* jointName) {
        return createNPCGoodsImpl(host, modelName, jointName, DrawBufferType_IndirectNpc);
    }

    void initDefaultPosAndQuat(NPCActor* actor, const JMapInfoIter& iter) {
        if (actor == nullptr || !iter.isValid()) {
            throw std::invalid_argument("NPC placement requires an actor and a valid JMap iterator.");
        }
        initDefaultPos(actor, iter);
        makeQuatRotateDegree(&actor->_A0, actor->mRotation);
        actor->_CC.set(actor->mRotation);
        actor->setInitPose();
    }

    f32 calcFloatOffset(const NPCActor* actor, f32 current, f32 maximum) {
        if (actor == nullptr) {
            throw std::invalid_argument("NPC float offset requires an actor.");
        }

        auto result = current - 0.5F;
        if (!(result >= 0.0F)) {
            result = 0.0F;
        }

        auto* message = actor->mMsgCtrl;
        if (message == nullptr || !isTalkTalking(message) || isShortTalk(message)) {
            return result;
        }

        const auto* playerPosition = getPlayerPos();
        if (playerPosition == nullptr) {
            throw std::logic_error("NPC talk float offset requires a real player position.");
        }
        auto delta = *playerPosition - actor->mPosition;
        auto playerUp = TVec3f{};
        getPlayerUpVec(&playerUp);
        if (!(delta.dot(playerUp) > 0.0F)) {
            return result;
        }

        const auto distance = delta.length();
        if (!(distance < 200.0F)) {
            return result;
        }

        const auto target = getLinerValueFromMinMax(
            distance, 0.0F, 200.0F, maximum, 0.0F);
        const auto capped = 0.5F + (5.0F + result);
        return capped >= target ? target : capped;
    }

    void calcAndSetFloatBaseMtx(NPCActor* actor, f32 offset) {
        if (actor == nullptr) {
            throw std::invalid_argument("NPC float base matrix requires an actor.");
        }

        const auto position = actor->mPosition;
        auto floatDirection = TVec3f{};
        actor->_A0.getYDir(floatDirection);
        floatDirection.scale(offset);
        actor->mPosition.add(floatDirection);
        try {
            actor->NPCActor::calcAndSetBaseMtx();
        } catch (...) {
            actor->mPosition.set(position);
            throw;
        }
        actor->mPosition.set(position);
    }

    bool tryStartTurnAction(NPCActor*) { throwNPCBehaviorUnavailable(); }
    bool tryStartReactionAndPushNerve(NPCActor*, const Nerve*) { throwNPCBehaviorUnavailable(); }
    bool tryStartReactionAndPopNerve(NPCActor*) { throwNPCBehaviorUnavailable(); }
    bool tryTalkNearPlayerAndStartTalkAction(NPCActor*) { throwNPCBehaviorUnavailable(); }
    bool tryTalkNearPlayerAtEndAndStartMoveTalkAction(NPCActor*) { throwNPCBehaviorUnavailable(); }

    void initShadowFromCSV(LiveActor* actor, const char* definitionName) {
        if (definitionName == nullptr) {
            throw std::invalid_argument("Shadow CSV initialization requires an exact definition name.");
        }
        smgpc::compat::initialize_actor_shadow_from_model_archive(actor, definitionName);
    }

}  // namespace MR
