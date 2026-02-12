#include "NerveExecutor.hpp"

#include <stdexcept>

#include "Spine.hpp"

namespace smgpc::game::title::runtime {

NerveExecutor::NerveExecutor() = default;
NerveExecutor::~NerveExecutor() = default;

void NerveExecutor::initNerve(const Nerve *nerve) {
    mSpine = std::make_unique<Spine>(this, nerve);
}

void NerveExecutor::updateNerve() {
    if (mSpine != nullptr) {
        mSpine->update();
    }
}

void NerveExecutor::setNerve(const Nerve *nerve) {
    if (mSpine == nullptr) {
        throw std::runtime_error("NerveExecutor::setNerve called before initNerve.");
    }
    mSpine->setNerve(nerve);
}

bool NerveExecutor::isNerve(const Nerve *nerve) const {
    if (mSpine == nullptr) {
        return false;
    }
    return mSpine->getCurrentNerve() == nerve;
}

std::int32_t NerveExecutor::getNerveStep() const {
    if (mSpine == nullptr) {
        return 0;
    }
    return mSpine->getStep();
}

Spine *NerveExecutor::getSpine() const {
    return mSpine.get();
}

}  // namespace smgpc::game::title::runtime
