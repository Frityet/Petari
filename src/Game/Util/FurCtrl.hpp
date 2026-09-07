#pragma once

#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/FurParam.hpp"
#include <JSystem/J3DGraphAnimator/J3DModel.hpp>

class FurDrawer;
class FurMulti;
class CShader;
class J3DModel;
class J3DModelData;
class ResTIMG;
class FogCtrl;
class FurLightParam;
class LiveActor;
class FurParam;

class J3DModel2 : public J3DModel {
public:
    J3DModel2(J3DModel*);
    virtual ~J3DModel2();
};

class FurBank {
public:
    FurBank() : mCount(0) {
        for (u32 i = 0; i < 32; i++) {
            mFurMulti[i] = nullptr;
            mLayerMask[i] = 0;
        }
    }

    FurMulti* check(J3DModelData*, u32);
    void regist(FurMulti*, u32);

    /* 0x00 */ u32 mCount;
    /* 0x04 */ FurMulti* mFurMulti[32];
    /* 0x84 */ u32 mLayerMask[32];
};

class FurCtrl {
public:
    FurCtrl(LiveActor*, FurParam*, bool, u8);

    void calcLayerForm();
    void drawFur();
    void createFurMap();
    void setupFur(J3DModel*, ResTIMG*, ResTIMG*, ResTIMG*, u16, u8);
    void setupFurClone(J3DModel*, FurCtrl*);

    LiveActor* _0;
    DynamicFurParam mDynamicParam;
    u8 _C;
    u8 _D;
    u8 _E;
    u8 _F;
    J3DModel* _10;
    u16 _14;
    FurParam* _18;
    u8 _1C;
    f32 _20;
    u16 _24;
    FurDrawer* _28;
    CShader* _2C;
    J3DModel** _30;
    ResTIMG* _34;
    ResTIMG* _38;
    ResTIMG* _3C;
};

class FurDrawManager : public NameObj {
public:
    FurDrawManager(u8);

    virtual ~FurDrawManager();
    virtual void draw() const;

    void add(FurCtrl*, u8);

    /* 0x0C */ u8 mCapacity;
    /* 0x0D */ u8 mNumFurCtrls[2];
    /* 0x10 */ FurCtrl** mFurCtrls[2];
    /* 0x18 */ FurBank* mBank;
};

namespace MR {
    FurDrawManager* getFurDrawManager();
}
