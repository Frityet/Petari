#include "Game/Map/WhirlPoolAccelerator.hpp"
#include "Game/Util/MathUtil.hpp"

WhirlPoolAccelerator::WhirlPoolAccelerator(const char* pName)
    : LiveActor(pName), mRadius(100.0f), mHeight(100.0f), mUp(0.0f, 1.0f, 0.0f), mPointCount(0), mPoints(nullptr),
      mRotationAngle(0.0f), mTexOffset1(MR::getRandom()), mTexOffset2(MR::getRandom()), mTexture(nullptr),
      mClippingCenter(0.0f, 0.0f, 0.0f) {
}

WhirlPoolAccelerator::~WhirlPoolAccelerator() {
}

bool WhirlPoolAccelerator::calcInfo(const TVec3f& rPos, TVec3f* pVelocity) const {
    pVelocity->zero();
    TVec3f offset(rPos);
    offset -= mPosition;
    f32 height = mUp.dot(offset);
    if (height < 0.0f || height > mHeight) {
        return false;
    }
    MR::vecKillElement(offset, mUp, &offset);
    if (PSVECMag(&offset) > mRadius) {
        return false;
    }
    PSVECCrossProduct(&offset, &mUp, pVelocity);
    MR::normalize(pVelocity);
    pVelocity->scale(3.0f);
    return true;
}
