#pragma once

#include "Game/MapObj/MapPartsRotator.hpp"

class MapPartsSeesaw1AxisRotator : public MapPartsRotatorBase {
public:
    MapPartsSeesaw1AxisRotator(LiveActor*, const char*, f32);
    virtual ~MapPartsSeesaw1AxisRotator();
    virtual void init(const JMapInfoIter&);
    virtual bool isWorking() const;
    virtual void start();
    virtual void end();
    virtual bool receiveMsg(u32);
    virtual const TMtx34f& getRotateMtx() const { return mRotateMtx; }
    virtual bool isMoving() const;

    void exeMove();
    void exeStay();
    void exeHipDrop();
    void rotate();
    void updateVelocity();
    void updateRestoreForce();
    void clampAngularSpeed();
    f32 getDistanceFromRotAxis() const;
    void addForceHipDrop();
    bool isGoingToReachTargetAngle() const;
    void calcRotatedAngle(f32*, const TPos3f&) const;
    bool tryHipDrop();

    /* 0x18 */ f32 mRotateSpeed;
    /* 0x1C */ f32 mInertia;
    /* 0x20 */ f32 mRotateAngle;
    /* 0x24 */ f32 mRestoreForce;
    /* 0x28 */ TPos3f mRotateMtx;
    /* 0x58 */ bool mIsPlayerOn;
    /* 0x5C */ TVec3f mRotateAxis;
    /* 0x68 */ f32 mAngularSpeed;
    /* 0x6C */ f32 mForce;
    /* 0x70 */ TVec3f mBaseUp;
    /* 0x7C */ const char* mSoundName;
    /* 0x80 */ f32 mSoundSpeedThreshold;
};
