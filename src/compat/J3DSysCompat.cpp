#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "JSystem/J3DGraphBase/J3DFifo.hpp"
#include "JSystem/J3DGraphBase/J3DTevs.hpp"

// J3DSys matrix globals remain with the joint-tree implementation.

J3DSys j3dSys;

J3DTexCoordScaleInfo J3DSys::sTexCoordScaleTable[8];

u32 j3dDefaultViewNo;

J3DSys::J3DSys() {
    makeTexCoordTable();
    makeTevSwapTable();
    makeAlphaCmpTable();
    makeZModeTable();

    mFlags = 0;
    PSMTXIdentity(mViewMtx);
    mDrawMode = 1;
    mMaterialMode = 0;
    mModel = NULL;
    mShape = NULL;

    for (int i = 0; i < 2; i++)
        mDrawBuffer[i] = NULL;

    mTexture = NULL;
    mMatPacket = NULL;
    mShapePacket = NULL;
    mModelDrawMtx = NULL;
    mModelNrmMtx = NULL;
    mVtxPos = NULL;
    mVtxNrm = NULL;
    mVtxCol = NULL;

    for (int i = 0; i < 8; i++) {
        sTexCoordScaleTable[i].field_0x00 = 1;
        sTexCoordScaleTable[i].field_0x02 = 1;
        sTexCoordScaleTable[i].field_0x04 = 0;
        sTexCoordScaleTable[i].field_0x06 = 0;
    }
}

void J3DSys::loadPosMtxIndx(int addr, u16 indx) const {
    J3DFifoLoadIndx(0x20, indx, 0xB000 | ((u16)(addr * 0x0C)));
}

void J3DSys::loadNrmMtxIndx(int addr, u16 indx) const {
    J3DFifoLoadNrmMtxIndx3x3(indx, addr * 3);
}
