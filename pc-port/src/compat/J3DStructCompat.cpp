#include "JSystem/J3DGraphBase/J3DStruct.hpp"
#include "JSystem/JMath/JMath.hpp"

void J3DTexMtxInfo::operator=(const J3DTexMtxInfo& other) {
    mProjection = other.mProjection;
    mInfo = other.mInfo;
    JMathInlineVEC::PSVECCopy(&other.mCenter, &mCenter);
    mSRT = other.mSRT;
    JMath::gekko_ps_copy16(mEffectMtx, other.mEffectMtx);
}

void J3DTexMtxInfo::setEffectMtx(Mtx matrix) {
    JMath::gekko_ps_copy12(mEffectMtx, matrix);
    mEffectMtx[3][0] = 0.0f;
    mEffectMtx[3][1] = 0.0f;
    mEffectMtx[3][2] = 0.0f;
    mEffectMtx[3][3] = 1.0f;
}
