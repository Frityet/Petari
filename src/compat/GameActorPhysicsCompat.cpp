#include <aurora/allocation.hpp>
#include "resource/TextEncoding.hpp"
#include <aurora/exception.hpp>
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorShadowUtil.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/JointUtil.hpp"
#include "compat/ActorMotionCompat.hpp"
#include "compat/ActorPhysicsRuntime.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/PlayerUtilCompat.hpp"
#include "runtime/RuntimeServices.hpp"

#include <cmath>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    // These stable Game names outlive every owner that borrows their bytes.
    std::string encode_owner_name(std::string_view name) {
        aurora::allocation::HostAllocationScope host;
        return smgpc::resource::encode_cp932(name);
    }

    const std::string cSurfaceCircleShadowName = encode_owner_name("水面丸影");
    const std::string cVolumeSphereShadowName = encode_owner_name("ボリューム影(球)");
    const std::string cVolumeCylinderShadowName = encode_owner_name("ボリューム影(円柱)");
}  // namespace

namespace {

    [[noreturn]] void throw_game_scene_layout_unavailable() {
        aurora::throw_host_exception<std::logic_error>("GameSceneLayoutHolder is unavailable in the active scene.");
    }

    LiveActor &require_actor(LiveActor *actor) {
        if (actor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("Actor utility requires a LiveActor.");
        }
        return *actor;
    }

    const LiveActor &require_actor(const LiveActor *actor) {
        if (actor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("Actor utility requires a LiveActor.");
        }
        return *actor;
    }

    ShadowController& require_shadow_controller(LiveActor* actor, const char* name) {
        (void)require_actor(actor);
        if (!actor->mShadowControllerList) {
            aurora::throw_host_exception<std::logic_error>("Actor has no original shadow controller list");
        }
        auto* controller = actor->mShadowControllerList->getController(name);
        if (!controller) {
            aurora::throw_host_exception<std::logic_error>("Actor has no matching original shadow controller");
        }
        return *controller;
    }

    smgpc::compat::ActorShadowControllerRuntimeState& require_shadow_definition(LiveActor* actor, const char* name) {
        auto* definition = smgpc::compat::actor_shadow_controller_runtime_state(actor, name);
        if (!definition) {
            aurora::throw_host_exception<std::logic_error>("Actor has no matching shadow shape definition");
        }
        return *definition;
    }

    template<typename Operation>
    void for_each_shadow_controller(LiveActor* actor, const char* name, Operation&& operation) {
        (void)require_actor(actor);
        if (name) {
            operation(require_shadow_controller(actor, name));
            return;
        }
        if (!actor->mShadowControllerList) {
            aurora::throw_host_exception<std::logic_error>("Actor has no original shadow controller list");
        }
        for (u32 i = 0; i < actor->mShadowControllerList->getControllerCount(); ++i) {
            operation(*actor->mShadowControllerList->getController(i));
        }
    }

    void initialize_single_shadow(LiveActor *actor, std::string_view name,
                                  smgpc::compat::ActorShadowControllerKind kind, float radius) {
        smgpc::compat::JkrHostAllocationScope host;
        actor = &require_actor(actor);
        auto shadow = smgpc::compat::ActorShadowRuntimeState{
            .capacity = 1U,
            .controllers = {},
        };
        shadow.controllers.reserve(1U);
        auto controller = smgpc::compat::make_actor_shadow_controller_runtime_state(actor, name, kind, radius);
        if (kind == smgpc::compat::ActorShadowControllerKind::VolumeCylinder) {
            controller.calculation_mode = smgpc::compat::ActorShadowCalculationMode::Disabled;
        }
        shadow.controllers.push_back(std::move(controller));
        smgpc::compat::replace_actor_shadow_runtime_state(actor, std::move(shadow));
    }

}  // namespace

namespace MR {
    f32 calcNerveValue(const LiveActor *pActor, s32 stepMax, f32 valueStart, f32 valueEnd) {
        const auto &actor = require_actor(pActor);
        const auto rate = stepMax <= 0 ? 1.0F : MR::clamp(static_cast<f32>(actor.getNerveStep()) / static_cast<f32>(stepMax), 0.0F, 1.0F);
        return valueStart + ((valueEnd - valueStart) * rate);
    }

    MirrorActor *tryCreateMirrorActor(LiveActor *pActor, const char *) {
        (void)require_actor(pActor);
        aurora::throw_host_exception<std::logic_error>("MirrorActor creation is unavailable without parsed MirrorArea ownership and mirror rendering.");
    }

    void setClippingFar100m(LiveActor *pActor) {
        smgpc::compat::configure_actor_clipping_far_level(pActor, 6);
    }

    bool isNoBind(const LiveActor *pActor) {
        return require_actor(pActor).mFlag.mIsNoBind;
    }

    void onBind(LiveActor *pActor) {
        require_actor(pActor).mFlag.mIsNoBind = false;
    }

    void offCalcGravity(LiveActor *pActor) {
        require_actor(pActor).mFlag.mIsCalcGravity = false;
    }

    void onCalcGravity(LiveActor *pActor) {
        auto &actor = require_actor(pActor);
        actor.mFlag.mIsCalcGravity = true;
        if (!actor.mFlag.mIsDead) {
            smgpc::compat::update_live_actor_gravity(actor);
        }
    }

    bool isBindedGroundDamageFire(const LiveActor *pActor) {
        const auto &actor = require_actor(pActor);
        if (actor.mBinder == nullptr || !actor.mBinder->isBindedGround()) {
            return false;
        }
        return MR::isGroundCodeDamageFire(&actor.mBinder->mGroundInfo.mParentTriangle);
    }

    void initShadowSurfaceCircle(LiveActor *actor, f32 radius) {
        initialize_single_shadow(actor, cSurfaceCircleShadowName.c_str(), smgpc::compat::ActorShadowControllerKind::SurfaceCircle, radius);
    }

    void initShadowVolumeSphere(LiveActor *actor, f32 radius) {
        initialize_single_shadow(actor, cVolumeSphereShadowName.c_str(), smgpc::compat::ActorShadowControllerKind::VolumeSphere, radius);
    }

    void initShadowVolumeCylinder(LiveActor *actor, f32 radius) {
        initialize_single_shadow(actor, cVolumeCylinderShadowName.c_str(), smgpc::compat::ActorShadowControllerKind::VolumeCylinder, radius);
    }

    void setShadowDropDirection(LiveActor* actor, const char* name, const TVec3f& direction) {
        require_shadow_controller(actor, name).setDropDirFix(direction);
    }

    void setShadowDropPositionPtr(LiveActor *actor, const char *name, const TVec3f *position) {
        require_shadow_controller(actor, name).setDropPosPtr(position);
    }

    void setShadowDropLength(LiveActor *actor, const char *name, f32 length) {
        require_shadow_controller(actor, name).setDropLength(length);
    }

    void setShadowVolumeStartDropOffset(LiveActor *actor, const char *name, f32 offset) {
        require_shadow_definition(actor, name).volume_start_offset = offset;
    }

    void setShadowVolumeEndDropOffset(LiveActor *actor, const char *name, f32 offset) {
        require_shadow_definition(actor, name).volume_end_offset = offset;
    }

    void onShadowVolumeCutDropLength(LiveActor *actor, const char *name) {
        require_shadow_definition(actor, name).volume_cut_drop_length = true;
    }

    void onCalcShadow(LiveActor *actor, const char *name) {
        for_each_shadow_controller(actor, name, [](auto &controller) {
            controller.onCalcCollision();
        });
    }

    void offCalcShadow(LiveActor *actor, const char *name) {
        for_each_shadow_controller(actor, name, [](auto &controller) {
            controller.offCalcCollision();
        });
    }

    void onCalcShadowOneTimeAll(LiveActor *actor) {
        for_each_shadow_controller(actor, nullptr, [](auto &controller) {
            controller.onCalcCollisionOneTime();
        });
    }

    void onCalcShadowOneTime(LiveActor *actor, const char *name) {
        for_each_shadow_controller(actor, name, [](auto &controller) {
            controller.onCalcCollisionOneTime();
        });
    }

    void onCalcShadowDropPrivateGravity(LiveActor *actor, const char *name) {
        require_shadow_controller(actor, name).onCalcDropPrivateGravity();
    }

    void onCalcShadowDropPrivateGravityOneTime(LiveActor *actor, const char *name) {
        require_shadow_controller(actor, name).onCalcDropPrivateGravityOneTime();
    }

    void offCalcShadowDropPrivateGravity(LiveActor *actor, const char *name) {
        require_shadow_controller(actor, name).offCalcDropPrivateGravity();
    }

    bool isExistShadow(const LiveActor *actor, const char *name) {
        (void)require_actor(actor);
        return actor->mShadowControllerList && actor->mShadowControllerList->getController(name);
    }

    void invalidateShadow(LiveActor *actor, const char *name) {
        for_each_shadow_controller(actor, name, [](auto &controller) {
            controller.invalidate();
        });
    }

    void validateShadow(LiveActor *actor, const char *name) {
        for_each_shadow_controller(actor, name, [](auto &controller) {
            controller.validate();
        });
    }



    void createPurpleCoinCounter() {
        throw_game_scene_layout_unavailable();
    }

    void validatePurpleCoinCounter() {
        throw_game_scene_layout_unavailable();
    }
}  // namespace MR

namespace MR {
    void setShadowDropPositionMtxPtr(LiveActor* actor, const char* name, MtxPtr matrix, const TVec3f& position) {
        require_shadow_controller(actor, name).setDropPosMtxPtr(matrix, position);
    }

    void setShadowDropPositionAtJoint(LiveActor* pActor, const char* pName1, const char* pName2, const TVec3f& rPos) {
        setShadowDropPositionMtxPtr(pActor, pName1, getJointMtx(pActor, pName2), rPos);
    }

    bool isBindedGroundWater(const LiveActor* pActor) {
        if (pActor->mBinder == nullptr) {
            return false;
        }

        if (!pActor->mBinder->isBindedGround()) {
            return false;
        }

        return isGroundCodeWaterIter(pActor->mBinder->mGroundInfo.mParentTriangle.getAttributes());
    }
}
