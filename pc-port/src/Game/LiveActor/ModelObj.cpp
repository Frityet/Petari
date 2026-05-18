#include "Game/LiveActor/ModelObj.hpp"

#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/LiveActorUtil.hpp"

ModelObj::ModelObj(const char *pName, const char *pModelName, MtxPtr pMtx, int drawBufferType, int movementType, int calcAnimType, bool useScale)
    : LiveActor(pName), mMtx(pMtx) {
    if (drawBufferType < -1) {
        drawBufferType = MR::DrawBufferType_MapObj;
    }
    if (movementType < -1) {
        movementType = MR::MovementType_MapObjDecoration;
    }
    if (calcAnimType < -1) {
        calcAnimType = MR::CalcAnimType_MapObjDecoration;
    }

    initModelManagerWithAnm(pModelName, nullptr, useScale);
    MR::connectToScene(this, movementType, calcAnimType, drawBufferType, -1);
    initSound(8, false);
    initEffectKeeper(8, nullptr, false);

    if (mMtx != nullptr) {
        mPosition.set(mMtx[0][3], mMtx[1][3], mMtx[2][3]);
    }
}

void ModelObj::init(const JMapInfoIter &) {
    makeActorAppeared();
}

void ModelObj::calcAndSetBaseMtx() {
    if (mMtx != nullptr) {
        mPosition.set(mMtx[0][3], mMtx[1][3], mMtx[2][3]);
        MR::setBaseTRMtx(this, mMtx);
        return;
    }

    LiveActor::calcAndSetBaseMtx();
}
