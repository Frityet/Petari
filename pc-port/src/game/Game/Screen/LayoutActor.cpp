#include "Game/Screen/LayoutActor.hpp"

#include <utility>

#include "compat/DecompIntegration.hpp"
#include "Game/LiveActor/Spine.hpp"
#include "Game/Screen/SimpleLayout.hpp"
#include "compat/LayoutSceneCompat.hpp"
#include "layout/LayoutRuntimeActor.hpp"

namespace {

// SMGPC_INTEGRATION_BEGIN
SMGPC_STUB(src/Game/Screen/LayoutManager.cpp);
SMGPC_STUB(src/Game/Screen/PaneEffectKeeper.cpp);
// SMGPC_INTEGRATION_END

}  // namespace

LayoutActor::LayoutActor(const char *pName, bool connectToScene)
    : mName(pName), mRuntimeActor(nullptr), mSpine(nullptr), mTrans(0.0F, 0.0F) {
    (void)connectToScene;
}

LayoutActor::LayoutActor(const char *pName, std::shared_ptr<smgpc::game::layout::LayoutRuntimeActor> runtime_actor)
    : mName(pName), mRuntimeActor(std::move(runtime_actor)), mSpine(nullptr), mTrans(0.0F, 0.0F) {
}

LayoutActor::~LayoutActor() {
    smgpc::game::compat::disconnect_layout_scene_actor(this);
    delete mSpine;
}

void LayoutActor::movement() {
    if (mRuntimeActor == nullptr || isDead()) {
        return;
    }

    updateSpine();
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

void LayoutActor::setNerve(const Nerve *pNerve) const {
    if (mSpine != nullptr) {
        mSpine->setNerve(pNerve);
    }
}

bool LayoutActor::isNerve(const Nerve *pNerve) const {
    if (mSpine == nullptr) {
        return false;
    }

    return mSpine->getCurrentNerve() == pNerve;
}

s32 LayoutActor::getNerveStep() const {
    if (mSpine == nullptr) {
        return 0;
    }

    return mSpine->mStep;
}

void LayoutActor::initNerve(const Nerve *pNerve) {
    delete mSpine;
    mSpine = new Spine(this, pNerve);
}

void LayoutActor::updateSpine() {
    if (mSpine != nullptr) {
        mSpine->update();
    }
}

TVec2f LayoutActor::getTrans() const {
    return mTrans;
}

void LayoutActor::setTrans(const TVec2f &rTrans) {
    mTrans = rTrans;
    if (mRuntimeActor != nullptr) {
        TVec2f root_translation = mTrans;
        const auto *resource = mRuntimeActor->resource();
        if (resource != nullptr && resource->layout.center_origin) {
            root_translation.x -= resource->layout.size.x * 0.5F;
            root_translation.y -= resource->layout.size.y * 0.5F;
        }
        mRuntimeActor->setRootTranslation(root_translation.x, root_translation.y);
    }
}

void LayoutActor::initLayoutManager(const char *pArchiveName, u32 groupCount) {
    (void)groupCount;
    mRuntimeActor = SimpleLayout::loadRuntimeActor(pArchiveName);
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->setRootTranslation(mTrans.x, mTrans.y);
    }
}

void LayoutActor::setPaneVisible(const char *pPaneName, bool visible) {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->setPaneVisible(pPaneName, visible);
    }
}

void LayoutActor::setPaneVisibleRecursive(const char *pPaneName, bool visible) {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->setPaneVisibleRecursive(pPaneName, visible);
    }
}

void LayoutActor::setTextBoxTextRecursive(const char *pPaneName, std::u16string text) {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->setTextBoxTextRecursive(pPaneName, std::move(text));
    }
}

void LayoutActor::clearTextBoxTextRecursive(const char *pPaneName) {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->clearTextBoxTextRecursive(pPaneName);
    }
}

void LayoutActor::setTextBoxVerticalPositionRecursive(const char *pPaneName, u8 position) {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->setTextBoxVerticalPositionRecursive(pPaneName, position);
    }
}

bool LayoutActor::getPaneTrans(const char *pPaneName, TVec2f *pOut) const {
    if (mRuntimeActor == nullptr || pOut == nullptr) {
        return false;
    }

    return mRuntimeActor->getPaneTrans(pPaneName, &pOut->x, &pOut->y);
}

bool LayoutActor::getPaneBounds(const char *pPaneName, f32 *pX0, f32 *pY0, f32 *pX1, f32 *pY1) const {
    if (mRuntimeActor == nullptr) {
        return false;
    }

    return mRuntimeActor->getPaneBounds(pPaneName, pX0, pY0, pX1, pY1);
}

void LayoutActor::setPaneFollowPos(const char *pPaneName, const TVec2f *pFollowPos) const {
    if (mRuntimeActor == nullptr) {
        return;
    }

    mRuntimeActor->setPaneFollowPosition(pPaneName, pFollowPos != nullptr ? &pFollowPos->x : nullptr, pFollowPos != nullptr ? &pFollowPos->y : nullptr);
}

void LayoutActor::replacePaneTexture(const char *pPaneName, const nw4r::lyt::TexMap *pTexMap, u8 slot) {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->replacePaneTexture(pPaneName, pTexMap, slot);
    }
}

nw4r::lyt::TexMap *LayoutActor::getPaneTexture(const char *pPaneName, u8 slot) const {
    if (mRuntimeActor == nullptr) {
        return nullptr;
    }
    return mRuntimeActor->getPaneTexture(pPaneName, slot);
}

bool LayoutActor::isExistPaneCtrl(const char *pPaneName) const {
    return mRuntimeActor != nullptr && mRuntimeActor->hasPane(pPaneName);
}

void LayoutActor::startPaneAnim(const char *pPaneName, const char *pAnimName, u32 slot) {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->startPaneAnim(pPaneName, pAnimName, slot);
    }
}

bool LayoutActor::isPaneAnimStopped(const char *pPaneName, u32 slot) const {
    if (mRuntimeActor == nullptr) {
        return true;
    }
    return mRuntimeActor->isPaneAnimStopped(pPaneName, slot);
}

void LayoutActor::setPaneAnimFrame(const char *pPaneName, f32 frame, u32 slot) {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->setPaneAnimFrame(pPaneName, frame, slot);
    }
}

f32 LayoutActor::getPaneAnimFrame(const char *pPaneName, u32 slot) const {
    if (mRuntimeActor == nullptr) {
        return 0.0F;
    }
    return mRuntimeActor->getPaneAnimFrame(pPaneName, slot);
}

void LayoutActor::setPaneAnimRate(const char *pPaneName, f32 rate, u32 slot) {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->setPaneAnimRate(pPaneName, rate, slot);
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

void LayoutActor::setAnimFrame(float frame, unsigned int layer) {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->setAnimFrame(frame, layer);
    }
}

void LayoutActor::setAnimRate(float rate, unsigned int layer) {
    if (mRuntimeActor != nullptr) {
        mRuntimeActor->setAnimRate(rate, layer);
    }
}

float LayoutActor::getAnimFrame(unsigned int layer) const {
    if (mRuntimeActor == nullptr) {
        return 0.0F;
    }
    return mRuntimeActor->getAnimFrame(layer);
}

float LayoutActor::getAnimRate(unsigned int layer) const {
    if (mRuntimeActor == nullptr) {
        return 0.0F;
    }
    return mRuntimeActor->getAnimRate(layer);
}

float LayoutActor::getAnimFrameMax(unsigned int layer) const {
    if (mRuntimeActor == nullptr) {
        return 0.0F;
    }
    return mRuntimeActor->getAnimFrameMax(layer);
}

float LayoutActor::getAnimFrameMax(const char *pAnimName) const {
    if (mRuntimeActor == nullptr) {
        return 0.0F;
    }
    return mRuntimeActor->getAnimFrameMax(pAnimName);
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
