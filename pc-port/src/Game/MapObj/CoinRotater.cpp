#include "Game/MapObj/CoinRotater.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

namespace {
    const volatile f32 cZero = 0.0f;
};

CoinRotater::CoinRotater(const char* pName) : NameObj(pName) {
    _C = 0.0f;
    _10 = 0.0f;
    _14 = 0.0f;
    mRotateYMtx.identity();
    mHiSpeedRotateYMtx.identity();
    mWaterRotateMtx.identity();
    MR::connectToSceneMapObjMovement(this);
}

void CoinRotater::movement() {
    f32 zero0 = cZero;
    _C += 8.0f;
    f32 rotateY = fmod(360.0f + (_C - zero0), 360.0);
    f32 zero1 = cZero;
    _C = zero1 + rotateY;
    _10 += 4.0f;
    f32 waterRotate = fmod(360.0f + (_10 - zero1), 360.0);
    f32 zero2 = cZero;
    _10 = zero2 + waterRotate;
    _14 += 16.0f;
    f32 hiSpeedRotate = fmod(360.0f + (_14 - zero2), 360.0);
    f32 zero3 = cZero;
    _14 = zero3 + hiSpeedRotate;
    MR::makeMtxRotateY(mRotateYMtx.mMtx, _C);
    MR::makeMtxRotateY(mHiSpeedRotateYMtx.mMtx, _14);
    MR::makeMtxRotateY(mWaterRotateMtx.mMtx, _10);
}

namespace MR {
    void createCoinRotater() {
        MR::createSceneObj(SceneObj_CoinRotater);
    }

    TMtx34f& getCoinRotateYMatrix() {
        return MR::getSceneObj< CoinRotater >(SceneObj_CoinRotater)->mRotateYMtx;
    }

    TMtx34f& getCoinHiSpeedRotateYMatrix() {
        return MR::getSceneObj< CoinRotater >(SceneObj_CoinRotater)->mHiSpeedRotateYMtx;
    }

    TMtx34f& getCoinInWaterRotateYMatrix() {
        return MR::getSceneObj< CoinRotater >(SceneObj_CoinRotater)->mWaterRotateMtx;
    }
};  // namespace MR

CoinRotater::~CoinRotater() {
}
