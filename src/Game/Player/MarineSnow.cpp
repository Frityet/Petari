#include "Game/Player/MarineSnow.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include <revolution.h>

extern "C" {
void GXTexCoord2f32(f32, f32);
}

namespace {
u8 sDrawOffset;
}

MarineSnow::MarineSnow() {
    mPointNum = 0x10;
    mPoints = new (0x20) TVec3f[mPointNum];
    mRadius = 1000.0f;

    u32 i = 0;
    if (mPointNum != 0) {
        while (i < mPointNum) {
            f32 z = mRadius * MR::getRandom();
            f32 y = mRadius * MR::getRandom();
            f32 x = mRadius * MR::getRandom();

            f32 point[3];
            point[0] = x;
            point[1] = y;
            point[2] = z;
            JGeometry::setTVec3f(point, &mPoints[i].x);
            i++;
        }
    }

    mViewCount = 0;

    JUTTexture* pTex = reinterpret_cast< JUTTexture* >(::operator new(sizeof(JUTTexture)));
    if (pTex) {
        const ResTIMG* pTIMG = MR::loadTexFromArc("MarineSnow");
        pTex->mEmbPalette = nullptr;
        pTex->storeTIMG(pTIMG, static_cast< u8 >(0));
        pTex->mFlag &= 2;
    }

    mTexture = pTex;
}

void MarineSnow::view() {
    if (mViewCount < 60) {
        mViewCount++;
    }
}

void MarineSnow::clear() {
    if (mViewCount > 0) {
        mViewCount--;
    }
}

void MarineSnow::draw(const TVec3f& rTrans, const TVec3f& rUp, f32 upDotThreshold) const {
    if (mViewCount == 0) {
        return;
    }

    const f32 viewRatio = static_cast< f32 >(mViewCount) / 60.0f;
    const f32 halfRadius = mRadius * 0.5f;
    const s32 gridX = static_cast< s32 >((rTrans.x + halfRadius) / mRadius);
    const s32 gridY = static_cast< s32 >((rTrans.y + halfRadius) / mRadius);
    const s32 gridZ = static_cast< s32 >((rTrans.z + halfRadius) / mRadius);

    const s32 baseX = static_cast< s32 >(static_cast< f32 >(gridX) * mRadius);
    const s32 baseY = static_cast< s32 >(static_cast< f32 >(gridY) * mRadius);
    const s32 baseZ = static_cast< s32 >(static_cast< f32 >(gridZ) * mRadius);

    TDDraw::setup(0, 1, 0);
    GXSetZMode(true, GX_LEQUAL, false);
    GXSetPointSize(0x1B, GX_TO_ZERO);

    const u32 halfPoints = mPointNum >> 1;
    sDrawOffset = (sDrawOffset + 1) & 3;
    const u32 altStart = halfPoints * sDrawOffset;
    const u32 altEnd = altStart + halfPoints;

    TVec3f camZRaw = MR::getCamZdir();
    TVec3f camZ = camZRaw.negateInline();
    TVec3f camY = MR::getCamYdir();

    TVec3f side;
    side.cross(camZ, camY);
    MR::normalizeOrZero(&side);

    TVec3f up;
    up.cross(side, camZ);
    MR::normalizeOrZero(&up);

    const TVec3f plus = (side + up) * 10.0f;
    const TVec3f minus = (side - up) * 10.0f;

    TDDraw::setup(1, 1, 0);
    GXSetZMode(true, GX_LEQUAL, false);

    mTexture->load(GX_TEXMAP0);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE0, static_cast< _GXTevColorArg >(0xF), static_cast< _GXTevColorArg >(0xC),
                    static_cast< _GXTevColorArg >(0x8), static_cast< _GXTevColorArg >(0xF));
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, static_cast< _GXTevAlphaArg >(0x7), static_cast< _GXTevAlphaArg >(0x1),
                    static_cast< _GXTevAlphaArg >(0x4), static_cast< _GXTevAlphaArg >(0x7));
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

    const f32 levelThreshold1 = 3.0f;
    const f32 levelThreshold2 = 5.0f;
    const f32 levelThreshold3 = 8.0f;
    const f32 levelDivisor = 12.0f;
    const f32 alphaScale = 242.0f;
    const f32 one = 1.0f;
    const f32 zero = 0.0f;

    for (s32 x = -4; x < 4; x++) {
        for (s32 y = -4; y < 4; y++) {
            for (s32 z = -4; z < 4; z++) {
                TVec3f cell;
                cell.x = static_cast< f32 >(baseX) + static_cast< f32 >(x) * mRadius;
                cell.y = static_cast< f32 >(baseY) + static_cast< f32 >(y) * mRadius;
                cell.z = static_cast< f32 >(baseZ) + static_cast< f32 >(z) * mRadius;

                TVec3f check = cell + (rUp * mRadius);
                if ((check - rTrans).dot(rUp) > upDotThreshold) {
                    continue;
                }

                s32 absY = -y;
                if (y >= 0) {
                    absY = y;
                }

                s32 absX = -x;
                if (x >= 0) {
                    absX = x;
                }

                s32 zSign = z >> 31;
                f32 level = static_cast< f32 >((absX + absY) + ((z ^ zSign) - zSign));

                u32 step = 1;
                if (level > levelThreshold1) {
                    step = 2;
                }
                if (level > levelThreshold2) {
                    step = 4;
                }
                if (level > levelThreshold3) {
                    step = 8;
                }

                f32 fade = one - (level / levelDivisor);
                if (fade < zero) {
                    fade = zero;
                }

                fade = fade * fade * fade;
                const u32 alpha = static_cast< u32 >(alphaScale * fade * viewRatio);
                if (alpha == 0) {
                    continue;
                }

                GXColor tevColor;
                tevColor.r = 0;
                tevColor.g = 0;
                tevColor.b = 0;
                tevColor.a = alpha;
                GXSetTevColor(static_cast< _GXTevRegID >(1), tevColor);

                Mtx mtx;
                PSMTXConcat(MR::getCameraViewMtx(), MR::tmpMtxTrans(cell), mtx);
                GXLoadPosMtxImm(mtx, 0);

                u16 startIdx = 0;
                u16 endIdx = static_cast< u16 >(mPointNum);
                if (fade > 3.0f) {
                    startIdx = static_cast< u16 >(altStart);
                    endIdx = static_cast< u16 >(altEnd);
                }

                for (u16 idx = startIdx; idx < endIdx; idx = static_cast< u16 >(idx + step)) {
                    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

                    TVec3f v0 = mPoints[idx] - minus;
                    TDDraw::sendPoint(v0);
                    GXTexCoord2f32(0.0f, 0.0f);

                    TVec3f v1 = mPoints[idx] + plus;
                    TDDraw::sendPoint(v1);
                    GXTexCoord2f32(1.0f, 0.0f);

                    TVec3f v2 = mPoints[idx] + minus;
                    TDDraw::sendPoint(v2);
                    GXTexCoord2f32(1.0f, 1.0f);

                    TVec3f v3 = mPoints[idx] - plus;
                    TDDraw::sendPoint(v3);
                    GXTexCoord2f32(0.0f, 1.0f);
                }
            }
        }
    }
}
