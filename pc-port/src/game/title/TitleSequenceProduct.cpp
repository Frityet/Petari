#include "TitleSequenceProduct.hpp"

#include <stdexcept>

#include "TitleRuntimeMR.hpp"
#include "runtime/Nerve.hpp"
#include "runtime/NerveUtil.hpp"
#include "runtime/Spine.hpp"

namespace smgpc::game::title {
namespace {

class NrvTitleSequenceProductDisplayEncouragePal60Window final : public runtime::Nerve {
public:
    static NrvTitleSequenceProductDisplayEncouragePal60Window sInstance;

    void execute(runtime::Spine *spine) const override {
        static_cast<TitleSequenceProduct *>(spine->getExecutor())->exeDisplayEncouragePal60Window();
    }
};

class NrvTitleSequenceProductBgmPrepare final : public runtime::Nerve {
public:
    static NrvTitleSequenceProductBgmPrepare sInstance;

    void execute(runtime::Spine *spine) const override {
        static_cast<TitleSequenceProduct *>(spine->getExecutor())->exeBgmPrepare();
    }
};

class NrvTitleSequenceProductLogoFadein final : public runtime::Nerve {
public:
    static NrvTitleSequenceProductLogoFadein sInstance;

    void execute(runtime::Spine *spine) const override {
        static_cast<TitleSequenceProduct *>(spine->getExecutor())->exeLogoFadein();
    }
};

class NrvTitleSequenceProductLogoWait final : public runtime::Nerve {
public:
    static NrvTitleSequenceProductLogoWait sInstance;

    void execute(runtime::Spine *spine) const override {
        static_cast<TitleSequenceProduct *>(spine->getExecutor())->exeLogoWait();
    }
};

class NrvTitleSequenceProductLogoDisplay final : public runtime::Nerve {
public:
    static NrvTitleSequenceProductLogoDisplay sInstance;

    void execute(runtime::Spine *spine) const override {
        static_cast<TitleSequenceProduct *>(spine->getExecutor())->exeLogoDisplay();
    }
};

class NrvTitleSequenceProductDecide final : public runtime::Nerve {
public:
    static NrvTitleSequenceProductDecide sInstance;

    void execute(runtime::Spine *spine) const override {
        static_cast<TitleSequenceProduct *>(spine->getExecutor())->exeDecide();
    }
};

class NrvTitleSequenceProductDead final : public runtime::Nerve {
public:
    static NrvTitleSequenceProductDead sInstance;

    void execute(runtime::Spine *spine) const override {
        static_cast<TitleSequenceProduct *>(spine->getExecutor())->exeDead();
    }
};

NrvTitleSequenceProductDisplayEncouragePal60Window NrvTitleSequenceProductDisplayEncouragePal60Window::sInstance {};
NrvTitleSequenceProductBgmPrepare NrvTitleSequenceProductBgmPrepare::sInstance {};
NrvTitleSequenceProductLogoFadein NrvTitleSequenceProductLogoFadein::sInstance {};
NrvTitleSequenceProductLogoWait NrvTitleSequenceProductLogoWait::sInstance {};
NrvTitleSequenceProductLogoDisplay NrvTitleSequenceProductLogoDisplay::sInstance {};
NrvTitleSequenceProductDecide NrvTitleSequenceProductDecide::sInstance {};
NrvTitleSequenceProductDead NrvTitleSequenceProductDead::sInstance {};

}  // namespace

TitleSequenceProduct::TitleSequenceProduct(TitleLayoutActor *pLogoLayout, TitleLayoutActor *pPressStartLayout)
    : mLogoLayout(pLogoLayout), mPressStartLayout(pPressStartLayout) {
    if (mLogoLayout == nullptr || mPressStartLayout == nullptr) {
        throw std::invalid_argument("TitleSequenceProduct requires non-null logo and press-start layouts.");
    }

    if (MR::isDisplayEncouragePal60Window()) {
        initNerve(&NrvTitleSequenceProductDisplayEncouragePal60Window::sInstance);
    } else {
        initNerve(&NrvTitleSequenceProductBgmPrepare::sInstance);
    }

    mIsDisplayEncouragePal60Window = MR::isDisplayEncouragePal60Window();
    kill();
}

void TitleSequenceProduct::appear() {
    if (mIsDisplayEncouragePal60Window) {
        setNerve(&NrvTitleSequenceProductDisplayEncouragePal60Window::sInstance);
    } else {
        setNerve(&NrvTitleSequenceProductBgmPrepare::sInstance);
    }
}

void TitleSequenceProduct::kill() {
    setNerve(&NrvTitleSequenceProductDead::sInstance);
}

bool TitleSequenceProduct::isActive() const {
    return not isNerve(&NrvTitleSequenceProductDead::sInstance);
}

void TitleSequenceProduct::update() {
    updateNerve();
    mLogoLayout->update(1.0F);
    mPressStartLayout->update(1.0F);
}

void TitleSequenceProduct::updateButtonReaction(runtime::TriggerChecker *pButtonChecker, const char *pAnimName) {
    if (pButtonChecker == nullptr || pAnimName == nullptr) {
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
    if (mAButtonChecker.getOnTrigger()) {
        MR::startAnim(mPressStartLayout, "ButtonReaction", 0U);
    } else if (mBButtonChecker.getOnTrigger()) {
        MR::startAnim(mPressStartLayout, "ButtonReaction", 0U);
    }
}

void TitleSequenceProduct::exeDisplayEncouragePal60Window() {
    setNerve(&NrvTitleSequenceProductBgmPrepare::sInstance);
}

void TitleSequenceProduct::exeBgmPrepare() {
    if (MR::isFirstStep(this)) {
        MR::startStageBGM("STM_TITLE", true);
    }

    if (MR::isPreparedStageBgm()) {
        setNerve(&NrvTitleSequenceProductLogoFadein::sInstance);
    }
}

void TitleSequenceProduct::exeLogoFadein() {
    if (MR::isFirstStep(this)) {
        mLogoLayout->appear();
        MR::startAnim(mLogoLayout, "Appear", 0U);
        MR::unlockStageBGM();
    }

    if (MR::isAnimStopped(mLogoLayout, 0U)) {
        setNerve(&NrvTitleSequenceProductLogoWait::sInstance);
    }
}

void TitleSequenceProduct::exeLogoWait() {
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
        setNerve(&NrvTitleSequenceProductLogoDisplay::sInstance);
    }
}

void TitleSequenceProduct::exeLogoDisplay() {
    if (MR::isFirstStep(this)) {
        mPressStartLayout->appear();
        MR::startAnim(mPressStartLayout, "Appear", 0U);
    }

    if (MR::isAnimStopped(mPressStartLayout, 0U)) {
        MR::startAnim(mPressStartLayout, "Wait", 0U);
    }

    mAButtonChecker.update(MR::testCorePadButtonA(0));
    mBButtonChecker.update(MR::testCorePadButtonB(0));

    if (mAButtonChecker.getLevel() && mBButtonChecker.getLevel()) {
        MR::stopStageBGM(75);
        MR::startSystemSE("SE_SY_GAME_START", -1, -1);
        MR::startCSSound("CS_CLICK_CLOSE", 0, 0);
        MR::tryRumblePadMiddle(this, 0);
        setNerve(&NrvTitleSequenceProductDecide::sInstance);
    } else {
        updateButtonReaction(&mAButtonChecker, "ReactionA");
        updateButtonReaction(&mBButtonChecker, "ReactionB");
        updatePressStartReaction();
    }
}

void TitleSequenceProduct::exeDecide() {
    if (MR::isFirstStep(this)) {
        MR::startAnim(mLogoLayout, "Decide", 0U);
        MR::deleteEffectAll(mLogoLayout);
        MR::startAnim(mPressStartLayout, "End", 0U);
    }

    if (MR::isAnimStopped(mLogoLayout, 0U) && MR::isAnimStopped(mPressStartLayout, 0U)) {
        setNerve(&NrvTitleSequenceProductDead::sInstance);
    }
}

void TitleSequenceProduct::exeDead() {
    if (MR::isFirstStep(this)) {
        mLogoLayout->kill();
        mPressStartLayout->kill();
    }
}

const TitleLayoutActor *TitleSequenceProduct::getLogoLayout() const {
    return mLogoLayout;
}

const TitleLayoutActor *TitleSequenceProduct::getPressStartLayout() const {
    return mPressStartLayout;
}

}  // namespace smgpc::game::title
