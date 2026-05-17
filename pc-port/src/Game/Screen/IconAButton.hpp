#pragma once

#include "Game/Screen/LayoutActor.hpp"

class IconAButton : public LayoutActor {
public:
    IconAButton(bool connectToScene, bool connectToPause);

    void init(const JMapInfoIter& rIter) override;
    void control() override;

    void setFollowActorPane(LayoutActor* pActor, const char* pName);
    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] bool isWait() const;
    void openWithTalk();
    void openWithRead();
    void openWithTurn();
    void openWithoutMessage();
    void term();
    void exeOpen();
    void exeWait();
    void exeTerm();
    void updateFollowPos();

    /* 0x20 */ TVec2f mFollowPos;
    /* 0x28 */ LayoutActor* mFollowActor;
    /* 0x2C */ char mFollowPaneName[24];
};
