#include "Game/Util/ActorSensorUtil.hpp"

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "runtime/RuntimeContext.hpp"

#include <string>

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

    [[nodiscard]] bool is_live_sensor(const HitSensor* pSensor) {
        return pSensor != nullptr && pSensor->mValidByHost && pSensor->mValidBySystem;
    }

    void emit_sensor_registration_trace(const LiveActor* pActor, const char* pName, const char* pJointName, const char* pTypeName,
                                        f32 radius) {
#ifndef NDEBUG
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
            runtime != nullptr && pActor != nullptr && pName != nullptr) {
            runtime->emit_semantic_trace_event("hit_sensor", "registered",
                                               "object=" + std::string(pActor->getName()) + ";sensor=" + pName +
                                                   ";joint=" + (pJointName != nullptr ? std::string(pJointName) : std::string{}) +
                                                   ";type=" + (pTypeName != nullptr ? std::string(pTypeName) : std::string{}) +
                                                   ";radius=" + std::to_string(radius));
        }
#else
        (void)pActor;
        (void)pName;
        (void)pJointName;
        (void)pTypeName;
        (void)radius;
#endif
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

    void initHitSensor(LiveActor* pActor, s32 sensorCount) {
        if (pActor != nullptr) {
            pActor->initHitSensor(sensorCount);
        }
    }

    HitSensor* addHitSensorPlayer(LiveActor* pActor, const char* pName, u16 groupSize, f32 radius, const TVec3f& offset) {
        if (pActor == nullptr) {
            return nullptr;
        }

        auto* sensor = pActor->addHitSensor(pName, ATYPE_PLAYER, groupSize, radius, offset);
        emit_sensor_registration_trace(pActor, pName, nullptr, "player", radius);
        return sensor;
    }

    HitSensor* addHitSensorEnemy(LiveActor* pActor, const char* pName, u16 groupSize, f32 radius, const TVec3f& offset) {
        if (pActor == nullptr) {
            return nullptr;
        }

        auto* sensor = pActor->addHitSensor(pName, ATYPE_ENEMY, groupSize, radius, offset);
        emit_sensor_registration_trace(pActor, pName, nullptr, "enemy", radius);
        return sensor;
    }

    HitSensor* addHitSensorAtJointEnemy(LiveActor* pActor, const char* pName, const char* pJointName, u16 groupSize, f32 radius,
                                        const TVec3f& offset) {
        if (pActor == nullptr) {
            return nullptr;
        }

        const auto type = pName != nullptr && std::string{pName} == "Catch" ? ATYPE_ENEMY_CATCH : ATYPE_ENEMY_BODY;
        auto* sensor = pActor->addHitSensor(pName, type, groupSize, radius, offset);
        emit_sensor_registration_trace(pActor, pName, pJointName, "enemy", radius);
        return sensor;
    }

    HitSensor* getSensor(LiveActor* pActor, const char* pName) {
        return pActor != nullptr ? pActor->getSensor(pName) : nullptr;
    }

    const HitSensor* getSensor(const LiveActor* pActor, const char* pName) {
        return pActor != nullptr ? pActor->getSensor(pName) : nullptr;
    }

    const char* getSensorName(const HitSensor* pSensor) {
        const auto* host = getSensorHost(pSensor);
        return host != nullptr ? host->getSensorName(pSensor) : "";
    }

    void validateHitSensors(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->validateHitSensors();
        }
    }

    void invalidateHitSensors(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->invalidateHitSensors();
        }
    }

    bool isSensor(const HitSensor* pSensor, const char* pName) {
        const auto* host = getSensorHost(pSensor);
        return host != nullptr && getSensor(host, pName) == pSensor;
    }

    bool isSensorPlayer(const HitSensor* pSensor) {
        return pSensor != nullptr && pSensor->isType(ATYPE_PLAYER);
    }

    bool isSensorEnemy(const HitSensor* pSensor) {
        return pSensor != nullptr && (pSensor->isType(ATYPE_ENEMY) || pSensor->isType(ATYPE_ENEMY_BODY) ||
                                      pSensor->isType(ATYPE_ENEMY_CATCH));
    }

    bool isPlayerInHitSensor(const HitSensor* pSensor) {
        if (!is_live_sensor(pSensor)) {
            return false;
        }

        const auto* player_pos = MR::getPlayerPos();
        return pSensor->mPosition.squareDistance(*player_pos) <= (pSensor->mRadius * pSensor->mRadius);
    }

    void sendMsgToAllLiveActor(u32 msg, LiveActor* pActor) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            runtime->scheduler().send_message_to_live_actors(msg, pActor);
        }
    }
}  // namespace MR
