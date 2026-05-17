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
};

class HitSensor;
class LiveActor;

namespace MR {
    bool isMsgAutoRushBegin(u32 msg);
    bool isMsgUpdateBaseMtx(u32 msg);
    const char* getActorMessageName(u32 msg);
    HitSensor* getMessageSensor();
    LiveActor* getSensorHost(const HitSensor* pSensor);
    void sendMsgToAllLiveActor(u32 msg, LiveActor* pActor);
}  // namespace MR
