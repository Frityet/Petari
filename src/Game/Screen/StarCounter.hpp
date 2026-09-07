#pragma once

#include "Game/Screen/LayoutActor.hpp"
#include "Game/Util/LayoutUtil.hpp"

class CountUpPaneRumbler;
class CounterLayoutAppearer;

class StarCounter : public LayoutActor {
public:
    /// @brief Creates a new `StarCounter`.
    StarCounter();

    virtual void init(const JMapInfoIter& rIter);
    virtual void appear();
    virtual void control();

    void disappear();
    bool isWait() const;
    bool isHiddenOrWait() const {
        return MR::isDead(this) || MR::isHiddenLayout(this) || isWait();
    }
    void exeAppear();
    void exeWait();
    void exeCountUp();
    void exeDisappear();

private:
    /* 0x20 */ s32 mPowerStarNum;
    /* 0x24 */ CounterLayoutAppearer* mLayoutAppearer;
    /* 0x28 */ CountUpPaneRumbler* mPaneRumbler;
};
