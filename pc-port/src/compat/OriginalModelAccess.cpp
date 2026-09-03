#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphBase/J3DTexture.hpp"


namespace MR {

    void updateModelManager(LiveActor* pActor) {
        pActor->mModelManager->update();
    }

    void calcAnimModelManager(LiveActor* pActor) {
        pActor->mModelManager->calcAnim();
    }

    void updateModelAnimPlayer(LiveActor* pActor) {
        XanimePlayer* pAnimePlayer = pActor->mModelManager->mXanimePlayer;
        pAnimePlayer->updateBeforeMovement();
        pAnimePlayer->updateAfterMovement();
    }

    void invalidateMtxCalc(J3DModelData* pModelData) {
        u16 jointNum = pModelData->getJointNum();
        for (u16 i = 0; i < jointNum; i++) {
            pModelData->getJointNodePointer(i)->setMtxCalc(nullptr);
        }
    }

    void invalidateJointCallback(J3DModelData* pModelData) {
        u16 jointNum = pModelData->getJointNum();
        for (u16 i = 0; i < jointNum; i++) {
            pModelData->getJointNodePointer(i)->setCallBack(nullptr);
        }
    }

    J3DModel* getJ3DModel(const LiveActor* pActor) {
        if (pActor->mModelManager == nullptr) {
            return nullptr;
        }
        return pActor->mModelManager->getJ3DModel();
    }

    void calcJ3DModel(LiveActor* pActor) {
        OSLockMutex(&MR::MutexHolder< 0 >::sMutex);
        getJ3DModel(pActor)->calc();
        OSUnlockMutex(&MR::MutexHolder< 0 >::sMutex);
    }

    J3DModelData* getJ3DModelData(const LiveActor* pActor) {
        if (pActor->mModelManager == nullptr) {
            return nullptr;
        }
        return pActor->mModelManager->getJ3DModelData();
    }

    s16 getBckFrameMax(const LiveActor* pActor, const char* pBckName) {
        return static_cast< J3DAnmBase* >(getResourceHolder(pActor)->mMotionResTable->getRes(pBckName))->mFrameMax;
    }

    s16 getBrkFrameMax(const LiveActor* pActor, const char* pBrkName) {
        return static_cast< J3DAnmBase* >(getResourceHolder(pActor)->mBrkResTable->getRes(pBrkName))->mFrameMax;
    }

    s16 getBvaFrameMax(const LiveActor* pActor, const char* pBvaName) {
        return static_cast< J3DAnmBase* >(getResourceHolder(pActor)->mBvaResTable->getRes(pBvaName))->mFrameMax;
    }

    bool isBckPlaying(XanimePlayer* pAnimePlayer, const char* pBckName) {
        return !pAnimePlayer->isTerminate() && pAnimePlayer->isRun(pBckName) && pAnimePlayer->getRate() != 0.0f;
    }

    u16 getMaterialNo(J3DModel* pModel, const char* pMaterialName) {
        // needs to be two lines for correct inlining behaviour
        JUTNameTab* materialName = pModel->getModelData()->getMaterialName();
        return materialName->getIndex(pMaterialName);
    }

    J3DMaterial* getMaterial(J3DModelData* pModelData, int idx) {
        return pModelData->getMaterialNodePointer(idx);
    }

    J3DMaterial* getMaterial(J3DModel* pModel, int idx) {
        return pModel->mModelData->getMaterialNodePointer(idx);
    }

    J3DMaterial* getMaterial(const LiveActor* pActor, int idx) {
        return getJ3DModelData(pActor)->getMaterialNodePointer(idx);
    }

    s32 getMaterialNum(J3DModel* pModel) {
        return pModel->mModelData->getMaterialNum();
    }

    const char* getMaterialName(const J3DModelData* pModelData, int idx) {
        return pModelData->getMaterialName()->getName(idx);
    }

    void updateModelDiffDL(LiveActor* pActor) {
        pActor->mModelManager->updateDL(true);
    }

    void hideMaterial(J3DModel* pModel, const char* pMaterialName) {
        J3DShapePacket* pckt = pModel->getMatPacket(getMaterialNo(pModel, pMaterialName))->getShapePacket();
        pckt->mFlags |= 0x10;
    }

    void hideMaterial(const LiveActor* pActor, const char* pMaterialName) {
        hideMaterial(getJ3DModel(pActor), pMaterialName);
    }

    void showMaterial(J3DModel* pModel, const char* pMaterialName) {
        J3DShapePacket* pckt = pModel->getMatPacket(getMaterialNo(pModel, pMaterialName))->getShapePacket();
        pckt->mFlags &= ~0x10;
    }

    void showMaterial(const LiveActor* pActor, const char* pMaterialName) {
        showMaterial(getJ3DModel(pActor), pMaterialName);
    }

    ResTIMG* getResTIMG(const LiveActor* pActor, int idx) {
        return getResTIMG(getJ3DModelData(pActor), idx);
    }

    ResTIMG* getResTIMG(const J3DModelData* pModelData, int idx) {
        return pModelData->mMaterialTable.getTexture()->getResTIMG(idx);
    }

    void copyJointAnimation(J3DModel* pCopyTo, J3DModel* pCopyFrom) {
        u16 jointNum = pCopyTo->getModelData()->getJointNum();
        for (u16 i = 0; i < jointNum; i++) {
            pCopyTo->setAnmMtx(i, pCopyFrom->getAnmMtx(i));
        }

        u16 wEvlpMtxNum = pCopyTo->getModelData()->getWEvlpMtxNum();
        for (u16 i = 0; i < wEvlpMtxNum; i++) {
            pCopyTo->setWeightAnmMtx(i, pCopyFrom->getWeightAnmMtx(i));
        }
    }

    void copyJointAnimation(LiveActor* pCopyTo, const LiveActor* pCopyFrom) {
        return copyJointAnimation(getJ3DModel(pCopyTo), getJ3DModel(pCopyFrom));
    }

    void syncJointAnimation(LiveActor* pActor1, const LiveActor* pActor2) {
        getJ3DModel(pActor1)->mMtxBuffer = getJ3DModel(pActor2)->getMtxBuffer();
    }

    void syncMaterialAnimation(J3DModel* pModel1, J3DModel* pModel2) {
        u16 materialNum = pModel1->getModelData()->getMaterialNum();
        u16 i;
        u16 j;
        u16 idx;
        for (i = 0; i < materialNum; i++) {
            J3DShapePacket* shapePacketModel2 = pModel2->getShapePacket(i);
            idx = shapePacketModel2->getShape()->getMaterial()->getIndex();
            for (j = 0; j < materialNum; j++) {
                J3DShapePacket* shapePacketModel1 = pModel1->getShapePacket(j);
                if (shapePacketModel1->getShape()->getMaterial()->getIndex() == idx) {
                    shapePacketModel1->setDisplayListObj(shapePacketModel2->getDisplayListObj());
                    break;
                }
            }
        }
    }

    void syncMaterialAnimation(LiveActor* pActor1, const LiveActor* pActor2) {
        return syncMaterialAnimation(getJ3DModel(pActor1), getJ3DModel(pActor2));
    }

}  // namespace MR
