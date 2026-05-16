#pragma once

#include "Game/Screen/LayoutActor.hpp"

class ButtonPaneController;
class JMapInfoIter;

class BackButton : public LayoutActor {
public:
    BackButton(const char *pName, bool param2);
    ~BackButton() override;

    void init(const JMapInfoIter &rIter);
    void initWithoutIter();
    void appear() override;
    void control() override;

    void disappear();
    bool isHidden() const;
    bool isAppearing() const;
    bool isDisappearing() const;
    bool isPointing() const;

    /* 0x20 */ ButtonPaneController *mPaneCtrl;
    /* 0x24 */ bool _24;
    /* 0x25 */ bool _25;
};
