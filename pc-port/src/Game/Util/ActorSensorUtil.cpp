#include "Game/Util/ActorSensorUtil.hpp"

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "runtime/RuntimeContext.hpp"

namespace {
    class MessageSensorHost final : public LiveActor {
    public:
        MessageSensorHost() : LiveActor("MessageSensorHolder") {
            makeActorAppeared();
        }
    };

    struct MessageSensorRuntime {
        MessageSensorHost host{};
        HitSensor sensor{ATYPE_MESSAGE_SENSOR, 0U, 0.0F, &host};
    };

    [[nodiscard]] MessageSensorRuntime& message_sensor_runtime() {
        static auto runtime = MessageSensorRuntime{};
        return runtime;
    }
}  // namespace

namespace MR {
    bool isMsgAutoRushBegin(u32 msg) {
        return msg == ACTMES_AUTORUSH_BEGIN;
    }

    bool isMsgUpdateBaseMtx(u32 msg) {
        return msg == ACTMES_UPDATE_BASEMTX;
    }

    const char* getActorMessageName(u32 msg) {
        switch (msg) {
        case ACTMES_RUSH_BEGIN:
            return "ACTMES_RUSH_BEGIN";
        case ACTMES_AUTORUSH_BEGIN:
            return "ACTMES_AUTORUSH_BEGIN";
        case ACTMES_RUSH_CANCEL:
            return "ACTMES_RUSH_CANCEL";
        case ACTMES_UPDATE_BASEMTX:
            return "ACTMES_UPDATE_BASEMTX";
        default:
            return "Unknown";
        }
    }

    HitSensor* getMessageSensor() {
        return &message_sensor_runtime().sensor;
    }

    LiveActor* getSensorHost(const HitSensor* pSensor) {
        return pSensor != nullptr ? pSensor->mHost : nullptr;
    }

    void sendMsgToAllLiveActor(u32 msg, LiveActor* pActor) {
        if (auto* runtime = smgpc::compat::RuntimeContext::try_instance(); runtime != nullptr) {
            runtime->scheduler().send_message_to_live_actors(msg, pActor);
        }
    }
}  // namespace MR
