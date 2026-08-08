#pragma once

#include "Game/LiveActor/LiveActor.hpp"
#include <JSystem/JGeometry/TMatrix.hpp>

class SphereSelectorHandle : public LiveActor {
public:
    SphereSelectorHandle(const char*);
    virtual ~SphereSelectorHandle();
    virtual void init(const JMapInfoIter&);
    virtual void appear();
    virtual MtxPtr getBaseMtx() const {
        return (MtxPtr)&mBaseMtx;
    }
    virtual void control();
    virtual bool receiveOtherMsg(u32, HitSensor*, HitSensor*);

    bool isPointing() const;
    bool isHolding() const;
    void validateRotate();
    void invalidateRotate();
    bool tryRelease();
    void clearPointerVelocity();
    void stackPointerVelocity();
    TVec2f* getPointerVelocity();
    void resetRotateParam();
    void rotateAxisY();
    void rotateAxisX();
    void updateBaseMtx();
    void changeBgmRotateState();
    void playRotateSE();
    void setStateConfirmStartAtFirstStep();
    void exeWait();
    void exeHold();
    void exeSpin();
    void exeDemoRotate();
    void exeDisappear();
    void exeGalaxyConfirmStart();
    void exeGalaxyConfirmCancel();
    void exeIdleEndForFileSelect();

public:
    /* 0x08C */ bool mIsFileSelect;
    /* 0x090 */ TPos3f mBaseMtx;
    /* 0x0C0 */ TVec3f mFrontDir;
    /* 0x0CC */ f32 mRotateSpeed;
    /* 0x0D0 */ f32 mPrevRotateSpeed;
    /* 0x0D4 */ f32 mTiltSpeed;
    /* 0x0D8 */ f32 mPrevTiltSpeed;
    /* 0x0DC */ TVec3f mRotateAxis;
    /* 0x0E8 */ TVec3f mUpDir;
    /* 0x0F4 */ TVec2f mPointerVelocity[3];
    /* 0x10C */ s32 mPointerOffscreenStep;
    /* 0x110 */ TVec3f mConfirmPosition;
    /* 0x11C */ f32 _11C;
    /* 0x120 */ f32 _120;
    /* 0x124 */ f32 _124;
    /* 0x128 */ bool mIsBgmRotating;
};
