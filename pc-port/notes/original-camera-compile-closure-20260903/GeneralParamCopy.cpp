#include "Game/Camera/CameraParamChunk.hpp"
CameraGeneralParam& CameraGeneralParam::operator=(const CameraGeneralParam& rOther) {
    mDist = rOther.mDist;
    mAxis = rOther.mAxis;
    mWPoint = rOther.mWPoint;
    mUp = rOther.mUp;
    mAngleA = rOther.mAngleA;
    mAngleB = rOther.mAngleB;
    mNum1 = rOther.mNum1;
    mNum2 = rOther.mNum2;
    mString = rOther.mString;

    return *this;
}
