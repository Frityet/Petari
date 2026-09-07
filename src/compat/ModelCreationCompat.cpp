#include "Game/Util/ModelUtil.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "Game/Player/J3DModelX.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/System/ShapePacketUserData.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "Game/Util/SchedulerUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include <JSystem/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3DGraphBase/J3DShape.hpp>
#include <cstring>

namespace MR {





J3DModel* newJ3DModel(const ResourceHolder* pResourceHolder, const char* pChar, J3DMdlFlag mdlFlag) {
        J3DModelData* modelData = static_cast< J3DModelData* >(pResourceHolder->mModelResTable->getRes(pChar));
        J3DModel* newModel;
        invalidateMtxCalc(modelData);
        invalidateJointCallback(modelData);

        for (u16 i = 0; i < modelData->getShapeNum(); i++) {
            modelData->getShapeNodePointer(i)->mFlags &= ~0x1;
        }

        if (MR::isEqualString(pChar, "Mario") || MR::isEqualString(pChar, "Luigi") || MR::isEqualString(pChar, "MarioHandL") ||
            MR::isEqualString(pChar, "MarioHandR") || MR::isEqualString(pChar, "MarioFace") || MR::isEqualString(pChar, "LuigiFace") ||
            MR::isEqualString(pChar, "MarioShadow") || MR::isEqualString(pChar, "LuigiShadow") || MR::isEqualString(pChar, "IceMario") ||
            MR::isEqualString(pChar, "IceLuigi") || MR::isEqualString(pChar, "IceMarioHandL") || MR::isEqualString(pChar, "IceMarioHandR") ||
            MR::isEqualString(pChar, "InvincibleMarioHandL") || MR::isEqualString(pChar, "InvincibleMarioHandR") ||
            MR::isEqualString(pChar, "InvincibleMario") || MR::isEqualString(pChar, "InvincibleLuigi") || MR::isEqualString(pChar, "BeeMario") ||
            MR::isEqualString(pChar, "BeeLuigi") || MR::isEqualString(pChar, "HopperMario") || MR::isEqualString(pChar, "HopperLuigi") ||
            MR::isEqualString(pChar, "BoneMario") || MR::isEqualString(pChar, "BoneLuigi") || MR::isEqualString(pChar, "SearchLightCone") ||
            strstr(pChar, "FlexibleSphere") || strstr(pChar, "FlexibleSandPlanetParts") || MR::isEqualString(pChar, "ScaleDownRelayPlanet") ||
            MR::isEqualString(pChar, "GhostMario") || MR::isEqualString(pChar, "GhostLuigi") || MR::isEqualString(pChar, "Koura")) {
            OSLockMutex(&MR::MutexHolder< 0 >::sMutex);
            newModel = static_cast< J3DModel* >(new J3DModelX(modelData, mdlFlag, 1));
            OSUnlockMutex(&MR::MutexHolder< 0 >::sMutex);
        } else {
            OSLockMutex(&MR::MutexHolder< 0 >::sMutex);
            newModel = new J3DModel(modelData, mdlFlag, 1);
            OSUnlockMutex(&MR::MutexHolder< 0 >::sMutex);
        }
        initJ3DShapePacketUserData(newModel);
        Mtx mtx;
        PSMTXIdentity(mtx);
        newModel->unlock();
        OSLockMutex(&MR::MutexHolder< 0 >::sMutex);
        PSMTXCopy(mtx, j3dSys.mViewMtx);
        newModel->calc();
        newModel->calcMaterial();
        newModel->viewCalc();
        newModel->viewCalc();
        OSUnlockMutex(&MR::MutexHolder< 0 >::sMutex);
        if (modelData->getModelDataType() == 0) {
            ProhibitSchedulerAndInterrupts prohibit(false);
            OSLockMutex(&MR::MutexHolder< 0 >::sMutex);
            newModel->makeDL();
            OSUnlockMutex(&MR::MutexHolder< 0 >::sMutex);
        }
        newModel->lock();
        return newModel;
    }

XanimePlayer* newXanimePlayer(const ResourceHolder* pModelResourceHolder, const char* pChar, const ResourceHolder* pUnused, J3DMdlFlag mdlFlag,
                                  XanimeResourceTable* pAnimeResourceTable) {
        J3DModel* newModel = newJ3DModel(pModelResourceHolder, pChar, mdlFlag);
        return new XanimePlayer(newModel, pAnimeResourceTable);
    }

XanimeResourceTable* newXanimeResourceTable(ResourceHolder* pResourceHolder) {
        return new XanimeResourceTable(pResourceHolder);
    }

}
