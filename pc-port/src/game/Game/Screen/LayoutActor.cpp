#include "Game/Screen/LayoutActor.hpp"

#include <utility>

#include "layout/LayoutRuntimeActor.hpp"

LayoutActor::LayoutActor(const char *pName, std::shared_ptr<smgpc::game::layout::LayoutRuntimeActor> runtime_actor)
    : mName(pName), mRuntimeActor(std::move(runtime_actor)) {
}

void LayoutActor::movement() {
    if (mRuntimeActor == nullptr || isDead()) {
        return;
    }

    control();
    mRuntimeActor->update(1.0F);
}

void LayoutActor::draw() const {
}

void LayoutActor::calcAnim() {
}

void LayoutActor::appear() {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->appear();
    }
}

void LayoutActor::kill() {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->kill();
    }
}

void LayoutActor::startAnim(const char *pAnimName, unsigned int layer) {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->startAnim(pAnimName, layer);
    }
}

bool LayoutActor::isAnimStopped(unsigned int layer) const {
    if (mRuntimeActor == nullptr) {
        return true;
    }

    return mRuntimeActor->isAnimStopped(layer);
}

void LayoutActor::setAnimFrameAndStop(float frame, unsigned int layer) {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->setAnimFrameAndStop(frame, layer);
    }
}

void LayoutActor::emitEffect(const char *pEffectName) {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->emitEffect(pEffectName);
    }
}

void LayoutActor::deleteEffectAll() {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->deleteEffectAll();
    }
}

bool LayoutActor::isDead() const {
    return mRuntimeActor == nullptr || mRuntimeActor->isDead();
}

void LayoutActor::appendDrawCommands(smgpc::render::layout::LayoutDrawList *pDrawList) const {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->appendDrawCommands(pDrawList);
    }
}

const smgpc::game::layout::LayoutArchiveData *LayoutActor::getResource() const {
    if (mRuntimeActor == nullptr) {
        return nullptr;
    }

    return mRuntimeActor->resource();
}

const char *LayoutActor::getName() const {
    return mName;
}
