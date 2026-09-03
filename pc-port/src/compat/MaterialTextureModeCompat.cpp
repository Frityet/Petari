#include "Game/Util/ModelUtil.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"

// Original material texture-mode predicates from Game/Util/ModelUtil.cpp.
namespace MR {
    bool isEnvelope(J3DMaterial* pMaterial) {
        return pMaterial->getShape()->mHasPNMTXIdx;
    }

    bool isUseTexMtx(J3DMaterial* pMaterial, u32 idx) {
        J3DTexMtx* texMtx = pMaterial->mTexGenBlock->getTexMtx(idx);

        if (texMtx != nullptr) {
            for (u32 stage = 0; stage < pMaterial->getTevStageNum(); stage++) {
                if (pMaterial->getTevBlock()->getTevOrder(stage)->mTexCoord == idx) {
                    return true;
                }
            }

            for (u32 stage = 0; stage < pMaterial->getIndBlock()->getIndTexStageNum(); stage++) {
                if (pMaterial->getIndBlock()->getIndTexOrder(stage)->mCoord == idx) {
                    return true;
                }
            }
        }

        return false;
    }

    bool isUseTexMtxEnvMap(J3DMaterial* pMaterial) {
        for (u32 idx = 0; idx < 8; idx++) {
            J3DTexMtx* texMtx = pMaterial->mTexGenBlock->getTexMtx(idx);
            if (texMtx == nullptr) {
                continue;
            }

            u32 temp = texMtx->getTexMtxInfo().mInfo & 0x3F;
            if (temp == 1 || temp == 6 || temp == 7) {
                if (!isUseTexMtx(pMaterial, idx)) {
                    return false;
                }

                return true;
            }
        }
        return false;
    }

    bool isUseTexMtxProjMap(J3DMaterial* pMaterial) {
        for (u32 idx = 0; idx < 8; idx++) {
            J3DTexMtx* texMtx = pMaterial->mTexGenBlock->getTexMtx(idx);
            if (texMtx == nullptr) {
                continue;
            }

            u32 temp = texMtx->getTexMtxInfo().mInfo & 0x3F;
            if (temp == 2 || temp == 3 || temp == 8 || temp == 9) {
                return true;
            }
        }

        return false;
    }
}  // namespace MR
