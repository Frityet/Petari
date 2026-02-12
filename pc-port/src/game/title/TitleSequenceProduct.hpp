#pragma once

#include <cstdint>

#include "TitleLayoutActor.hpp"
#include "runtime/NerveExecutor.hpp"
#include "runtime/TriggerChecker.hpp"

namespace smgpc::game::title {

class TitleSequenceProduct final : public runtime::NerveExecutor {
public:
    static constexpr std::int32_t sPressABAppearFrame = 25;

    TitleSequenceProduct(TitleLayoutActor *pLogoLayout, TitleLayoutActor *pPressStartLayout);

    void appear();
    void kill();
    [[nodiscard]] bool isActive() const;
    void update();

    void updateButtonReaction(runtime::TriggerChecker *pButtonChecker, const char *pAnimName);
    void updatePressStartReaction();

    void exeDisplayEncouragePal60Window();
    void exeBgmPrepare();
    void exeLogoFadein();
    void exeLogoWait();
    void exeLogoDisplay();
    void exeDecide();
    void exeDead();

    [[nodiscard]] const TitleLayoutActor *getLogoLayout() const;
    [[nodiscard]] const TitleLayoutActor *getPressStartLayout() const;

private:
    TitleLayoutActor *mLogoLayout {};
    TitleLayoutActor *mPressStartLayout {};
    runtime::TriggerChecker mAButtonChecker {};
    runtime::TriggerChecker mBButtonChecker {};
    bool mIsDisplayEncouragePal60Window {};
};

}  // namespace smgpc::game::title
