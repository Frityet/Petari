#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/ClippingActorHolder.hpp"
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
#include "Game/Util/StringUtil.hpp"

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/JointUtil.hpp"
#include "compat/ActorMotionCompat.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
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

    void bind_line_endpoint(smgpc::compat::ActorShadowControllerRuntimeState& definition,
                            LiveActor* owner, LiveActor* endpoint_actor, const char* name, bool start) {
        auto& actor = require_actor(endpoint_actor);
        auto* list = actor.mShadowControllerList;
        if (!list) {
            aurora::throw_host_exception<std::logic_error>("Shadow line endpoint requires an original controller list");
        }
        auto& index = start ? definition.line_start_controller_index : definition.line_end_controller_index;
        auto& borrowed = start ? definition.line_start_controller : definition.line_end_controller;
        if (&actor != owner) {
            borrowed = list->getController(name);
            return;
        }
        // Retail adds the new controller before resolving endpoints. Resolve
        // the same post-add sequence without publishing a partial owner.
        const auto count = list->getControllerCount();
        if (count == 0) {
            index = 0;
            return;
        }
        for (u32 item = 0; item < count; ++item) {
            if (MR::isEqualString(name, list->getController(item)->mName)) {
                index = item;
                return;
            }
        }
        if (MR::isEqualString(name, definition.name_raw.c_str())) index = count;
    }


}  // namespace

namespace MR {
    f32 calcNerveValue(const LiveActor *pActor, s32 stepMax, f32 valueStart, f32 valueEnd) {
        const auto &actor = require_actor(pActor);
        const auto rate = stepMax <= 0 ? 1.0F : MR::clamp(static_cast<f32>(actor.getNerveStep()) / static_cast<f32>(stepMax), 0.0F, 1.0F);
        return valueStart + ((valueEnd - valueStart) * rate);
    }

    void setClippingFar100m(LiveActor* pActor) {
        MR::getClippingDirector()->mActorHolder->setFarClipLevel(pActor, 6);
    }

    bool isNoBind(const LiveActor *pActor) {
        return require_actor(pActor).mFlag.mIsNoBind;
    }

    void onBind(LiveActor *pActor) {
        require_actor(pActor).mFlag.mIsNoBind = false;
    }

    void offCalcGravity(LiveActor* pActor) {
        pActor->mFlag.mIsCalcGravity = false;
    }

    void onCalcGravity(LiveActor* pActor) {
        if (!isDead(pActor)) {
            calcGravity(pActor);
        }

        pActor->mFlag.mIsCalcGravity = true;
    }

    bool isBindedGroundDamageFire(const LiveActor *pActor) {
        const auto &actor = require_actor(pActor);
        if (actor.mBinder == nullptr || !actor.mBinder->isBindedGround()) {
            return false;
        }
        return MR::isGroundCodeDamageFire(&actor.mBinder->mGroundInfo.mParentTriangle);
    }

    void initShadowController(LiveActor* actor, u32 count) {
        require_actor(actor).initShadowControllerList(count);
    }

    void addShadowVolumeSphere(LiveActor* actor, const char* name, f32 radius) {
        (void)smgpc::compat::add_actor_shadow_controller(actor, name, smgpc::compat::ActorShadowControllerKind::VolumeSphere, radius);
    }

    void addShadowVolumeLine(LiveActor* actor, const char* name, LiveActor* from_actor, const char* from_name, f32 from_width,
                             LiveActor* to_actor, const char* to_name, f32 to_width) {
        const smgpc::compat::JkrHostAllocationScope host;
        auto definition = smgpc::compat::make_actor_shadow_controller_runtime_state(
            actor, name, smgpc::compat::ActorShadowControllerKind::VolumeLine, 0.0F);
        definition.calculation_mode = smgpc::compat::ActorShadowCalculationMode::Disabled;
        definition.line_start_radius = from_width;
        definition.line_end_radius = to_width;
        bind_line_endpoint(definition, actor, from_actor, from_name, true);
        bind_line_endpoint(definition, actor, to_actor, to_name, false);
        (void)smgpc::compat::add_actor_shadow_controller(actor, std::move(definition));
    }

    void setShadowDropDirectionPtr(LiveActor* actor, const char* name, const TVec3f* direction) {
        require_shadow_controller(actor, name).setDropDirPtr(direction);
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
