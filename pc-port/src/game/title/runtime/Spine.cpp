#include "Spine.hpp"

#include "Nerve.hpp"

namespace smgpc::game::title::runtime {

Spine::Spine(void *executor, const Nerve *nerve)
    : mExecutor(executor), mCurrNerve(nerve), mNextNerve(nullptr), mStep(0) {
}

void Spine::update() {
    changeNerve();
    if (mCurrNerve != nullptr) {
        mCurrNerve->execute(this);
    }
    ++mStep;
    changeNerve();
}

void Spine::setNerve(const Nerve *nerve) {
    if (mCurrNerve != nullptr && mStep >= 0) {
        mCurrNerve->executeOnEnd(this);
    }

    mNextNerve = nerve;
    mStep = -1;
}

const Nerve *Spine::getCurrentNerve() const {
    if (mNextNerve != nullptr) {
        return mNextNerve;
    }
    return mCurrNerve;
}

void *Spine::getExecutor() const {
    return mExecutor;
}

std::int32_t Spine::getStep() const {
    return mStep;
}

void Spine::changeNerve() {
    if (mNextNerve == nullptr) {
        return;
    }

    mCurrNerve = mNextNerve;
    mNextNerve = nullptr;
    mStep = 0;
}

}  // namespace smgpc::game::title::runtime
