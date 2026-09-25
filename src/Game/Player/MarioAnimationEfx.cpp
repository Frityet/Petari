#include "resource/TextEncoding.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Util/HashUtil.hpp"

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
}  // namespace NrvMarioActor

struct MarioCallbackInfo {
    const char* mName;
    s32 mType;
    void (MarioAnimator::*mEntry)();
    void (MarioAnimator::*mUpdate)();
    void (MarioAnimator::*mClose)();
    u32 _2C;
};

MarioCallbackInfo marioCallbackTable[] = {
    {CP932("空中ひねり"), 0, &MarioAnimator::spinEntry, &MarioAnimator::spinUpdate, &MarioAnimator::spinClose, 0},
    {CP932("地上ひねり"), 0, &MarioAnimator::spinEntry, nullptr, &MarioAnimator::spinClose, 0},
    {CP932("アイスひねり"), 1, &MarioAnimator::spinEntry, nullptr, &MarioAnimator::spinClose, 0},
    {CP932("アイスひねり静止"), 1, &MarioAnimator::spinEntry, nullptr, &MarioAnimator::spinClose, 0},
    {CP932("ファイアスピン"), 2, &MarioAnimator::spinEntry, nullptr, &MarioAnimator::spinClose, 0},
    {CP932("ファイアスピン空中"), 2, &MarioAnimator::spinEntry, nullptr, &MarioAnimator::spinClose, 0},
    {CP932("ハチスピン"), 3, &MarioAnimator::spinEntry, nullptr, &MarioAnimator::spinClose, 0},
    {CP932("ハチスピン空中"), 3, &MarioAnimator::spinEntry, nullptr, &MarioAnimator::spinClose, 0},
    {CP932("ステージインA"), 0, nullptr, &MarioAnimator::stageInCheck, nullptr, 0},
    {CP932("投げ"), 0, &MarioAnimator::throwEntry, &MarioAnimator::throwCheck, &MarioAnimator::throwClose, 0},
    {CP932("ファイア投げ"), 1, &MarioAnimator::throwEntry, nullptr, &MarioAnimator::throwClose, 0},
    {CP932("サマーソルト"), 0, nullptr, &MarioAnimator::squatSpinCheck, nullptr, 0},
    {CP932("ウォークイン"), 0, nullptr, nullptr, &MarioAnimator::walkinClose, 0},
    {CP932("見る"), 0, nullptr, nullptr, &MarioAnimator::walkinClose, 0},
    {"ResultWait", 0, nullptr, nullptr, &MarioAnimator::walkinClose, 0},
    {"ResultWaitGrandStar", 0, nullptr, nullptr, &MarioAnimator::walkinClose, 0},
    {"WatchUpMore", 0, nullptr, nullptr, &MarioAnimator::walkinClose, 0},
    {"", 0, nullptr, nullptr, nullptr, 0},
};

void MarioAnimator::initCallbackTable() {
    u32 num = 0;
    MarioCallbackInfo* info = marioCallbackTable;
    while (true) {
        if (!info->mName[0]) {
            break;
        }

        num++;
        info++;
    }

    mNativeCallbacks.reset(new HashSortTable(num));
    _120 = mNativeCallbacks.get();

    info = marioCallbackTable;
    for (u32 i = 0; i < num; i++) {
        _120->add(info->mName, i, false);
        info++;
    }

    _120->sort();
    _11C = -1;
    _10F = 0;
}

void MarioAnimator::entryCallback(const char* pName) {
    _10F = 0;
    closeCallback();

    HashSortTable::Value callbackIdx;
    if (_120->search(pName, &callbackIdx)) {
        _11C = callbackIdx;

        if (marioCallbackTable[callbackIdx].mEntry) {
            (this->*marioCallbackTable[callbackIdx].mEntry)();
        }
    }
}

void MarioAnimator::runningCallback() {
    if (_11C == -1) {
        return;
    }

    _10F = 1;

    if (isAnimationStop() || isAnimationTerminate(nullptr)) {
        closeCallback();
        return;
    }

    _10F = 0;

    if (!isAnimationRun(marioCallbackTable[_11C].mName)) {
        closeCallback();
        return;
    }

    if (marioCallbackTable[_11C].mUpdate) {
        (this->*marioCallbackTable[_11C].mUpdate)();
    }
}

void MarioAnimator::closeCallback() {
    if (_11C == -1) {
        return;
    }

    if (marioCallbackTable[_11C].mClose) {
        (this->*marioCallbackTable[_11C].mClose)();
    }

    _11C = -1;
}

void MarioAnimator::spinEntry() {
    switch (marioCallbackTable[_11C].mType) {
    case 0:
        playEffect(CP932("スピンライト"));
        break;
    case 1:
        playEffect(CP932("アイススピン"));
        break;
    case 2:
        playEffect(CP932("ファイアスピン"));
        break;
    case 3:
        if (gIsLuigi) {
            playEffect(CP932("ハチルイージスピン"));
        } else {
            playEffect(CP932("ハチスピン"));
        }
        break;
    }
}

void MarioAnimator::spinUpdate() {
    if (getFrame() > 30.0f) {
        stopEffect(CP932("スピンライト"));
    }
}

void MarioAnimator::spinClose() {
    switch (marioCallbackTable[_11C].mType) {
    case 0:
        stopEffect(CP932("スピンライト"));
        break;
    case 1:
        stopEffect(CP932("アイススピン"));
        break;
    case 2:
        stopEffect(CP932("ファイアスピン"));
        break;
    case 3:
        if (gIsLuigi) {
            stopEffect(CP932("ハチルイージスピン"));
        } else {
            stopEffect(CP932("ハチスピン"));
        }
        break;
    }
}

void MarioAnimator::stageInCheck() {
    if ((s32)getFrame() == 0x32) {
        Mario* pPlayer = getPlayer();
        playEffectRT(CP932("属性ステージイン"), pPlayer->_368, getTrans());
    }
}

void MarioAnimator::throwCheck() {
    if (mActor->_38C) {
        return;
    }

    if (getStickP() == 0.0f) {
        return;
    }

    if (getPlayer()->mMovementStates.jumping) {
        return;
    }

    stopAnimation(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));
}

void MarioAnimator::throwEntry() {
    MarioCallbackInfo& info = marioCallbackTable[_11C];
    s32 type = info.mType;
    if (type == 1) {
        goto effect_fire_throw;
    } else if (type >= 1) {
        return;
    } else if (type < 0) {
        return;
    }

effect_shell_throw:
    playEffect(CP932("こうら投げ"));
    return;

effect_fire_throw:
    playEffect(CP932("ファイアボール投げ"));
}

void MarioAnimator::throwClose() {
    MarioCallbackInfo& info = marioCallbackTable[_11C];
    s32 type = info.mType;
    if (type == 1) {
        goto effect_fire_throw;
    } else if (type >= 1) {
        return;
    } else if (type < 0) {
        return;
    }

effect_shell_throw:
    stopEffect(CP932("こうら投げ"));
    return;

effect_fire_throw:
    stopEffect(CP932("ファイアボール投げ"));
}

void MarioAnimator::squatSpinCheck() {
    if (getPlayer()->mMovementStates._A) {
        return;
    }

    if (getFrame() <= 40.0f) {
        stopAnimation(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));
    }
}

void MarioAnimator::walkinClose() {
    if (!_10F) {
        return;
    }

    stopAnimation(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));
    getPlayer()->changeAnimationInterpoleFrame(0x10);
}

XanimeSwapTable luigiAnimeSwapTable[] = {
    {"Run", "LuigiRun"},
    {"Jump", "LuigiJump"},
    {"JumpRoll", "LuigiJumpRoll"},
    {"JumpBack", "LuigiJumpBack"},
    {"RunEnd", "LuigiRunEnd"},
    {"Spin", "LuigiSpin"},
    {"SpinGround", "LuigiSpinGround"},
    {"SpaceFlyShort", "LuigiSpaceFlyShort"},
    {"Wait", "LuigiWait"},
    {"WaitSlopeL", "LuigiWaitSlopeL"},
    {"WaitSlopeR", "LuigiWaitSlopeR"},
    {"WaitSlopeU", "LuigiWaitSlopeU"},
    {"WaitSlopeD", "LuigiWaitSlopeD"},
    {"", nullptr},
};
