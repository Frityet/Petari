#pragma once

#include "Game/System/NerveExecutor.hpp"

class ButtonPaneController;
class LayoutActor;

class YesNoController : public NerveExecutor {
public:
    explicit YesNoController(LayoutActor* pHost);
    ~YesNoController() override;

    void appear();
    void kill();
    void update();
    [[nodiscard]] bool isSelected() const;
    [[nodiscard]] bool isSelectedYes() const;
    [[nodiscard]] bool isDisappearStart() const;
    void setSE(const char* pCursorSE, const char* pYesSE, const char* pNoSE);
    bool trySelect();
    void exeSelecting();
    void exeDecided();
    void exeDisappear();
    void exeSelected();
    void exeNotSelected();

    /* 0x08 */ LayoutActor* mHost;
    /* 0x0C */ bool _C;
    /* 0x10 */ ButtonPaneController* mButtonYesPaneCtrl;
    /* 0x14 */ ButtonPaneController* mButtonNoPaneCtrl;
    /* 0x18 */ const char* mCursorSE;
    /* 0x1C */ const char* mYesSE;
    /* 0x20 */ const char* mNoSE;
};
