#pragma once

#include <revolution.h>

enum {
    ACTMES_RUSH_BEGIN = 0x91,
    ACTMES_AUTORUSH_BEGIN = 0x92,
    ACTMES_RUSH_CANCEL = 0x93,
    ACTMES_UPDATE_BASEMTX = 0xA1,
};

enum {
    ATYPE_MESSAGE_SENSOR = 0x83,
    ATYPE_PLAYER = 0x01,
    ATYPE_ENEMY = 0x10,
    ATYPE_ENEMY_BODY = 0x11,
    ATYPE_ENEMY_CATCH = 0x12,
};

class HitSensor;
class LiveActor;
struct TVec3f;

namespace MR {
    bool isMsgAutoRushBegin(u32 msg);
    bool isMsgUpdateBaseMtx(u32 msg);
    const char* getActorMessageName(u32 msg);
    HitSensor* getMessageSensor();
    LiveActor* getSensorHost(const HitSensor* pSensor);
    void initHitSensor(LiveActor* pActor, s32 sensorCount);
    HitSensor* addHitSensorPlayer(LiveActor* pActor, const char* pName, u16 groupSize, f32 radius, const TVec3f& offset);
    HitSensor* addHitSensorEnemy(LiveActor* pActor, const char* pName, u16 groupSize, f32 radius, const TVec3f& offset);
    HitSensor* addHitSensorAtJointEnemy(LiveActor* pActor, const char* pName, const char* pJointName, u16 groupSize, f32 radius,
                                        const TVec3f& offset);
    HitSensor* getSensor(LiveActor* pActor, const char* pName);
    const HitSensor* getSensor(const LiveActor* pActor, const char* pName);
    const char* getSensorName(const HitSensor* pSensor);
    void validateHitSensors(LiveActor* pActor);
    void invalidateHitSensors(LiveActor* pActor);
    bool isSensor(const HitSensor* pSensor, const char* pName);
    bool isSensorPlayer(const HitSensor* pSensor);
    bool isSensorEnemy(const HitSensor* pSensor);
    bool isPlayerInHitSensor(const HitSensor* pSensor);
    void sendMsgToAllLiveActor(u32 msg, LiveActor* pActor);
}  // namespace MR
