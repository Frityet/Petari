#pragma once

#include "Game/AreaObj/AreaObj.hpp"

class ZoneLightID;

class LightAreaHolder : public AreaObjMgr {
public:
    LightAreaHolder(s32, const char*);

    ~LightAreaHolder() override;
    virtual void initAfterPlacement();

    bool tryFindLightID(const TVec3f&, ZoneLightID*) const;
    void sort();
};
