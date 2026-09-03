#include "Game/Player/DLchanger.hpp"
#include "Game/Player/J3DModelX.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"
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
#include <revolution/os/OSCache.h>

void MarioActor::initScreenBox() {
    _B44 = new (32) u8[0x20000];
    mScreenBoxMin.y = 0.0f;
    mScreenBoxMin.x = 0.0f;
    mScreenBoxMax.y = 0.0f;
    mScreenBoxMax.x = 0.0f;
    _B38 = 0.0f;
    _B34 = 0.0f;
    _B40 = 0.0f;
    _B3C = 0.0f;
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
    _B34 = x;
    _B38 = y;
    _B3C = width;
    _B40 = height;

    TBox2f box(_B34, _B38, _B34 + _B3C, _B38 + _B40);
    TBox2f screen(0.0f, 0.0f, MR::getFrameBufferWidth(), MR::getScreenHeight());
    TBox2f clipped(box);
    if (!clipped.intersect(screen)) {
        clipped.i.set(0.0f);
        clipped.f.set(0.0f);
    }
    _B34 = clipped.i.x;
    _B38 = clipped.i.y;
    _B3C = clipped.getWidth();
    _B40 = clipped.getHeight();
}

void MarioActor::captureScreenBox() const {
    if (!isUseScreenBox()) {
        return;
    }
    if (_B3C < 1.0f || _B40 < 1.0f) {
        return;
    }

    GXSetTexCopyDst(_B3C, _B40, GX_TF_RGB565, GX_FALSE);
    GXSetTexCopySrc(_B34, _B38, _B3C, _B40);
    GXSetColorUpdate(GX_FALSE);
    GXSetAlphaUpdate(GX_TRUE);

    GXColor clear = {};

    GXSetCopyClear(clear, 0xFFFFFF);
    GXCopyTex(_B44, GX_TRUE);
    GXSetColorUpdate(GX_ENABLE);
    GXSetAlphaUpdate(GX_FALSE);
    GXSetDstAlpha(GX_FALSE, 0);
}

// void MarioActor::writeBackScreenBox() const {}

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

// void MarioActor::drawScreenBlend() const {}

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

// void MarioActor::drawWallShade(const TVec3f&, const TVec3f&, f32) const {}

void MarioActor::drawSpinInhibit() const {
}

// void MarioActor::drawColdWaterDamage() const {}

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

// void MarioActor::drawRasterScroll(f32, s16, f32) const {}

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
