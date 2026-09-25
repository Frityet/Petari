#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/ClippingActorHolder.hpp"
#include <aurora/exception.hpp>
#include "Game/Util/LiveActorUtil.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/LiveActor/DisplayListMaker.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include <JSystem/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3DGraphBase/J3DTexture.hpp>
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/PartsModel.hpp"
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "render/J3dMatrix.hpp"

#include "runtime/RuntimeContext.hpp"

namespace MR {

    void validateClipping(LiveActor* pActor) {
        if (pActor->mFlag.mIsInvalidClipping) {
            MR::getClippingDirector()->mActorHolder->validateClipping(pActor);
        }
    }

    void invalidateClipping(LiveActor* pActor) {
        if (pActor->mFlag.mIsInvalidClipping) {
            if (pActor->mFlag.mIsClipped) {
                pActor->endClipped();
            }
        } else {
            MR::getClippingDirector()->mActorHolder->invalidateClipping(pActor);
        }
    }

    void setClippingFarMax(LiveActor* pActor) {
        MR::getClippingDirector()->mActorHolder->setFarClipLevel(pActor, 0);
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

    PartsModel* createPartsModelNpc(LiveActor* pHost, const char* pName, const char* pModelName, MtxPtr pMtx) {
        PartsModel* pModel = new PartsModel(pHost, pName, pModelName, pMtx, MR::DrawBufferType_NPC, false);
        pModel->initWithoutIter();
        pModel->_99 = true;
        return pModel;
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

    bool isInvalidClipping(const LiveActor* pActor) {
        return pActor->mFlag.mIsInvalidClipping;
    }

    bool isClipped(const LiveActor* pActor) {
        return pActor != nullptr && pActor->mFlag.mIsClipped;
    }

    bool isNoEntryDrawBuffer(const LiveActor* pActor) {
        return pActor == nullptr || pActor->mFlag.mIsHiddenModel;
    }

    void onEntryDrawBuffer(LiveActor* pActor) {
        if (!isNoEntryDrawBuffer(pActor)) {
            return;
        }

        if (!isDead(pActor) && !pActor->mFlag.mIsClipped) {
            connectToDrawTemporarily(pActor);
        }

        pActor->mFlag.mIsHiddenModel = false;
    }

    void offEntryDrawBuffer(LiveActor* pActor) {
        if (isNoEntryDrawBuffer(pActor)) {
            return;
        }

        if (!isDead(pActor) && !pActor->mFlag.mIsClipped) {
            disconnectToDrawTemporarily(pActor);
        }

        pActor->mFlag.mIsHiddenModel = true;
    }

    void showModel(LiveActor* pActor) {
        if (isNoCalcAnim(pActor)) {
            onCalcAnim(pActor);
        }

        if (isNoCalcView(pActor)) {
            pActor->mFlag.mIsNoCalcView = false;
        }

        if (isNoEntryDrawBuffer(pActor)) {
            onEntryDrawBuffer(pActor);
        }
    }

    void hideModel(LiveActor* pActor) {
        if (!isNoCalcAnim(pActor)) {
            offCalcAnim(pActor);
        }

        if (!isNoCalcView(pActor)) {
            pActor->mFlag.mIsNoCalcView = true;
        }

        if (!isNoEntryDrawBuffer(pActor)) {
            offEntryDrawBuffer(pActor);
        }
    }

    bool isNoCalcAnim(const LiveActor* pActor) {
        return pActor != nullptr && pActor->mFlag.mIsNoCalcAnim;
    }

    bool isNoCalcView(const LiveActor* pActor) {
        return pActor->mFlag.mIsNoCalcView;
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
            aurora::throw_host_exception<std::invalid_argument>("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() == step;
    }

    bool isFirstStep(const LiveActor* pActor) {
        return isStep(pActor, 0);
    }

    bool isLessStep(const LiveActor* pActor, s32 step) {
        if (pActor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() < step;
    }

    bool isLessEqualStep(const LiveActor* pActor, s32 step) {
        if (pActor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() <= step;
    }

    bool isGreaterStep(const LiveActor* pActor, s32 step) {
        if (pActor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() > step;
    }

    bool isGreaterEqualStep(const LiveActor* pActor, s32 step) {
        if (pActor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() >= step;
    }

    bool isIntervalStep(const LiveActor* pActor, s32 step) {
        return pActor->getNerveStep() % step == 0;
    }

    bool isNewNerve(const LiveActor* pActor) {
        if (pActor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("A LiveActor nerve query requires a real actor.");
        }
        return pActor->getNerveStep() < 0;
    }

    f32 calcNerveRate(const LiveActor* pActor, s32 stepMax) {
        if (pActor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("A LiveActor nerve rate requires a real actor.");
        }
        return stepMax <= 0
                   ? 1.0F
                   : std::clamp(static_cast<f32>(pActor->getNerveStep()) / static_cast<f32>(stepMax),
                                0.0F, 1.0F);
    }

    f32 calcNerveEaseInRate(const LiveActor* pActor, s32 stepMax) {
        return getEaseInValue(calcNerveRate(pActor, stepMax), 0.0f, 1.0f, 1.0f);
    }

    void setNerveAtStep(LiveActor* pActor, const Nerve* pNerve, s32 step) {
        if (pActor != nullptr && pActor->getNerveStep() == step) {
            pActor->setNerve(pNerve);
        }
    }















}  // namespace MR

namespace MR {
    void calcWallNormalHorizontal(TVec3f* pVec, const LiveActor* pActor) {
        const TVec3f* normal = getWallNormal(pActor);
        const TVec3f* grav = &pActor->mGravity;
        pVec->killElement(*normal, *grav);
    }
}

namespace MR {
    bool isBinded(const LiveActor* pActor) {
        if (isBindedGround(pActor) || isBindedRoof(pActor) || isBindedWall(pActor)) {
            return true;
        }

        return false;
    }

    TVec3f* getBindedFixReactionVector(const LiveActor* pActor) {
        return &pActor->mBinder->mFixReactionVector;
    }
}

namespace MR {
    bool changeShowModelFlagSyncNearClipping(LiveActor* pActor, f32 nearClip) {
        if (MR::isJudgedToNearClip(pActor->mPosition, nearClip)) {
            MR::hideModelAndOnCalcAnimIfShown(pActor);
            return false;
        }

        MR::showModelIfHidden(pActor);
        return true;
    }

    void showModelIfHidden(LiveActor* pActor) {
        if (isHiddenModel(pActor)) {
            showModel(pActor);
        }
    }

    void hideModelIfShown(LiveActor* pActor) {
        if (isHiddenModel(pActor)) {
            return;
        }

        hideModel(pActor);
    }

    void hideModelAndOnCalcAnimIfShown(LiveActor* pActor) {
        if (isHiddenModel(pActor)) {
            return;
        }

        hideModel(pActor);
        onCalcAnim(pActor);
    }

    f32 calcNerveEaseInValue(const LiveActor* pActor, s32 stepMax, f32 valueStart, f32 valueEnd) {
        return getEaseInValue(calcNerveRate(pActor, stepMax), valueStart, valueEnd, 1.0f);
    }

    f32 calcNerveEaseOutValue(const LiveActor* pActor, s32 stepMax, f32 valueStart, f32 valueEnd) {
        return getEaseOutValue(calcNerveRate(pActor, stepMax), valueStart, valueEnd, 1.0f);
    }
}  // namespace MR

// LiveActorUtil remains excluded; these original methods stay here until its full import.
namespace MR {
    const GXColor* getLightAmbientColor(const LiveActor* pActor) {
        return &pActor->mActorLightCtrl->getActorLight()->mColor;
    }

    bool isExistIndirectTexture(const LiveActor* pActor) {
        const char* name = "IndDummy";
        return MR::getJ3DModelData(pActor)->mMaterialTable.mTextureName->getIndex(name) != -1;
    }

    void initDLMakerMatColor0(LiveActor* pActor, const char* pMatName, const J3DGXColor* pColor) {
        pActor->mModelManager->mDisplayListMaker->addMatColorCtrl(pMatName, 0, pColor);
    }

    void initDLMakerChangeTex(LiveActor* pActor, const char* pTexName) {
        J3DModelData* pModelData = getJ3DModelData(pActor);
        DisplayListMaker* pDLMaker = pActor->mModelManager->mDisplayListMaker;

        for (u16 texIndex = 0; texIndex < pModelData->mMaterialTable.getTexture()->getNum(); texIndex++) {
            if (!MR::isEqualString(pModelData->mMaterialTable.getTextureName()->getName(texIndex), pTexName)) {
                continue;
            }

            for (u16 matIndex = 0; matIndex < pModelData->mMaterialTable.getMaterialNum(); matIndex++) {
                if (isUseTex(pModelData->mMaterialTable.getMaterialNodePointer(matIndex), texIndex)) {
                    pDLMaker->onPrgFlag(matIndex, 0x4020000);
                }
            }
        }
    }

    ProjmapEffectMtxSetter* initDLMakerProjmapEffectMtxSetter(LiveActor* pActor) {
        return pActor->mModelManager->mDisplayListMaker->addProjmapEffectMtxSetter();
    }
}  // namespace MR
