#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class ActorCameraInfo;
class RunawayRabbit;
class RunawayTico;

class RunawayRabbitCollect : public LiveActor {
public:
    RunawayRabbitCollect(const char*);

    virtual ~RunawayRabbitCollect();
    virtual void init(const JMapInfoIter&);
    virtual void initAfterPlacement();
    virtual void control();

    s32 calcCompleteRabbitCount() const;
    void linkMsgCtrl();
    void noticeAppearRabbit(RunawayRabbit*);
    void noticeCaughtRabbit(RunawayRabbit*);
    void exeWait();
    void appearTico(RunawayTico*, const TVec3f&);
    void exeActive();

    ActorCameraInfo* mCameraInfo;  // 0x8C
    RunawayRabbit** mRabbits;      // 0x90
    s32 mRabbitCount;              // 0x94
    RunawayTico** mTicos;          // 0x98
    s32 mTicoCount;                // 0x9C
    s32 mAppearedTicoCount;        // 0xA0
    s32 mCaughtRabbitCount;        // 0xA4
    s32 mCompleteRabbitCount;      // 0xA8
    s32 mBgmState;                 // 0xAC
    bool mHasAppearedTico[3];      // 0xB0
};
