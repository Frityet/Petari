#pragma once

#include <JSystem/JGeometry/TVec.hpp>
#include <revolution.h>

class DynamicFurParam;
class JUTTexture;
class ResTIMG;

class FurDrawer {
public:
    class CLayerParam {
    public:
        f32 calcValue(s32, s32) const;

        f32 mEnd;
        f32 mStart;
        f32 mExponent;
    };

    FurDrawer(u32, ResTIMG*, ResTIMG*);

    void update();
    void setupMaterial(DynamicFurParam*) const;
    void setupLayerMaterial(s32) const;
    void createFurMap();

    /* 0x00 */ JUTTexture* mBaseTexture;
    /* 0x04 */ JUTTexture* mFurTexture;
    /* 0x08 */ JUTTexture* mIndirectTexture;
    /* 0x0C */ u32 mLayerCount;
    /* 0x10 */ CLayerParam mLength;
    /* 0x1C */ CLayerParam mIndirect;
    /* 0x28 */ CLayerParam mBrightness;
    /* 0x34 */ CLayerParam mAlpha;
    /* 0x40 */ f32 mFurScale;
    /* 0x44 */ u32 mMixFog;
    /* 0x48 */ f32 mBaseScale;
    /* 0x4C */ f32 mDensity[4];
    /* 0x5C */ f32 mIntensity[4];
    /* 0x6C */ u8 mTransparency[4];
    /* 0x70 */ CLayerParam mOffset;
    /* 0x7C */ GXColor mColor;
    /* 0x80 */ u8 mCullMode;
    /* 0x81 */ u8 mMaterial;
    /* 0x82 */ u8 mAmbient;
    /* 0x83 */ u8 mUseIndirect;
    /* 0x84 */ u8 mZCompLoc;
    /* 0x85 */ u8 mZUpdate;
    /* 0x86 */ u8 mAdditive;
    /* 0x87 */ u8 mAlphaRef;
    /* 0x88 */ Mtx mFurMtx;
    /* 0xB8 */ Mtx mIndirectMtx;
    /* 0xE8 */ TVec3f mFogPosition;
    /* 0xF4 */ f32 mFogRadius;
    /* 0xF8 */ u8 _F8;
};
