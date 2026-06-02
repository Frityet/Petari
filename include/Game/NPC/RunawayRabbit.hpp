#pragma once

#include "Game/LiveActor/LiveActor.hpp"
#include "JSystem/JGeometry/TQuat.hpp"

class FootPrint;
class RunawayRabbitCollect;
class SpotMarkLight;
class TalkMessageCtrl;
class WalkerStateBlowDamage;
class WalkerStateRunaway;

class RunawayRabbit : public LiveActor {
public:
    RunawayRabbit(const char*, RunawayRabbitCollect*);

    virtual ~RunawayRabbit();
    virtual void init(const JMapInfoIter&);
    virtual void initAfterPlacement();
    virtual void appear();
    virtual void control();
    virtual void calcAndSetBaseMtx();
    virtual void attackSensor(HitSensor*, HitSensor*);
    virtual bool receiveMsgPlayerAttack(u32, HitSensor*, HitSensor*);
    virtual bool receiveMsgEnemyAttack(u32, HitSensor*, HitSensor*);
    virtual bool receiveMsgPush(HitSensor*, HitSensor*);
    virtual bool receiveOtherMsg(u32, HitSensor*, HitSensor*);

    void initSensor();
    void updatePose() NO_INLINE;
    void updateBindActorMatrix();
    void activate();
    void startRunnaway();
    void incrementRunawayLevel();
    void setLastMessage();
    void setMessage();
    void setNotCaughtable();
    void startJumpSound();
    void setMsgCtrl(TalkMessageCtrl*);
    void exeHide();
    void exeAppear();
    void exeRunaway();
    void exeCaught();
    void exeCaughtTalk();
    void exeCaughtEnd();
    void exeStop();
    bool isCaught() const NO_INLINE;
    bool isCaughtable() const NO_INLINE;
    bool isRunnaway() const;
    bool isChasing() const;
    bool isEnableBlow() const;
    bool isValidFollow(s32) const;

    WalkerStateRunaway* mRunawayState;        // 0x8C
    WalkerStateBlowDamage* mBlowDamageState;  // 0x90
    RunawayRabbitCollect* mCollector;         // 0x94
    FootPrint* mFootPrint;           // 0x98
    SpotMarkLight* mSpotLight;       // 0x9C
    TalkMessageCtrl* mMsgCtrl;       // 0xA0
    TQuat4f mQuat;                   // 0xA4
    TVec3f mFrontVec;                // 0xB4
    TQuat4f mBindQuat;               // 0xC0
    TVec3f mBindFrontVec;            // 0xD0
    u32 _DC;
    s32 mGroupId;                    // 0xE0
    s32 mRunawayLevel;               // 0xE4
    s32 mMessageId;                  // 0xE8
    s32 _EC;
    s32 _F0;
    bool mIsCaughtable;              // 0xF4
    bool mHasAppearedTico;           // 0xF5
    f32 mRunawayDistance;            // 0xF8
};
