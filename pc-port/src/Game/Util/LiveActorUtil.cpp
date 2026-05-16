#include "Game/Util/LiveActorUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/compat/RuntimeContext.hpp"

ProjmapEffectMtxSetter::ProjmapEffectMtxSetter(LiveActor* pActor) : mActor(pActor) {
}

void ProjmapEffectMtxSetter::updateMtxUseBaseMtx() {
    if (mActor == nullptr) {
        return;
    }
}

namespace MR {

    ProjmapEffectMtxSetter* initDLMakerProjmapEffectMtxSetter(LiveActor* pActor) {
        return new ProjmapEffectMtxSetter(pActor);
    }

    void invalidateClipping(LiveActor*) {
    }

    void startBck(LiveActor* pActor, const char* pName, const char* pFileName) {
        if (pActor != nullptr) {
            pActor->startBck(pName, pFileName);
        }
    }

    void startBtk(LiveActor* pActor, const char* pName) {
        if (pActor != nullptr) {
            pActor->startBtk(pName);
        }
    }

    void setBaseTRMtx(LiveActor* pActor, const smgpc::game::J3dMatrix3x4& matrix) {
        if (pActor != nullptr) {
            pActor->setBaseMatrix(matrix);
        }
    }

    bool isDead(const LiveActor* pActor) {
        return pActor == nullptr || pActor->isDead();
    }

    bool isStep(const LiveActor* pActor, s32 step) {
        return pActor != nullptr && pActor->getNerveStep() == step;
    }

    bool isFirstStep(const LiveActor* pActor) {
        return isStep(pActor, 0);
    }

}  // namespace MR
