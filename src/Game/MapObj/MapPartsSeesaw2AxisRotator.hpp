#pragma once

#include "Game/MapObj/MapPartsRotator.hpp"

class MapPartsSeesaw2AxisRotator : public MapPartsRotatorBase {
public:
    MapPartsSeesaw2AxisRotator(LiveActor*, const char*, f32);
    virtual ~MapPartsSeesaw2AxisRotator();
    virtual void init(const JMapInfoIter&);
    virtual bool isWorking() const;
    virtual void start();
    virtual void end();
    virtual bool receiveMsg(u32);
    virtual const TMtx34f& getRotateMtx() const { return mRotateMtx; }
    virtual bool isMoving() const;

    void rotate();
    void restoreMove();
    f32 getInertiaConst() const;
    void exeMove();
    void exeHipDrop();

    /* 0x18 */ f32 mRotateAngle;
    /* 0x1C */ f32 mInertia;
    /* 0x20 */ f32 mRestoreForce;
    /* 0x24 */ TPos3f mBaseMtx;
    /* 0x54 */ TPos3f mBaseMtxInv;
    /* 0x84 */ TPos3f mRotateMtx;
    /* 0xB4 */ TVec3f mBaseUp;
    /* 0xC0 */ bool mIsPlayerOn;
    /* 0xC4 */ f32 mRotateSpeed;
    /* 0xC8 */ const char* mSoundName;
    /* 0xCC */ f32 mSoundSpeedThreshold;
};
