#include "Game/Player/MarioActor.hpp"
#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include <revolution/gx.h>
#include <revolution/os.h>

void MarioActor::initScreenBox() {
    //_B44 = new (32) u8[0x80000];
    _B28 = 0.0f;
    _B24 = 0.0f;
    _B30 = 0.0f;
    _B2C = 0.0f;
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

// void MarioActor::calcScreenBoxRange() {}

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

// void MarioActor::hideBeeFur() {}

// void MarioActor::calcFogLighting() {}

void MarioActor::resetFog() {
    // FIXME: contructing a color8, but never using it
    Color8 color(0, 0, 0, 0);
    _1A8 = 0;
    _1A4 = 0.0f;
    _1AA = 0;
}

// void MarioActor::updateAlphaDL(u8) {}

// void MarioActor::updateSimpleAlphaDL(u8) {}

// void MarioActor::updateReflectAlphaDL(u8) {}

// void MarioActor::updateLightDL(const Color8&, const Color8&, const Color8&, f32) {}

// void MarioActor::createRainbowDL() {}

// void MarioActor::drawScreenBlend() const {}

// void MarioActor::updateRandomTexture(f32) {}

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



// void MarioActor::drawSphereMask() const {}




// void MarioActor::showBeeFur() {}

void MarioActor::initDarkMask() {
    for (u32 i = 0; i < 2; i++) {
        u8* image = _B80[i]->mImage;
        for (u32 y = 0; y < 8; y++) {
            u8* pixel = image + y * 8;
            for (u32 x = 0; x < 8; x++) {
                *pixel++ = 0;
            }
        }
    }
    _1C3 = true;
    _1C4 = 0;
}

void MarioActor::updateDarkMask(u16) {
    if (!_1C3) {
        return;
    }

    u8* previous = _B80[_B88]->mImage;
    _B88 = 1 - _B88;
    u8* current = _B80[_B88]->mImage;
    u8 x = MR::getRandom(0L, 8L);
    u8 y = MR::getRandom(0L, 8L);
    current[y * 8 + x] = 0xF0;
    previous[y * 8 + x] = 0xF0;
    DCStoreRange(current, 64);
    _1C4++;
}

bool MarioActor::drawDarkMask() const {
    if (!_1C3) {
        return false;
    }

    TDDraw::setup(1, 0, 2);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetColorUpdate(GX_FALSE);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    _B80[_B88]->load(GX_TEXMAP0);
    GXSetZCompLoc(GX_FALSE);
    GXSetAlphaCompare(GX_GREATER, 1, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetDstAlpha(GX_TRUE, 255);

    TVec3f jointPosition;
    getRealPos("Spine1", &jointPosition);
    f32 size = _1C4;
    if (size >= 240.0f) {
        size = 240.0f;
    }

    TVec3f position(mPosition);
    TVec2f center;
    TDDraw::project2D(&center, position);
    TVec2f right;
    TVec2f up;
    TDDraw::project2D(&right, position + mCamDirX * size);
    TDDraw::project2D(&up, position + mCamDirY * size);
    u16 width = ((static_cast< u32 >((right - center).length()) + 3) & ~3) + 4;
    u16 height = ((static_cast< u32 >((up - center).length()) + 3) & ~3) + 4;

    TVec2f screenPosition;
    TDDraw::project2D(&screenPosition, position);
    screenPosition.x -= width / 2;
    screenPosition.y -= width / 2;
    TVec2f framePosition;
    MR::convertScreenPosToFrameBufferPos(&framePosition, screenPosition);
    u32 x = static_cast< u32 >(framePosition.x) & ~1;
    u32 y = static_cast< u32 >(framePosition.y) & ~1;
    TVec2f first;
    TVec2f last;
    MR::convertFrameBufferPosToScreenPos(&first, TVec2f(x, y));
    MR::convertFrameBufferPosToScreenPos(&last, TVec2f(x + width, y + height));

    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(first.x, first.y, 0.0f);
    GXTexCoord2f32(0.0f, 0.0f);
    GXPosition3f32(last.x, first.y, 0.0f);
    GXTexCoord2f32(4.0f, 0.0f);
    GXPosition3f32(last.x, last.y, 0.0f);
    GXTexCoord2f32(4.0f, 4.0f);
    GXPosition3f32(first.x, last.y, 0.0f);
    GXTexCoord2f32(0.0f, 4.0f);
    GXEnd();

    GXSetDstAlpha(GX_FALSE, 0);
    GXSetColorUpdate(GX_TRUE);
    GXSetZCompLoc(GX_TRUE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    TDDraw::close();
    return true;
}

void MarioActor::calcSpinEffect() {
    _6D4 = 0.0f;
    _6D8 = 0.0f;
    f32 minimum = 30.0f;
    f32 scale = 5.8f;
    if (selectAction("スピン回復エフェクト") != 1 || !_945 || _944 || !_946) {
        return;
    }

    s32 step = _946 - 9;
    if (step <= 0 || step > 15) {
        _6D4 = 0.0f;
    } else if (step < 10) {
        _6D4 = minimum + (static_cast< f32 >(step) / 10.0f) * (_945 * scale);
        _6D8 = 120.0f;
    } else {
        _6D4 = minimum + _945 * scale;
        _6D8 = 120.0f;
    }
}

void MarioActor::drawSpinEffect() const {
    if (_6D4 == 0.0f || !isEnableNerveChange()) {
        return;
    }

    TDDraw::setup(0, 1, 0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
    TVec3f center(_2A0 + mMario->mHeadVec * _6D8);
    TVec3f direction(mMario->mFrontVec);
    Mtx rotation;
    PSMTXRotAxisRad(rotation, &mMario->mHeadVec, 0.09817477f);
    TVec3f previous;

    GXSetLineWidth(36, GX_TO_ZERO);
    for (u32 i = 0; i <= 64; i++) {
        TVec3f point(direction);
        point.setLength(_6D4);
        f32 random = MR::getRandom();
        point += (mMario->mHeadVec * random) * 10.0f;
        if (i != 0) {
            TDDraw::drawLine(center + previous, center + point, 0xFFFFFF20);
        }
        previous = point;
        PSMTXMultVecSR(rotation, &direction, &direction);
    }

    GXSetLineWidth(18, GX_TO_ZERO);
    for (u32 i = 0; i <= 64; i++) {
        TVec3f point(direction);
        point.setLength(_6D4);
        f32 random = MR::getRandom();
        point += (mMario->mHeadVec * random) * 10.0f;
        f32 wave = MR::sin((static_cast< f32 >((_37C + i) & 31) * 0.03125f) * 3.1415927f);
        f32 squared = wave * wave;
        u32 color = 160.0f * (squared * squared);
        if (i != 0) {
            TDDraw::drawLine(center + previous, center + point, (color << 24) | 0x0040FF00 | color);
        }
        previous = point;
        PSMTXMultVecSR(rotation, &direction, &direction);
    }
    TDDraw::close();
}
