#pragma once

#include "Game/Screen/LayoutActor.hpp"

class ButtonPaneController;

class BrosButton : public LayoutActor {
public:
    explicit BrosButton(const char* pName);
    ~BrosButton() override;

    void init(const JMapInfoIter& rIter) override;
    void appear() override;
    void control() override;

    void appear(bool isSelectedMario);
    void disappear();
    bool isSelected() const;
    bool isSelectedMario() const;
    void resume();
    void exeAppear();
    void exeSelect();
    void exeDecide();
    void exeDisappear();

private:
    /* 0x20 */ bool mIsSelectedMario;
    /* 0x24 */ ButtonPaneController* mPaneCtrl;
};
