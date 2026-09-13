#pragma once

#include "Game/MapObj/MapPartsFunction.hpp"
#include <JSystem/JGeometry.hpp>

class MapPartsRailRotator : public MapPartsFunction {
public:
    MapPartsRailRotator(LiveActor*);

    enum AxisType {
        Axis_X,
        Axis_Y,
        Axis_Z
    };

    virtual ~MapPartsRailRotator();
    virtual void init(const JMapInfoIter&);
    virtual bool isWorking() const;
    virtual void start();
    virtual void end();
    virtual f32 getJMapArgAngleFactor() const;

    bool hasRotation(s32) const;
    void rotateAtPoint(s32);
    bool hasRotationBetweenPoints(s32) const;
    void rotateBetweenPoints(s32, f32);
    void updateHostRotateMtx();
    void updateInfo(s32);
    bool isReachedTargetAngle() const;
    void calcRotateAxisDir(AxisType, TVec3f*) const;
    void updateRotateMtx(AxisType, f32);

    void initWithRotateMtx(const JMapInfoIter&, MtxPtr);

    void exeRotate();

    s32 mRotateAxis;
    s32 mRotateType;
    f32 mRotateSpeed;
    f32 mTargetAngle;
    f32 mAngle;
    TPos3f _2C;
    TPos3f _5C;
    MtxPtr mHostRotateMtx;
};
