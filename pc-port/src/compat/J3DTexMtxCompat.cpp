#include "JSystem/J3DGraphBase/J3DTevs.hpp"
#include "JSystem/J3DGraphBase/J3DTexture.hpp"
#include "JSystem/J3DGraphBase/J3DGD.hpp"
#include "JSystem/J3DGraphBase/J3DTransform.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

static void J3DGDLoadTexMtxImm(f32 (*)[4], u32, GXTexMtxType);
static void J3DGDLoadPostTexMtxImm(f32 (*)[4], u32);

J3DTexMtxInfo const j3dDefaultTexMtxInfo = {
    0x01, 0x00, 0xFF, 0xFF, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0, 0.0f, 0.0f},
};

#if defined(__clang__)
#pragma clang fp contract(off)
#endif

void J3DTexMtx::load(u32 mtxIdx) const {
    if (j3dSys.checkFlag(J3DSysFlag_PostTexMtx)) {
        loadPostTexMtx(mtxIdx);
    } else {
        loadTexMtx(mtxIdx);
    }
}

void J3DTexMtx::calc(f32 const (*param_0)[4]) {
    calcTexMtx(param_0);
}

void J3DTexMtx::calcTexMtx(const Mtx param_0) {
    Mtx44 mtx1;
    Mtx44 mtx2;

    static Mtx qMtx = {
        0.5f, 0.0f, 0.5f, 0.0f, 0.0f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    };
    static Mtx qMtx2 = {
        0.5f, 0.0f, 0.0f, 0.5f, 0.0f, -0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
    };

    u32 r28 = mTexMtxInfo.mInfo & 0x3f;
    u32 r30 = (mTexMtxInfo.mInfo >> 7) & 1;
    switch (r28) {
    case 8:
    case 9:
    case 11:
        if (r30 == 0) {
            J3DGetTextureMtx(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
        } else if (r30 == 1) {
            J3DGetTextureMtxMaya(mTexMtxInfo.mSRT, mtx2);
        }
        PSMTXConcat(mtx2, qMtx, mtx2);
        J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mtx1);
        PSMTXConcat(mtx1, param_0, mMtx);
        break;
    case 7:
        if (r30 == 0) {
            J3DGetTextureMtx(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx1);
        } else if (r30 == 1) {
            J3DGetTextureMtxMaya(mTexMtxInfo.mSRT, mtx1);
        }
        PSMTXConcat(mtx1, qMtx, mtx1);
        PSMTXConcat(mtx1, param_0, mMtx);
        break;
    case 10:
        if (r30 == 0) {
            J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
        } else if (r30 == 1) {
            J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx2);
        }
        PSMTXConcat(mtx2, qMtx2, mtx2);
        J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mtx1);
        PSMTXConcat(mtx1, param_0, mMtx);
        break;
    case 6:
        if (r30 == 0) {
            J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx1);
        } else if (r30 == 1) {
            J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx1);
        }
        PSMTXConcat(mtx1, qMtx2, mtx1);
        PSMTXConcat(mtx1, param_0, mMtx);
        break;
    case 1:
        if (r30 == 0) {
            J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx1);
        } else if (r30 == 1) {
            J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx1);
        }
        PSMTXConcat(mtx1, param_0, mMtx);
        break;
    case 2:
    case 3:
    case 5:
        if (r30 == 0) {
            J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
        } else if (r30 == 1) {
            J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx2);
        }
        J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mtx1);
        PSMTXConcat(mtx1, param_0, mMtx);
        break;
    case 4:
        if (r30 == 0) {
            J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
        } else if (r30 == 1) {
            J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx2);
        }
        J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mMtx);
        break;
    default:
        if (r30 == 0) {
            J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mMtx);
        } else if (r30 == 1) {
            J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mMtx);
        }
        break;
    }
}

void J3DTexMtx::calcPostTexMtx(const Mtx param_0) {
    Mtx44 mtx1;
    Mtx44 mtx2;

    static Mtx qMtx = {
        0.5f, 0.0f, 0.5f, 0.0f, 0.0f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    };
    static Mtx qMtx2 = {
        0.5f, 0.0f, 0.0f, 0.5f, 0.0f, -0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
    };

    u32 r29 = mTexMtxInfo.mInfo & 0x3f;
    u32 r30 = (mTexMtxInfo.mInfo >> 7) & 1;
    switch (r29) {
    case 8:
    case 11:
        if (r30 == 0) {
            J3DGetTextureMtx(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
        } else if (r30 == 1) {
            J3DGetTextureMtxMaya(mTexMtxInfo.mSRT, mtx2);
        }
        PSMTXConcat(mtx2, qMtx, mtx2);
        J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mtx1);
        PSMTXConcat(mtx1, param_0, mMtx);
        break;
    case 9:
        if (r30 == 0) {
            J3DGetTextureMtx(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
        } else if (r30 == 1) {
            J3DGetTextureMtxMaya(mTexMtxInfo.mSRT, mtx2);
        }
        PSMTXConcat(mtx2, qMtx, mtx2);
        J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mMtx);
        break;
    case 7:
        if (r30 == 0) {
            J3DGetTextureMtx(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx1);
        } else if (r30 == 1) {
            J3DGetTextureMtxMaya(mTexMtxInfo.mSRT, mtx1);
        }
        PSMTXConcat(mtx1, qMtx, mMtx);
        break;
    case 10:
        if (r30 == 0) {
            J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
        } else if (r30 == 1) {
            J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx2);
        }
        PSMTXConcat(mtx2, qMtx2, mtx2);
        J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mtx1);
        PSMTXConcat(mtx1, param_0, mMtx);
        break;
    case 6:
        if (r30 == 0) {
            J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx1);
        } else if (r30 == 1) {
            J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx1);
        }
        PSMTXConcat(mtx1, qMtx2, mMtx);
        break;
    case 1:
        if (r30 == 0) {
            J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mMtx);
        } else if (r30 == 1) {
            J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mMtx);
        }
        break;
    case 2:
    case 5:
        if (r30 == 0) {
            J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
        } else if (r30 == 1) {
            J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx2);
        }
        J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mtx1);
        PSMTXConcat(mtx1, param_0, mMtx);
        break;
    case 3:
        if (r30 == 0) {
            J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
        } else if (r30 == 1) {
            J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx2);
        }
        J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mMtx);
        break;
    case 4:
        if (r30 == 0) {
            J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
        } else if (r30 == 1) {
            J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx2);
        }
        J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mMtx);
        break;
    default:
        if (r30 == 0) {
            J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mMtx);
        } else if (r30 == 1) {
            J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mMtx);
        }
        break;
    }
}

void J3DTexMtx::loadTexMtx(u32 param_0) const {
    GDOverflowCheck(0x35);
    J3DGDLoadTexMtxImm((MtxPtr)mMtx, param_0 * 3 + 30, (GXTexMtxType)mTexMtxInfo.mProjection);
}

void J3DTexMtx::loadPostTexMtx(u32 param_0) const {
    GDOverflowCheck(0x35);
    J3DGDLoadPostTexMtxImm((MtxPtr)mMtx, param_0 * 3 + 0x40);
}

inline void J3DGDLoadTexMtxImm(f32 (*param_1)[4], u32 param_2, GXTexMtxType param_3) {
    u16 addr = param_2 << 2;
    u8 len = param_3 == GX_MTX2x4 ? 8 : 12;
    J3DGDWriteXFCmdHdr(addr, len);
    J3DGDWrite_f32(param_1[0][0]);
    J3DGDWrite_f32(param_1[0][1]);
    J3DGDWrite_f32(param_1[0][2]);
    J3DGDWrite_f32(param_1[0][3]);
    J3DGDWrite_f32(param_1[1][0]);
    J3DGDWrite_f32(param_1[1][1]);
    J3DGDWrite_f32(param_1[1][2]);
    J3DGDWrite_f32(param_1[1][3]);
    if (param_3 == GX_MTX3x4) {
        J3DGDWrite_f32(param_1[2][0]);
        J3DGDWrite_f32(param_1[2][1]);
        J3DGDWrite_f32(param_1[2][2]);
        J3DGDWrite_f32(param_1[2][3]);
    }
}

inline void J3DGDLoadPostTexMtxImm(f32 (*param_1)[4], u32 param_2) {
    u16 addr = (param_2 - 0x40) * 4 + 0x500;
    int stride = 12;

    J3DGDWriteXFCmdHdr(addr, stride);
    J3DGDWrite_f32(param_1[0][0]);
    J3DGDWrite_f32(param_1[0][1]);
    J3DGDWrite_f32(param_1[0][2]);
    J3DGDWrite_f32(param_1[0][3]);
    J3DGDWrite_f32(param_1[1][0]);
    J3DGDWrite_f32(param_1[1][1]);
    J3DGDWrite_f32(param_1[1][2]);
    J3DGDWrite_f32(param_1[1][3]);
    J3DGDWrite_f32(param_1[2][0]);
    J3DGDWrite_f32(param_1[2][1]);
    J3DGDWrite_f32(param_1[2][2]);
    J3DGDWrite_f32(param_1[2][3]);
}
