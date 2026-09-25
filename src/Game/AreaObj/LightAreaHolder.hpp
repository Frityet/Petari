#pragma once

#include "Game/AreaObj/AreaObj.hpp"

class ZoneLightID;
class LightDirector;

class LightAreaHolder : public AreaObjMgr {
public:
    LightAreaHolder(s32, const char*);

    ~LightAreaHolder() override;
    virtual void initAfterPlacement();

    bool tryFindLightID(const TVec3f&, ZoneLightID*) const;
    void sort();

    // Native retirement clears the director's borrowed area manager.
    LightDirector* mRegisteredLightDirector = nullptr;
};
