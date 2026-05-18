#pragma once

#include <revolution/types.h>

#include "Game/Util/JMapInfo.hpp"

class ActorCameraInfo {
public:
    explicit ActorCameraInfo(const JMapInfoIter &rIter);
    ActorCameraInfo(s32 cameraSetID, s32 zoneID);

    /* 0x0 */ s32 mCameraSetID;
    /* 0x4 */ s32 mZoneID;
};
