// Original MR entry points, copied from the decomp reference.
#include "Game/Screen/ImageEffectDirector.hpp"
#include "Game/Screen/ImageEffectSystemHolder.hpp"
#include "Game/Map/WaterAreaHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/DrawUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "JSystem/JGeometry/TMatrix.hpp"
#include "Game/Util/MathUtil.hpp"

namespace { s32 getScreenHeightInline() { return MR::getScreenHeight(); } }
namespace MR {
    void createNormalBloom() {
        createSceneObj(SceneObj_BloomEffect);
    }

    void createSimpleBloom() {
        createSceneObj(SceneObj_BloomEffectSimple);
    }

    void createScreenBlur() {
        createSceneObj(SceneObj_ScreenBlurEffect);
    }

    void createDepthOfFieldBlur() {
        createSceneObj(SceneObj_DepthOfFieldBlur);
    }

    void turnOnNormalBloom() {
        getImageEffectDirector()->turnOnNormal();
    }

    void turnOnDepthOfField(bool param1) {
        getImageEffectDirector()->turnOnDepthOfField(param1);
    }

    void turnOffImageEffect() {
        getImageEffectDirector()->turnOff();
    }

    void setNormalBloomIntensity(u8 intensity) {
        getImageEffectDirector()->setNormalBloomIntensity(intensity);
    }

    void setNormalBloomThreshold(u8 threshold) {
        getImageEffectDirector()->setNormalBloomThreshold(threshold);
    }

    void setNormalBloomBlurIntensity1(u8 intensity1) {
        getImageEffectDirector()->setNormalBloomBlurIntensity1(intensity1);
    }

    void setNormalBloomBlurIntensity2(u8 intensity2) {
        getImageEffectDirector()->setNormalBloomBlurIntensity2(intensity2);
    }

    void setDepthOfFieldBlurIntensity(f32 intensity) {
        getImageEffectDirector()->setDepthOfFieldIntensity(intensity);
    }

    void turnOffDOFInSubjective() {
        if (isExistImageEffectDirector()) {
            getImageEffectDirector()->turnOffDOFInSubjective();
        }
    }

    void turnOnDOFInSubjective() {
        if (isExistImageEffectDirector()) {
            getImageEffectDirector()->turnOnDOFInSubjective();
        }
    }

    void fillScreenSetup(const GXColor& rColor) {
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        TMtx34f mtxImm;
        mtxImm.identity();
        GXLoadPosMtxImm(mtxImm, GX_PNMTX0);
        GXSetCurrentMtx(GX_PNMTX0);
        Mtx44 projMtx;
        C_MTXOrtho(projMtx, 0.0f, getScreenHeightInline(), 0.0f, MR::getFrameBufferWidth(), -1.0f, 1.0f);
        GXSetProjection(projMtx, GX_ORTHOGRAPHIC);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
        GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
        GXSetZCompLoc(GX_FALSE);
        GXSetCullMode(GX_CULL_NONE);
        GXSetDither(GX_FALSE);
        GXSetNumTexGens(0);
        GXSetNumTevStages(1);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
        GXSetTevColor(GX_TEVREG0, rColor);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    }

    void fillScreenArea(const TVec2s& rMin, const TVec2s& rMax) {
        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        {
            u16 maxX = rMax.x;
            u16 minX = rMin.x;
            u16 minY = rMin.y;
            u16 maxY = rMax.y;

            GXPosition2u16(minX, minY);
            GXPosition2u16(maxX, minY);
            GXPosition2u16(maxX, maxY);
            GXPosition2u16(minX, maxY);
        }
        GXEnd();
    }

    void fillScreen(const GXColor& color) {
        fillScreenSetup(color);
        u16 width = getFrameBufferWidth();
        u16 height = getScreenHeight();
        fillScreenArea(TVec2s(0, 0), TVec2s(width, height));
    }

    bool isCameraInWater() { return WaterAreaFunction::isCameraInWaterForCameraUtil(); }
}

namespace MR {
    u8 lerp(u8 start, u8 end, f32 t) {
        return JGeometry::TUtil< f32 >::clamp(start + (end - start) * t, 0.0f, 255.0f);
    }

    GXColor lerp(GXColor start, GXColor end, f32 t) {
        u8 a = lerp(start.a, end.a, t);
        u8 b = lerp(start.b, end.b, t);
        u8 g = lerp(start.g, end.b, t);
        u8 r = lerp(start.r, end.r, t);

        GXColor color = {r, g, b, a};

        return color;
    }

}
