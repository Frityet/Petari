#pragma once

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "JSystem/JGeometry/TQuat.hpp"

class AnimScaleController;
class ItemGenerator;
class WalkerStateBindStarPointer;
class WalkerStateChase;
class WalkerStateFindPlayer;
class WalkerStateStagger;
class WalkerStateWander;

class KuriboMini : public LiveActor {
public:
    KuriboMini(const char*);

    virtual ~KuriboMini();
    virtual void init(const JMapInfoIter&);
    virtual void initAfterPlacement();
    virtual void makeActorAppeared();
    virtual void kill();
    virtual void control();
    virtual void calcAndSetBaseMtx();
    virtual void attackSensor(HitSensor*, HitSensor*);
    virtual bool receiveMsgPush(HitSensor*, HitSensor*);
    virtual bool receiveMsgPlayerAttack(u32, HitSensor*, HitSensor*);
    virtual bool receiveMsgEnemyAttack(u32, HitSensor*, HitSensor*);
    virtual bool receiveOtherMsg(u32, HitSensor*, HitSensor*);

    void initState();
    bool requestHipDropDown(HitSensor*, HitSensor*);
    bool requestFlatDown(HitSensor*, HitSensor*);
    bool requestPressDown();
    bool requestBlowDown(HitSensor*, HitSensor*);
    bool requestStagger(HitSensor*, HitSensor*);
    bool requestAttackSuccess();
    bool tryFind();
    bool tryPointBind();
    bool tryDeadMap();
    void exeWander();
    void exeFindPlayer();
    void exeChase();
    void exeStagger();
    void exeAttackSuccess();
    void exeHipDropDown();
    void exeFlatDown();
    void exePressDown();
    void exeBlowDown();
    void calcPassiveMovement();
    bool isEnableAttack() const;
    bool isEnableKick() const;
    bool isDown() const;

    AnimScaleController* mScaleController;         // 0x8C
    ItemGenerator* mItemGenerator;                 // 0x90
    WalkerStateWander* mStateWander;               // 0x94
    WalkerStateFindPlayer* mStateFindPlayer;       // 0x98
    WalkerStateChase* mStateChase;                 // 0x9C
    WalkerStateStagger* mStateStagger;             // 0xA0
    WalkerStateBindStarPointer* mBindStarPointer;  // 0xA4
    TQuat4f _A8;
    TVec3f _B8;
};
