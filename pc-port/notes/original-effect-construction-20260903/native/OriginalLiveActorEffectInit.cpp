#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/EffectKeeper.hpp"
#include "Game/Util/LiveActorUtil.hpp"

void LiveActor::initEffectKeeper(int a1, const char* a2, bool doSort) {
    mEffectKeeper = new EffectKeeper(getName(), MR::getModelResourceHolder(this), a1, a2);

    if (doSort) {
        mEffectKeeper->enableSort();
    }

    mEffectKeeper->init(this);

    if (mBinder != nullptr) {
        mEffectKeeper->setBinder(mBinder);
    }
}
