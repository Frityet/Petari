#include "Game/Util/LiveActorUtil.hpp"

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/PartsModel.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Scene/SceneFunction.hpp"
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

    void startBrk(LiveActor* pActor, const char* pName) {
        if (pActor != nullptr) {
            pActor->startBrk(pName);
        }
    }

    void startBtk(LiveActor* pActor, const char* pName) {
        if (pActor != nullptr) {
            pActor->startBtk(pName);
        }
    }

    void startAction(LiveActor* pActor, const char* pName) {
        if (pActor != nullptr) {
            pActor->startBck(pName, nullptr);
            pActor->startBrk(pName);
            pActor->startBtk(pName);
        }
    }

    void setBrkFrame(LiveActor* pActor, f32 frame) {
        if (pActor != nullptr) {
            pActor->setBrkFrame(frame);
        }
    }

    void setBrkFrameAndStop(LiveActor* pActor, f32 frame) {
        if (pActor != nullptr) {
            pActor->setBrkFrameAndStop(frame);
        }
    }

    void setBrkFrameEndAndStop(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->setBrkFrameEndAndStop();
        }
    }

    J3DFrameCtrl* getBrkCtrl(LiveActor* pActor) {
        return pActor != nullptr ? pActor->getBrkCtrl() : nullptr;
    }

    void setBaseTRMtx(LiveActor* pActor, const smgpc::game::J3dMatrix3x4& matrix) {
        if (pActor != nullptr) {
            pActor->setBaseMatrix(matrix);
        }
    }

    PartsModel* createPartsModelMapObj(LiveActor* pHost, const char* pName, const char* pModelName, MtxPtr pMtx) {
        return new PartsModel(pHost, pName, pModelName, pMtx, MR::DrawBufferType_MapObj, false);
    }

    void emitEffect(LiveActor* pActor, const char* pEffectName) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance(); runtime != nullptr && pActor != nullptr && pEffectName != nullptr) {
            runtime->emit_effect(pActor->getName(), pEffectName);
        }
    }

    void deleteEffect(LiveActor* pActor, const char* pEffectName) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance(); runtime != nullptr && pActor != nullptr && pEffectName != nullptr) {
            runtime->delete_effect(pActor->getName(), pEffectName);
        }
    }

    void initLightCtrl(LiveActor* pActor) {
        if (pActor == nullptr) {
            return;
        }

        pActor->initActorLightCtrl();
        pActor->mActorLightCtrl->init(-1, false);
    }

    void initLightCtrlForPlayer(LiveActor* pActor) {
        if (pActor == nullptr) {
            return;
        }

        pActor->initActorLightCtrl();
        pActor->mActorLightCtrl->init(-1, false);
        LightFunction::registerPlayerLightCtrl(pActor->mActorLightCtrl);
    }

    void initLightCtrlNoDrawEnemy(LiveActor* pActor) {
        if (pActor == nullptr) {
            return;
        }

        pActor->initActorLightCtrl();
        pActor->mActorLightCtrl->init(1, true);
    }

    void initLightCtrlNoDrawMapObj(LiveActor* pActor) {
        if (pActor == nullptr) {
            return;
        }

        pActor->initActorLightCtrl();
        pActor->mActorLightCtrl->init(3, true);
    }

    void updateLightCtrl(LiveActor* pActor) {
        if (pActor != nullptr && pActor->mActorLightCtrl != nullptr) {
            pActor->mActorLightCtrl->update(false);
        }
    }

    void updateLightCtrlDirect(LiveActor* pActor) {
        if (pActor != nullptr && pActor->mActorLightCtrl != nullptr) {
            pActor->mActorLightCtrl->update(true);
        }
    }

    void loadActorLight(const LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->loadActorLight();
        }
    }

    ActorLightCtrl* getLightCtrl(const LiveActor* pActor) {
        return pActor == nullptr ? nullptr : pActor->mActorLightCtrl;
    }

    bool isHiddenModel(const LiveActor*) {
        return false;
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

    bool isBtpStopped(const LiveActor*) {
        return true;
    }

    bool isBrkOneTimeAndStopped(const LiveActor* pActor) {
        return pActor == nullptr || pActor->isBrkOneTimeAndStopped();
    }

}  // namespace MR
