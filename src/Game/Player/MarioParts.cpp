#include "Game/Player/MarioParts.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ModelUtil.hpp"

MarioParts::MarioParts(LiveActor* pHost, const char* pName, const char* pModelName, bool usePlayerDecoration, MtxPtr pFixedMtx, MtxPtr pBaseMtx)
    : PartsModel(pHost, pName, pModelName, pBaseMtx, usePlayerDecoration ? MR::DrawBufferType_CrystalBox : MR::DrawBufferType_PlayerDecoration, true),
      _9C(nullptr) {
    MR::initDLMakerFog(this, true);
    MR::newDifferedDLBuffer(this);

    if (MR::getLightNumMax(this) > 0) {
        MR::initLightCtrl(this);
    }

    if (pFixedMtx != nullptr) {
        initFixedPosition(pFixedMtx, TVec3f(0.0f, 0.0f, 0.0f), TVec3f(0.0f, 0.0f, 0.0f));
        offFixedPosNormalizeScale();
    }
}

MarioParts::MarioParts(LiveActor* pHost, const char* pName, const char* pModelName, MtxPtr)
    : PartsModel(pHost, pName, pModelName, nullptr, 0x17, true), _9C(nullptr) {
    MR::initDLMakerFog(this, true);
    MR::newDifferedDLBuffer(this);

    if (MR::getLightNumMax(this) > 0) {
        MR::initLightCtrl(this);
    }
}

void MarioParts::init(const JMapInfoIter&) {
    initEffectKeeper(8, _9C, false);
    initSound(8, false);
    MR::invalidateClipping(this);
    makeActorAppeared();
    makeActorAppeared();
}

MarioParts::~MarioParts() {
}
