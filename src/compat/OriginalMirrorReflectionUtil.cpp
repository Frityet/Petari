#include "Game/LiveActor/DisplayListMaker.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/MirrorCamera.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include <JSystem/J3DGraphAnimator/J3DModelData.hpp>
#include <JSystem/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3DGraphBase/J3DTexture.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <cstring>

namespace MR {
    void initMirrorReflection(LiveActor* pActor) {
        initDLMakerChangeTex(pActor, "MirrorTex");
        pActor->mModelManager->newDifferedDLBuffer();
        changeModelDataTexAll(pActor, "MirrorTex", *MR::getScreenResTIMG());
        pActor->mModelManager->mDisplayListMaker->addMirrorReflectionMtxSetter();
    }

    void setMirrorReflectionInfoFromMtxYUp(const TPos3f& rMtx) {
        TVec3f up;
        TVec3f pos;
        f32 upZ = rMtx.mMtx[2][1];
        f32 upY = rMtx.mMtx[1][1];
        up.set< f32 >(rMtx.mMtx[0][1], upY, upZ);

        f32 posZ = rMtx.mMtx[2][3];
        f32 posY = rMtx.mMtx[1][3];
        pos.set< f32 >(rMtx.mMtx[0][3], posY, posZ);

        MR::getMirrorCamera()->setMirrorMapInfo(up, pos);
    }

    void setMirrorReflectionInfoFromModel(LiveActor* pActor) {
        J3DModelData* pModelData = MR::getJ3DModelData(pActor);
        MR::getMirrorCamera()->setMirrorMapInfo(pModelData);
    }

    void changeModelDataTexAll(LiveActor* pActor, const char* pTexName, const ResTIMG& rTimg) {
        J3DModelData* pModelData = MR::getJ3DModelData(pActor);
        DisplayListMaker* pDLMaker = pActor->mModelManager->mDisplayListMaker;

        for (u16 texIndex = 0; texIndex < pModelData->mMaterialTable.getTexture()->getNum(); texIndex++) {
            const char* name = pModelData->mMaterialTable.getTextureName()->getName(texIndex);
            if (strcmp(name, pTexName) != 0) {
                continue;
            }

            J3DTexture* pTexture = pModelData->mMaterialTable.getTexture();
            pTexture->setResTIMG(texIndex, rTimg);

            for (u16 matIndex = 0; matIndex < pModelData->mMaterialTable.getMaterialNum(); matIndex++) {
                J3DMaterial* material = pModelData->mMaterialTable.getMaterialNodePointer(matIndex);
                if (MR::isUseTex(material, texIndex)) {
                    pDLMaker->onCurFlag(matIndex, 0x4020000);
                }
            }
        }
    }
}
