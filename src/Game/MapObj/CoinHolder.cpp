#include "Game/MapObj/CoinHolder.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/MapObj/Coin.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util.hpp"

void FORCE_SCALE() {
    TVec3f vec;
    vec.scale(1.0f);
}

CoinHolder::CoinHolder(const char* pName) : DeriveActorGroup< Coin >(pName, 0x200), mHostInfoArr(nullptr), mHostInfoCount(0) {
    mHostInfoArr = new CoinHostInfo[0x200];
}

bool CoinHolder::hopCoin(const NameObj* pObj, const TVec3f& a2, const TVec3f& a3) {
    CoinHostInfo* hostInfo = findHostInfo(pObj);

    if (hostInfo->_8 >= hostInfo->_4) {
        return false;
    }

    Coin* coin = getDeadMember();

    if (coin) {
        coin->setHostInfo(hostInfo);
        coin->appearHop(a2, a3);
        return true;
    }

    return false;
}

bool CoinHolder::appearCoinFix(const NameObj* pObj, const TVec3f& a2, s32 a3) {
    TVec3f stack_8(0.0f, 0.0f, 0.0f);
    return appearCoin(pObj, a2, stack_8, a3, -1, -1, a3 == 1 ? 0.0f : 4.0f);
}

bool CoinHolder::appearCoinPop(const NameObj* pObj, const TVec3f& a2, s32 a3) {
    TVec3f stack_20;
    MR::calcGravityVector(this, a2, &stack_20, nullptr, nullptr);
    TVec3f stack_14 = (-stack_20) * 25.0f;
    return appearCoin(pObj, a2, stack_14, a3, -1, -1, a3 == 1 ? 0.0f : 4.0f);
}

bool CoinHolder::appearCoinPopToDirection(const NameObj* pObj, const TVec3f& a2, const TVec3f& a3, s32 a4) {
    TVec3f stack_14;
    MR::normalize(a3, &stack_14);
    f32 randomRange = a4 == 1 ? 0.0f : 4.0f;
    TVec3f stack_8 = stack_14 * 25.0f;
    return appearCoin(pObj, a2, stack_8, a4, -1, -1, randomRange);
}

bool CoinHolder::appearCoinToVelocity(const NameObj* pObj, const TVec3f& a2, const TVec3f& a3, s32 a4) {
    return appearCoin(pObj, a2, a3, a4, -1, -1, a4 == 1 ? 0.0f : 4.0f);
}

bool CoinHolder::appearCoinCircle(const NameObj* pObj, const TVec3f& a2, s32 a3) {
    if (a3 == 1) {
        return appearCoinPop(pObj, a2, a3);
    }

    TVec3f gravity;
    MR::calcGravityVector(this, a2, &gravity, nullptr, nullptr);

    TVec3f axis;
    MR::makeAxisVerticalZX(&axis, gravity);

    bool appeared = false;

    for (s32 i = 0; i < a3; i++) {
        TVec3f direction;
        MR::rotateVecDegree(&direction, axis, gravity, 360.0f / a3 * i);
        direction.setLength(0.25f);

        TVec3f velocity = direction - gravity;
        velocity.setLength(30.0f);
        appeared |= appearCoin(pObj, a2, velocity, 1, -1, -1, 0.0f);
    }

    return appeared;
}

CoinHostInfo* CoinHolder::declare(const NameObj* pObj, s32 a2) {
    if (a2 <= 0) {
        return nullptr;
    }

    CoinHostInfo* hostInfo = findHostInfo(pObj);
    if (!hostInfo) {
        hostInfo = &mHostInfoArr[mHostInfoCount];
        hostInfo->mHostActor = pObj;
        mHostInfoCount++;
    }

    hostInfo->_4 += a2;
    return hostInfo;
}

s32 CoinHolder::getDeclareRemnantCoinCount(const NameObj* pObj) const {
    CoinHostInfo* hostInfo = findHostInfo(pObj);

    if (MR::isGalaxyDarkCometAppearInCurrentStage()) {
        return 0;
    }

    return hostInfo->_4 - hostInfo->_8;
}

CoinHostInfo* CoinHolder::findHostInfo(const NameObj* pObj) const {
    for (s32 i = 0; i < mHostInfoCount; i++) {
        if (mHostInfoArr[i].mHostActor == pObj) {
            return &mHostInfoArr[i];
        }
    }

    return nullptr;
}

void CoinHolder::init(const JMapInfoIter& rIter) {
    for (int i = 0; i < 0x20; i++) {
        Coin* coin = new Coin("コイン(共用)");
        coin->initWithoutIter();
        registerActor(coin);
    }
}

bool CoinHolder::appearCoin(const NameObj* pObj, const TVec3f& a2, const TVec3f& a3, s32 a4, s32 a5, s32 a6, f32 a7) {
    CoinHostInfo* hostInfo = findHostInfo(pObj);

    if (!hostInfo) {
        return false;
    }

    bool appeared = false;

    for (s32 i = 0; i < a4; i++) {
        if (hostInfo->_8 >= hostInfo->_4) {
            break;
        }

        Coin* coin = getDeadMember();

        if (!coin) {
            break;
        }

        TVec3f velocity(a3);

        if (!MR::isNearZero(a7, 0.001f)) {
            MR::addRandomVector(&velocity, velocity, a7);
        }

        coin->setHostInfo(hostInfo);
        coin->appearMove(a2, velocity, a5, a6);
        appeared = true;
    }

    if (!MR::isGalaxyDarkCometAppearInCurrentStage() && appeared) {
        if (MR::hasME()) {
            MR::startSystemME("ME_COIN_APPEAR_S");
        } else {
            MR::startSystemSE("SE_SY_COIN_APPEAR_S");
        }
    }

    return appeared;
}

namespace MR {
    void createCoinHolder() {
        MR::createSceneObj(SceneObj_CoinRotater);
        MR::createSceneObj(SceneObj_CoinHolder);
    }

    CoinHolder* getCoinHolder() {
        return getSceneObj< CoinHolder >(SceneObj_CoinHolder);
    }

    void addToCoinHolder(const NameObj* pNameObj, Coin* pCoin) {
        getCoinHolder();
        pCoin->setHostInfo(getCoinHolder()->declare(pNameObj, 1));
    }
};  // namespace MR
