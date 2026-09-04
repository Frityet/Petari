#pragma once

#include "Game/LiveActor/LiveActor.hpp"
#include "JSystem/JGeometry/TVec.hpp"

class LodCtrl;
class ModelObj;
class PartsModel;

class PlanetMap : public LiveActor {
public:
    PlanetMap(const char*, const char* = nullptr);

    virtual ~PlanetMap();
    virtual void init(const JMapInfoIter&);
    virtual void makeActorAppeared();
    virtual void makeActorDead();
    virtual void startClipped();
    virtual void endClipped();
    virtual void control();
    virtual f32 getFarClipDistance() const;
    virtual s32 getLowMovementType() const;

    void initClipping(const JMapInfoIter&);
    void initModel(const char*, const JMapInfoIter&);
    void initBloomModel(const char*);
    bool tryEmitMyEffect();
    bool tryDeleteMyEffect();

    /* 0x8C */ const char* mModelName;
    TVec3f _90;
    /* 0x9C */ LodCtrl* mLODCtrl;
    /* 0xA0 */ ModelObj* mBloomModel;
    /* 0xA4 */ PartsModel* mWaterModel;
    /* 0xA8 */ PartsModel* mIndirectModel;
};

class FurPlanetMap : public PlanetMap {
public:
    FurPlanetMap(const char*);

    virtual ~FurPlanetMap();
    virtual void init(const JMapInfoIter&);
};

class RailPlanetMap : public PlanetMap {
public:
    RailPlanetMap(const char*);

    virtual ~RailPlanetMap();
    virtual void init(const JMapInfoIter&);
};

class PlanetMapAnimLow : public PlanetMap {
public:
    PlanetMapAnimLow(const char* pName) : PlanetMap(pName, nullptr) {}

    virtual ~PlanetMapAnimLow();
    virtual s32 getLowMovementType() const;
};

struct PlanetMapClippingInfo {
    const char* mName;
    f32 mRadius;
    Vec mCenterOffset;
};
