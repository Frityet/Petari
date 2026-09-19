#include "compat/Cp932Literal.hpp"
#include "Game/Player/MarioSukekiyo.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Util/MtxUtil.hpp"

MarioSukekiyo::MarioSukekiyo(MarioActor* pActor) : MarioState(pActor, MarioStatus_Sukekiyo) {
    _44 = new Triangle();
    _14.zero();
    _20.zero();
    _2C.zero();
    _38.zero();
    _48 = 0;
    _4A = 0;
}

MarioBury::MarioBury(MarioActor* pActor) : MarioSukekiyo(pActor) {
    mStatusId = MarioStatus_Bury;
}

bool MarioSukekiyo::close() {
    getPlayer()->stopWalk();
    mActor->_F44 = 1;
    stopAnimation(static_cast< const char* >(nullptr), CP932("基本"));
    return true;
}

bool MarioSukekiyo::notice() {
    return false;
}

bool MarioSukekiyo::postureCtrl(MtxPtr mtx) {
    MR::makeMtxUpSide(reinterpret_cast< TPos3f* >(mtx), _14, _20);
    return true;
}

bool MarioSukekiyo::start() {
    playSound(CP932("スケキヨ開始"));
    Mario* player = getPlayer();
    playEffectRT(CP932("属性尻ドロップ"), player->_368, getTrans());
    startPadVib(CP932("最強"));
    startCamVib(3);
    mActor->_F44 = 0;
    _14 = getPlayer()->_368;
    _20 = getPlayer()->mSideVec;
    getPlayer()->forceSetHeadVecKeepSide(_14);
    _48 = 0;
    _4A = 0;
    getPlayer()->stopJump();
    getPlayer()->stopWalk();

    if (mStatusId == MarioStatus_Sukekiyo) {
        changeAnimation(CP932("スケキヨ"), static_cast< const char* >(nullptr));
    } else {
        playSound(CP932("声足埋まり開始"));
        changeAnimation(CP932("埋まり"), static_cast< const char* >(nullptr));
    }
    return true;
}

bool MarioSukekiyo::update() {
    if (!getPlayer()->isCurrentFloorSand()) {
        return false;
    }

    if (_4A) {
        return isAnimationRun(nullptr) != false;
    }

    if (mActor->isRequestRush()) {
        _4A = 1;
    }

    if (checkTrgA()) {
        _4A = 1;
    }

    if (_4A) {
        if (mStatusId == MarioStatus_Sukekiyo) {
            changeAnimation(CP932("スケキヨ脱出"), CP932("基本"));
            playSound(CP932("声スケキヨ終了"));
        } else {
            changeAnimation(CP932("埋まり脱出"), CP932("基本"));
            playSound(CP932("声足埋まり終了"));
        }

        playSound(CP932("スケキヨ終了"));
    }

    return true;
}

namespace NrvMarioActor {
    INIT_NERVE(MarioActorNrvWait);
    INIT_NERVE(MarioActorNrvGameOver);
    INIT_NERVE(MarioActorNrvGameOverAbyss);
    INIT_NERVE(MarioActorNrvGameOverAbyss2);
    INIT_NERVE(MarioActorNrvGameOverFire);
    INIT_NERVE(MarioActorNrvGameOverBlackHole);
    INIT_NERVE(MarioActorNrvGameOverNonStop);
    INIT_NERVE(MarioActorNrvGameOverSink);
    INIT_NERVE(MarioActorNrvTimeWait);
    INIT_NERVE(MarioActorNrvNoRush);
};  // namespace NrvMarioActor
