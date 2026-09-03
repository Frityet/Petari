#include "Game/Util/ModelUtil.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/LiveActor/DisplayListMaker.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/Player/MarioActor.hpp"
#include <JSystem/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
namespace MR {
u16 getMaterialNo(J3DModelData* pModelData, const char* pMaterialName) {
        return pModelData->getMaterialName()->getIndex(pMaterialName);
    }
J3DMaterial* getMaterial(J3DModelData* pModelData, const char* pMaterialName) {
        return pModelData->getMaterialNodePointer(getMaterialNo(pModelData, pMaterialName));
    }
bool isUseTex(J3DMaterial* pMaterial, u16 a2) {
        for (u32 idx = 0; idx < 8; idx++) {
            if (pMaterial->getTexNo(idx) != a2) {
                continue;
            }

            for (u32 stage = 0; stage < pMaterial->getTevStageNum(); stage++) {
                if (pMaterial->getTevBlock()->getTevOrder(stage)->mTexMap == idx) {
                    return true;
                }
            }
        }

        return false;
    }
bool isNormalTexMtx(J3DMaterial* pMaterial) {
        for (u32 idx = 0; idx < 8; idx++) {
            J3DTexMtx* texMtx = pMaterial->mTexGenBlock->getTexMtx(idx);

            if (texMtx != nullptr && (texMtx->getTexMtxInfo().mInfo & 0x3F) != 0 && isUseTexMtx(pMaterial, idx)) {
                return false;
            }
        }

        return true;
    }
TVec3f* getPlayerShadowRotate() {
        return &MarioAccess::getPlayerActor()->_A18;
    }
    ProjmapEffectMtxSetter* initDLMakerProjmapEffectMtxSetter(LiveActor* pActor) {
        return pActor->mModelManager->mDisplayListMaker->addProjmapEffectMtxSetter();
    }
J3DModel* getJ3DModel(const LiveActor* pActor) {
        if (pActor->mModelManager == nullptr) {
            return nullptr;
        }
        return pActor->mModelManager->getJ3DModel();
    }
J3DMaterial* getMaterial(J3DModel* pModel, int idx) {
        return pModel->mModelData->getMaterialNodePointer(idx);
    }
s32 getMaterialNum(J3DModel* pModel) {
        return pModel->mModelData->getMaterialNum();
    }
const char* getMaterialName(const J3DModelData* pModelData, int idx) {
        return pModelData->getMaterialName()->getName(idx);
    }
ResourceHolder* getModelResourceHolder(const LiveActor* pActor) {
        if (pActor->mModelManager != nullptr) {
            return pActor->mModelManager->getModelResourceHolder();
        }

        return nullptr;
    }
const char* getModelResName(const LiveActor* pActor) {
        return getModelResourceHolder(pActor)->mModelResTable->getResName(0U);
    }
}
