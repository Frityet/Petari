#include "Game/Util/LiveActorUtil.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/PartsModel.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "render/J3dMatrix.hpp"

#include "runtime/RuntimeContext.hpp"

namespace MR {

    void validateClipping(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mFlag.mIsInvalidClipping = false;
        }
    }

    void invalidateClipping(LiveActor* pActor) {
        if (pActor == nullptr) {
            return;
        }
        if (pActor->mFlag.mIsClipped) {
            pActor->endClipped();
        }
        pActor->mFlag.mIsInvalidClipping = true;
    }

    void setClippingFarMax(LiveActor* pActor) {
        smgpc::compat::configure_actor_clipping_far_level(pActor, 0);
    }













































    PartsModel* createPartsModelMapObj(LiveActor* pHost, const char* pName, const char* pModelName, MtxPtr pMtx) {
        PartsModel* pModel = new PartsModel(pHost, pName, pModelName, pMtx, MR::DrawBufferType_MapObj, false);
        pModel->initWithoutIter();
        return pModel;
    }

    PartsModel* createPartsModelNoSilhouettedMapObj(LiveActor* pHost, const char* pName, const char* pModelName, MtxPtr pMtx) {
        PartsModel* pModel = new PartsModel(pHost, pName, pModelName, pMtx, MR::DrawBufferType_NoSilhouettedMapObj, false);
        pModel->initWithoutIter();
        return pModel;
    }

    void emitEffect(LiveActor* pActor, const char* pEffectName) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pActor != nullptr && pEffectName != nullptr) {
            runtime->emit_effect(pActor->getName(), pEffectName, pActor);
        }
    }

    void deleteEffect(LiveActor* pActor, const char* pEffectName) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pActor != nullptr && pEffectName != nullptr) {
            runtime->delete_effect(pActor->getName(), pEffectName, pActor);
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
            if (pActor->mActorLightCtrl != nullptr) {
                pActor->mActorLightCtrl->loadLight();
            }
        }
    }

    ActorLightCtrl* getLightCtrl(const LiveActor* pActor) {
        return pActor == nullptr ? nullptr : pActor->mActorLightCtrl;
    }

    void initDefaultPos(LiveActor* pActor, const JMapInfoIter& rIter) {
        if (pActor == nullptr || !rIter.isValid()) {
            return;
        }

        (void)MR::getJMapInfoTrans(rIter, &pActor->mPosition);
        (void)MR::getJMapInfoRotate(rIter, &pActor->mRotation);
        (void)MR::getJMapInfoScale(rIter, &pActor->mScale);
        pActor->mRotation.x = MR::repeat(pActor->mRotation.x, 0.0F, 360.0F);
        pActor->mRotation.y = MR::repeat(pActor->mRotation.y, 0.0F, 360.0F);
        pActor->mRotation.z = MR::repeat(pActor->mRotation.z, 0.0F, 360.0F);
    }

    void initDefaultPosNoRepeat(LiveActor* pActor, const JMapInfoIter& rIter) {
        if (pActor == nullptr || !rIter.isValid()) {
            return;
        }

        (void)MR::getJMapInfoTrans(rIter, &pActor->mPosition);
        (void)MR::getJMapInfoRotate(rIter, &pActor->mRotation);
        (void)MR::getJMapInfoScale(rIter, &pActor->mScale);
    }

    bool isHiddenModel(const LiveActor* pActor) {
        return pActor == nullptr || pActor->mFlag.mIsHiddenModel;
    }

    bool isClipped(const LiveActor* pActor) {
        return pActor != nullptr && pActor->mFlag.mIsClipped;
    }

    bool isNoEntryDrawBuffer(const LiveActor* pActor) {
        return pActor == nullptr || pActor->mFlag.mIsHiddenModel;
    }

    void onEntryDrawBuffer(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mFlag.mIsHiddenModel = false;
        }
    }

    void offEntryDrawBuffer(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mFlag.mIsHiddenModel = true;
        }
    }

    void showModel(LiveActor* pActor) {
        onEntryDrawBuffer(pActor);
    }

    void hideModel(LiveActor* pActor) {
        offEntryDrawBuffer(pActor);
    }

    bool isNoCalcAnim(const LiveActor* pActor) {
        return pActor != nullptr && pActor->mFlag.mIsNoCalcAnim;
    }

    void offCalcAnim(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mFlag.mIsNoCalcAnim = true;
        }
    }

    bool isDead(const LiveActor* pActor) {
        return pActor == nullptr || pActor->mFlag.mIsDead;
    }

    bool isStep(const LiveActor* pActor, s32 step) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() == step;
    }

    bool isFirstStep(const LiveActor* pActor) {
        return isStep(pActor, 0);
    }

    bool isLessStep(const LiveActor* pActor, s32 step) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() < step;
    }

    bool isLessEqualStep(const LiveActor* pActor, s32 step) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() <= step;
    }

    bool isGreaterStep(const LiveActor* pActor, s32 step) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() > step;
    }

    bool isGreaterEqualStep(const LiveActor* pActor, s32 step) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() >= step;
    }

    bool isNewNerve(const LiveActor* pActor) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve query requires a real actor.");
        }
        return pActor->getNerveStep() < 0;
    }

    f32 calcNerveRate(const LiveActor* pActor, s32 stepMax) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve rate requires a real actor.");
        }
        return stepMax <= 0
                   ? 1.0F
                   : std::clamp(static_cast<f32>(pActor->getNerveStep()) / static_cast<f32>(stepMax),
                                0.0F, 1.0F);
    }

    f32 calcNerveEaseInRate(const LiveActor *pActor, s32 stepMax) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve rate requires a real actor.");
        }
        const auto rate = stepMax <= 0
                              ? 1.0F
                              : std::clamp(static_cast<f32>(pActor->getNerveStep()) /
                                               static_cast<f32>(stepMax),
                                           0.0F, 1.0F);
        constexpr auto cHalfPi = 1.57079632679489661923F;
        return 1.0F - std::cos(rate * cHalfPi);
    }

    void setNerveAtStep(LiveActor* pActor, const Nerve* pNerve, s32 step) {
        if (pActor != nullptr && pActor->getNerveStep() == step) {
            pActor->setNerve(pNerve);
        }
    }















}  // namespace MR
