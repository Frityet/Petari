#include "Game/Player/J3DModelX.hpp"

void J3DModelX::copyExtraMtxBuffer(const J3DModelX* pModel) {
    _DD = pModel->_DD;

    for (u32 i = 0; i < _DD; i++) {
        mExtraMtxBuffer[i] = pModel->mExtraMtxBuffer[i];
    }
}

void J3DModelX::swapDrawBuffer(u32 drawBuffer) {
    if (_DC == drawBuffer) {
        _DC = 0;
        return;
    }

    _DC = drawBuffer;
}

void J3DModelX::setDynamicDL(u8* pDL, u32 dlSize) {
    if (pDL == nullptr) {
        dlSize = 0;
    }

    _1B8 = pDL;
    _1BC = dlSize;
}
