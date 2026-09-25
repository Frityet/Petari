#pragma once

#include <revolution.h>

#include "Game/LiveActor/LiveActor.hpp"

class ActorJointCtrl;
class LodCtrl;

class ModelObj : public LiveActor {
public:
    ModelObj(const char *pName, const char *pModelName, MtxPtr pMtx, int drawBufferType, int movementType, int calcAnimType, bool useScale);

    void init(const JMapInfoIter &rIter) override;
    void calcAndSetBaseMtx() override;

    /* 0x8C */ MtxPtr mMtx;
};

class ModelObjNpc : public LiveActor {
public:
    ModelObjNpc(const char*, const char*, MtxPtr);

    virtual ~ModelObjNpc();
    virtual void init(const JMapInfoIter&) override;
    virtual void control() override;
    virtual void calcAndSetBaseMtx() override;

    /* 0x8C */ MtxPtr mMtx;
    /* 0x90 */ LodCtrl* mLodCtrl;
    /* 0x94 */ ActorJointCtrl* mJointCtrl;
};
