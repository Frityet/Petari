#include "Game/Screen/TitleSequenceProduct.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "Game/Util/TriggerChecker.hpp"

namespace TitleSequenceProductSub {

LogoLayout::LogoLayout()
    : SimpleLayout("ロゴ", "TitleLogo", 2, -1) {
    kill();
}

}  // namespace TitleSequenceProductSub

namespace {

NEW_NERVE(TitleSequenceProductDisplayEncouragePal60Window, TitleSequenceProduct, DisplayEncouragePal60Window);
NEW_NERVE(TitleSequenceProductBgmPrepare, TitleSequenceProduct, BgmPrepare);
NEW_NERVE(TitleSequenceProductLogoFadein, TitleSequenceProduct, LogoFadein);
NEW_NERVE(TitleSequenceProductLogoWait, TitleSequenceProduct, LogoWait);
NEW_NERVE(TitleSequenceProductLogoDisplay, TitleSequenceProduct, LogoDisplay);
NEW_NERVE(TitleSequenceProductDecide, TitleSequenceProduct, Decide);
NEW_NERVE(TitleSequenceProductDead, TitleSequenceProduct, Dead);

}  // namespace

TitleSequenceProduct::TitleSequenceProduct()
    : NerveExecutor("TitleSequenceProduct"),
      mLogoLayout(nullptr),
      mPressStartLayout(nullptr),
      mAButtonChecker(nullptr),
      mBButtonChecker(nullptr),
      mIsDisplayEncouragePal60Window(false),
      mOwnLayouts(true) {
    if (MR::isDisplayEncouragePal60Window()) {
        initNerve(&TitleSequenceProductDisplayEncouragePal60Window::sInstance);
    } else {
        initNerve(&TitleSequenceProductBgmPrepare::sInstance);
    }

    mLogoLayout = new TitleSequenceProductSub::LogoLayout();
    auto *press_start_layout = new SimpleLayout("Press[A][B]", "PressStart", 1, -1);
    press_start_layout->initWithoutIter();
    mPressStartLayout = press_start_layout;
    mAButtonChecker = new TriggerChecker();
    mBButtonChecker = new TriggerChecker();
    mIsDisplayEncouragePal60Window = MR::isDisplayEncouragePal60Window();

    kill();
}

TitleSequenceProduct::TitleSequenceProduct(LayoutActor *pLogoLayout, LayoutActor *pPressStartLayout)
    : NerveExecutor("TitleSequenceProduct"),
      mLogoLayout(pLogoLayout),
      mPressStartLayout(pPressStartLayout),
      mAButtonChecker(new TriggerChecker()),
      mBButtonChecker(new TriggerChecker()),
      mIsDisplayEncouragePal60Window(MR::isDisplayEncouragePal60Window()),
      mOwnLayouts(false) {
    if (MR::isDisplayEncouragePal60Window()) {
        initNerve(&TitleSequenceProductDisplayEncouragePal60Window::sInstance);
    } else {
        initNerve(&TitleSequenceProductBgmPrepare::sInstance);
    }

    kill();
}

TitleSequenceProduct::~TitleSequenceProduct() {
    if (mOwnLayouts) {
        delete mLogoLayout;
        delete mPressStartLayout;
    }
    delete mAButtonChecker;
    delete mBButtonChecker;
}

void TitleSequenceProduct::appear() {
    if (mIsDisplayEncouragePal60Window) {
        setNerve(&TitleSequenceProductDisplayEncouragePal60Window::sInstance);
    } else {
        setNerve(&TitleSequenceProductBgmPrepare::sInstance);
    }
}

void TitleSequenceProduct::kill() {
    setNerve(&TitleSequenceProductDead::sInstance);
}

bool TitleSequenceProduct::isActive() const {
    return not isNerve(&TitleSequenceProductDead::sInstance);
}

void TitleSequenceProduct::update() {
    updateNerve();

    if (mLogoLayout != nullptr) {
        mLogoLayout->movement();
    }

    if (mPressStartLayout != nullptr) {
        mPressStartLayout->movement();
    }
}

void TitleSequenceProduct::updateButtonReaction(TriggerChecker *pButtonChecker, const char *pAnimName) {
    if (pButtonChecker == nullptr || pAnimName == nullptr || mLogoLayout == nullptr) {
        return;
    }

    if (pButtonChecker->getOnTrigger()) {
        MR::startAnim(mLogoLayout, pAnimName, 1U);
        MR::setAnimFrameAndStop(mLogoLayout, 0.0F, 1U);
    } else if (pButtonChecker->getOffTrigger()) {
        MR::startAnim(mLogoLayout, pAnimName, 1U);
    }
}

void TitleSequenceProduct::updatePressStartReaction() {
    if (mPressStartLayout == nullptr || mAButtonChecker == nullptr || mBButtonChecker == nullptr) {
        return;
    }

    if (mAButtonChecker->getOnTrigger()) {
        MR::startAnim(mPressStartLayout, "ButtonReaction", 0U);
    } else if (mBButtonChecker->getOnTrigger()) {
        MR::startAnim(mPressStartLayout, "ButtonReaction", 0U);
    }
}

void TitleSequenceProduct::exeDisplayEncouragePal60Window() {
    setNerve(&TitleSequenceProductBgmPrepare::sInstance);
}

void TitleSequenceProduct::exeBgmPrepare() {
    if (MR::isFirstStep(this)) {
        MR::startStageBGM("STM_TITLE", true);
    }

    if (MR::isPreparedStageBgm()) {
        setNerve(&TitleSequenceProductLogoFadein::sInstance);
    }
}

void TitleSequenceProduct::exeLogoFadein() {
    if (mLogoLayout == nullptr) {
        setNerve(&TitleSequenceProductDead::sInstance);
        return;
    }

    if (MR::isFirstStep(this)) {
        mLogoLayout->appear();
        MR::startAnim(mLogoLayout, "Appear", 0U);
        MR::unlockStageBGM();
    }

    if (MR::isAnimStopped(mLogoLayout, 0U)) {
        setNerve(&TitleSequenceProductLogoWait::sInstance);
    }
}

void TitleSequenceProduct::exeLogoWait() {
    if (mLogoLayout == nullptr) {
        setNerve(&TitleSequenceProductDead::sInstance);
        return;
    }

    if (MR::isFirstStep(this)) {
        MR::startAnim(mLogoLayout, "Wait", 0U);
        MR::emitEffect(mLogoLayout, "TitleLogoLight");
        MR::emitEffect(mLogoLayout, "TitleLogoLightB");
        MR::emitEffect(mLogoLayout, "TitleLogoLightC");
        MR::emitEffect(mLogoLayout, "TitleLogoLightD");
        MR::emitEffect(mLogoLayout, "TitleLogoLightE");
        MR::emitEffect(mLogoLayout, "TitleLogoLightF");
        MR::emitEffect(mLogoLayout, "TitleLogoLightG");
    }

    if (MR::isStep(this, sPressABAppearFrame)) {
        setNerve(&TitleSequenceProductLogoDisplay::sInstance);
    }
}

void TitleSequenceProduct::exeLogoDisplay() {
    if (mPressStartLayout == nullptr || mAButtonChecker == nullptr || mBButtonChecker == nullptr) {
        setNerve(&TitleSequenceProductDead::sInstance);
        return;
    }

    if (MR::isFirstStep(this)) {
        mPressStartLayout->appear();
        MR::startAnim(mPressStartLayout, "Appear", 0U);
    }

    if (MR::isAnimStopped(mPressStartLayout, 0U)) {
        MR::startAnim(mPressStartLayout, "Wait", 0U);
    }

    mAButtonChecker->update(MR::testCorePadButtonA(0));
    mBButtonChecker->update(MR::testCorePadButtonB(0));

    if (mAButtonChecker->getLevel() && mBButtonChecker->getLevel()) {
        MR::stopStageBGM(75);
        MR::startSystemSE("SE_SY_GAME_START", -1, -1);
        MR::startCSSound("CS_CLICK_CLOSE", 0, 0);
        MR::tryRumblePadMiddle(this, 0);
        setNerve(&TitleSequenceProductDecide::sInstance);
    } else {
        updateButtonReaction(mAButtonChecker, "ReactionA");
        updateButtonReaction(mBButtonChecker, "ReactionB");
        updatePressStartReaction();
    }
}

void TitleSequenceProduct::exeDecide() {
    if (mLogoLayout == nullptr || mPressStartLayout == nullptr) {
        setNerve(&TitleSequenceProductDead::sInstance);
        return;
    }

    if (MR::isFirstStep(this)) {
        MR::startAnim(mLogoLayout, "Decide", 0U);
        MR::deleteEffectAll(mLogoLayout);
        MR::startAnim(mPressStartLayout, "End", 0U);
    }

    if (MR::isAnimStopped(mLogoLayout, 0U) && MR::isAnimStopped(mPressStartLayout, 0U)) {
        setNerve(&TitleSequenceProductDead::sInstance);
    }
}

void TitleSequenceProduct::exeDead() {
    if (not MR::isFirstStep(this)) {
        return;
    }

    if (mLogoLayout != nullptr) {
        mLogoLayout->kill();
    }

    if (mPressStartLayout != nullptr) {
        mPressStartLayout->kill();
    }
}

const LayoutActor *TitleSequenceProduct::getLogoLayout() const {
    return mLogoLayout;
}

const LayoutActor *TitleSequenceProduct::getPressStartLayout() const {
    return mPressStartLayout;
}
