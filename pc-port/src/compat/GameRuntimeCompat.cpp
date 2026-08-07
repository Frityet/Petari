#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "runtime/RuntimeContext.hpp"

#include <cstring>
#include <string_view>

namespace MR {
    HitSensor* addHitSensorEye(LiveActor* pActor, const char* pName, u16 groupSize, f32 radius, const TVec3f& rOffset) {
        return pActor != nullptr ? pActor->addHitSensor(pName, ATYPE_EYE, groupSize, radius, rOffset) : nullptr;
    }

    bool sendArbitraryMsg(u32 msg, HitSensor* pReceiver, HitSensor* pSender) {
        return pReceiver != nullptr && pReceiver->receiveMessage(msg, pSender);
    }

    void setClippingFar50m(LiveActor*) {
        // The host scheduler currently keeps all registered actors active. This
        // remains a general clipping-policy boundary until host culling lands.
    }

    void setClippingTypeSphere(LiveActor*, f32) {
    }

    void setClippingTypeSphere(LiveActor*, f32, const TVec3f*) {
    }

    void setClippingFar(LiveActor*, f32) {
    }

    void setGroupClipping(LiveActor*, const JMapInfoIter&, int) {
    }

    MsgSharedGroup* joinToGroupArray(LiveActor*, const JMapInfoIter&, const char*, s32) {
        return nullptr;
    }

    void registerDemoSimpleCastAll(LiveActor*) {
    }

    void deleteEffectAll(LiveActor* pActor) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pActor != nullptr) {
            runtime->delete_effect_all(pActor->getName(), pActor);
        }
    }

    bool tryRumblePad(const void*, const char* pPatternName, s32 channel) {
        if (pPatternName == nullptr) {
            return false;
        }

        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            const auto pattern = std::string_view(pPatternName);
            if (pattern == "中") {
                runtime->rumble().request_middle(channel);
            } else if (pattern == "弱" || pattern == "微弱") {
                runtime->rumble().request_weak(channel);
            } else {
                runtime->rumble().request_strong(channel);
            }
        }

        return true;
    }

    void shakeCameraStrong() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            runtime->camera_system().request_strong_shake();
        }
    }

    void shakeCameraWeak() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            runtime->camera_system().request_weak_shake();
        }
    }

    bool isEqualString(const char* pStr1, const char* pStr2) {
        return pStr1 != nullptr && pStr2 != nullptr && std::strcmp(pStr1, pStr2) == 0;
    }
}  // namespace MR
