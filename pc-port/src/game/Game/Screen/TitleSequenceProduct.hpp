#pragma once

#include "Game/Screen/SimpleLayout.hpp"
#include "Game/System/NerveExecutor.hpp"

class TriggerChecker;

namespace TitleSequenceProductSub {
class LogoLayout;
}

class TitleSequenceProduct : public NerveExecutor {
public:
    static constexpr s32 sPressABAppearFrame = 25;

    TitleSequenceProduct();
    TitleSequenceProduct(LayoutActor *pLogoLayout, LayoutActor *pPressStartLayout);
    ~TitleSequenceProduct() override;

    void appear();
    void kill();
    [[nodiscard]] bool isActive() const;
    void update();

    void updateButtonReaction(TriggerChecker *pButtonChecker, const char *pAnimName);
    void updatePressStartReaction();

    void exeDisplayEncouragePal60Window();
    void exeBgmPrepare();
    void exeLogoFadein();
    void exeLogoWait();
    void exeLogoDisplay();
    void exeDecide();
    void exeDead();

    [[nodiscard]] const LayoutActor *getLogoLayout() const;
    [[nodiscard]] const LayoutActor *getPressStartLayout() const;

private:
    /* 0x10 */ LayoutActor *mLogoLayout;
    /* 0x18 */ LayoutActor *mPressStartLayout;
    /* 0x20 */ TriggerChecker *mAButtonChecker;
    /* 0x28 */ TriggerChecker *mBButtonChecker;
    /* 0x30 */ bool mIsDisplayEncouragePal60Window;
    /* 0x31 */ bool mOwnLayouts;
};

namespace TitleSequenceProductSub {

class LogoLayout : public SimpleLayout {
public:
    LogoLayout();
};

}  // namespace TitleSequenceProductSub
