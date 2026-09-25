#include "Game/LiveActor/HitSensorKeeper.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/HitSensorInfo.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/HashUtil.hpp"
#include <algorithm>
#include <aurora/exception.hpp>
#include <cstring>
#include <stdexcept>
#include <utility>

HitSensorKeeper::HitSensorKeeper(int sensorCount) {
    if (sensorCount < 0) {
        aurora::throw_host_exception< std::invalid_argument >("HitSensorKeeper capacity must be non-negative");
    }
    mSensorCount = sensorCount;
    mSensorInfosSize = 0;
    mSensorInfos = nullptr;
    mTaking = nullptr;
    mTaken = nullptr;
    mSensorInfos = new HitSensorInfo*[sensorCount];

    for (s32 i = 0; i < mSensorCount; i++) {
        mSensorInfos[i] = nullptr;
    }
}

HitSensorKeeper::~HitSensorKeeper() {
    auto** infos = std::exchange(mSensorInfos, nullptr);
    const s32 size = std::exchange(mSensorInfosSize, 0);
    mTaking = nullptr;
    mTaken = nullptr;
    // A callback may retire another actor or keeper. Each actual live borrower
    // receives the sensor identity before the owned sensor storage is freed.
    for (s32 i = 0; i < size; ++i) {
        NameObj::notifyNativeSensorRetirement(infos[i]->mSensor);
    }
    for (s32 i = 0; i < size; ++i) {
        delete infos[i];
    }
    delete[] infos;
}

void HitSensorKeeper::releaseNativeReference(const HitSensor* removed) noexcept {
    if (mTaking == removed) {
        mTaking = nullptr;
    }
    if (mTaken == removed) {
        mTaken = nullptr;
    }
    for (s32 i = 0; i < mSensorInfosSize; ++i) {
        auto* sensor = mSensorInfos[i]->mSensor;
        if (sensor->mSensorCount == 0) {
            continue;
        }
        auto* end = sensor->mSensors + sensor->mSensorCount;
        auto* remaining = std::remove(sensor->mSensors, end, removed);
        std::fill(remaining, end, nullptr);
        sensor->mSensorCount = static_cast< u16 >(remaining - sensor->mSensors);
    }
}

HitSensor* HitSensorKeeper::add(const char* pName, u32 sensorType, u16 sensorGroupSize, f32 radius, LiveActor* pActor, const TVec3f& a6) {
    HitSensorInfo* pInfo = new HitSensorInfo(pName, new HitSensor(sensorType, sensorGroupSize, radius, pActor), nullptr, nullptr, a6, false);
    registHitSensorInfo(pInfo);
    return pInfo->mSensor;
}

HitSensor* HitSensorKeeper::addPos(const char* pName, u32 sensorType, u16 sensorGroupSize, f32 radius, LiveActor* pActor, const TVec3f* pPos,
                                   const TVec3f& a7) {
    HitSensorInfo* pInfo = new HitSensorInfo(pName, new HitSensor(sensorType, sensorGroupSize, radius, pActor), pPos, nullptr, a7, false);
    registHitSensorInfo(pInfo);
    return pInfo->mSensor;
}

HitSensor* HitSensorKeeper::addMtx(const char* pName, u32 sensorType, u16 sensorGroupSize, f32 radius, LiveActor* pActor, MtxPtr mtx,
                                   const TVec3f& a7) {
    HitSensorInfo* pInfo = new HitSensorInfo(pName, new HitSensor(sensorType, sensorGroupSize, radius, pActor), nullptr, mtx, a7, false);
    registHitSensorInfo(pInfo);
    return pInfo->mSensor;
}

HitSensor* HitSensorKeeper::addCallback(const char* pName, u32 sensorType, u16 sensorGroupSize, f32 radius, LiveActor* pActor) {
    HitSensorInfo* pInfo =
        new HitSensorInfo(pName, new HitSensor(sensorType, sensorGroupSize, radius, pActor), nullptr, nullptr, TVec3f(0.0f, 0.0f, 0.0f), true);
    registHitSensorInfo(pInfo);
    return pInfo->mSensor;
}

HitSensor* HitSensorKeeper::getSensor(const char* pSensorName) const {
    if (mSensorInfosSize == 1) {
        return mSensorInfos[0]->mSensor;
    }

    u32 hashCode = MR::getHashCode(pSensorName);

    for (s32 i = 0; i < mSensorInfosSize; i++) {
        if (hashCode == mSensorInfos[i]->mHashCode && !strcmp(mSensorInfos[i]->mName, pSensorName)) {
            return mSensorInfos[i]->mSensor;
        }
    }

    return nullptr;
}

void HitSensorKeeper::update() {
    for (s32 i = 0; i < mSensorInfosSize; i++) {
        mSensorInfos[i]->update();
    }
}

void HitSensorKeeper::doObjCol() {
    for (s32 i = 0; i < mSensorInfosSize; i++) {
        mSensorInfos[i]->doObjCol();
    }
}

void HitSensorKeeper::clear() {
    for (s32 i = 0; i < mSensorInfosSize; i++) {
        HitSensorInfo* pInfo = mSensorInfos[i];
        pInfo->mSensor->mSensorCount = 0;
    }
}

void HitSensorKeeper::validate() {
    for (s32 i = 0; i < mSensorInfosSize; i++) {
        mSensorInfos[i]->mSensor->validate();
    }
}

void HitSensorKeeper::invalidate() {
    for (s32 i = 0; i < mSensorInfosSize; i++) {
        mSensorInfos[i]->mSensor->invalidate();
    }
}

void HitSensorKeeper::validateBySystem() {
    for (s32 i = 0; i < mSensorInfosSize; i++) {
        mSensorInfos[i]->mSensor->validateBySystem();
    }
}

void HitSensorKeeper::invalidateBySystem() {
    for (s32 i = 0; i < mSensorInfosSize; i++) {
        mSensorInfos[i]->mSensor->invalidateBySystem();
    }
}

HitSensorInfo* HitSensorKeeper::getNthSensorInfo(int n) const {
    return mSensorInfos[n];
}

HitSensorInfo* HitSensorKeeper::getSensorInfo(const char* pSensorName) const {
    if (mSensorInfosSize == 1) {
        return mSensorInfos[0];
    }

    u32 hashCode = MR::getHashCode(pSensorName);

    for (s32 i = 0; i < mSensorInfosSize; i++) {
        if (hashCode == mSensorInfos[i]->mHashCode && !strcmp(mSensorInfos[i]->mName, pSensorName)) {
            return mSensorInfos[i];
        }
    }

    return nullptr;
}

void HitSensorKeeper::registHitSensorInfo(HitSensorInfo* pInfo) {
    mSensorInfos[mSensorInfosSize] = pInfo;
    mSensorInfosSize++;
    pInfo->update();
}
