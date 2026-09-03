#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"

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
