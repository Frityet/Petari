#include "Game/LiveActor/PartsModel.hpp"

#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/LiveActorUtil.hpp"

PartsModel::PartsModel(LiveActor* pHost, const char* pName, const char* pModelName, MtxPtr pMtx, int drawBufferType, bool useLight)
    : LiveActor(pName), mHost(pHost), mMtx(pMtx) {
    initModelManagerWithAnm(pModelName, nullptr, useLight);
    if (drawBufferType < 0) {
        drawBufferType = MR::DrawBufferType_MapObj;
    }
    MR::connectToScene(this, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, drawBufferType, -1);
    makeActorAppeared();
}

void PartsModel::movement() {
    if (mHost == nullptr || MR::isDead(mHost)) {
        makeActorDead();
        return;
    }

    LiveActor::movement();
}

void PartsModel::calcAndSetBaseMtx() {
    if (mMtx != nullptr) {
        MR::setBaseTRMtx(this, mMtx);
        return;
    }

    LiveActor::calcAndSetBaseMtx();
}
