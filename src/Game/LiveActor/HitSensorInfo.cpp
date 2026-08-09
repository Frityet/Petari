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

    TVec3f sensorPos;

    if (_1C != nullptr) {
        sensorPos.set<f32>((*_1C)[3], (*_1C)[7], (*_1C)[11]);
        sensorPos.x += ((*_1C)[2] * _C.z) + (((*_1C)[0] * _C.x) + ((*_1C)[1] * _C.y));
        MtxPtr followMtx = _1C;
        sensorPos.y += ((*_1C)[6] * _C.z) + (((*_1C)[4] * _C.x) + ((*_1C)[5] * _C.y));
        sensorPos.z += ((*_1C)[10] * _C.z) + (((*_1C)[8] * _C.x) + ((*_1C)[9] * _C.y));
    }
    else {
        if (_18 != nullptr) {
            sensorPos.set<f32>(_18->x, _18->y, _18->z);
        }
        else {
            sensorPos.set<f32>(mSensor->mHost->mPosition);
        }

        MtxPtr baseMtx = mSensor->mHost->getBaseMtx();

        if (baseMtx != nullptr) {
            sensorPos.x += ((*baseMtx)[2] * _C.z) + (((*baseMtx)[0] * _C.x) + ((*baseMtx)[1] * _C.y));
            sensorPos.y += ((*baseMtx)[6] * _C.z) + (((*baseMtx)[4] * _C.x) + ((*baseMtx)[5] * _C.y));
            sensorPos.z += ((*baseMtx)[10] * _C.z) + (((*baseMtx)[8] * _C.x) + ((*baseMtx)[9] * _C.y));
        }
        else {
            sensorPos.add(_C);
        }
    }

    mSensor->mPosition.set<f32>(sensorPos);
}

void HitSensorInfo::doObjCol() {
    for (s32 i = 0; i < mSensor->mSensorCount; i++) {
        if (!MR::isDead(mSensor->mSensors[i]->mHost)) {
            mSensor->mHost->attackSensor(mSensor, mSensor->mSensors[i]);
        }
    }
}
