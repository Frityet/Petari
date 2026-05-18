#pragma once

#include <revolution.h>

#include "Game/LiveActor/LiveActor.hpp"

struct CameraTargetMatrix {
    Mtx mMtx;

    void identity();
};

class CameraTargetMtx : public NameObj {
public:
    explicit CameraTargetMtx(const char *pName);
    ~CameraTargetMtx() override;

    void movement() override;
    void invalidateLastMove();

    /* 0x10 */ CameraTargetMatrix mMatrix;
    /* 0x40 */ TVec3f mPosition;
    /* 0x4C */ TVec3f mLastMove;
    /* 0x58 */ TVec3f mGravityVector;
    /* 0x64 */ TVec3f mUp;
    /* 0x70 */ TVec3f mFront;
    /* 0x7C */ TVec3f mSide;
    /* 0x88 */ bool mInvalidLastMove;
};
