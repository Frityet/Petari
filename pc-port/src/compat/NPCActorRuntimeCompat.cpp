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
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/LiveActorMatrixCompat.hpp"
#include "render/J3dMaterialRuntime.hpp"
#include "render/live_actor/LiveActorModel.hpp"
#include "runtime/RuntimeContext.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string>

namespace {
    [[noreturn]] void throwAnimScaleUnavailable() {
        throw std::logic_error("NPC AnimScaleController is unavailable without the real J3D joint-controller pipeline.");
    }

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

    smgpc::render::live_actor::LiveActorModel& requireModel(const LiveActor* actor) {
        if (actor == nullptr) {
            throw std::invalid_argument("NPC model operation requires a LiveActor.");
        }
        auto* model = smgpc::compat::actor_model(actor);
        if (model == nullptr) {
            throw std::logic_error("NPC model state is unavailable.");
        }
        return *model;
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

AnimScaleParam::AnimScaleParam()
    : _0(0.2F), _4(0.91F), _8(0.2F), _C(1.8F), _10(12.0F), _14(0.55F), _18(0.12F), _1C(2.0F),
      _20(0.06F), _24(0.12F), _28(0.91F), _2C(0x14), _30(0.25F) {
}

AnimScaleController::AnimScaleController(AnimScaleParam* parameter)
    : NerveExecutor("スケールアニメコントロール"), _8(parameter), _C(1.0F, 1.0F, 1.0F), _18(0.0F) {
    throwAnimScaleUnavailable();
}

AnimScaleController::~AnimScaleController() = default;
void AnimScaleController::update() { throwAnimScaleUnavailable(); }
void AnimScaleController::updateScale(f32, f32) { throwAnimScaleUnavailable(); }
void AnimScaleController::startCrush() { throwAnimScaleUnavailable(); }
void AnimScaleController::startAnim() { throwAnimScaleUnavailable(); }
void AnimScaleController::startAndAddScaleVelocityY(f32) { throwAnimScaleUnavailable(); }
void AnimScaleController::setParamTight() { throwAnimScaleUnavailable(); }
void AnimScaleController::startHitReaction() { throwAnimScaleUnavailable(); }
void AnimScaleController::startDpdHitVibration() { throwAnimScaleUnavailable(); }
bool AnimScaleController::isHitReaction(s32) const { throwAnimScaleUnavailable(); }
void AnimScaleController::stopAndReset() { throwAnimScaleUnavailable(); }
void AnimScaleController::resetScale() { throwAnimScaleUnavailable(); }
bool AnimScaleController::tryStop() { throwAnimScaleUnavailable(); }
void AnimScaleController::exeAnim() { throwAnimScaleUnavailable(); }
void AnimScaleController::exeDpdVibration() { throwAnimScaleUnavailable(); }
void AnimScaleController::exeHitReaction() { throwAnimScaleUnavailable(); }
void AnimScaleController::exeCrush() { throwAnimScaleUnavailable(); }

void JointController::registerCallBack() {
    throw std::logic_error("J3D joint callbacks are unavailable without the real joint-controller pipeline.");
}

const void* smgpcNPCActorModelPresence(const LiveActor* actor) {
    return smgpc::compat::actor_model(actor);
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

    void setBaseTRMtx(LiveActor* actor, const TQuat4f& quaternion) {
        if (actor == nullptr) {
            throw std::invalid_argument("Base-matrix assignment requires a LiveActor.");
        }
        auto normalizedQuat = quaternion;
        normalizedQuat.normalize();
        auto side = TVec3f{};
        auto up = TVec3f{};
        auto front = TVec3f{};
        normalizedQuat.getXDir(side);
        normalizedQuat.getYDir(up);
        normalizedQuat.getZDir(front);
        setBaseTRMtx(actor, smgpc::render::J3dMatrix3x4{{
                                side.x, up.x, front.x, actor->mPosition.x,
                                side.y, up.y, front.y, actor->mPosition.y,
                                side.z, up.z, front.z, actor->mPosition.z,
                            }});
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

    bool isExistBck(const LiveActor* actor, const char* name) {
        if (name == nullptr || *name == '\0') {
            return false;
        }
        return requireModel(actor).bck_frame_max(name).has_value();
    }

    void startBckNoInterpole(const LiveActor* actor, const char* name) {
        if (actor == nullptr || name == nullptr || *name == '\0') {
            throw std::invalid_argument("BCK playback requires an actor and animation name.");
        }
        const_cast<LiveActor*>(actor)->startBck(name, nullptr);
    }

    void setBckFrameAtRandom(const LiveActor*) {
        throw std::logic_error("Random BCK frame selection is unavailable without a writable real BCK frame controller.");
    }

    void connectToSceneIndirectNpc(LiveActor* actor) {
        connectToScene(actor, MovementType_NPC, CalcAnimType_NPC, DrawBufferType_IndirectNpc, -1);
    }

    void addToAttributeGroupSearchTurtle(const LiveActor*) {
        throw std::logic_error("NPC SearchTurtle attributes are unavailable without the real GroupCheckManager.");
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

    bool tryStartTurnAction(NPCActor*) { throwNPCBehaviorUnavailable(); }
    bool tryStartReactionAndPushNerve(NPCActor*, const Nerve*) { throwNPCBehaviorUnavailable(); }
    bool tryStartReactionAndPopNerve(NPCActor*) { throwNPCBehaviorUnavailable(); }
    bool tryTalkNearPlayerAndStartTalkAction(NPCActor*) { throwNPCBehaviorUnavailable(); }
    bool tryTalkNearPlayerAtEndAndStartMoveTalkAction(NPCActor*) { throwNPCBehaviorUnavailable(); }

    void initShadowFromCSV(LiveActor*, const char*) {
        throw std::logic_error("Actor shadows are unavailable without real projection, collision, and draw behavior.");
    }

    bool isExistShadow(const LiveActor*, const char*) {
        throw std::logic_error("Actor shadow queries are unavailable without real shadow-controller ownership.");
    }
}  // namespace MR
