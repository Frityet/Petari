#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class HitSensor;
class PlantMember;

class PlantGroup : public LiveActor {
public:
    PlantGroup(const char*);
    virtual ~PlantGroup();

    virtual void init(const JMapInfoIter&);
    virtual void initAfterPlacement();
    virtual void makeActorAppeared();
    virtual void makeActorDead();
    virtual void startClipped();
    virtual void endClipped();
    virtual void control();
    virtual void attackSensor(HitSensor*, HitSensor*);
    virtual bool receiveOtherMsg(u32, HitSensor*, HitSensor*);

    void initMember(s32, const JMapInfoIter&);
    s32 placeOnCollisionFormCircle(TVec3f*, const TVec3f&, const TVec3f&, const TVec3f&);
    f32 calcBoundingSphereRadius(const TVec3f&);
    void emitHintEffect();

private:
    friend class PlantMember;

    /* 0x8C */ PlantMember** mMembers;
    /* 0x90 */ s32 mMemberCount;
    /* 0x94 */ s32 mPlantType;
    /* 0x98 */ bool mIsStarPiece;
    /* 0x99 */ u8 _99[3];
    /* 0x9C */ TVec3f mHintEffectPosition;
    /* 0xA8 */ TVec3f mHintEffectRotation;
    /* 0xB4 */ s32 mHintEffectTimer;
    /* 0xB8 */ s32 mHintStartIndex;
};
