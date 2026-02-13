#include "Game/System/NerveExecutor.hpp"

#include "Game/LiveActor/Spine.hpp"

NerveExecutor::NerveExecutor(const char *pName)
    : mSpine(nullptr) {
    (void)pName;
}

NerveExecutor::~NerveExecutor() {
    delete mSpine;
}

void NerveExecutor::initNerve(const Nerve *pNerve) {
    mSpine = new Spine(this, pNerve);
}

void NerveExecutor::updateNerve() {
    if (mSpine != nullptr) {
        mSpine->update();
    }
}

void NerveExecutor::setNerve(const Nerve *pNerve) {
    if (mSpine == nullptr) {
        return;
    }

    mSpine->setNerve(pNerve);
}

bool NerveExecutor::isNerve(const Nerve *pNerve) const {
    if (mSpine == nullptr) {
        return false;
    }

    return mSpine->getCurrentNerve() == pNerve;
}

s32 NerveExecutor::getNerveStep() const {
    if (mSpine == nullptr) {
        return 0;
    }

    return mSpine->mStep;
}
