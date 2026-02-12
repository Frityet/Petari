#include "TriggerChecker.hpp"

namespace smgpc::game::title::runtime {

TriggerChecker::TriggerChecker()
    : mPrevLevel(false), mCurrLevel(false) {
}

void TriggerChecker::update(bool input) {
    mPrevLevel = mCurrLevel;
    mCurrLevel = input;
}

void TriggerChecker::setInput(bool input) {
    mPrevLevel = input;
    mCurrLevel = input;
}

bool TriggerChecker::getLevel() const {
    return mCurrLevel;
}

bool TriggerChecker::getOnTrigger() const {
    return (not mPrevLevel) and mCurrLevel;
}

bool TriggerChecker::getOffTrigger() const {
    return mPrevLevel and (not mCurrLevel);
}

}  // namespace smgpc::game::title::runtime
