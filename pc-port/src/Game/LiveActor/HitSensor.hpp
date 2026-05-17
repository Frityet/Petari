#pragma once

#include <revolution.h>

#include "Game/LiveActor/LiveActor.hpp"

class SensorGroup;

class HitSensor {
public:
    HitSensor(u32 type, u16 groupSize, f32 radius, LiveActor* pHost);

    bool receiveMessage(u32 msg, HitSensor* pSender);
    void setType(u32 type);
    bool isType(u32 type) const;
    void validate();
    void invalidate();
    void validateBySystem();
    void invalidateBySystem();
    void addHitSensor(HitSensor* pSensor);

    /* 0x00 */ u32 mType = 0U;
    /* 0x04 */ TVec3f mPosition{};
    /* 0x10 */ f32 mRadius = 0.0F;
    /* 0x14 */ u16 mSensorCount = 0U;
    /* 0x16 */ u16 mGroupSize = 0U;
    /* 0x18 */ HitSensor** mSensors = nullptr;
    /* 0x1C */ SensorGroup* mSensorGroup = nullptr;
    /* 0x20 */ bool mValidBySystem = false;
    /* 0x21 */ bool mValidByHost = true;
    /* 0x24 */ LiveActor* mHost = nullptr;
};
