#pragma once

#include "Game/Screen/LayoutActor.hpp"
#include "Game/System/NerveExecutor.hpp"

namespace FileSelectNumberSub {
    class SelectAnimController : public NerveExecutor {
    public:
        SelectAnimController(LayoutActor* pHost);

        void appear();
        void selectIn();
        void selectOut();
        void exeSelectInStart();
        void exeSelectIn();
        void exeSelectOutStart();
        void exeSelectOut();

    private:
        /* 0x8 */ LayoutActor* mHost;
        /* 0xC */ const Nerve* _C = nullptr;
    };
};  // namespace FileSelectNumberSub

class FileSelectNumber : public LayoutActor {
public:
    FileSelectNumber(const char* pName);

    void init(const JMapInfoIter&) override;
    void appear() override;
    void control() override;

    void disappear();
    void setNumber(s32);
    void onSelectIn();
    void onSelectOut();
    void exeAppear();
    void exeWait();
    void exeEnd();

    [[nodiscard]] s32 getNumber() const;

private:
    /* 0x20 */ s32 mNumber;
    /* 0x24 */ FileSelectNumberSub::SelectAnimController* mSelectAnimCtrl;
};
