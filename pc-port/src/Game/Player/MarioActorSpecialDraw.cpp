#include "Game/Player/DLchanger.hpp"
#include "Game/Player/J3DModelX.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioState.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Screen/FullScreenBlur.hpp"
#include "Game/Player/MarioParts.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/FurMulti.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/SchedulerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include <JSystem/JGeometry/TBox.hpp>
#include <JSystem/JKernel/JKRHeap.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <revolution/gd/GDBase.h>
#include <revolution/gd/GDGeometry.h>
#include <revolution/gd/GDLight.h>
#include <revolution/gd/GDPixel.h>
#include <revolution/gd/GDTev.h>
#include <revolution/gx/GXEnum.h>
#include <revolution/gx/GXFrameBuf.h>
#include <revolution/gx/GXPixel.h>
#include <revolution/gx/GXCull.h>
#include <revolution/gx/GXTransform.h>
#include <revolution/gx/GXTexture.h>
#include <revolution/gx/GXGeometry.h>
#include <revolution/gx/GXManage.h>
#include <revolution/gx/GXVert.h>
#include <revolution/os/OSCache.h>

void MarioActor::initScreenBox() {
    _B44 = new (32) u8[0x20000];
    mScreenBoxMin.y = 0.0f;
    mScreenBoxMin.x = 0.0f;
    mScreenBoxMax.y = 0.0f;
    mScreenBoxMax.x = 0.0f;
    mScreenBoxPosition.y = 0.0f;
    mScreenBoxPosition.x = 0.0f;
    mScreenBoxSize.y = 0.0f;
    mScreenBoxSize.x = 0.0f;
}

bool MarioActor::isUseScreenBox() const {
    if (_A08 == 3) {
        return true;
    }

    return !(_A08 - 7);
}

void MarioActor::calcScreenBoxRange() {
    if (!isUseScreenBox()) {
        return;
    }

    TVec3f center;
    center = _B18;
    TVec2f screenCenter, screenX, screenY, screenZ;
    MR::calcScreenPosition(&screenCenter, center);
    TVec3f offsetX(80.0f, 0.0f, 0.0f);
    TVec3f pointX(center);
    pointX += offsetX;
    MR::calcScreenPosition(&screenX, pointX);
    TVec3f offsetY(0.0f, 80.0f, 0.0f);
    TVec3f pointY(center);
    pointY += offsetY;
    MR::calcScreenPosition(&screenY, pointY);
    TVec3f offsetZ(0.0f, 0.0f, 80.0f);
    TVec3f pointZ(center);
    pointZ += offsetZ;
    MR::calcScreenPosition(&screenZ, pointZ);
    TVec3f extent((screenX - screenCenter).length(), (screenY - screenCenter).length(), (screenZ - screenCenter).length());
    f32 radius = extent.length();
    TVec2f minimum = screenCenter - TVec2f(radius, radius);
    TVec2f radius2(radius, radius);
    TVec2f maximum = screenCenter + radius2;
    mScreenBoxMin.x = minimum.x < maximum.x ? minimum.x : maximum.x;
    mScreenBoxMax.x = minimum.x > maximum.x ? minimum.x : maximum.x;
    mScreenBoxMin.y = minimum.y < maximum.y ? minimum.y : maximum.y;
    mScreenBoxMax.y = minimum.y > maximum.y ? minimum.y : maximum.y;

    TVec2f bufferMin, bufferMax;
    MR::convertScreenPosToFrameBufferPos(&bufferMin, mScreenBoxMin);
    MR::convertScreenPosToFrameBufferPos(&bufferMax, mScreenBoxMax);
    s16 x = static_cast< s32 >(bufferMin.x) & ~1;
    s16 y = static_cast< s32 >(bufferMin.y) & ~1;
    s16 width = static_cast< s32 >(bufferMax.x) - x;
    s16 height = static_cast< s32 >(bufferMax.y) - y;
    width = (width + 1) & ~1;
    height = (height + 1) & ~1;
    if (width > 256)
        width = 256;
    if (height > 256)
        height = 256;
    if (width == 0)
        width = 2;
    if (height == 0)
        height = 2;
    mScreenBoxPosition.x = x;
    mScreenBoxPosition.y = y;
    mScreenBoxSize.x = width;
    mScreenBoxSize.y = height;

    TBox2f box(mScreenBoxPosition.x, mScreenBoxPosition.y, mScreenBoxPosition.x + mScreenBoxSize.x, mScreenBoxPosition.y + mScreenBoxSize.y);
    TBox2f screen(0.0f, 0.0f, MR::getFrameBufferWidth(), MR::getScreenHeight());
    TBox2f clipped(box);
    if (!clipped.intersect(screen)) {
        clipped.i.set(0.0f);
        clipped.f.set(0.0f);
    }
    mScreenBoxPosition.x = clipped.i.x;
    mScreenBoxPosition.y = clipped.i.y;
    mScreenBoxSize.x = clipped.getWidth();
    mScreenBoxSize.y = clipped.getHeight();
}

void MarioActor::captureScreenBox() const {
    if (!isUseScreenBox()) {
        return;
    }
    if (mScreenBoxSize.x < 1.0f || mScreenBoxSize.y < 1.0f) {
        return;
    }

    GXSetTexCopyDst(mScreenBoxSize.x, mScreenBoxSize.y, GX_TF_RGB565, GX_FALSE);
    GXSetTexCopySrc(mScreenBoxPosition.x, mScreenBoxPosition.y, mScreenBoxSize.x, mScreenBoxSize.y);
    GXSetColorUpdate(GX_FALSE);
    GXSetAlphaUpdate(GX_TRUE);

    GXColor clear = {};

    GXSetCopyClear(clear, 0xFFFFFF);
    GXCopyTex(_B44, GX_TRUE);
    GXSetColorUpdate(GX_ENABLE);
    GXSetAlphaUpdate(GX_FALSE);
    GXSetDstAlpha(GX_FALSE, 0);
}

void MarioActor::writeBackScreenBox() const {
    if (!isUseScreenBox()) {
        return;
    }
    if (mScreenBoxSize.x < 1.0f || mScreenBoxSize.y < 1.0f) {
        return;
    }
    TDDraw::setup(1, 0, 2);
    GXTexObj texture;
    GXInitTexObj(&texture, _B44, mScreenBoxSize.x, mScreenBoxSize.y, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&texture, GX_TEXMAP0);
    TVec2f minimum, maximum;
    MR::convertFrameBufferPosToScreenPos(&minimum, mScreenBoxPosition);
    MR::convertFrameBufferPosToScreenPos(&maximum, mScreenBoxPosition + mScreenBoxSize + TVec2f(0.99f, 0.99f));
    GXSetBlendMode(GX_BM_BLEND, GX_BL_INVDSTALPHA, GX_BL_DSTALPHA, GX_LO_NOOP);
    GXSetDstAlpha(GX_TRUE, 0);
    GXSetAlphaUpdate(GX_TRUE);
    GXPixModeSync();
    GXTexModeSync();
    s16 right = static_cast< s32 >(maximum.x) + 1;
    right &= ~1;
    maximum.x = right;
    GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(minimum.x, minimum.y, 0.9999f);
    GXTexCoord2f32(0.0f, 0.0f);
    GXPosition3f32(maximum.x, minimum.y, 0.9999f);
    GXTexCoord2f32(1.0f, 0.0f);
    GXPosition3f32(maximum.x, maximum.y, 0.9999f);
    GXTexCoord2f32(1.0f, 1.0f);
    GXPosition3f32(minimum.x, maximum.y, 0.9999f);
    GXTexCoord2f32(0.0f, 1.0f);
    GXEnd();
    GXSetDstAlpha(GX_FALSE, 0);
    GXSetAlphaUpdate(GX_FALSE);
    TDDraw::close();
}


void MarioActor::calc1stPersonView() {
    f32 length = (mCamPos - mPosition).length();
    f32 minVal = 300.0f;
    f32 maxVal = 1000.0f;
    f32 val;

    if (length > maxVal) {
        val = 1.0f;
    } else if (length < minVal) {
        val = 0.0f;
    } else {
        val = 1.0f - (maxVal - length) / (maxVal - minVal);
    }

    u8 compareVal = val * 255.0f;
    if (compareVal == 0) {
        _1A1 = true;
        hideBeeFur();
    } else {
        _1A1 = false;
        updateAlphaDL(compareVal);
    }
}

void MarioActor::hideBeeFur() {
    if (mMario->isPlayerModeBee()) {
        _9E8->kill();
        _9EC->offDraw(0xFFFFFFFF);
    }
    if (getCarrySensor()) {
        MR::hideModel(getCarrySensor()->mHost);
    }
    if (mMario->isPlayerModeInvincible()) {
        _A6E = 0;
        stopEffect("無敵中");
    }
}

void MarioActor::calcFogLighting() {
    Color8 fog(240, 16, 80, 0);
    if (!mMario->isStatusActive(11) && isNeedDamageFog()) {
        f32 rate = ((_1A8 + 31) & 63) / 63.0f;
        if (rate > 0.5f)
            rate = 1.0f - rate;
        _1A4 = (2.0f * rate * (mConst->getTable()->mDamageFogHigh - mConst->getTable()->mDamageFogLow) + mConst->getTable()->mDamageFogLow) / 255.0f;
    } else if (_1AA != 0) {
        fog.mGXColor = _1B0.mGXColor;
        if (_1B5) {
            fog.g = 255.0f * MR::sin(PI * (_1AA / 20.0f));
        }
        _1A4 = _1AC * MR::sin(PI * (_1AA / static_cast< f32 >(mConst->getTable()->mStarPieceFogTime)));
    } else if (mMario->_434 != 0) {
        fog.set(255, 255, 0, 0);
        _1A4 = 0.8f * MR::sin(0.5f * (PI * (mMario->_434 / static_cast< f32 >(mConst->getTable()->mItemDashTimer))));
    } else {
        resetFog();
    }

    Color8 ambient;
    ambient.mGXColor = GXColor(*MR::getLightAmbientColor(this));
    Color8 material(255, 255, 255, 255);
    updateLightDL(ambient, material, fog, _1A4);
}

void MarioActor::resetFog() {
    // FIXME: contructing a color8, but never using it
    Color8 color(0, 0, 0, 0);
    _1A8 = 0;
    _1A4 = 0.0f;
    _1AA = 0;
}

void MarioActor::updateAlphaDL(u8 alpha) {
    MR::ProhibitSchedulerAndInterrupts prohibit(false);
    u8 displayList[0x100] ATTRIBUTE_ALIGN(32);
    GDLObj obj;
    GDInitGDLObj(&obj, displayList, sizeof(displayList));
    __GDCurrentDL = &obj;
    static const GXColor zero = {0, 0, 0, 0};
    GXColor color = zero;
    color.a = alpha;
    GDSetTevColor(GX_TEVREG2, color);
    GDSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GDSetGenMode2(1, 0, 1, 0, GX_CULL_BACK);
    GDSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
    GDSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
    GDSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GDSetTevAlphaCalcAndSwap(GX_TEVSTAGE0, GX_CA_A2, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV,
                             GX_TEV_SWAP0, GX_TEV_SWAP0);
    GDSetTevColorCalc(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GDSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);
    GDPadCurr32();
    u32 size = (obj.ptr - obj.start + 31) & ~31;
    DLholder* holder = mDLchanger->swap();
    holder->mSize = obj.ptr - obj.start;
    MR::copyMemory(holder->mDL, displayList, size);
    DCStoreRange(holder->mDL, size);
}

void MarioActor::updateSimpleAlphaDL(u8 alpha) {
    MR::ProhibitSchedulerAndInterrupts prohibit(false);
    u8 displayList[0x100] ATTRIBUTE_ALIGN(32);
    GDLObj obj;
    GDInitGDLObj(&obj, displayList, sizeof(displayList));
    __GDCurrentDL = &obj;
    GDSetDstAlpha(GX_TRUE, alpha);
    GDPadCurr32();
    u32 size = (obj.ptr - obj.start + 31) & ~31;
    DLholder* holder = mDLchanger->swap();
    holder->mSize = obj.ptr - obj.start;
    MR::copyMemory(holder->mDL, displayList, size);
    DCStoreRange(holder->mDL, size);
}

void MarioActor::updateReflectAlphaDL(u8 alpha) {
    MR::ProhibitSchedulerAndInterrupts prohibit(false);
    u8 displayList[0x100] ATTRIBUTE_ALIGN(32);
    GDLObj obj;
    GDInitGDLObj(&obj, displayList, sizeof(displayList));
    __GDCurrentDL = &obj;
    static const GXColor zero = {0, 0, 0, 0};
    GXColor color = zero;
    color.a = alpha;
    GDSetTevColor(GX_TEVREG2, color);
    GDSetTevAlphaCalcAndSwap(GX_TEVSTAGE0, GX_CA_A2, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV,
                             GX_TEV_SWAP0, GX_TEV_SWAP0);
    GDPadCurr32();
    u32 size = (obj.ptr - obj.start + 31) & ~31;
    DLholder* holder = mDLchanger->swap();
    holder->mSize = obj.ptr - obj.start;
    MR::copyMemory(holder->mDL, displayList, size);
    DCStoreRange(holder->mDL, size);
}

void MarioActor::updateLightDL(const Color8& ambient, const Color8& material, const Color8& fog, f32 rate) {
    MR::ProhibitSchedulerAndInterrupts prohibit(false);
    u8 displayList[0x200] ATTRIBUTE_ALIGN(32);
    GDLObj obj;
    GDInitGDLObj(&obj, displayList, sizeof(displayList));
    __GDCurrentDL = &obj;
    GDSetChanAmbColor(GX_COLOR0A0, static_cast< GXColor >(ambient));
    GDSetChanMatColor(GX_COLOR0A0, static_cast< GXColor >(material));
    GDSetChanCtrl(GX_COLOR1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
    if (!MR::isNearZero(rate, 0.001f)) {
        f32 nearZ = MR::getNearZ();
        f32 farZ = MR::getFarZ();
        GXColor color = fog.mGXColor;
        f32 start, end;
        MR::calcFogStartEnd(mPosition, rate, &start, &end);
        GDSetFog(GX_FOG_PERSP_LIN, start, end, nearZ, farZ, color);
    }
    GDPadCurr32();
    mCurrDL = 1 - mCurrDL;
    mDLSize = obj.ptr - obj.start;
    u32 size = (mDLSize + 31) & ~31;
    MR::copyMemory(mDL[mCurrDL], displayList, size);
    DCStoreRange(mDL[mCurrDL], size);
}

void MarioActor::createRainbowDL() {
    MR::ProhibitSchedulerAndInterrupts prohibit(false);
    u8 displayList[0x100] ATTRIBUTE_ALIGN(32);
    GDLObj obj;

    for (u32 alpha = 0; alpha < 8; alpha++) {
        for (u32 colorIndex = 0; colorIndex < 8; colorIndex++) {
            _94[colorIndex + alpha * 8] = new DLchanger(32, 1);
            GDInitGDLObj(&obj, displayList, sizeof(displayList));
            __GDCurrentDL = &obj;

            GXColor color = {0, 0, 0, 0};
            if (colorIndex & 1) {
                color.r = 255;
            }
            if (colorIndex & 2) {
                color.g = 255;
            }
            if (colorIndex & 4) {
                color.b = 255;
            }
            if (colorIndex == 0) {
                color.r = 64;
                color.g = 64;
                color.b = 64;
            }
            color.a = (alpha + 1) * 24 + 16;
            GDSetTevColor(GX_TEVREG0, color);
            GDPadCurr32();
            _94[colorIndex + alpha * 8]->setDL(displayList, obj.ptr - obj.start);
        }
    }
}

void MarioActor::drawScreenBlend() const {
    if (mMario->_97C && mMario->_97C->getBlurOffset() != 0.0f) {
        MR::drawFullScreenBlur(mMario->_97C->getBlurOffset());
    }
    if (_1C6) {
        MR::drawFullScreenBlur(_1C8, _1CC, _1D0, _1D1);
    }
    if (mBeeWallWalk) {
        TVec2f low, high, minimum, maximum;
        TDDraw::project2D(&low, mMario->mShadowPos);
        high = low + TVec2f(100.0f, 100.0f);
        low = low + TVec2f(-100.0f, -100.0f);
        if (low.x >= high.x) {
            minimum.x = high.x;
            maximum.x = low.x;
        } else {
            minimum.x = low.x;
            maximum.x = high.x;
        }
        if (low.y >= high.y) {
            minimum.y = high.y;
            maximum.y = low.y;
        } else {
            minimum.y = low.y;
            maximum.y = high.y;
        }
        low = minimum + TVec2f(-10.0f, -10.0f);
        high = maximum + TVec2f(10.0f, 10.0f);
        TDDraw::setup(0, 0, 2);
        GXSetCullMode(GX_CULL_NONE);
        GXSetAlphaUpdate(GX_FALSE);
        GXSetColorUpdate(GX_TRUE);
        GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_INVDSTALPHA, GX_LO_NOOP);
        GXSetDstAlpha(GX_TRUE, 0);
        TDDraw::drawFillBox(low, high, 0);
        GXSetAlphaUpdate(GX_FALSE);
        MR::loadProjectionMtx();
        MR::loadViewMtx();
    }
    if (_BC4) {
        u32 color = 0xFF000080;
        if (mMario->_1C._3) {
            color = 0x00004020;
        }
        TDDraw::setup(0, 1, 2);
        TDDraw::drawFillBox(TVec3f(0.0f, 0.0f, 0.0f), TVec3f(static_cast< f32 >(MR::getScreenWidth()), static_cast< f32 >(MR::getScreenHeight()), 0.0f), color);
        TDDraw::close();
    }
}


void MarioActor::updateRandomTexture(f32 value) {
    _B88 = 1 - _B88;
    u8* pImage = _B80[_B88]->mImage;
    f32 chance = MR::clamp(1.0f - value / 1000.0f, 0.0f, 1.0f);

    for (u32 y = 0; y < 8; y++) {
        for (u32 x = 0; x < 8; x++) {
            s32 intensity = pImage[x + y * 8] >> 4;
            if (MR::getRandom() < chance) {
                intensity += 4;
            } else {
                intensity--;
            }
            pImage[x + y * 8] = MR::clamp(intensity, 0, 15) << 4;
        }
    }
    DCStoreRange(pImage, 64);
}

void MarioActor::drawWallShade(const TVec3f& position, const TVec3f& normal, f32) const {
    f32 radius = 100.0f;
    TDDraw::setup(1, 1, 0);
    GXSetZMode(GX_TRUE, GX_GREATER, GX_FALSE);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    _B80[_B88]->load(GX_TEXMAP0);
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    TVec3f tangent(0.0f, normal.z, -normal.y);
    if (MR::isNearZero(tangent, 0.001f)) {
        tangent.set<f32>(normal.z, 0.0f, normal.x);
    }
    MR::normalizeOrZero(&tangent);
    Mtx rotation;
    PSMTXRotAxisRad(rotation, &normal, 0.3926991f);
    GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, 18);
    GXPosition3f32(position.x - 5.0f * normal.x, position.y - 5.0f * normal.y, position.z - 5.0f * normal.z);
    GXColor1u32(0x80);
    GXTexCoord2f32(0.0f, 0.0f);
    for (u32 i = 0; i <= 16; i++) {
        f32 angle = 2.0f * (i * 0.0625f * PI);
        TVec3f radial(tangent);
        radial.scale(radius);
        TVec3f offset(normal);
        offset.scale(5.0f);
        TVec3f center(position - offset);
        TVec3f vertex(center);
        vertex += radial;
        GXPosition3f32(vertex.x, vertex.y, vertex.z);
        GXColor1u32(1);
        GXTexCoord2f32(10.0f * MR::cos(angle), 10.0f * MR::sin(angle));
        PSMTXMultVecSR(rotation, &tangent, &tangent);
    }
}

void MarioActor::drawSpinInhibit() const {
}

void MarioActor::drawColdWaterDamage() const {
    GXDrawDone();
    GXTexModeSync();
    GXPixModeSync();
    u16 width = MR::getFrameBufferWidth();
    TDDraw::setup(1, 0, 2);
    GXTexObj textures[2];
    u16 current = 0;
    u32 stripHeight = 4;
    GXInitTexObj(&textures[0], mRasterBuffers[0], width, stripHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXInitTexObj(&textures[1], mRasterBuffers[1], width, stripHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXSetLineWidth(6, GX_TO_ZERO);
    GXSetTexCopyDst(width, stripHeight, GX_TF_RGB565, GX_FALSE);
    GXSetTexCopySrc(0, 0, width, stripHeight);
    GXCopyTex(mRasterBuffers[0], GX_FALSE);
    f32 increment = 1.0f / stripHeight;
    f32 phase = 89.0f * MR::sin((6.2831855f * mMario->mSwim->mColdWaterDamageInterval) / 120.0f);
    if (phase > 0.0f) {
        phase = 0.0f;
    }
    for (u32 y = 0; y < MR::getScreenHeight(); y += stripHeight) {
        GXTexModeSync();
        GXPixModeSync();
        GXSetTexCopySrc(0, y + stripHeight, width, stripHeight);
        u32 next = 1 - current;
        GXCopyTex(mRasterBuffers[next], GX_FALSE);
        GXLoadTexObj(&textures[current], GX_TEXMAP0);
        f32 texY = 0.0f;
        for (u32 line = y; line < y + stripHeight; line++) {
            f32 offset = 5.0f * MR::sin(2.0f * (((_37C + line * phase) / MR::getScreenHeight()) * 3.1415927f));
            GXBegin(GX_LINES, GX_VTXFMT0, 2);
            GXPosition3f32(offset, line, 0.0f);
            GXTexCoord2f32(0.0f, texY);
            GXPosition3f32(offset + MR::getScreenWidth(), line, 0.0f);
            GXTexCoord2f32(1.0f, texY);
            GXEnd();
            texY += increment;
        }
        current = next;
    }
    GXDrawDone();
}


void MarioActor::setRasterScroll(s32 i1, s32 i2, s32 i3) {
    // FIXME: regswap
    _1E8 = i2;
    _1E2 = 1;
    _1E4 = _1E4 * 0.9f + i1 * 0.1f;
    _1EC = i3;
}

void MarioActor::updateRasterScroll() {
    if (_1E2) {
        _1E2 = 0;
        return;
    }

    _1E4 *= 0.9f;

    if (_1E4 < 1.0f) {
        _1E8 = 0;
        _1E4 = 0.0f;
    }

    if (_1EC < 1.0f) {
        _1EC = 0.0f;
    }
}

void MarioActor::drawRasterScroll(f32 amplitude, s16 period, f32 wavelength) const {
    if (wavelength == 0.0f) {
        return;
    }
    if (period == 0) {
        return;
    }
    GXDrawDone();
    GXTexModeSync();
    GXPixModeSync();
    u16 width = MR::getFrameBufferWidth();
    TDDraw::setup(1, 0, 2);
    GXTexObj textures[2];
    u16 current = 0;
    u32 stripHeight = 4;
    GXInitTexObj(&textures[0], mRasterBuffers[0], width, stripHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXInitTexObj(&textures[1], mRasterBuffers[1], width, stripHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXSetLineWidth(6, GX_TO_ZERO);
    GXSetTexCopyDst(width, stripHeight, GX_TF_RGB565, GX_FALSE);
    GXSetTexCopySrc(0, 0, width, stripHeight);
    GXCopyTex(mRasterBuffers[0], GX_FALSE);
    f32 increment = 1.0f / stripHeight;
    f32 frequency = 6.2831855f / wavelength;
    f32 phase = (_37C * 6.2831855f) / period;
    for (u32 y = 0; y < MR::getScreenHeight(); y += stripHeight) {
        GXTexModeSync();
        GXPixModeSync();
        GXSetTexCopySrc(0, y + stripHeight, width, stripHeight);
        u32 next = 1 - current;
        GXCopyTex(mRasterBuffers[next], GX_FALSE);
        GXLoadTexObj(&textures[current], GX_TEXMAP0);
        f32 texY = 0.0f;
        for (u32 line = y; line < y + stripHeight; line++) {
            f32 offset = amplitude * MR::sin(MR::sin(frequency * line) * 3.1415927f + phase);
            GXBegin(GX_LINES, GX_VTXFMT0, 2);
            GXPosition3f32(offset, line, 0.0f);
            GXTexCoord2f32(0.0f, texY);
            GXPosition3f32(offset + MR::getScreenWidth(), line, 0.0f);
            GXTexCoord2f32(1.0f, texY);
            GXEnd();
            texY += increment;
        }
        current = next;
    }
    GXDrawDone();
}


void MarioActor::drawMosaic() const {
}

void MarioActor::drawLifeUp() const {
}

// void MarioActor::calcSpinEffect() {}

// void MarioActor::drawSpinEffect() const {}

void MarioActor::drawSphereMask() const {
    if (mMario->isVisibleRecoveryWarpBubble()) {
        TDDraw::setup(0, 1, 0);
        GXSetZScaleOffset(1.0f, 0.00001f);
        GXSetCullMode(GX_CULL_FRONT);
        GXSetZMode(GX_TRUE, GX_GREATER, GX_TRUE);
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
        TVec3f position;
        getRealPos("Spine1", &position);
        TDDraw::drawSphere3D(position, 180.0f, 0xFFFFFF30, 16);
        GXSetZScaleOffset(1.0f, 0.0f);
    }
}

// void MarioActor::initDarkMask() {}

// void MarioActor::updateDarkMask(u16) {}

// bool MarioActor::drawDarkMask() const {}

void MarioActor::showBeeFur() {
    if (mMario->isPlayerModeBee()) {
        _9E8->appear();
        _9EC->onDraw(0xFFFFFFFF);
    }
    if (getCarrySensor()) {
        MR::showModel(getCarrySensor()->mHost);
    }
    if (mMario->isPlayerModeInvincible()) {
        MR::showJoint(getJ3DModel(), "Face0");
        _A6E = 2;
        playEffect("無敵中");
    }
}

DLholder* DLchanger::swap() {
    mCurrentBuffer = (mCurrentBuffer + 1) % mNumBuffers;
    return &mBuffers[mCurrentBuffer];
}
