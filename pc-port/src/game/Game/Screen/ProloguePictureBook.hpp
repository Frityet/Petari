#pragma once

#include "Game/Screen/LayoutActor.hpp"

class IconAButton;
class JMapInfoIter;

class ProloguePictureBook : public LayoutActor {
public:
    ProloguePictureBook();
    ~ProloguePictureBook() override;

    virtual void init(const JMapInfoIter &rIter);
    void initWithoutIter();
    virtual void appear();
    virtual void kill();
    virtual void control();

    void exeActive();
    void exePlaying();
    void exeKeyWait();
    void exeEnd();
    bool isEnd() const;

private:
    /* 0x20 */ IconAButton *mAButtonIcon;
    /* 0x28 */ u32 mPage;
};
