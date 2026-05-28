#include "Game/NameObj/NameObj.hpp"

#include "runtime/RuntimeContext.hpp"

namespace {
    constexpr auto FLAG_SUSPENDED = u16{1U << 0U};
}

NameObj::NameObj(const char* pName) {
    setName(pName);
}

NameObj::~NameObj() {
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        runtime->scheduler().disconnect_name_obj(*this);
    }
}

void NameObj::init(const JMapInfoIter&) {
}

void NameObj::initAfterPlacement() {
}

void NameObj::movement() {
}

void NameObj::draw() const {
}

void NameObj::calcAnim() {
}

void NameObj::calcViewAndEntry() {
}

void NameObj::initWithoutIter() {
    const auto iter = JMapInfoIter{};
    init(iter);
}

void NameObj::setName(const char* pName) {
    mNameStorage = pName != nullptr ? pName : "";
    mName = mNameStorage.c_str();
}

void NameObj::executeMovement() {
    if (!isSuspended()) {
        movement();
    }
}

void NameObj::requestSuspend() {
    mFlag |= FLAG_SUSPENDED;
}

void NameObj::requestResume() {
    mFlag &= static_cast< u16 >(~FLAG_SUSPENDED);
}

void NameObj::syncWithFlags() {
}

bool NameObj::isSuspended() const {
    return (mFlag & FLAG_SUSPENDED) != 0U;
}

const char* NameObj::getName() const {
    return mName;
}

void NameObjFunction::requestMovementOn(NameObj* pObj) {
    if (pObj != nullptr) {
        pObj->requestResume();
    }
}

void NameObjFunction::requestMovementOff(NameObj* pObj) {
    if (pObj != nullptr) {
        pObj->requestSuspend();
    }
}
