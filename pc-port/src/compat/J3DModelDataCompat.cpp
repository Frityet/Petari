#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

// Original typed model-data and embedded table lifecycle bodies.
// Correspondence: notes/xanime-core-matrix-calculation-20260903/model-foundation.md.

void J3DModelData::clear() {
    mpRawData = 0;
    mFlags = 0;
    mbHasBumpArray = 0;
    mbHasBillboard = 0;
}

J3DModelData::J3DModelData() {
    clear();
}

J3DModelData::~J3DModelData() {
}

void J3DMaterialTable::clear() {
    mMaterialNum = 0;
    mUniqueMatNum = 0;
    mMaterialNodePointer = NULL;
    mMaterialName = NULL;
    field_0x10 = 0;
    mTexture = NULL;
    mTextureName = NULL;
    field_0x1c = 0;
}

J3DMaterialTable::J3DMaterialTable() {
    mMaterialNum = 0;
    mUniqueMatNum = 0;
    mMaterialNodePointer = NULL;
    mMaterialName = NULL;
    field_0x10 = 0;
    mTexture = NULL;
    mTextureName = NULL;
    field_0x1c = 0;
}

J3DMaterialTable::~J3DMaterialTable() {
}

J3DVertexData::J3DVertexData()
    : mVtxNum(0), mNrmNum(0), mColNum(0), mTexCoordNum(0), mPacketNum(0), mVtxAttrFmtList(nullptr), mVtxPosArray(nullptr), mVtxNrmArray(nullptr),
      mVtxNBTArray(nullptr) {
    for (int i = 0; i < 2; i++) {
        mVtxColorArray[i] = nullptr;
    }

    for (int i = 0; i < 8; i++) {
        mVtxTexCoordArray[i] = nullptr;
    }

    mVtxPosFrac = 0;
    mVtxPosType = GX_F32;
    mVtxNrmFrac = 0;
    mVtxNrmType = GX_F32;
}

void J3DModelData::syncJ3DSysFlags() const {
    if (checkFlag(0x20)) {
        j3dSys.onFlag(J3DSysFlag_PostTexMtx);
    } else {
        j3dSys.offFlag(J3DSysFlag_PostTexMtx);
    }
}

s32 J3DModelData::newSharedDisplayList(u32 mdlFlags) {
    u16 matNum = getMaterialNum();

    for (u16 i = 0; i < matNum; i++) {
        s32 ret;
        if (mdlFlags & J3DMdlFlag_UseSingleDL) {
            ret = getMaterialNodePointer(i)->newSingleSharedDisplayList(getMaterialNodePointer(i)->countDLSize());
            if (ret != kJ3DError_Success)
                return ret;
        } else {
            ret = getMaterialNodePointer(i)->newSharedDisplayList(getMaterialNodePointer(i)->countDLSize());
            if (ret != kJ3DError_Success)
                return ret;
        }
    }

    return kJ3DError_Success;
}

void J3DModelData::indexToPtr() {
    j3dSys.setTexture(getTexture());

    static BOOL sInterruptFlag = OSDisableInterrupts();
    OSDisableScheduler();

    GDLObj gdl_obj;
    u16 matNum = getMaterialNum();
    for (u16 i = 0; i < matNum; i++) {
        J3DMaterial* matNode = getMaterialNodePointer(i);
        J3DDisplayListObj* dl_obj = matNode->getSharedDisplayListObj();

        GDInitGDLObj(&gdl_obj, dl_obj->getDisplayList(0), dl_obj->getDisplayListSize());
        GDSetCurrent(&gdl_obj);
        matNode->getTevBlock()->indexToPtr();
    }

    GDSetCurrent(NULL);
    OSEnableScheduler();
    OSRestoreInterrupts(sInterruptFlag);
}
