#include "Game/NPC/RunawayRabbitCollect.hpp"
#include "Game/NPC/RunawayRabbit.hpp"
#include "Game/NPC/RunawayTico.hpp"
#include "Game/Util.hpp"

namespace NrvRunawayRabbitCollect {
    NEW_NERVE(RunawayRabbitCollectNrvWait, RunawayRabbitCollect, Wait);
    NEW_NERVE(RunawayRabbitCollectNrvActive, RunawayRabbitCollect, Active);
};  // namespace NrvRunawayRabbitCollect

namespace {
    s32 sStartRunAwayBgmState = 2;
    s32 sFoundRabbitBgmState = 3;
    s32 sCaughtRabbitBgmState = 2;
    s32 sEndRunAwayBgmState = 1;
};  // namespace

RunawayRabbitCollect::RunawayRabbitCollect(const char* pName) : LiveActor(pName) {
    mRabbits = nullptr;
    mRabbitCount = 0;
    mAppearedTicoCount = 0;
    mCaughtRabbitCount = 0;
    mCompleteRabbitCount = 0;
    mBgmState = 0;
}

void RunawayRabbitCollect::init(const JMapInfoIter& rIter) {
    MR::connectToSceneNpcMovement(this);
    mRabbitCount = 0;
    mTicoCount = 0;

    for (s32 i = 0; i < MR::getChildObjNum(rIter); i++) {
        const char* objName = nullptr;
        MR::getChildObjName(&objName, rIter, i);

        if (MR::isEqualString(objName, "RunawayRabbit")) {
            mRabbitCount++;
        }
        else if (MR::isEqualString(objName, "RunawayTico")) {
            mTicoCount++;
        }
    }

    mRabbits = new RunawayRabbit*[mRabbitCount];
    mTicos = new RunawayTico*[mTicoCount];
    mRabbitCount = 0;
    mTicoCount = 0;

    for (s32 i = 0; i < MR::getChildObjNum(rIter); i++) {
        const char* objName = nullptr;
        MR::getChildObjName(&objName, rIter, i);

        if (MR::isEqualString(objName, "RunawayRabbit")) {
            mRabbits[mRabbitCount] = new RunawayRabbit("球面逃げウサギ", this);
            MR::initChildObj(mRabbits[mRabbitCount], rIter, i);
            mRabbitCount++;
        }
        else if (MR::isEqualString(objName, "RunawayTico")) {
            mTicos[mTicoCount] = new RunawayTico("逃げチコ");
            MR::initChildObj(mTicos[mTicoCount], rIter, i);
            mTicoCount++;
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
        mHasAppearedTico[i] = false;
    }
}

void RunawayRabbitCollect::initAfterPlacement() {
    MR::sendMsgToAllLiveActor(ACTMES_HEAVENSDOOR_RUNAWAY_RABBIT_WAIT, nullptr);
}

s32 RunawayRabbitCollect::calcCompleteRabbitCount() const {
    s32 completeCount = 0;

    for (s32 i = 0; i < mRabbitCount; i++) {
        const s32 groupId = mRabbits[i]->mGroupId;

        if (groupId < 0) {
            completeCount++;
        }
        else {
            bool isCounted = false;

            for (s32 j = 0; j < i; j++) {
                if (groupId == mRabbits[j]->mGroupId) {
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
    for (s32 i = 0; i < mRabbitCount; i++) {
        RunawayRabbit* rabbit = mRabbits[i];
        const s32 groupId = rabbit->mGroupId;

        if (groupId >= 0) {
            for (s32 j = 0; j < mTicoCount; j++) {
                RunawayTico* tico = mTicos[j];

                if (groupId == tico->mDemoCastID) {
                    rabbit->setMsgCtrl(tico->mMsgCtrl);
                    break;
                }
            }
        }
    }
}

void RunawayRabbitCollect::noticeAppearRabbit(RunawayRabbit* pRabbit) {
    const s32 groupId = pRabbit->mGroupId;

    if (groupId == -1) {
        return;
    }

    for (s32 i = 0; i < mRabbitCount; i++) {
        RunawayRabbit* rabbit = mRabbits[i];

        if (rabbit != pRabbit && groupId == rabbit->mGroupId) {
            rabbit->mIsCaughtable = false;
        }
    }
}

void RunawayRabbitCollect::noticeCaughtRabbit(RunawayRabbit* pRabbit) {
    mCaughtRabbitCount++;

    for (s32 i = 0; i < mRabbitCount; i++) {
        RunawayRabbit* rabbit = mRabbits[i];

        if (rabbit != pRabbit) {
            rabbit->setNotCaughtable();
        }
    }

    for (s32 i = 0; i < mTicoCount; i++) {
        RunawayTico* tico = mTicos[i];

        if (tico->mDemoCastID == pRabbit->mGroupId) {
            TVec3f jointPos;
            MR::copyJointPos(pRabbit, "Spine", &jointPos);
            tico->setPosAfterCaught(jointPos);
            break;
        }
    }

    if (mCaughtRabbitCount == mCompleteRabbitCount) {
        pRabbit->setLastMessage();
    }
    else {
        pRabbit->setMessage();
    }

    for (s32 i = 0; i < mRabbitCount; i++) {
        mRabbits[i]->incrementRunawayLevel();
    }
}

void RunawayRabbitCollect::control() {}

void RunawayRabbitCollect::exeWait() {
    for (s32 i = 0; i < mTicoCount; i++) {
        if (mTicos[i]->isStartRunaway()) {
            for (s32 j = 0; j < mRabbitCount; j++) {
                mRabbits[j]->activate();
            }

            MR::sendMsgToAllLiveActor(ACTMES_HEAVENSDOOR_RUNAWAY_RABBIT_START, nullptr);
            setNerve(&NrvRunawayRabbitCollect::RunawayRabbitCollectNrvActive::sInstance);
            break;
        }
    }
}

void RunawayRabbitCollect::appearTico(RunawayTico* pTico, const TVec3f& rPosition) {
    mHasAppearedTico[pTico->mDemoCastID] = true;

    for (s32 i = 0; i < 3; i++) {
        if (!mHasAppearedTico[i]) {
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
        MR::setStageBGMState(sStartRunAwayBgmState, 60);
        mBgmState = sStartRunAwayBgmState;
    }

    bool isAllCaught = true;
    bool hasNewCaughtRabbit = false;
    s32 caughtCount = 0;
    s32 chasingCount = 0;
    const bool appeared = true;

    for (s32 i = 0; i < mRabbitCount; i++) {
        if (!mRabbits[i]->mIsCaughtable) {
            continue;
        }

        if (mRabbits[i]->isChasing()) {
            chasingCount++;
        }

        if (mRabbits[i]->isCaught()) {
            caughtCount++;

            if (!mRabbits[i]->mHasAppearedTico) {
                mAppearedTicoCount++;

                for (s32 j = 0; j < mTicoCount; j++) {
                    if (mTicos[j]->mDemoCastID == mRabbits[i]->mGroupId) {
                        TVec3f jointPos;
                        MR::copyJointPos(mRabbits[i], "Spine", &jointPos);
                        appearTico(mTicos[j], jointPos);
                    }
                }

                hasNewCaughtRabbit = true;
                mRabbits[i]->mHasAppearedTico = appeared;
            }
        }
    }

    if (caughtCount < mCompleteRabbitCount) {
        isAllCaught = false;
    }

    if (hasNewCaughtRabbit) {
        if (caughtCount == mCompleteRabbitCount) {
            MR::startSystemSE("SE_SYS_RUNAWAY_RABBIT_GET_3", -1, -1);
        }
        else if (caughtCount == mCompleteRabbitCount - 1) {
            MR::startSystemSE("SE_SYS_RUNAWAY_RABBIT_GET_2", -1, -1);
        }
        else {
            MR::startSystemSE("SE_SYS_RUNAWAY_RABBIT_GET_1", -1, -1);
        }
    }

    if (chasingCount > 0) {
        if (mBgmState != sFoundRabbitBgmState) {
            MR::setStageBGMState(sFoundRabbitBgmState, 30);
        }

        mBgmState = sFoundRabbitBgmState;
    }
    else {
        if (mBgmState != sCaughtRabbitBgmState) {
            MR::setStageBGMState(sCaughtRabbitBgmState, 90);
        }

        mBgmState = sCaughtRabbitBgmState;
    }

    if (isAllCaught) {
        MR::isValidSwitchA(this);
        MR::setStageBGMState(sEndRunAwayBgmState, 120);
        kill();
    }
}

RunawayRabbitCollect::~RunawayRabbitCollect() {}
