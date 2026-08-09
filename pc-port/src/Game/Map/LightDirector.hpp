#pragma once

#include "Game/Map/LightDataHolder.hpp"
#include "Game/Map/LightPointCtrl.hpp"
#include "Game/Map/LightZoneDataHolder.hpp"
#include "Game/NameObj/NameObj.hpp"

class ActorLightCtrl;
class ResourceHolder;

class LightDirector : public NameObj {
public:
    LightDirector();

    ~LightDirector() override;
    void init(const JMapInfoIter&) override;
    void movement() override;

    void initData();
    void loadLightPlayer() const;
    void loadLightCoin() const;

    u32 _C = 0U;
    LightDataHolder* mHolder = nullptr;
    LightZoneDataHolder* mZoneHolder = nullptr;
    AreaLightInfo* _18 = nullptr;
    const ActorLightCtrl* _1C = nullptr;
    LightPointCtrl* mPointCtrl = nullptr;
    ResourceHolder* mResourceHolder = nullptr;
};
