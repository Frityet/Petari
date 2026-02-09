#include "game/TitleSequenceProduct.hpp"

#include "core/Logger.hpp"

#include <stdexcept>
#include <string>

namespace pcport::game {

TitleSequenceProduct::TitleSequenceProduct(BrlytLayout& logoLayout, BrlytLayout& pressStartLayout, Animations animations)
    : mLogoLayout(&logoLayout), mPressStartLayout(&pressStartLayout), mAnimations(animations) {
    if (mAnimations.logoAppear == nullptr || mAnimations.logoWait == nullptr || mAnimations.logoDecide == nullptr ||
        mAnimations.logoReactionA == nullptr || mAnimations.logoReactionB == nullptr || mAnimations.pressAppear == nullptr ||
        mAnimations.pressWait == nullptr || mAnimations.pressEnd == nullptr || mAnimations.pressButtonReaction == nullptr) {
        throw std::runtime_error("TitleSequenceProduct requires all title and press-start animations");
    }

    kill();
}

void TitleSequenceProduct::appear() {
    mLogoLayout->ResetAnimationState();
    mPressStartLayout->ResetAnimationState();
    setState(State::BgmPrepare);
}

void TitleSequenceProduct::kill() {
    mLogoPrimary = Playback{};
    mLogoReactionA = Playback{};
    mLogoReactionB = Playback{};
    mPressPrimary = Playback{};
    mPressReaction = Playback{};

    mLogoReactionAHeld = false;
    mLogoReactionBHeld = false;

    mAButtonChecker.setInput(false);
    mBButtonChecker.setInput(false);

    mLogoLayout->ResetAnimationState();
    mPressStartLayout->ResetAnimationState();

    setState(State::Dead);
    applyCurrentAnimations();
}

bool TitleSequenceProduct::isActive() const {
    return mState != State::Dead;
}

TitleSequenceProduct::State TitleSequenceProduct::getState() const {
    return mState;
}

float TitleSequenceProduct::getStateFrame() const {
    return mStateFrame;
}

void TitleSequenceProduct::setState(State state) {
    mState = state;
    mStateFrame = 0.0F;

    switch (state) {
    case State::DisplayEncouragePal60Window:
        Log(LogLevel::Info, LogCategory::Menu, "TitleSequence state -> DisplayEncouragePal60Window");
        break;
    case State::BgmPrepare:
        Log(LogLevel::Info, LogCategory::Menu, "TitleSequence state -> BgmPrepare");
        break;
    case State::LogoFadein:
        Log(LogLevel::Info, LogCategory::Menu, "TitleSequence state -> LogoFadein");
        break;
    case State::LogoWait:
        Log(LogLevel::Info, LogCategory::Menu, "TitleSequence state -> LogoWait");
        break;
    case State::LogoDisplay:
        Log(LogLevel::Info, LogCategory::Menu, "TitleSequence state -> LogoDisplay");
        break;
    case State::Decide:
        Log(LogLevel::Info, LogCategory::Menu, "TitleSequence state -> Decide");
        break;
    case State::Dead:
        Log(LogLevel::Info, LogCategory::Menu, "TitleSequence state -> Dead");
        break;
    }
}

bool TitleSequenceProduct::isFirstStep() const {
    return mStateFrame <= 0.0F;
}

void TitleSequenceProduct::startLogoPrimary(const BrlanAnimation* animation) {
    mLogoPrimary.animation = animation;
    mLogoPrimary.frame = 0.0F;
}

void TitleSequenceProduct::startLogoReaction(Playback& slot, const BrlanAnimation* animation, bool holdAtStart) {
    slot.animation = animation;
    slot.frame = 0.0F;
    if (!holdAtStart) {
        return;
    }
}

void TitleSequenceProduct::startPressPrimary(const BrlanAnimation* animation) {
    mPressPrimary.animation = animation;
    mPressPrimary.frame = 0.0F;
}

void TitleSequenceProduct::startPressReaction(const BrlanAnimation* animation) {
    mPressReaction.animation = animation;
    mPressReaction.frame = 0.0F;
}

bool TitleSequenceProduct::isPlaybackStopped(const Playback& playback) {
    if (playback.animation == nullptr) {
        return true;
    }

    if (playback.animation->IsLooped()) {
        return false;
    }

    return playback.frame >= static_cast<float>(playback.animation->GetFrameSize());
}

void TitleSequenceProduct::advancePlayback(Playback& playback, float deltaFrames, bool pause) {
    if (playback.animation == nullptr || pause || deltaFrames <= 0.0F) {
        return;
    }

    playback.frame += deltaFrames;

    if (!playback.animation->IsLooped()) {
        const float end = static_cast<float>(playback.animation->GetFrameSize());
        if (playback.frame > end) {
            playback.frame = end;
        }
    }
}

void TitleSequenceProduct::applyCurrentAnimations() {
    if (mLogoPrimary.animation != nullptr) {
        mLogoPrimary.animation->ApplyToLayout(*mLogoLayout, mLogoPrimary.frame);
    }

    if (mLogoReactionA.animation != nullptr && !isPlaybackStopped(mLogoReactionA)) {
        mLogoReactionA.animation->ApplyToLayout(*mLogoLayout, mLogoReactionA.frame);
    }

    if (mLogoReactionB.animation != nullptr && !isPlaybackStopped(mLogoReactionB)) {
        mLogoReactionB.animation->ApplyToLayout(*mLogoLayout, mLogoReactionB.frame);
    }

    if (mPressPrimary.animation != nullptr) {
        mPressPrimary.animation->ApplyToLayout(*mPressStartLayout, mPressPrimary.frame);
    }

    if (mPressReaction.animation != nullptr && !isPlaybackStopped(mPressReaction)) {
        mPressReaction.animation->ApplyToLayout(*mPressStartLayout, mPressReaction.frame);
    }
}

void TitleSequenceProduct::updateButtonReaction(TriggerChecker* checker, Playback& playback, bool& held, const BrlanAnimation* animation) {
    if (checker->getOnTrigger()) {
        startLogoReaction(playback, animation, true);
        held = true;
    } else if (checker->getOffTrigger()) {
        startLogoReaction(playback, animation, false);
        held = false;
    }
}

void TitleSequenceProduct::updatePressStartReaction() {
    if (mAButtonChecker.getOnTrigger() || mBButtonChecker.getOnTrigger()) {
        startPressReaction(mAnimations.pressButtonReaction);
    }
}

void TitleSequenceProduct::exeDisplayEncouragePal60Window() {
    setState(State::BgmPrepare);
}

void TitleSequenceProduct::exeBgmPrepare() {
    if (isFirstStep()) {
        // Title BGM preparation is external in this PC bootstrap.
    }

    setState(State::LogoFadein);
}

void TitleSequenceProduct::exeLogoFadein() {
    if (isFirstStep()) {
        startLogoPrimary(mAnimations.logoAppear);
    }

    if (isPlaybackStopped(mLogoPrimary)) {
        setState(State::LogoWait);
    }
}

void TitleSequenceProduct::exeLogoWait() {
    if (isFirstStep()) {
        startLogoPrimary(mAnimations.logoWait);
    }

    if (mStateFrame >= static_cast<float>(kPressABAppearFrame)) {
        setState(State::LogoDisplay);
    }
}

void TitleSequenceProduct::exeLogoDisplay() {
    if (isFirstStep()) {
        startPressPrimary(mAnimations.pressAppear);
    }

    if (mPressPrimary.animation == mAnimations.pressAppear && isPlaybackStopped(mPressPrimary)) {
        startPressPrimary(mAnimations.pressWait);
    }

    mAButtonChecker.update(mInputA);
    mBButtonChecker.update(mInputB);

    if (mAButtonChecker.getLevel() && mBButtonChecker.getLevel()) {
        setState(State::Decide);
    } else {
        updateButtonReaction(&mAButtonChecker, mLogoReactionA, mLogoReactionAHeld, mAnimations.logoReactionA);
        updateButtonReaction(&mBButtonChecker, mLogoReactionB, mLogoReactionBHeld, mAnimations.logoReactionB);
        updatePressStartReaction();
    }
}

void TitleSequenceProduct::exeDecide() {
    if (isFirstStep()) {
        startLogoPrimary(mAnimations.logoDecide);
        startPressPrimary(mAnimations.pressEnd);
        mLogoReactionAHeld = false;
        mLogoReactionBHeld = false;
    }

    if (isPlaybackStopped(mLogoPrimary) && isPlaybackStopped(mPressPrimary)) {
        setState(State::Dead);
    }
}

void TitleSequenceProduct::exeDead() {}

void TitleSequenceProduct::update(float deltaFrames, bool aButtonPressed, bool bButtonPressed) {
    if (deltaFrames < 0.0F) {
        deltaFrames = 0.0F;
    }

    mInputA = aButtonPressed;
    mInputB = bButtonPressed;

    const State stateBefore = mState;

    switch (mState) {
    case State::DisplayEncouragePal60Window:
        exeDisplayEncouragePal60Window();
        break;
    case State::BgmPrepare:
        exeBgmPrepare();
        break;
    case State::LogoFadein:
        exeLogoFadein();
        break;
    case State::LogoWait:
        exeLogoWait();
        break;
    case State::LogoDisplay:
        exeLogoDisplay();
        break;
    case State::Decide:
        exeDecide();
        break;
    case State::Dead:
        exeDead();
        break;
    }

    advancePlayback(mLogoPrimary, deltaFrames, false);
    advancePlayback(mLogoReactionA, deltaFrames, mLogoReactionAHeld);
    advancePlayback(mLogoReactionB, deltaFrames, mLogoReactionBHeld);
    advancePlayback(mPressPrimary, deltaFrames, false);
    advancePlayback(mPressReaction, deltaFrames, false);

    if (mState == stateBefore) {
        mStateFrame += deltaFrames;
    }

    applyCurrentAnimations();
}

}  // namespace pcport::game
