#pragma once

#include <revolution.h>

class LightArea;

class ZoneLightID {
public:
    ZoneLightID();

    void clear();
    bool isTargetArea(const LightArea*) const;
    bool isOutOfArea() const;

    s32 _0;             // 0x0
    s32 mLightID = -1;  // 0x4
};

struct AreaInfo {
    s32 mID;                     // 0x0
    const char* mAreaLightName;  // 0x4
};

class LightZoneInfo {
public:
    LightZoneInfo() = default;

    const char* getAreaLightNameInZoneData(s32) const {
        return nullptr;
    }

    s32 mAreaCount = 0;        // 0x0
    AreaInfo* mAreaInfo = {};  // 0x4
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
