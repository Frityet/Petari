#include "Game/Util/FurMulti.hpp"
#include "Game/Util/FurCtrl.hpp"
#include "Game/Util/ModelUtil.hpp"

FurMulti::FurMulti(LiveActor* actor, u32 count) {
    mActor = actor;
    mModel = MR::getJ3DModel(actor);
    mLayerCount = count;
    _4 = new u8[count];
    _8 = new u8[count];
    mFurCtrls = new FurCtrl*[count];
    for (u32 i = 0; i < count; i++) {
        _8[i] = 0xFF;
    }
    _0 = 0;
    _1 = 1;
}

void FurMulti::offDraw(u32 mask) {
    for (u32 i = 0; i < mLayerCount; i++) {
        if (mask & (1 << i)) {
            mFurCtrls[i]->_1C = 0;
        }
    }
}

void FurMulti::onDraw(u32 mask) {
    for (u32 i = 0; i < mLayerCount; i++) {
        if (mask & (1 << i)) {
            mFurCtrls[i]->_1C = 1;
        }
    }
}
