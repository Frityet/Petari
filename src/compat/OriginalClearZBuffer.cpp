#include "Game/Util/DrawUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include <JSystem/JGeometry/TMatrix.hpp>
#include <JSystem/JUtility/JUTVideo.hpp>
#include <dolphin/gx.h>

static u8 sTexImgObj[] = {0,    0xFF, 0,    0xFF, 0,    0xFF, 0,    0xFF, 0,    0xFF, 0,    0xFF, 0,    0xFF, 0,    0xFF,
                          0,    0xFF, 0,    0xFF, 0,    0xFF, 0,    0xFF, 0,    0xFF, 0,    0xFF, 0,    0xFF, 0,    0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

namespace {
    GXTexObj clear_z_tobj;
}

namespace MR {
    void clearZBuffer() {
        JUTVideo* pJUTVideo = JUTVideo::getManager();
        u16 width = pJUTVideo->getFbWidth();
        u16 height = pJUTVideo->getEfbHeight();
        Mtx44 projMtx;
        C_MTXOrtho(projMtx, 0.0f, height, 0.0f, width, 0.0f, 1.0f);
        GXSetProjection(projMtx, GX_ORTHOGRAPHIC);
        MR::setDefaultViewportAndScissor();
        TMtx34f mtxImm;
        mtxImm.identity();
        GXLoadPosMtxImm(mtxImm, GX_PNMTX0);
        GXSetCurrentMtx(GX_PNMTX0);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_POS_XYZ, GX_U8, 0);
        GXSetNumChans(0);
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
        GXInitTexObj(&::clear_z_tobj, sTexImgObj, 4, 4, GX_TF_Z24X8, GX_REPEAT, GX_REPEAT, GX_FALSE);
        GXInitTexObjLOD(&::clear_z_tobj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
        GXLoadTexObj(&::clear_z_tobj, GX_TEXMAP0);
        GXSetNumTevStages(1);
        GXColor tev0;
        tev0.r = 0;
        tev0.g = 0;
        tev0.b = 0;
        tev0.a = 0;
        GXSetTevColor(GX_TEVREG0, tev0);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
        GXSetZTexture(GX_ZT_REPLACE, GX_TF_Z24X8, 0);
        GXSetZCompLoc(GX_FALSE);
        GXSetBlendMode(GX_BM_NONE, GX_BL_ZERO, GX_BL_ZERO, GX_LO_NOOP);
        GXSetColorUpdate(GX_FALSE);
        GXSetAlphaUpdate(GX_FALSE);
        GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
        GXSetCullMode(GX_CULL_BACK);

        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        {
            GXPosition2u16(0, 0);
            GXTexCoord2u8(0, 0);

            GXPosition2u16(width, 0);
            GXTexCoord2u8(1, 0);

            GXPosition2u16(width, height);
            GXTexCoord2u8(1, 1);

            GXPosition2u16(0, height);
            GXTexCoord2u8(0, 1);
        }
        GXEnd();

        GXSetZTexture(GX_ZT_DISABLE, GX_TF_Z24X8, 0);
        GXSetZCompLoc(GX_TRUE);
        GXSetColorUpdate(GX_TRUE);
    }

}
