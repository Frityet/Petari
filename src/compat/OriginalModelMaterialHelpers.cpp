#include "Game/Util/ModelUtil.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/LiveActor/DisplayListMaker.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/Player/MarioActor.hpp"
#include <JSystem/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3DGraphBase/J3DTexture.hpp>
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

    ProjmapEffectMtxSetter* initDLMakerProjmapEffectMtxSetter(LiveActor* pActor) {
        return pActor->mModelManager->mDisplayListMaker->addProjmapEffectMtxSetter();
    }

}

namespace MR {
    void initDLMakerMatColor0(LiveActor* pActor, const char* pMatName, const J3DGXColor* pColor) {
        pActor->mModelManager->mDisplayListMaker->addMatColorCtrl(pMatName, 0, pColor);
    }

    void initDLMakerChangeTex(LiveActor* pActor, const char* pTexName) {
        J3DModelData* pModelData = getJ3DModelData(pActor);
        DisplayListMaker* pDLMaker = pActor->mModelManager->mDisplayListMaker;

        for (u16 texIndex = 0; texIndex < pModelData->mMaterialTable.getTexture()->getNum(); ++texIndex) {
            if (!MR::isEqualString(pModelData->mMaterialTable.getTextureName()->getName(texIndex), pTexName)) {
                continue;
            }

            for (u16 matIndex = 0; matIndex < pModelData->mMaterialTable.getMaterialNum(); ++matIndex) {
                if (isUseTex(pModelData->mMaterialTable.getMaterialNodePointer(matIndex), texIndex)) {
                    pDLMaker->onPrgFlag(matIndex, 0x4020000);
                }
            }
        }
    }
}
