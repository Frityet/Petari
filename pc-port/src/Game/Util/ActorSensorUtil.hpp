#pragma once

#include <JSystem/JGeometry/TVec.hpp>
#include <revolution.h>

enum {
    ACTMES_PUSH_FORCE = 0x2A,
    ACTMES_RUSH_BEGIN = 0x91,
    ACTMES_AUTORUSH_BEGIN = 0x92,
    ACTMES_RUSH_CANCEL = 0x93,
    ACTMES_UPDATE_BASEMTX = 0xA1,
    ACTMES_INHALE_BLACK_HOLE = 0x73,
    ACTMES_ITEM_GET = 0x87,
    ACTMES_ITEM_PULL = 0x89,
    ACTMES_ITEM_SHOW = 0x8A,
    ACTMES_ITEM_HIDE = 0x8B,
    ACTMES_ITEM_START_MOVE = 0x8C,
    ACTMES_ITEM_END_MOVE = 0x8D,
};

enum {
    ATYPE_EYE = 0x7F,
    ATYPE_MESSAGE_SENSOR = 0x83,
    ATYPE_PLAYER = 0x01,
    ATYPE_ENEMY = 0x10,
    ATYPE_ENEMY_BODY = 0x11,
    ATYPE_ENEMY_CATCH = 0x12,
    ATYPE_COIN = 0x4A,
};

class HitSensor;
class LiveActor;

namespace MR {
    bool isMsgAutoRushBegin(u32 msg);
    bool isMsgUpdateBaseMtx(u32 msg);
    const char* getActorMessageName(u32 msg);
    HitSensor* getMessageSensor();
    LiveActor* getSensorHost(const HitSensor* pSensor);
    void initHitSensor(LiveActor* pActor, s32 sensorCount);
    HitSensor* addHitSensorPlayer(LiveActor* pActor, const char* pName, u16 groupSize, f32 radius, const TVec3f& offset);
    HitSensor* addHitSensorEnemy(LiveActor* pActor, const char* pName, u16 groupSize, f32 radius, const TVec3f& offset);
    HitSensor* addHitSensorEye(LiveActor* pActor, const char* pName, u16 groupSize, f32 radius, const TVec3f& offset);
    HitSensor* addHitSensor(LiveActor* pActor, const char* pName, u32 type, u16 groupSize, f32 radius, const TVec3f& offset);
    HitSensor* addHitSensorAtJointEnemy(LiveActor* pActor, const char* pName, const char* pJointName, u16 groupSize, f32 radius,
                                        const TVec3f& offset);
    HitSensor* getSensor(LiveActor* pActor, const char* pName);
    const HitSensor* getSensor(const LiveActor* pActor, const char* pName);
    const char* getSensorName(const HitSensor* pSensor);
    void validateHitSensors(LiveActor* pActor);
    void invalidateHitSensors(LiveActor* pActor);
    void setSensorRadius(LiveActor* pActor, const char* pName, f32 radius);
    bool isSensor(const HitSensor* pSensor, const char* pName);
    bool isSensorPlayer(const HitSensor* pSensor);
    bool isSensorEnemy(const HitSensor* pSensor);
    bool isPlayerInHitSensor(const HitSensor* pSensor);
    bool sendArbitraryMsg(u32 msg, HitSensor* pReceiver, HitSensor* pSender);
    void sendMsgToAllLiveActor(u32 msg, LiveActor* pActor);
    bool isMsgItemGet(u32 msg);
    bool isMsgItemPull(u32 msg);
    bool isMsgItemShow(u32 msg);
    bool isMsgItemHide(u32 msg);
    bool isMsgItemStartMove(u32 msg);
    bool isMsgItemEndMove(u32 msg);
    bool isMsgInhaleBlackHole(u32 msg);
}  // namespace MR
