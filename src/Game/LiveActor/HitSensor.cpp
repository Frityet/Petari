#include "Game/LiveActor/HitSensor.hpp"

HitSensor::HitSensor(u32 type, u16 groupSize, f32 radius, LiveActor* pHost)
    : mType(type), mPosition(0.0f, 0.0f, 0.0f), mRadius(radius), mSensorCount(0), mGroupSize(groupSize),
      mSensors(nullptr), mSensorGroup(nullptr), mValidBySystem(false), mValidByHost(true), mHost(pHost) {
    if (mGroupSize == 0U) {
        return;
    }

    mSensors = new HitSensor*[mGroupSize];
    for (u16 i = 0U; i < mGroupSize; ++i) {
        mSensors[i] = nullptr;
    }
}

bool HitSensor::receiveMessage(u32 msg, HitSensor* pSender) {
    if (mHost == nullptr) {
        return false;
    }

    return mHost->receiveMessage(msg, pSender, this);
}

void HitSensor::setType(u32 type) {
    mType = type;
    mSensorCount = 0U;
}

bool HitSensor::isType(u32 type) const {
    return mType == type;
}

void HitSensor::validate() {
    mValidByHost = true;
}

void HitSensor::invalidate() {
    mValidByHost = false;
    mSensorCount = 0U;
}

void HitSensor::validateBySystem() {
    mValidBySystem = true;
}

void HitSensor::invalidateBySystem() {
    mValidBySystem = false;
    mSensorCount = 0U;
}

void HitSensor::addHitSensor(HitSensor* pSensor) {
    if (mSensors == nullptr || mSensorCount >= mGroupSize) {
        return;
    }

    mSensors[mSensorCount++] = pSensor;
}
