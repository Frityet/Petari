#pragma once

#include <revolution/types.h>

class FurCtrl;
class J3DModel;
class LiveActor;

class FurMulti {
public:
    FurMulti(LiveActor*, u32);

    void offDraw(u32);
    void onDraw(u32);

    u8 _0;
    u8 _1;
    u8 mLayerCount;
    u8* _4;
    u8* _8;
    u32 _C;
    u32 _10;
    u32 _14;
    LiveActor* mActor;
    J3DModel* mModel;
    FurCtrl** mFurCtrls;
};

namespace MR {
    FurMulti* initMultiFur(LiveActor*, s32);
}  // namespace MR
