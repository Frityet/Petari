#include "Game/Util/DrawUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include <JSystem/JUtility/JUTVideo.hpp>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>

namespace MR {
    void clearAlphaBuffer(u8 alpha) {
        JUTVideo* pVideo = JUTVideo::getManager();
        TVec2f size(pVideo->getFbWidth(), pVideo->getEfbHeight());
        TVec2f position(0.0f, 0.0f);
        clearAlphaBuffer(alpha, position, size);
    }

    void clearAlphaBuffer(u8 alpha, const TVec2f& rPosition, const TVec2f& rSize) {
        u16 width = rSize.x;
        u16 height = rSize.y;
        Mtx44 projection;
        s32 screenHeight = MR::getScreenHeight();
        C_MTXOrtho(projection, 0.0f, screenHeight, 0.0f, MR::getFrameBufferWidth(), -1.0f, 1.0f);
        GXSetProjection(projection, GX_ORTHOGRAPHIC);
        GXSetCurrentMtx(GX_PNMTX0);
        Mtx matrix;
        PSMTXIdentity(matrix);
        GXLoadPosMtxImm(matrix, GX_PNMTX0);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GXSetNumTexGens(0);
        GXSetNumTevStages(1);
        GXSetTevDirect(GX_TEVSTAGE0);
        GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        GXSetCoPlanar(GX_FALSE);
        GXSetClipMode(GX_CLIP_DISABLE);
        GXSetCullMode(GX_CULL_NONE);
        GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
        GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
        GXSetBlendMode(GX_BM_NONE, GX_BL_ZERO, GX_BL_ZERO, GX_LO_COPY);
        GXSetColorUpdate(GX_FALSE);
        GXSetAlphaUpdate(GX_TRUE);
        GXSetDstAlpha(GX_TRUE, alpha);
        GXBegin(GX_TRIANGLES, GX_VTXFMT0, 6);
        GXPosition3f32(rPosition.x, rPosition.y, 0.0f);
        GXPosition3f32(width + rPosition.x, rPosition.y, 0.0f);
        GXPosition3f32(width + rPosition.x, height + rPosition.y, 0.0f);
        GXPosition3f32(rPosition.x, rPosition.y, 0.0f);
        GXPosition3f32(width + rPosition.x, height + rPosition.y, 0.0f);
        GXPosition3f32(rPosition.x, height + rPosition.y, 0.0f);
        GXEnd();
        GXSetDstAlpha(GX_FALSE, 0);
        GXSetClipMode(GX_CLIP_ENABLE);
    }
}
