#include "Game/NPC/RunawayRabbitCollect.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/NPC/RunawayRabbit.hpp"
#include "Game/NPC/RunawayTico.hpp"
#include "Game/Util/ActorCameraUtil.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/ActorSwitchUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StringUtil.hpp"

namespace {
    static const s32 sStartRunAwayBgmState = 2;
    static const s32 sFoundRabbitBgmState = 3;
    static const s32 sCaughtRabbitBgmState = 2;
    static const s32 sEndRunAwayBgmState = 1;
    static const s32 sStartRunAwayBgmChangeFrames = 60;
    static const s32 sFoundRabbitBgmChangeFrames = 30;
    static const s32 sCaughtRabbitBgmChangeFrames = 90;
    static const s32 sEndRunAwayBgmChangeFrames = 120;
};  // namespace

namespace NrvRunawayRabbitCollect {
    NEW_NERVE(RunawayRabbitCollectNrvWait, RunawayRabbitCollect, Wait);
    NEW_NERVE(RunawayRabbitCollectNrvActive, RunawayRabbitCollect, Active);
};  // namespace NrvRunawayRabbitCollect

RunawayRabbitCollect::RunawayRabbitCollect(const char* pName)
    : LiveActor(pName), mRabbit(), mRabbitNum(), _A0(), _A4(), mCompleteRabbitCount(), mBgmState() {
}

void RunawayRabbitCollect::init(const JMapInfoIter& rIter) {
    MR::connectToSceneNpcMovement(this);
    mRabbitNum = 0;
    mTicoNum = 0;
    const char* objName;

    for (s32 i = 0; i < MR::getChildObjNum(rIter); i++) {
        MR::getChildObjName(&objName, rIter, i);

        if (MR::isEqualString(objName, "RunawayRabbit")) {
            mRabbitNum++;
        } else if (MR::isEqualString(objName, "RunawayTico")) {
            mTicoNum++;
        }
    }

    mRabbit = new RunawayRabbit*[mRabbitNum];
    mTico = new RunawayTico*[mTicoNum];
    mRabbitNum = 0;
    mTicoNum = 0;

    for (s32 i = 0; i < MR::getChildObjNum(rIter); i++) {
        MR::getChildObjName(&objName, rIter, i);

        if (MR::isEqualString(objName, "RunawayRabbit")) {
            mRabbit[mRabbitNum] = new RunawayRabbit("球面逃げウサギ", this);
            MR::initChildObj(mRabbit[mRabbitNum], rIter, i);
            mRabbitNum++;
        } else if (MR::isEqualString(objName, "RunawayTico")) {
            mTico[mTicoNum] = new RunawayTico("逃げチコ");
            MR::initChildObj(mTico[mTicoNum], rIter, i);
            mTicoNum++;
        }
    }

    linkMsgCtrl();
    mCompleteRabbitCount = calcCompleteRabbitCount();
    initNerve(&NrvRunawayRabbitCollect::RunawayRabbitCollectNrvWait::sInstance);
    mCameraInfo = MR::createActorCameraInfo(rIter);
    MR::initActorCamera(this, rIter, &mCameraInfo);
    MR::tryRegisterDemoCast(this, rIter);
    MR::useStageSwitchWriteA(this, rIter);
    MR::invalidateClipping(this);
    makeActorAppeared();

    for (s32 i = 0; i < 3; i++) {
        _B0[i] = false;
    }
}

void RunawayRabbitCollect::initAfterPlacement() {
    MR::sendMsgToAllLiveActor(ACTMES_HEAVENSDOOR_RUNAWAY_RABBIT_WAIT, nullptr);
}

s32 RunawayRabbitCollect::calcCompleteRabbitCount() const {
    s32 completeCount = 0;

    for (s32 i = 0; i < mRabbitNum; i++) {
        const s32 groupId = mRabbit[i]->mObjArg0;

        if (groupId < 0) {
            completeCount++;
        } else {
            bool isCounted = false;

            for (s32 j = 0; j < i; j++) {
                if (groupId == mRabbit[j]->mObjArg0) {
                    isCounted = true;
                    break;
                }
            }

            if (!isCounted) {
                completeCount++;
            }
        }
    }

    return completeCount;
}

void RunawayRabbitCollect::linkMsgCtrl() {
    for (s32 i = 0; i < mRabbitNum; i++) {
        RunawayRabbit* rabbit = mRabbit[i];
        const s32 groupId = rabbit->mObjArg0;

        if (groupId >= 0) {
            for (s32 j = 0; j < mTicoNum; j++) {
                RunawayTico* tico = mTico[j];

                if (groupId == tico->mDemoCastID) {
                    rabbit->setMsgCtrl(tico->getMsgCtrl());
                    break;
                }
            }
        }
    }
}

void RunawayRabbitCollect::noticeAppearRabbit(RunawayRabbit* pRabbit) {
    const s32 groupId = pRabbit->mObjArg0;

    if (groupId == -1) {
        return;
    }

    for (s32 i = 0; i < mRabbitNum; i++) {
        RunawayRabbit* rabbit = mRabbit[i];

        if (rabbit != pRabbit && groupId == rabbit->mObjArg0) {
            rabbit->_F4 = false;
        }
    }
}

void RunawayRabbitCollect::noticeCaughtRabbit(RunawayRabbit* pRabbit) {
    _A4++;

    for (s32 i = 0; i < mRabbitNum; i++) {
        RunawayRabbit* rabbit = mRabbit[i];

        if (rabbit != pRabbit) {
            rabbit->setNotCaughtable();
        }
    }

    for (s32 i = 0; i < mTicoNum; i++) {
        if (pRabbit->mObjArg0 == mTico[i]->mDemoCastID) {
            TVec3f jointPos;
            MR::copyJointPos(pRabbit, "Spine", &jointPos);
            mTico[i]->setPosAfterCaught(jointPos);
            break;
        }
    }

    if (_A4 == mCompleteRabbitCount) {
        pRabbit->setLastMessage();
    } else {
        pRabbit->setMessage();
    }

    for (s32 i = 0; i < mRabbitNum; i++) {
        mRabbit[i]->incrementRunawayLevel();
    }
}

void RunawayRabbitCollect::control() {
}

void RunawayRabbitCollect::exeWait() {
    for (s32 i = 0; i < mTicoNum; i++) {
        if (mTico[i]->isStartRunaway()) {
            for (s32 j = 0; j < mRabbitNum; j++) {
                mRabbit[j]->activate();
            }

            MR::sendMsgToAllLiveActor(ACTMES_HEAVENSDOOR_RUNAWAY_RABBIT_START, nullptr);
            setNerve(&NrvRunawayRabbitCollect::RunawayRabbitCollectNrvActive::sInstance);
            break;
        }
    }
}

void RunawayRabbitCollect::appearTico(RunawayTico* pTico, const TVec3f& rPosition) {
    _B0[pTico->mDemoCastID] = true;

    for (s32 i = 0; i < 3; i++) {
        if (!_B0[i]) {
            switch (i) {
            case 0:
                pTico->appearHoleComment(rPosition);
                break;
            case 1:
                pTico->appearPipeComment(rPosition);
                break;
            case 2:
                pTico->appearBushComment(rPosition);
                break;
            }

            return;
        }
    }

    pTico->appearMamaComment(rPosition);
}

void RunawayRabbitCollect::exeActive() {
    if (MR::isFirstStep(this)) {
        MR::setStageBGMState(sStartRunAwayBgmState, sStartRunAwayBgmChangeFrames);
        mBgmState = sStartRunAwayBgmState;
    }

    bool isAllCaught = true;
    bool hasNewCaughtRabbit = false;
    s32 caughtCount = 0;
    s32 chasingCount = 0;
    bool appeared = true;

    for (s32 i = 0; i < mRabbitNum; i++) {
        if (!mRabbit[i]->_F4) {
            continue;
        }

        if (mRabbit[i]->isChasing()) {
            chasingCount++;
        }

        if (mRabbit[i]->isCaught()) {
            caughtCount++;

            if (!mRabbit[i]->_F5) {
                _A0++;

                for (s32 j = 0; j < mTicoNum; j++) {
                    if (mTico[j]->mDemoCastID == mRabbit[i]->mObjArg0) {
                        TVec3f jointPos;
                        MR::copyJointPos(mRabbit[i], "Spine", &jointPos);
                        appearTico(mTico[j], jointPos);
                    }
                }

                hasNewCaughtRabbit = true;
                mRabbit[i]->_F5 = appeared;
            }
        }
    }

    if (caughtCount < mCompleteRabbitCount) {
        isAllCaught = false;
    }

    if (hasNewCaughtRabbit) {
        if (caughtCount == mCompleteRabbitCount) {
            MR::startSystemSE("SE_SY_RUNAWAY_RABBIT_GET_3", -1, -1);
        } else if (caughtCount == mCompleteRabbitCount - 1) {
            MR::startSystemSE("SE_SY_RUNAWAY_RABBIT_GET_2", -1, -1);
        } else {
            MR::startSystemSE("SE_SY_RUNAWAY_RABBIT_GET_1", -1, -1);
        }
    }

    if (chasingCount > 0) {
        if (mBgmState != sFoundRabbitBgmState) {
            MR::setStageBGMState(sFoundRabbitBgmState, sFoundRabbitBgmChangeFrames);
        }

        mBgmState = sFoundRabbitBgmState;
    } else {
        if (mBgmState != sCaughtRabbitBgmState) {
            MR::setStageBGMState(sCaughtRabbitBgmState, sCaughtRabbitBgmChangeFrames);
        }

        mBgmState = sCaughtRabbitBgmState;
    }

    if (isAllCaught) {
        MR::isValidSwitchA(this);
        MR::setStageBGMState(sEndRunAwayBgmState, sEndRunAwayBgmChangeFrames);
        kill();
    }
}

RunawayRabbitCollect::~RunawayRabbitCollect() {
}
