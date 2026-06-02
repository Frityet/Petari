#pragma once

#include <revolution.h>

#include "Game/Map/LightDataHolder.hpp"
#include "Game/Map/LightPointCtrl.hpp"
#include "Game/Map/LightZoneDataHolder.hpp"

class ActorLightCtrl;
class JMapInfo;
class LightAreaHolder;
class ResourceHolder;

class LightFunction {
public:
    static void initLightRegisterAll();
    static void initLightData();
    static ResourceHolder* loadLightArchive();
    static s32 createLightDataParser(JMapInfo**);
    static s32 createZoneDataParser(const char*, JMapInfo**);

    static void loadAllLightWhite();

    static AreaLightInfo* getAreaLightInfo(const ZoneLightID&);
    static s32 getDefaultStepInterpolate();
    static bool tryFindNewAreaLightID(const TVec3f&, ZoneLightID*);

    static void loadActorLightInfo(const ActorLightInfo*);
    static void blendActorLightInfo(ActorLightInfo*, const ActorLightInfo&, const ActorLightInfo&, f32);

    static void getAreaLightLightData(JMapInfo*, int, AreaLightInfo*);
    static const char* getDefaultAreaLightName();

    static void loadPointLightInfo(const PointLightInfo*);

    static void loadLightInfoCoin(const LightInfoCoin*);

    static void registerLightAreaHolder(LightAreaHolder*);

    static void calcLightWorldPos(TVec3f*, const LightInfo&);

    static void registerPlayerLightCtrl(const ActorLightCtrl*);
};
