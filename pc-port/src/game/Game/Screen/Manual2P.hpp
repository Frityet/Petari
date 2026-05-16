#pragma once

#include "Game/Screen/LayoutActor.hpp"

class BackButton;
class ButtonPaneController;
class JMapInfoIter;

namespace smgpc::render::layout {
class LayoutDrawList;
}

class Manual2P : public LayoutActor {
public:
    Manual2P(const char *pName);
    ~Manual2P() override;

    void init(const JMapInfoIter &rIter);
    void initWithoutIter();
    void appear() override;
    void control() override;
    void movementWithChildren();
    void appendDrawCommandsWithChildren(smgpc::render::layout::LayoutDrawList *pDrawList) const;

    bool isClosed() const;
    void exeAppear();
    void exeWait();
    void exeScrollRight();
    void exeScrollRightAfter();
    void exeScrollLeft();
    void exeScrollLeftAfter();
    void exeDisappear();
    void reflectPageIndex();
    bool checkSelectedBackButton();

private:
    /* 0x20 */ s32 mPageIndex;
    /* 0x24 */ s32 _24;
    /* 0x28 */ ButtonPaneController *mLeftPaneCtrl;
    /* 0x2C */ ButtonPaneController *mRightPaneCtrl;
    /* 0x30 */ bool _30;
    /* 0x31 */ bool _31;
    /* 0x34 */ BackButton *mBackButton;
};
