#pragma once

#include "layout/BrlanRuntime.hpp"
#include "layout/BrlytRuntime.hpp"

#include "game/TriggerChecker.hpp"

namespace pcport::game {

// Ported from src/Game/Screen/TitleSequenceProduct.cpp with minimal runtime adaptation.
class TitleSequenceProduct {
public:
    enum class State {
        DisplayEncouragePal60Window,
        BgmPrepare,
        LogoFadein,
        LogoWait,
        LogoDisplay,
        Decide,
        Dead,
    };

    struct Animations {
        const BrlanAnimation* logoAppear = nullptr;
        const BrlanAnimation* logoWait = nullptr;
        const BrlanAnimation* logoDecide = nullptr;
        const BrlanAnimation* logoReactionA = nullptr;
        const BrlanAnimation* logoReactionB = nullptr;

        const BrlanAnimation* pressAppear = nullptr;
        const BrlanAnimation* pressWait = nullptr;
        const BrlanAnimation* pressEnd = nullptr;
        const BrlanAnimation* pressButtonReaction = nullptr;
    };

    static constexpr int kPressABAppearFrame = 25;

    TitleSequenceProduct(BrlytLayout& logoLayout, BrlytLayout& pressStartLayout, Animations animations);

    void appear();
    void kill();
    bool isActive() const;

    void update(float deltaFrames, bool aButtonPressed, bool bButtonPressed);

    State getState() const;
    float getStateFrame() const;

private:
    struct Playback {
        const BrlanAnimation* animation = nullptr;
        float frame = 0.0F;
    };

    void setState(State state);
    bool isFirstStep() const;

    void startLogoPrimary(const BrlanAnimation* animation);
    void startLogoReaction(Playback& slot, const BrlanAnimation* animation, bool holdAtStart);
    void startPressPrimary(const BrlanAnimation* animation);
    void startPressReaction(const BrlanAnimation* animation);

    static bool isPlaybackStopped(const Playback& playback);
    static void advancePlayback(Playback& playback, float deltaFrames, bool pause);

    void applyCurrentAnimations();

    void updateButtonReaction(TriggerChecker* checker, Playback& playback, bool& held, const BrlanAnimation* animation);
    void updatePressStartReaction();

    void exeDisplayEncouragePal60Window();
    void exeBgmPrepare();
    void exeLogoFadein();
    void exeLogoWait();
    void exeLogoDisplay();
    void exeDecide();
    void exeDead();

    BrlytLayout* mLogoLayout;
    BrlytLayout* mPressStartLayout;
    Animations mAnimations;

    State mState = State::Dead;
    float mStateFrame = 0.0F;

    TriggerChecker mAButtonChecker;
    TriggerChecker mBButtonChecker;

    bool mInputA = false;
    bool mInputB = false;
    bool mLogoReactionAHeld = false;
    bool mLogoReactionBHeld = false;

    Playback mLogoPrimary;
    Playback mLogoReactionA;
    Playback mLogoReactionB;
    Playback mPressPrimary;
    Playback mPressReaction;
};

}  // namespace pcport::game
