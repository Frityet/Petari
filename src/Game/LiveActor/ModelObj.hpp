#pragma once

#include <revolution.h>

#include "Game/LiveActor/LiveActor.hpp"

class ModelObj : public LiveActor {
public:
    ModelObj(const char *pName, const char *pModelName, MtxPtr pMtx, int drawBufferType, int movementType, int calcAnimType, bool useScale);

    void init(const JMapInfoIter &rIter) override;
    void calcAndSetBaseMtx() override;

    /* 0x8C */ MtxPtr mMtx;
};
