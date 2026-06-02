#include "Game/MapObj/MeteorStrikeLauncher.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/ActorSwitchUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/RailUtil.hpp"

#define METEOR_STRIKE_DEFAULT_SCREEN_RADIUS 200.0f
#define METEOR_STRIKE_SECONDS_TO_FRAMES 60

class MeteorStrike : public LiveActor {
public:
    MeteorStrike(const char*);

    virtual void init(const JMapInfoIter&);
    virtual void appear();
    virtual void kill();

    void appear(s32);
    bool getMovedPos(TVec3f*, s32) const;

    static f32 getSpeed(const JMapInfoIter&);

private:
    u8 mPad[0xDC - sizeof(LiveActor)];
};

namespace NrvMeteorStrikeLauncher {
    class MeteorStrikeLauncherNrvCreate : public Nerve {
    public:
        virtual void execute(Spine*) const;
        static MeteorStrikeLauncherNrvCreate sInstance;
    };

    class MeteorStrikeLauncherNrvInterval : public Nerve {
    public:
        virtual void execute(Spine*) const;
        static MeteorStrikeLauncherNrvInterval sInstance;
    };

    MeteorStrikeLauncherNrvCreate MeteorStrikeLauncherNrvCreate::sInstance;
    MeteorStrikeLauncherNrvInterval MeteorStrikeLauncherNrvInterval::sInstance;
};  // namespace NrvMeteorStrikeLauncher

MeteorStrikeLauncher::MeteorStrikeLauncher(const char* pName)
    : LiveActor(pName), mMeteorStrikes(nullptr), mMeteorStrikeCount(0), mCreateInterval(-1), mCreateOffset(0), mIsObjectMeteorStrike(false),
      mUseScreenPositionCheck(false) {
}

void MeteorStrikeLauncher::init(const JMapInfoIter& rIter) {
    const char* meteorName = mName;

    setName("メテオストライクランチャー");
    initMapToolInfo(rIter);
    MR::connectToSceneMapObjMovement(this);
    initRailRider(rIter);
    MR::moveCoordAndTransToRailPoint(this, 0);
    initNerve(&NrvMeteorStrikeLauncher::MeteorStrikeLauncherNrvCreate::sInstance);
    MR::needStageSwitchReadAppear(this, rIter);
    MR::syncStageSwitchAppear(this);

    if (mIsObjectMeteorStrike && mUseScreenPositionCheck) {
        const f32 speed = MeteorStrike::getSpeed(rIter);
        mMeteorStrikeCount = static_cast<s32>(MR::getRailTotalLength(this) / (speed * static_cast<f32>(mCreateInterval))) + 2;
    }
    else if (mIsObjectMeteorStrike || mCreateInterval < 0) {
        mMeteorStrikeCount = 1;
    }
    else {
        mMeteorStrikeCount = 2;
    }

    mMeteorStrikes = new MeteorStrike*[mMeteorStrikeCount];

    for (s32 i = 0; i < mMeteorStrikeCount; i++) {
        mMeteorStrikes[i] = new MeteorStrike(meteorName);
        mMeteorStrikes[i]->init(rIter);
    }

    makeActorDead();
}

void MeteorStrikeLauncher::appear() {
    LiveActor::appear();
    MR::invalidateClipping(this);
    setNerve(&NrvMeteorStrikeLauncher::MeteorStrikeLauncherNrvCreate::sInstance);
}

void MeteorStrikeLauncher::kill() {
    LiveActor::kill();

    if (mCreateInterval < 0) {
        return;
    }

    for (s32 i = 0; i < mMeteorStrikeCount; i++) {
        if (!MR::isDead(mMeteorStrikes[i])) {
            mMeteorStrikes[i]->kill();
        }
    }
}

void MeteorStrikeLauncher::initMapToolInfo(const JMapInfoIter& rIter) {
    MR::initDefaultPos(this, rIter);
    MR::useStageSwitchReadAppear(this, rIter);

    if (MR::getJMapInfoArg1NoInit(rIter, &mCreateInterval)) {
        mCreateInterval *= METEOR_STRIKE_SECONDS_TO_FRAMES;
    }

    mIsObjectMeteorStrike = MR::isEqualObjectName(rIter, "MeteorStrike");
    MR::getJMapInfoArg2NoInit(rIter, &mUseScreenPositionCheck);
}

MeteorStrike* MeteorStrikeLauncher::getUnusedMeteorStrike() {
    for (s32 i = 0; i < mMeteorStrikeCount; i++) {
        if (MR::isDead(mMeteorStrikes[i])) {
            return mMeteorStrikes[i];
        }
    }

    return nullptr;
}

bool MeteorStrikeLauncher::create() {
    MeteorStrike* meteor = getUnusedMeteorStrike();

    if (meteor == nullptr) {
        mCreateOffset++;
        return false;
    }

    if (!mIsObjectMeteorStrike) {
        meteor->appear();
        return true;
    }

    if (mUseScreenPositionCheck) {
        TVec3f movedPos;
        meteor->getMovedPos(&movedPos, 0);

        if (MR::isJudgedToClipFrustum(movedPos, METEOR_STRIKE_DEFAULT_SCREEN_RADIUS)) {
            meteor->appear();
        }

        return true;
    }

    while (mCreateOffset >= 0) {
        TVec3f movedPos;

        if (meteor->getMovedPos(&movedPos, mCreateOffset)) {
            if (MR::isJudgedToClipFrustum(movedPos, METEOR_STRIKE_DEFAULT_SCREEN_RADIUS)) {
                meteor->appear(mCreateOffset);
                return true;
            }
        }

        mCreateOffset -= mCreateInterval;
    }

    mCreateOffset = 0;
    return false;
}

void MeteorStrikeLauncher::exeCreate() {
    if (MR::isFirstStep(this)) {
        mCreateOffset = 0;
    }

    if (create()) {
        if (mCreateInterval < 0) {
            kill();
        }
        else {
            setNerve(&NrvMeteorStrikeLauncher::MeteorStrikeLauncherNrvInterval::sInstance);
        }
    }
}

MeteorStrikeLauncher::~MeteorStrikeLauncher() {}

namespace NrvMeteorStrikeLauncher {
    void MeteorStrikeLauncherNrvCreate::execute(Spine* pSpine) const {
        reinterpret_cast<MeteorStrikeLauncher*>(pSpine->mExecutor)->exeCreate();
    }

    void MeteorStrikeLauncherNrvInterval::execute(Spine* pSpine) const {
        MeteorStrikeLauncher* launcher = reinterpret_cast<MeteorStrikeLauncher*>(pSpine->mExecutor);

        if (MR::isStep(launcher, launcher->mCreateInterval)) {
            launcher->setNerve(&MeteorStrikeLauncherNrvCreate::sInstance);
        }
    }
};  // namespace NrvMeteorStrikeLauncher
