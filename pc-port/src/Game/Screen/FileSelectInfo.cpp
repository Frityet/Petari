#include "Game/Screen/FileSelectInfo.hpp"

#include <algorithm>
#include <cwchar>

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

namespace {
    constexpr auto sDisappearAnimRate = 0.25F;

    NEW_NERVE(FileSelectInfoNrvAppear, FileSelectInfo, Appear);
    NEW_NERVE(FileSelectInfoNrvDisplay, FileSelectInfo, Display);
    NEW_NERVE(FileSelectInfoNrvDisappear, FileSelectInfo, Disappear);
}  // namespace

namespace FileSelectInfoSub {
    NEW_NERVE(SlideStateNrvNormalPos, SlideState, NormalPos);
    NEW_NERVE(SlideStateNrvSliding, SlideState, Sliding);
    NEW_NERVE(SlideStateNrvSlidePos, SlideState, SlidePos);
    NEW_NERVE(SlideStateNrvSlidingBack, SlideState, SlidingBack);

    NEW_NERVE(CharaStateNrvMario, CharaState, Mario);
    NEW_NERVE(CharaStateNrvToLuigi, CharaState, ToLuigi);
    NEW_NERVE(CharaStateNrvLuigi, CharaState, Luigi);
    NEW_NERVE(CharaStateNrvToMario, CharaState, ToMario);
}  // namespace FileSelectInfoSub

FileSelectInfo::FileSelectInfo(s32 nameBufferSize, const char* pName)
    : LayoutActor(pName, true), mNumber(0), mStarNum(0), mStarPieceNum(0), mNameBufferSize(nameBufferSize), mName(new wchar_t[nameBufferSize]{}),
      mDateMessage{}, mTimeMessage{}, mMissNum(-1), mIsSelectedMarioPrev(true), mIsSelectedMario(true), mIsViewNormalEnding(false),
      mIsViewCompleteEnding(false), mSlideState(new FileSelectInfoSub::SlideState(this)), mCharaState(new FileSelectInfoSub::CharaState(this)) {
}

FileSelectInfo::~FileSelectInfo() {
    delete mSlideState;
    delete mCharaState;
    delete[] mName;
}

void FileSelectInfo::init(const JMapInfoIter&) {
    initLayoutManager("FileInfo", 3);
    MR::connectToSceneLayout(this);
    initNerve(&FileSelectInfoNrvAppear::sInstance);
}

void FileSelectInfo::appear() {
    if (!MR::isDead(this) && isNerve(&FileSelectInfoNrvDisappear::sInstance)) {
        const auto animFrame = MR::getAnimFrame(this, 0);
        MR::startAnim(this, "Appear", 0);
        MR::setAnimFrame(this, animFrame, 0);
    } else {
        MR::startAnim(this, "Appear", 0);
    }

    setNerve(&FileSelectInfoNrvAppear::sInstance);
    LayoutActor::appear();
}

void FileSelectInfo::disappear() {
    if (MR::isDead(this) || isNerve(&FileSelectInfoNrvDisappear::sInstance)) {
        return;
    }

    auto animFrame = 0.0F;
    if (isNerve(&FileSelectInfoNrvAppear::sInstance)) {
        animFrame = MR::getAnimFrame(this, 0);
    } else {
        MR::startAnim(this, "Appear", 0);
        animFrame = static_cast< f32 >(MR::getAnimCtrl(this, 0)->mEnd) - 1.0F;
    }

    MR::startAnim(this, "Appear", 0);
    MR::setAnimFrame(this, animFrame, 0);
    MR::setAnimRate(this, 0.0F, 0);
    setNerve(&FileSelectInfoNrvDisappear::sInstance);
}

void FileSelectInfo::slide() {
    mSlideState->setNerve(&FileSelectInfoSub::SlideStateNrvSliding::sInstance);
}

void FileSelectInfo::slideBack() {
    mSlideState->setNerve(&FileSelectInfoSub::SlideStateNrvSlidingBack::sInstance);
}

void FileSelectInfo::setInfo(u16* pName, s32 number, s32 starNum, s32 starPieceNum, bool isSelectedMario, bool isViewNormalEnding,
                             bool isViewCompleteEnding, const wchar_t* pDateMessage, const wchar_t* pTimeMessage, s32 missNum) {
    mNumber = number;
    mStarNum = starNum;
    mStarPieceNum = starPieceNum;

    std::wmemset(mName, 0, static_cast< std::size_t >(mNameBufferSize));
    if (pName != nullptr) {
        for (auto i = s32{0}; i < mNameBufferSize - 1 && pName[i] != 0U; ++i) {
            mName[i] = static_cast< wchar_t >(pName[i]);
        }
    }

    mIsSelectedMario = isSelectedMario;
    mIsViewNormalEnding = isViewNormalEnding;
    mIsViewCompleteEnding = isViewCompleteEnding;

    std::wmemset(mDateMessage, 0, std::size(mDateMessage));
    std::wmemset(mTimeMessage, 0, std::size(mTimeMessage));
    if (pDateMessage != nullptr) {
        std::wmemcpy(mDateMessage, pDateMessage, std::min(std::size(mDateMessage) - 1U, std::wcslen(pDateMessage)));
    }
    if (pTimeMessage != nullptr) {
        std::wmemcpy(mTimeMessage, pTimeMessage, std::min(std::size(mTimeMessage) - 1U, std::wcslen(pTimeMessage)));
    }

    mMissNum = missNum;
}

void FileSelectInfo::change() {
    if (mIsSelectedMarioPrev && !mIsSelectedMario) {
        if (!mCharaState->isNerve(&FileSelectInfoSub::CharaStateNrvLuigi::sInstance) &&
            !mCharaState->isNerve(&FileSelectInfoSub::CharaStateNrvToLuigi::sInstance)) {
            mCharaState->setNerve(&FileSelectInfoSub::CharaStateNrvToLuigi::sInstance);
        }
    } else if (!mIsSelectedMarioPrev && mIsSelectedMario) {
        if (!mCharaState->isNerve(&FileSelectInfoSub::CharaStateNrvMario::sInstance) &&
            !mCharaState->isNerve(&FileSelectInfoSub::CharaStateNrvToMario::sInstance)) {
            mCharaState->setNerve(&FileSelectInfoSub::CharaStateNrvToMario::sInstance);
        }
    }

    mIsSelectedMarioPrev = mIsSelectedMario;
}

void FileSelectInfo::forceChange() {
    if (mIsSelectedMarioPrev && !mIsSelectedMario) {
        mCharaState->setNerve(&FileSelectInfoSub::CharaStateNrvLuigi::sInstance);
        reflectInfo();
    } else if (!mIsSelectedMarioPrev && mIsSelectedMario) {
        mCharaState->setNerve(&FileSelectInfoSub::CharaStateNrvMario::sInstance);
        reflectInfo();
    }

    mIsSelectedMarioPrev = mIsSelectedMario;
}

void FileSelectInfo::exeAppear() {
    if (MR::isFirstStep(this)) {
        reflectInfo();
    }

    if (MR::isAnimStopped(this, 0)) {
        setNerve(&FileSelectInfoNrvDisplay::sInstance);
    }
}

void FileSelectInfo::exeDisplay() {
}

void FileSelectInfo::exeDisappear() {
    if (MR::isFirstStep(this)) {
        MR::setAnimRate(this, -sDisappearAnimRate, 0);
    }

    if (MR::getAnimFrame(this, 0) - sDisappearAnimRate <= 0.0F) {
        kill();
    }
}

void FileSelectInfo::control() {
    mSlideState->updateNerve();
    mCharaState->updateNerve();
}

void FileSelectInfo::reflectInfo() {
    MR::setTextBoxNumberRecursive(this, "FileNumber", mNumber);
    MR::setTextBoxMessageRecursive(this, "FileName", mName);
    MR::setTextBoxNumberRecursive(this, "Star", mStarNum);
    MR::setTextBoxNumberRecursive(this, "StarPiece", mStarPieceNum);
    MR::setTextBoxMessageRecursive(this, "TxtDay", mDateMessage);
    MR::setTextBoxMessageRecursive(this, "TxtTime", mTimeMessage);

    if (mMissNum >= 0) {
        MR::setTextBoxNumberRecursive(this, "MissCounter", mMissNum);
        MR::showPane(this, "MissCounter");
    } else {
        MR::hidePane(this, "MissCounter");
    }

    mIsViewNormalEnding ? MR::showPane(this, "Complete1") : MR::hidePane(this, "Complete1");
    mIsViewCompleteEnding ? MR::showPane(this, "Complete2") : MR::hidePane(this, "Complete2");

    if (!mIsSelectedMarioPrev || mIsViewCompleteEnding) {
        MR::showPane(this, "BrosIcon");
        mIsSelectedMarioPrev ? MR::showPane(this, "TxtMario") : MR::hidePane(this, "TxtMario");
        mIsSelectedMarioPrev ? MR::hidePane(this, "TxtLuigi") : MR::showPane(this, "TxtLuigi");
    } else {
        MR::hidePane(this, "BrosIcon");
    }
}

s32 FileSelectInfo::getFileNumber() const {
    return mNumber;
}

s32 FileSelectInfo::getStarNum() const {
    return mStarNum;
}

s32 FileSelectInfo::getStarPieceNum() const {
    return mStarPieceNum;
}

bool FileSelectInfo::isSelectedMario() const {
    return mIsSelectedMario;
}

#ifndef NDEBUG
s32 FileSelectInfo::getMissNum() const {
    return mMissNum;
}

const wchar_t* FileSelectInfo::getDateMessage() const {
    return mDateMessage;
}

const wchar_t* FileSelectInfo::getTimeMessage() const {
    return mTimeMessage;
}
#endif

namespace FileSelectInfoSub {
    SlideState::SlideState(FileSelectInfo* pHost) : NerveExecutor("スライド状態"), mHost(pHost) {
        initNerve(&SlideStateNrvNormalPos::sInstance);
    }

    void SlideState::exeNormalPos() {
        if (MR::isFirstStep(this)) {
            MR::startAnim(mHost, "ButtonAppear", 1);
            MR::stopAnim(mHost, 1);
        }
    }

    void SlideState::exeSliding() {
        if (MR::isFirstStep(this)) {
            MR::startAnim(mHost, "ButtonAppear", 1);
        }
        if (MR::isAnimStopped(mHost, 1)) {
            setNerve(&SlideStateNrvSlidePos::sInstance);
        }
    }

    void SlideState::exeSlidePos() {
        if (MR::isFirstStep(this)) {
            MR::startAnim(mHost, "ButtonEnd", 1);
            MR::stopAnim(mHost, 1);
        }
    }

    void SlideState::exeSlidingBack() {
        if (MR::isFirstStep(this)) {
            MR::startAnim(mHost, "ButtonEnd", 1);
        }
        if (MR::isAnimStopped(mHost, 1)) {
            setNerve(&SlideStateNrvNormalPos::sInstance);
        }
    }

    CharaState::CharaState(FileSelectInfo* pHost) : NerveExecutor("キャラ選択状態"), mHost(pHost) {
        initNerve(&CharaStateNrvMario::sInstance);
    }

    void CharaState::exeMario() {
        if (MR::isFirstStep(this)) {
            MR::startAnim(mHost, "MarioWait", 2);
        }
    }

    void CharaState::exeToLuigi() {
        if (MR::isFirstStep(this)) {
            MR::startAnim(mHost, "MariotoLuigi", 2);
        }
        if (MR::isStep(this, 10)) {
            mHost->reflectInfo();
        }
        if (MR::isAnimStopped(mHost, 2)) {
            setNerve(&CharaStateNrvLuigi::sInstance);
        }
    }

    void CharaState::exeLuigi() {
        if (MR::isFirstStep(this)) {
            MR::startAnim(mHost, "LuigiWait", 2);
        }
    }

    void CharaState::exeToMario() {
        if (MR::isFirstStep(this)) {
            MR::startAnim(mHost, "LuigitoMario", 2);
        }
        if (MR::isStep(this, 10)) {
            mHost->reflectInfo();
        }
        if (MR::isAnimStopped(mHost, 2)) {
            setNerve(&CharaStateNrvMario::sInstance);
        }
    }
}  // namespace FileSelectInfoSub
