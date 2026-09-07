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

J3DIndTexMtxInfo& J3DIndTexMtxInfo::operator=(const J3DIndTexMtxInfo& other) {
    f32 a = other.field_0x0[0][0], b = other.field_0x0[0][1], c = other.field_0x0[0][2];
    f32 d = other.field_0x0[1][0], e = other.field_0x0[1][1], f = other.field_0x0[1][2];
    u8 exponent = other.field_0x18;
    field_0x0[0][0] = a;
    field_0x0[0][1] = b;
    field_0x0[0][2] = c;
    field_0x0[1][0] = d;
    field_0x0[1][1] = e;
    field_0x0[1][2] = f;
    field_0x18 = exponent;
    return *this;
}

J3DFogInfo& J3DFogInfo::operator=(const J3DFogInfo& other) {
    u8 type = other.mType, adjEnable = other.mAdjEnable;
    u16 center = other.mCenter;
    f32 startZ = other.mStartZ, endZ = other.mEndZ, nearZ = other.mNearZ, farZ = other.mFarZ;
    u8 red = other.mColor.r, green = other.mColor.g, blue = other.mColor.b, alpha = other.mColor.a;
    mType = type;
    mAdjEnable = adjEnable;
    mCenter = center;
    mStartZ = startZ;
    mEndZ = endZ;
    mNearZ = nearZ;
    mFarZ = farZ;
    mColor.r = red;
    mColor.g = green;
    mColor.b = blue;
    mColor.a = alpha;
    for (u32 i = 0; i < 10; ++i) {
        mFogAdjTable.r[i] = other.mFogAdjTable.r[i];
    }
    return *this;
}

J3DNBTScaleInfo& J3DNBTScaleInfo::operator=(const J3DNBTScaleInfo& other) {
    mbHasScale = other.mbHasScale;
    mScale.x = other.mScale.x;
    mScale.y = other.mScale.y;
    mScale.z = other.mScale.z;
    return *this;
}
