#include "JSystem/J3DGraphBase/J3DVertex.hpp"

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

J3DDrawMtxData::J3DDrawMtxData() {
    mEntryNum = 0;
    mDrawMtxFlag = NULL;
    mDrawMtxIndex = NULL;
}

J3DDrawMtxData::~J3DDrawMtxData() {
}
