#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class PartsModel : public LiveActor {
public:
    PartsModel(LiveActor* pHost, const char* pName, const char* pModelName, MtxPtr pMtx, int drawBufferType, bool useLight);
    ~PartsModel() override = default;

    void movement() override;
    void calcAndSetBaseMtx() override;

private:
    /* 0x8C */ LiveActor* mHost = nullptr;
    /* 0x90 */ MtxPtr mMtx = nullptr;
};
