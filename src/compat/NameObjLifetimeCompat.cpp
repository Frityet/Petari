#include "Game/NameObj/NameObj.hpp"

#include "Game/Scene/SceneNameObjMovementController.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/SceneNameObjRegistry.hpp"

namespace {
    constexpr auto FLAG_MOVEMENT_OFF = u16{1U};
    constexpr auto FLAG_SUSPEND = u16{2U};
    constexpr auto FLAG_RESUME = u16{4U};
}  // namespace

NameObj::NameObj(const char *pName)
    : mName(smgpc::compat::register_name_obj_runtime_state(this, pName)), mFlag(), mExecutorIdx(-1) {
    try {
        smgpc::scene::register_scene_name_obj(*this);
    } catch (...) {
        smgpc::compat::release_name_obj_runtime_state(this);
        throw;
    }
}

NameObj::~NameObj() {
    if (auto *scheduler = smgpc::runtime::try_active_scene_scheduler()) {
        scheduler->disconnect_name_obj(*this);
    } else if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        runtime->scheduler().disconnect_name_obj(*this);
    }
    smgpc::scene::unregister_scene_name_obj(*this);
    smgpc::compat::release_name_obj_runtime_state(this);
}

void NameObj::init(const JMapInfoIter &) {
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

void NameObj::setName(const char *pName) {
    mName = smgpc::compat::update_name_obj_runtime_name(this, pName);
}

void NameObj::executeMovement() {
    if ((mFlag & FLAG_MOVEMENT_OFF) == FLAG_MOVEMENT_OFF) {
        return;
    }

    movement();
}

void NameObj::requestSuspend() {
    if ((getFlag() & FLAG_RESUME) == FLAG_RESUME) {
        mFlag &= ~FLAG_RESUME;
    }

    mFlag |= FLAG_SUSPEND;
}

void NameObj::requestResume() {
    if ((getFlag() & FLAG_SUSPEND) == FLAG_SUSPEND) {
        mFlag &= ~FLAG_SUSPEND;
    }

    mFlag |= FLAG_RESUME;
}

void NameObj::syncWithFlags() {
    if ((getFlag() & FLAG_SUSPEND) == FLAG_SUSPEND) {
        mFlag &= ~FLAG_SUSPEND;
        mFlag |= FLAG_MOVEMENT_OFF;
    }

    if ((getFlag() & FLAG_RESUME) == FLAG_RESUME) {
        mFlag &= ~FLAG_RESUME;
        mFlag &= ~FLAG_MOVEMENT_OFF;
    }
}

void NameObjFunction::requestMovementOn(NameObj *pObj) {
    pObj->requestResume();
    MR::notifyRequestNameObjMovementOnOff();
}

void NameObjFunction::requestMovementOff(NameObj *pObj) {
    pObj->requestSuspend();
    MR::notifyRequestNameObjMovementOnOff();
}
