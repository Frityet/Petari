#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/JUtility/JUTNameTab.hpp"
#include <cstring>

// Original model resource post-load routines from Game/Util/ModelUtil.cpp.
namespace MR {
    inline void setShapeVcdVatCmdSelf(J3DShape* shape) {
        void* vcdVatCmd = shape->getVcdVatCmd();
        u8* arr = new (0x20) u8[J3DShape::kVcdVatDLSize];
        copyMemory(arr, vcdVatCmd, J3DShape::kVcdVatDLSize);
        shape->setVcdVatCmd(arr);
    }

    void initEnvelopeAndEnvMapOrProjMapModelData(J3DModelData* pModelData) {
        bool doSort = false;

        for (int i = 0; i < pModelData->getMaterialNum(); i++) {
            J3DMaterial* material = pModelData->getMaterialNodePointer(i);
            if (!isEnvelope(material)) {
                continue;
            }

            bool isUseEnvMap = isUseTexMtxEnvMap(material);
            bool isUseProjMap = isUseTexMtxProjMap(material);
            if (!isUseEnvMap && !isUseProjMap) {
                continue;
            }
            J3DShape* shape = material->getShape();
            setShapeVcdVatCmdSelf(shape);
            doSort = true;
            if (isUseEnvMap) {
                shape->setTexMtxLoadType(0x2000);
            }

            for (u32 idx = 0; idx < 8; idx++) {
                J3DTexCoord* texCoord = material->getTexCoord(idx);
                GXAttr attr = static_cast< GXAttr >(idx + 1);
                if (texCoord->mTexGenSrc == 1) {
                    shape->addTexMtxIndexInDL(attr, 30);
                    shape->addTexMtxIndexInVcd(attr);
                } else if (texCoord->mTexGenSrc == 0) {
                    shape->addTexMtxIndexInDL(attr, 0);
                    shape->addTexMtxIndexInVcd(attr);
                }
            }
        }

        if (doSort) {
            pModelData->mShapeTable.sortVcdVatCmd();
        }
    }

    void downFracVtx(J3DModelData* pModelData) {
        u32 vtxNum = pModelData->getVtxNum();
        GXVtxAttrFmtList* vtxAttrFmtList = pModelData->getVertexData().getVtxAttrFmtList();
        while (vtxAttrFmtList->attr != GX_VA_NULL) {
            if (vtxAttrFmtList->attr == GX_VA_POS) {
                if (vtxAttrFmtList->frac == 0) {
                    return;
                }
                vtxAttrFmtList->frac--;
                break;
            }
            vtxAttrFmtList++;
        }

        s16* vertexPosArray = static_cast< s16* >(pModelData->getVertexData().getVtxPosArray());
        for (u32 i = 0; i < vtxNum * 3; i++) {
            vertexPosArray[i] >>= 1;
        }
        DCStoreRange(vertexPosArray, vtxNum * 6);

        for (u32 i = 0; i < pModelData->getShapeTable()->getShapeNum(); i++) {
            pModelData->getShapeTable()->getShapeNodePointer(i)->makeVcdVatCmd();
        }
    }

    bool isUseFur(const J3DModelData* pModelData) {
        for (u16 i = 0; i < pModelData->getMaterialNum(); i++) {
            if (strstr(pModelData->getMaterialName()->getName(i), "Fur")) {
                return true;
            }
        }
        return false;
    }

}  // namespace MR
