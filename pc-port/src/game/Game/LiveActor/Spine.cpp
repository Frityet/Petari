#include "Game/LiveActor/Spine.hpp"

#include "Game/LiveActor/Nerve.hpp"

Spine::Spine(void *pExecutor, const Nerve *pNerve)
    : mExecutor(pExecutor), mCurrNerve(pNerve), mNextNerve(nullptr), mStep(0) {
}

void Spine::update() {
    changeNerve();
    if (mCurrNerve != nullptr) {
        mCurrNerve->execute(this);
    }
    ++mStep;
    changeNerve();
}

void Spine::setNerve(const Nerve *pNerve) {
    if (mCurrNerve != nullptr && mStep >= 0) {
        mCurrNerve->executeOnEnd(this);
    }

    mNextNerve = pNerve;
    mStep = -1;
}

const Nerve *Spine::getCurrentNerve() const {
    if (mNextNerve != nullptr) {
        return mNextNerve;
    }

    return mCurrNerve;
}

void Spine::changeNerve() {
    if (mNextNerve == nullptr) {
        return;
    }

    mCurrNerve = mNextNerve;
    mNextNerve = nullptr;
    mStep = 0;
}
