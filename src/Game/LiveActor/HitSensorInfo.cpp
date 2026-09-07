#include "Game/LiveActor/HitSensorInfo.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"

HitSensorInfo::HitSensorInfo(const char* pName, HitSensor* pSensor, const TVec3f* a3, MtxPtr mtx, const register TVec3f& a5, bool a6) {
    mName = pName;
    mHashCode = MR::getHashCode(pName);
    mSensor = pSensor;

    _C = a5;

    _18 = a3;
    _1C = mtx;
    _20 = a6;
}

void HitSensorInfo::update() {
    if (_20) {
        mSensor->mHost->updateHitSensor(mSensor);
        return;
    }

    TVec3f position;
    if (_1C != nullptr) {
        position.set(_1C[0][3], _1C[1][3], _1C[2][3]);
        position.x += _1C[0][0] * _C.x + _1C[0][1] * _C.y + _1C[0][2] * _C.z;
        position.y += _1C[1][0] * _C.x + _1C[1][1] * _C.y + _1C[1][2] * _C.z;
        position.z += _1C[2][0] * _C.x + _1C[2][1] * _C.y + _1C[2][2] * _C.z;
    } else {
        if (_18 != nullptr) {
            position.set(_18->x, _18->y, _18->z);
        } else {
            position.set(mSensor->mHost->mPosition);
        }

        MtxPtr matrix = mSensor->mHost->getBaseMtx();
        if (matrix != nullptr) {
            position.x += matrix[0][0] * _C.x + matrix[0][1] * _C.y + matrix[0][2] * _C.z;
            position.y += matrix[1][0] * _C.x + matrix[1][1] * _C.y + matrix[1][2] * _C.z;
            position.z += matrix[2][0] * _C.x + matrix[2][1] * _C.y + matrix[2][2] * _C.z;
        } else {
            position += _C;
        }
    }
    mSensor->mPosition.set(position);
}

void HitSensorInfo::doObjCol() {
    for (s32 i = 0; i < mSensor->mSensorCount; i++) {
        if (!MR::isDead(mSensor->mSensors[i]->mHost)) {
            mSensor->mHost->attackSensor(mSensor, mSensor->mSensors[i]);
        }
    }
}
