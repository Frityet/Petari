#include "JSystem/J3DGraphBase/J3DVertex.hpp"

// Original RMGK01 lifecycle and attachment bodies, recovered in root first.
// Correspondence: notes/j3d-vertex-buffer-lifecycle-20260903/README.md.

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
