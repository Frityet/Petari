#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "JSystem/J3DGraphBase/J3DFifo.hpp"

// Original J3DSys construction and J3DTevs table builders. The remaining
// J3DSys matrix globals are supplied by J3DJointTreeCompat.cpp.

static u8 j3dTexCoordTable[7623];

void makeTexCoordTable() {
    u8 texMtx[] = {
        GX_TEXMTX0, GX_TEXMTX1, GX_TEXMTX2, GX_TEXMTX3, GX_TEXMTX4, GX_TEXMTX5, GX_TEXMTX6, GX_TEXMTX7, GX_TEXMTX8, GX_TEXMTX9, GX_IDENTITY,
    };

    u8* table = j3dTexCoordTable;
    for (u32 i = 0; i < 11; i++) {
        for (u32 j = 0; j < 21; j++) {
            for (int k = 0; k < 0xB; k++) {
                u32 idx = j * 11 + i * 0xe7 + k;
                table[idx * 3 + 0] = i;
                table[idx * 3 + 1] = j;
                table[idx * 3 + 2] = texMtx[k];
            }
        }
    }
}

u8 j3dTevSwapTableTable[1024];

u8 j3dAlphaCmpTable[768];

void makeAlphaCmpTable() {
    u8* table = j3dAlphaCmpTable;
    for (u32 i = 0; i < 8; i++) {
        for (int j = 0; j < 4; j++) {
            for (u32 k = 0; k < 8; k++) {
                u32 idx = i * 32 + j * 8 + k;
                table[idx * 3] = i;
                table[idx * 3 + 1] = j;
                table[idx * 3 + 2] = k;
            }
        }
    }
}

u8 j3dZModeTable[96];

void makeZModeTable() {
    u8* table = j3dZModeTable;
    for (int i = 0; i < 2; i++) {
        for (u32 j = 0; j < 8; j++) {
            for (int k = 0; k < 2; k++) {
                u32 idx = j * 2 + i * 16 + k;
                table[idx * 3 + 0] = i;
                table[idx * 3 + 1] = j;
                table[idx * 3 + 2] = k;
            }
        }
    }
}

void makeTevSwapTable() {
    u8* table = j3dTevSwapTableTable;
    int i = 0;
    do {
        table[0] = i >> 6;
        table[1] = (i >> 4) & 3;
        table[2] = (i >> 2) & 3;
        table[3] = i & 3;
        i++;
        table += 4;
    } while (i < 256);
}

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
