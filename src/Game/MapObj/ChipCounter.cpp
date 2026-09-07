#include "Game/MapObj/ChipCounter.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/MapObj/ChipBase.hpp"
#include "Game/MapObj/ChipHolder.hpp"
#include "Game/MapObj/CollectCounter.hpp"
#include "Game/NPC/TalkDirector.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

namespace {
    static const char* sChipPainName[] = {"Chip1", "Chip2", "Chip3", "Chip4", "Chip5"};
    static s32 sChipPainCount = ARRAY_SIZE(sChipPainName);
};  // namespace

namespace NrvChipCounter {
    NEW_NERVE(ChipCounterNrvHide, ChipCounter, Hide);

    class ChipCounterNrvFrameIn : public Nerve {
    public:
        virtual void execute(Spine*) const;

        static ChipCounterNrvFrameIn sInstance;
    };

    ChipCounterNrvFrameIn ChipCounterNrvFrameIn::sInstance;

    class ChipCounterNrvShow : public Nerve {
    public:
        virtual void execute(Spine*) const;

        static ChipCounterNrvShow sInstance;
    };

    ChipCounterNrvShow ChipCounterNrvShow::sInstance;

    class ChipCounterNrvFrameOut : public Nerve {
    public:
        virtual void execute(Spine*) const;

        static ChipCounterNrvFrameOut sInstance;
    };

    ChipCounterNrvFrameOut ChipCounterNrvFrameOut::sInstance;

    class ChipCounterNrvTryDemo : public Nerve {
    public:
        virtual void execute(Spine*) const;

        static ChipCounterNrvTryDemo sInstance;
    };

    ChipCounterNrvTryDemo ChipCounterNrvTryDemo::sInstance;

    NEW_NERVE(ChipCounterNrvComplete, ChipCounter, Complete);
    NEW_NERVE(ChipCounterNrvCompleteOut, ChipCounter, CompleteOut);
};  // namespace NrvChipCounter

ChipCounter::ChipCounter(const char* pName, s32 type) : LayoutActor(pName, true) {
    mCollectCounter = 0;
    mCount = 0;
    mType = type;
    _2C = -1;
    _30 = 1.0f;
}

void ChipCounter::init(const JMapInfoIter& rIter) {
    MR::connectToSceneLayout(this);

    switch (mType) {
    case ChipBase::Type_Blue:
        initLayoutManager("BlueChipCounter", 2);
        break;
    case ChipBase::Type_Yellow:
        initLayoutManager("YellowChipCounter", 2);
        break;
    }

    initNerve(&NrvChipCounter::ChipCounterNrvHide::sInstance);

    for (s32 i = 0; i < ::sChipPainCount; i++) {
        MR::createAndAddPaneCtrl(this, ::sChipPainName[i], 2);
        MR::startPaneAnim(this, ::sChipPainName[i], "ChipGet", 0);
    }

    MR::startAnim(this, "ShowHide", 1);
    MR::setAnimFrameAndStop(this, _30 * 20.0f, 1);
    mCollectCounter = new CollectCounter("集め数字");
    mCollectCounter->initWithoutIter();
    kill();
}

void ChipCounter::control() {
    if (MR::isActiveTalkBalloonShort()) {
        _30 -= 0.05f;

        if (_30 < 0.0f) {
            _30 = 0.0f;
        }
    } else {
        _30 += 0.05f;

        if (_30 > 1.0f) {
            _30 = 1.0f;
        }
    }

    MR::setAnimFrameAndStop(this, _30 * 20.0f, 1);
}

void ChipCounter::setCount(s32 count) {
    mCollectCounter->setCount(count);
    mCount = count;

    for (s32 i = 0; i < ::sChipPainCount; i++) {
        if (i < mCount - 1) {
            MR::setPaneAnimFrameAndStop(this, ::sChipPainName[i], 1.0f, 0);
            continue;
        }

        if (i == mCount - 1) {
            if (i == ::sChipPainCount - 1) {
                MR::setPaneAnimFrameAndStop(this, ::sChipPainName[i], 1.0f, 0);
                continue;
            }

            MR::startPaneAnim(this, ::sChipPainName[i], "ChipGet", 0);
        } else {
            MR::setPaneAnimFrameAndStop(this, ::sChipPainName[i], 0.0f, 0);
        }
    }
}

void ChipCounter::requestShow(s32 groupId, s32 count) {
    bool isShown = !isNerve(&NrvChipCounter::ChipCounterNrvHide::sInstance) && !isNerve(&NrvChipCounter::ChipCounterNrvFrameOut::sInstance);

    if (!isShown) {
        appear();
        mCount = count;

        for (s32 i = 0; i < ::sChipPainCount; i++) {
            if (i < mCount) {
                MR::setPaneAnimFrameAndStop(this, ::sChipPainName[i], 1.0f, 0);
            } else {
                MR::setPaneAnimFrameAndStop(this, ::sChipPainName[i], 0.0f, 0);
            }
        }

        setNerve(&NrvChipCounter::ChipCounterNrvFrameIn::sInstance);
    }

    _2C = groupId;
}

void ChipCounter::requestComplete(s32 groupId) {
    bool isShown = !isNerve(&NrvChipCounter::ChipCounterNrvHide::sInstance) && !isNerve(&NrvChipCounter::ChipCounterNrvFrameOut::sInstance);

    if (!isShown) {
        appear();
    }

    _2C = groupId;
    MR::requestStartDemoWithoutCinemaFrame(this, "チップコンプリート", &NrvChipCounter::ChipCounterNrvComplete::sInstance,
                                           &NrvChipCounter::ChipCounterNrvTryDemo::sInstance);
}

void ChipCounter::requestHide(s32 groupId) {
    if (_2C != groupId) {
        return;
    }

    bool isShown = false;
    if (!isNerve(&NrvChipCounter::ChipCounterNrvHide::sInstance) && !isNerve(&NrvChipCounter::ChipCounterNrvFrameOut::sInstance)) {
        isShown = true;
    }

    if (isShown) {
        setNerve(&NrvChipCounter::ChipCounterNrvFrameOut::sInstance);
    }
}

void ChipCounter::requestActive() {
    bool isShown = false;
    if (!isNerve(&NrvChipCounter::ChipCounterNrvHide::sInstance) && !isNerve(&NrvChipCounter::ChipCounterNrvFrameOut::sInstance)) {
        isShown = true;
    }

    if (isShown) {
        appear();
    }
}

void ChipCounter::requestDeactive() {
    bool isDeactive = false;
    if (!isNerve(&NrvChipCounter::ChipCounterNrvTryDemo::sInstance) && !isNerve(&NrvChipCounter::ChipCounterNrvComplete::sInstance) &&
        !isNerve(&NrvChipCounter::ChipCounterNrvCompleteOut::sInstance)) {
        isDeactive = true;
    }

    if (isDeactive) {
        kill();
        mCollectCounter->kill();
    }
}

bool ChipCounter::tryEndFrameIn() {
    if (MR::isAnimStopped(this, 0)) {
        setNerve(&NrvChipCounter::ChipCounterNrvShow::sInstance);
        return true;
    }

    return false;
}

bool ChipCounter::tryEndFrameOut() {
    if (MR::isAnimStopped(this, 0)) {
        setNerve(&NrvChipCounter::ChipCounterNrvHide::sInstance);
        kill();
        return true;
    }

    return false;
}

bool ChipCounter::tryEndComplete() {
    if (MR::isAnimStopped(this, 0)) {
        setNerve(&NrvChipCounter::ChipCounterNrvCompleteOut::sInstance);
        return true;
    }

    return false;
}

void ChipCounter::exeHide() {
}

void ChipCounter::exeComplete() {
    if (MR::isFirstStep(this)) {
        for (s32 i = 0; i < ::sChipPainCount; i++) {
            MR::setPaneAnimFrameAndStop(this, ::sChipPainName[i], 1.0f, 0);
        }

        MR::showLayout(this);
        MR::startAnim(this, "Complete", 0);
        MR::requestMovementOn(mCollectCounter);
    }

    tryEndComplete();
}

void ChipCounter::exeCompleteOut() {
    if (MR::isFirstStep(this)) {
        MR::startAnim(this, "End", 0);
    }

    if (tryEndFrameOut()) {
        setNerve(&NrvChipCounter::ChipCounterNrvHide::sInstance);
        mCollectCounter->kill();
        MR::endDemo(this, "チップコンプリート");
        MR::noticeEndChipCompleteDemo(mType, _2C);
    }
}

ChipCounter::~ChipCounter() {
}

void NrvChipCounter::ChipCounterNrvFrameIn::execute(Spine* pSpine) const {
    ChipCounter* pActor = reinterpret_cast< ChipCounter* >(pSpine->mExecutor);

    if (MR::isFirstStep(pActor)) {
        MR::startAnim(pActor, "Appear", 0);
    }

    pActor->tryEndFrameIn();
}

void NrvChipCounter::ChipCounterNrvShow::execute(Spine* pSpine) const {
    ChipCounter* pActor = reinterpret_cast< ChipCounter* >(pSpine->mExecutor);

    if (MR::isFirstStep(pActor)) {
        MR::startAnim(pActor, "Wait", 0);
    }
}

void NrvChipCounter::ChipCounterNrvFrameOut::execute(Spine* pSpine) const {
    ChipCounter* pActor = reinterpret_cast< ChipCounter* >(pSpine->mExecutor);

    if (MR::isFirstStep(pActor)) {
        MR::startAnim(pActor, "End", 0);
    }

    pActor->tryEndFrameOut();
}

void NrvChipCounter::ChipCounterNrvTryDemo::execute(Spine* pSpine) const {
    ChipCounter* pActor = reinterpret_cast< ChipCounter* >(pSpine->mExecutor);

    if (MR::isFirstStep(pActor)) {
        MR::hideLayout(pActor);
    }
}
