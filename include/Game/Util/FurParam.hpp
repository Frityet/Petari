#pragma once

#include <revolution/gx/GXStruct.h>

class FogCtrl;

class FurParam {
public:
    /* 0x00 */ s32 mLayerCount;
    /* 0x04 */ f32 _4;
    /* 0x08 */ f32 _8;
    /* 0x0C */ f32 _C;
    /* 0x10 */ f32 _10;
    /* 0x14 */ f32 _14;
    /* 0x18 */ f32 _18;
    /* 0x1C */ f32 _1C;
    /* 0x20 */ f32 _20;
    /* 0x24 */ f32 _24;
    /* 0x28 */ f32 _28;
    /* 0x2C */ f32 _2C;
    /* 0x30 */ f32 _30;
    /* 0x34 */ f32 _34;
    /* 0x38 */ f32 _38;
    /* 0x3C */ f32 _3C;
    /* 0x40 */ GXColor _40;
    /* 0x44 */ f32 _44[4];
    /* 0x54 */ f32 _54[4];
    /* 0x64 */ GXColor _64;
};

class FurLightParam {
public:
    FurLightParam() {
        mLight0Enabled = 1;
        mLight1Enabled = 0;
        mLight0Material = 255;
        mLight0Ambient = 50;
        mLight1Material = 255;
        mLight1Ambient = 0;
        mLightType = -1;
        mLightColorSource = 0;
    }

    /* 0x00 */ u16 mLight0Enabled;
    /* 0x02 */ u16 mLight1Enabled;
    /* 0x04 */ u8 mLight0Material;
    /* 0x05 */ u8 mLight0Ambient;
    /* 0x06 */ u8 mLight1Material;
    /* 0x07 */ u8 mLight1Ambient;
    /* 0x08 */ u8 mLightColorSource;
    /* 0x0C */ s32 mLightType;
};

class DynamicFurParam {
public:
    /* 0x00 */ FogCtrl* mFogCtrl;
    /* 0x04 */ FurLightParam* mLightParam;
};

namespace MR {
    void initFurParamFromDvd(FurParam*, DynamicFurParam*, char*, u32);
}
