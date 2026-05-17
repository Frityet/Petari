#include "Game/Screen/LayoutActor.hpp"

#include <algorithm>

#include "Game/LiveActor/Spine.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/SimpleLayout.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/compat/RuntimeContext.hpp"

class PaneEffectKeeper {};
class StarPointerLayoutTargetKeeper {};

LayoutActor::LayoutActor(const char* pName, bool)
    : NameObj(pName), mManager(nullptr), mSpine(nullptr), mPaneEffectKeeper(nullptr), mStarPointerTargetKeeper(nullptr) {
}

LayoutActor::~LayoutActor() {
    if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->unregister_layout_actor(*this);
    }
    delete mSpine;
}

void LayoutActor::movement() {
    if (MR::isDead(this)) {
        return;
    }

    updateSpine();

    if (MR::isDead(this)) {
        return;
    }

    control();

    if (mManager != nullptr) {
        mManager->movement();
    }

    if (mSimpleLayout != nullptr && !mFlag.mIsStopAnimFrame) {
        mSimpleLayout->update();
        for (auto i = u32{0}; i < mAnimCtrls.size(); ++i) {
            syncAnimCtrlFromLayout(i);
        }
    }
}

void LayoutActor::draw() const {
}

void LayoutActor::calcAnim() {
    if (mManager != nullptr) {
        mManager->calcAnim();
    }
}

void LayoutActor::appear() {
    mFlag.mIsDead = false;
    mFlag.mIsHidden = false;
    if (mSimpleLayout != nullptr) {
        mSimpleLayout->appear();
    }
    calcAnim();
}

void LayoutActor::kill() {
    mFlag.mIsDead = true;
    if (mSimpleLayout != nullptr) {
        mSimpleLayout->kill();
    }
}

void LayoutActor::setNerve(const Nerve* pNerve) const {
    if (mSpine != nullptr) {
        mSpine->setNerve(pNerve);
    }
}

bool LayoutActor::isNerve(const Nerve* pNerve) const {
    return mSpine != nullptr && mSpine->getCurrentNerve() == pNerve;
}

s32 LayoutActor::getNerveStep() const {
    if (mSpine == nullptr) {
        return 0;
    }

    return mSpine->mStep;
}

TVec2f LayoutActor::getTrans() const {
    return mTrans;
}

void LayoutActor::setTrans(const TVec2f& rTrans) {
    mTrans = rTrans;
    if (mSimpleLayout != nullptr) {
        mSimpleLayout->setTrans(mTrans.x, mTrans.y);
    }
}

LayoutManager* LayoutActor::getLayoutManager() const {
    return mManager;
}

void LayoutActor::createPaneMtxRef(const char*) {
}

MtxPtr LayoutActor::getPaneMtxRef(const char*) {
    return nullptr;
}

void LayoutActor::initLayoutManager(const char* pName, u32 animLayerNum) {
    mSimpleLayout = std::make_unique< SimpleLayout >(getName(), pName, animLayerNum, MR::DrawType_Layout);
    if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->unregister_layout(*mSimpleLayout);
    }
    mSimpleLayout->initWithoutIter();
    mSimpleLayout->kill();
    mSimpleLayout->setTrans(mTrans.x, mTrans.y);
    mManagerOwner = std::make_unique< LayoutManager >(this);
    mManager = mManagerOwner.get();
}

void LayoutActor::initLayoutManagerNoConvertFilename(const char* pName, u32 animLayerNum) {
    initLayoutManager(pName, animLayerNum);
}

void LayoutActor::initLayoutManagerWithTextBoxBufferLength(const char* pName, u32, u32 animLayerNum) {
    initLayoutManager(pName, animLayerNum);
}

void LayoutActor::initNerve(const Nerve* pNerve) {
    delete mSpine;
    mSpine = new Spine(this, pNerve);
}

void LayoutActor::initEffectKeeper(int, const char*, const EffectSystem*) {
}

void LayoutActor::initPointingTarget(int) {
}

void LayoutActor::updateSpine() {
    if (mSpine != nullptr) {
        mSpine->update();
    }
}

void LayoutActor::drawLayout(smgpc::render::IRendererEngine& renderer) {
    if (mSimpleLayout != nullptr && !MR::isDead(this) && !mFlag.mIsHidden) {
        mSimpleLayout->draw(renderer);
    }
}

bool LayoutActor::isDead() const {
    return mFlag.mIsDead;
}

SimpleLayout* LayoutActor::getSimpleLayout() {
    return mSimpleLayout.get();
}

const SimpleLayout* LayoutActor::getSimpleLayout() const {
    return mSimpleLayout.get();
}

J3DFrameCtrl* LayoutActor::getAnimCtrl(u32 animLayer) {
    syncAnimCtrlFromLayout(animLayer);
    return &animCtrl(animLayer);
}

void LayoutActor::startAnim(const char* pAnimName, u32 animLayer) {
    if (mSimpleLayout != nullptr) {
        mSimpleLayout->startAnim(pAnimName, animLayer);
    }
    syncAnimCtrlFromLayout(animLayer);
}

void LayoutActor::setAnimFrame(f32 frame, u32 animLayer) {
    if (mSimpleLayout != nullptr) {
        mSimpleLayout->setAnimFrame(frame, animLayer);
    }
    animCtrl(animLayer).mFrame = frame;
}

void LayoutActor::setAnimFrameAndStop(f32 frame, u32 animLayer) {
    if (mSimpleLayout != nullptr) {
        mSimpleLayout->setAnimFrameAndStop(frame, animLayer);
    }
    auto& ctrl = animCtrl(animLayer);
    ctrl.mFrame = frame;
    ctrl.mRate = 0.0F;
}

void LayoutActor::setAnimRate(f32 rate, u32 animLayer) {
    if (mSimpleLayout != nullptr) {
        mSimpleLayout->setAnimRate(rate, animLayer);
    }
    animCtrl(animLayer).mRate = rate;
}

f32 LayoutActor::getAnimFrame(u32 animLayer) const {
    if (mSimpleLayout == nullptr) {
        return animCtrl(animLayer).mFrame;
    }

    return mSimpleLayout->getAnimFrame(animLayer);
}

bool LayoutActor::isAnimStopped(u32 animLayer) {
    if (mSimpleLayout == nullptr) {
        return true;
    }

    return mSimpleLayout->isAnimStopped(animLayer);
}

void LayoutActor::setTextBoxNumberRecursive(const char* pPaneName, s32 number) {
    if (mSimpleLayout != nullptr) {
        mSimpleLayout->setTextBoxNumberRecursive(pPaneName, number);
    }
}

void LayoutActor::setTextBoxStringRecursive(const char* pPaneName, std::u16string_view text) {
    if (mSimpleLayout != nullptr) {
        mSimpleLayout->setTextBoxStringRecursive(pPaneName, text);
    }
}

J3DFrameCtrl& LayoutActor::animCtrl(u32 animLayer) {
    return mAnimCtrls.at(std::min< std::size_t >(animLayer, mAnimCtrls.size() - 1U));
}

const J3DFrameCtrl& LayoutActor::animCtrl(u32 animLayer) const {
    return mAnimCtrls.at(std::min< std::size_t >(animLayer, mAnimCtrls.size() - 1U));
}

void LayoutActor::syncAnimCtrlFromLayout(u32 animLayer) {
    if (mSimpleLayout == nullptr) {
        return;
    }

    auto& ctrl = animCtrl(animLayer);
    ctrl.mFrame = mSimpleLayout->getAnimFrame(animLayer);
    ctrl.mEnd = static_cast< s16 >(mSimpleLayout->debugAnimEndFrame(animLayer));
    ctrl.mRate = mSimpleLayout->debugAnimRate(animLayer);
}
