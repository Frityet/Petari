#include "JSystem/J3DGraphBase/J3DVertex.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

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

void J3DVertexBuffer::setVertexData(J3DVertexData* pVertexData) {
    mVtxData = pVertexData;
    mVtxPosArray[0] = pVertexData->getVtxPosArray();
    mVtxNrmArray[0] = pVertexData->getVtxNrmArray();
    mVtxColArray[0] = pVertexData->getVtxColorArray(0);
    mVtxPosArray[1] = nullptr;
    mVtxNrmArray[1] = nullptr;
    mVtxColArray[1] = nullptr;
    mTransformedVtxPosArray[0] = pVertexData->getVtxPosArray();
    mTransformedVtxNrmArray[0] = pVertexData->getVtxNrmArray();
    mTransformedVtxPosArray[1] = nullptr;
    mTransformedVtxNrmArray[1] = nullptr;
    frameInit();
}

void J3DVertexBuffer::init() {
    mVtxData = nullptr;
    mVtxPosArray[0] = mVtxPosArray[1] = nullptr;
    mVtxNrmArray[0] = mVtxNrmArray[1] = nullptr;
    mVtxColArray[0] = mVtxColArray[1] = nullptr;
    mTransformedVtxPosArray[0] = mTransformedVtxPosArray[1] = nullptr;
    mTransformedVtxNrmArray[0] = mTransformedVtxNrmArray[1] = nullptr;
    mCurrentVtxPos = nullptr;
    mCurrentVtxNrm = nullptr;
    mCurrentVtxCol = nullptr;
    frameInit();
}

J3DVertexBuffer::~J3DVertexBuffer() {
}

void J3DVertexBuffer::setArray() const {
    j3dSys.setVtxPos(mCurrentVtxPos);
    j3dSys.setVtxNrm(mCurrentVtxNrm);
    j3dSys.setVtxCol(mCurrentVtxCol);
}
