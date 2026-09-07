#include "Game/LiveActor/EffectKeeper.hpp"
#include "Game/Effect/SyncBckEffectChecker.hpp"

void EffectKeeper::changeBck() {
    if (_20 != nullptr) {
        _20->reset();
    }
}

void SyncBckEffectChecker::reset() {
    _8 = true;
    _4 = 0.0f;
}
