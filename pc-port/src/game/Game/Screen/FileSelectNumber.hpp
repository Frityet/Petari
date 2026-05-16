#pragma once

#include "Game/Screen/LayoutActor.hpp"
#include "Game/System/NerveExecutor.hpp"

namespace FileSelectNumberSub {

class SelectAnimController : public NerveExecutor {
public:
    SelectAnimController(LayoutActor *pHost);

    void appear();
    void selectIn();
    void selectOut();
    void exeSelectInStart();
    void exeSelectIn();
    void exeSelectOutStart();
    void exeSelectOut();

private:
    /* 0x8 */ LayoutActor *mHost;
    /* 0x10 */ const Nerve *_C;
};

}  // namespace FileSelectNumberSub

class JMapInfoIter;

class FileSelectNumber : public LayoutActor {
public:
    FileSelectNumber(const char *pName);
    ~FileSelectNumber() override;

    virtual void init(const JMapInfoIter &);
    void initWithoutIter();
    virtual void appear();
    virtual void control();

    void disappear();
    void setNumber(s32);
    void onSelectIn();
    void onSelectOut();
    void exeAppear();
    void exeWait();
    void exeEnd();

private:
    /* 0x20 */ s32 mNumber;
    /* 0x28 */ FileSelectNumberSub::SelectAnimController *mSelectAnimCtrl;
};
