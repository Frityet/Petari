#pragma once

#include "Game/LiveActor/LiveActor.hpp"
#include "JSystem/JGeometry/TQuat.hpp"

class AnimScaleController;
class HitSensor;
class ItemGenerator;
class KeySwitch;
class NameObjArchiveListCollector;
class WalkerStateBindStarPointer;
class WalkerStateChase;
class WalkerStateFindPlayer;
class WalkerStateStagger;
class WalkerStateWander;
class JMapInfoIter;

class KuriboChief : public LiveActor {
public:
    KuriboChief(const char*);
    virtual ~KuriboChief();
    virtual void init(const JMapInfoIter&);
    virtual void initAfterPlacement();
    virtual void kill();
    virtual void control();
    virtual void calcAndSetBaseMtx();
    virtual void attackSensor(HitSensor*, HitSensor*);
    virtual bool receiveMsgPlayerAttack(u32, HitSensor*, HitSensor*);
    virtual bool receiveMsgEnemyAttack(u32, HitSensor*, HitSensor*);
    virtual bool receiveOtherMsg(u32, HitSensor*, HitSensor*);

    void initSensor();
    void initState();
    void initKeySwitch(const JMapInfoIter&);
    static void makeArchiveList(NameObjArchiveListCollector*, const JMapInfoIter&);
    bool requestStagger(HitSensor*, HitSensor*);
    bool requestBlowDown(HitSensor*, HitSensor*);
    bool tryFind();
    bool tryPointBind();
    void exeWander();
    void exeFindPlayer();
    void exeChase();
    void exeStagger();
    void exeTrample();
    void exeAttackSuccess();
    void exeBindStarPointer();
    void endBindStarPointer();
    void exeBlowDown();
    void exeBlowDownLand();
    bool isEnableAttack() const;
    bool isEnableKick() const;
    bool isDown() const;

    AnimScaleController* mScaleController;              // 0x8C
    WalkerStateWander* mStateWander;                    // 0x90
    WalkerStateFindPlayer* mStateFindPlayer;            // 0x94
    WalkerStateChase* mStateChase;                      // 0x98
    WalkerStateStagger* mStateStagger;                  // 0x9C
    WalkerStateBindStarPointer* mBindStarPointer;       // 0xA0
    ItemGenerator* mItemGenerator;                      // 0xA4
    KeySwitch* mKeySwitch;                              // 0xA8
    TQuat4f mBaseQuat;                                  // 0xAC
    TVec3f mFrontVec;                                   // 0xBC
};
