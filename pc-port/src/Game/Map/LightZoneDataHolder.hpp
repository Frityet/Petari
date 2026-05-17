#pragma once

#include <revolution.h>

class LightArea;

class ZoneLightID {
public:
    ZoneLightID() = default;

    bool isTargetArea(const LightArea*) const {
        return false;
    }

    s32 mZoneID = -1;   // 0x0
    s32 mLightID = -1;  // 0x4
};

class LightZoneInfo {
public:
    LightZoneInfo() = default;

    const char* getAreaLightNameInZoneData(s32) const {
        return nullptr;
    }

    s32 mZoneID = -1;                 // 0x0
    const char* mAreaLightName = {};  // 0x4
};

class LightZoneDataHolder {
public:
    LightZoneDataHolder() = default;

    const char* getAreaLightNameInZoneData(const ZoneLightID&) const {
        return nullptr;
    }

    const char* getDefaultStageAreaLightName() const {
        return nullptr;
    }

    s32 mZoneCount = 0;             // 0x0
    LightZoneInfo* mZoneInfo = {};  // 0x4
};
