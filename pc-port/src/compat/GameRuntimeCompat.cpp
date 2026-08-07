#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "runtime/RuntimeContext.hpp"

#include <cstring>
#include <stdexcept>

namespace MR {
    void setClippingFar50m(LiveActor* pActor) {
        smgpc::compat::configure_actor_clipping_far_level(pActor, 7);
    }

    void setClippingTypeSphere(LiveActor* pActor, f32 radius) {
        smgpc::compat::configure_actor_clipping_sphere(pActor, radius, nullptr);
    }

    void setClippingTypeSphere(LiveActor* pActor, f32 radius, const TVec3f* pCenter) {
        smgpc::compat::configure_actor_clipping_sphere(pActor, radius, pCenter);
    }

    void setClippingFar(LiveActor* pActor, f32 distance) {
        switch (static_cast<s32>(distance)) {
        case 50:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 7);
            break;
        case 100:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 6);
            break;
        case 200:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 5);
            break;
        case 300:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 4);
            break;
        case 400:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 3);
            break;
        case 500:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 2);
            break;
        case 600:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 1);
            break;
        case -1:
            smgpc::compat::configure_actor_clipping_far_level(pActor, 0);
            break;
        default:
            break;
        }
    }

    void setGroupClipping(LiveActor*, const JMapInfoIter&, int) {
        throw std::logic_error("Group clipping is unavailable without ClippingGroupHolder.");
    }

    MsgSharedGroup* joinToGroupArray(LiveActor* pActor, const JMapInfoIter& rIter, const char*, s32) {
        if (pActor == nullptr) {
            throw std::invalid_argument("LiveActorGroupArray registration requires a LiveActor.");
        }
        if (!rIter.isValid()) {
            return nullptr;
        }

        auto group_id = s32{-1};
        if (!MR::getJMapInfoGroupID(rIter, &group_id) || group_id < 0) {
            return nullptr;
        }
        throw std::logic_error(
            "LiveActorGroupArray registration is unavailable without the real scene-owned group manager.");
    }

    void registerDemoSimpleCastAll(LiveActor*) {
        throw std::logic_error(
            "Simple-cast demo registration is unavailable without the real DemoDirector executor registry.");
    }

    void deleteEffectAll(LiveActor* pActor) {
        if (pActor == nullptr) {
            throw std::invalid_argument("Effect deletion requires a LiveActor.");
        }
        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) {
            throw std::logic_error("Effect deletion requires an active effect runtime.");
        }
        if (!runtime->effects().registered_keeper(pActor->getName(), pActor).has_value()) {
            throw std::logic_error("Effect deletion requires the actor's registered effect keeper.");
        }
        runtime->delete_effect_all(pActor->getName(), pActor);
    }

    bool tryRumblePad(const void* pSource, const char* pPatternName, s32 channel) {
        if (pPatternName == nullptr) {
            return false;
        }

        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            return runtime->rumble().try_request_pattern(pSource, pPatternName, channel);
        }

        return false;
    }

    bool tryRumblePadVeryStrongLong(const void* pSource, s32 channel) {
        return tryRumblePad(pSource, "最強【長】", channel);
    }

    bool tryRumblePadVeryStrong(const void* pSource, s32 channel) {
        return tryRumblePad(pSource, "最強【長】", channel);
    }

    bool tryRumblePadStrong(const void* pSource, s32 channel) {
        return tryRumblePad(pSource, "最強", channel);
    }

    bool tryRumblePadMiddle(const void* pSource, s32 channel) {
        return tryRumblePad(pSource, "最強", channel);
    }

    bool tryRumblePadWeak(const void* pSource, s32 channel) {
        return tryRumblePad(pSource, "微弱", channel);
    }

    bool tryRumblePadVeryWeak(const void* pSource, s32 channel) {
        return tryRumblePad(pSource, "微弱", channel);
    }

    bool tryRumbleDefaultHit(const void* pSource, s32 channel) {
        return tryRumblePad(pSource, "強", channel);
    }

    namespace {
        smgpc::runtime::CameraSystemService& cameraSystemForShake() {
            auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
            if (runtime == nullptr) {
                throw std::logic_error("Camera shake requires an active camera runtime.");
            }
            return runtime->camera_system();
        }
    }

    void shakeCameraVeryStrong() {
        cameraSystemForShake().request_very_strong_shake();
    }

    void shakeCameraStrong() {
        cameraSystemForShake().request_strong_shake();
    }

    void shakeCameraNormalStrong() {
        cameraSystemForShake().request_normal_strong_shake();
    }

    void shakeCameraNormal() {
        cameraSystemForShake().request_normal_shake();
    }

    void shakeCameraNormalWeak() {
        cameraSystemForShake().request_normal_weak_shake();
    }

    void shakeCameraWeak() {
        cameraSystemForShake().request_weak_shake();
    }

    void shakeCameraVeryWeak() {
        cameraSystemForShake().request_very_weak_shake();
    }

    bool isEqualString(const char* pStr1, const char* pStr2) {
        return pStr1 != nullptr && pStr2 != nullptr && std::strcmp(pStr1, pStr2) == 0;
    }
}  // namespace MR
