#include "Game/Screen/FileSelectInfo.hpp"

#include <algorithm>

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

namespace {

static const f32 sDisappearAnimRate = 0.25f;

NEW_NERVE(FileSelectInfoNrvAppear, FileSelectInfo, Appear);
NEW_NERVE(FileSelectInfoNrvDisplay, FileSelectInfo, Display);
NEW_NERVE(FileSelectInfoNrvDisappear, FileSelectInfo, Disappear);

void copy_u16_to_wchar(wchar_t *pDst, s32 dstCount, const u16 *pSrc) {
    if (pDst == nullptr || dstCount <= 0) {
        return;
    }

    s32 i = 0;
    if (pSrc != nullptr) {
        for (; i + 1 < dstCount && pSrc[i] != 0U; ++i) {
            pDst[i] = static_cast<wchar_t>(pSrc[i]);
        }
    }
    pDst[i] = L'\0';
}

void copy_wchar(wchar_t *pDst, s32 dstCount, const wchar_t *pSrc) {
    if (pDst == nullptr || dstCount <= 0) {
        return;
    }

    s32 i = 0;
    if (pSrc != nullptr) {
        for (; i + 1 < dstCount && pSrc[i] != L'\0'; ++i) {
            pDst[i] = pSrc[i];
        }
    }
    pDst[i] = L'\0';
}

}  // namespace

namespace FileSelectInfoSub {
NEW_NERVE(SlideStateNrvNormalPos, SlideState, NormalPos);
NEW_NERVE(SlideStateNrvSliding, SlideState, Sliding);
NEW_NERVE(SlideStateNrvSlidePos, SlideState, SlidePos);
NEW_NERVE(SlideStateNrvSlidingBack, SlideState, SlidingBack);
}  // namespace FileSelectInfoSub

namespace FileSelectInfoSub {
NEW_NERVE(CharaStateNrvMario, CharaState, Mario);
NEW_NERVE(CharaStateNrvToLuigi, CharaState, ToLuigi);
NEW_NERVE(CharaStateNrvLuigi, CharaState, Luigi);
NEW_NERVE(CharaStateNrvToMario, CharaState, ToMario);
}  // namespace FileSelectInfoSub

FileSelectInfo::FileSelectInfo(s32 nameBufferSize, const char *pName)
    : LayoutActor(pName, true),
      mNumber(0),
      mStarNum(0),
      mStarPieceNum(0),
      mNameBufferSize(std::max(1, nameBufferSize)),
      mName(new wchar_t[static_cast<std::size_t>(std::max(1, nameBufferSize))] {}),
      mMissNum(-1),
      mIsSelectedMarioPrev(true),
      mIsSelectedMario(true),
      mIsViewNormalEnding(false),
      mIsViewCompleteEnding(false),
      mSlideState(new FileSelectInfoSub::SlideState(this)),
      mCharaState(new FileSelectInfoSub::CharaState(this)) {
    mDateMessage[0] = L'\0';
    mTimeMessage[0] = L'\0';
}

FileSelectInfo::~FileSelectInfo() {
    delete[] mName;
    delete mSlideState;
    delete mCharaState;
}

void FileSelectInfo::init(const JMapInfoIter &) {
    initLayoutManager("FileInfo", 3);
    MR::connectToSceneLayout(this);
    initNerve(&FileSelectInfoNrvAppear::sInstance);
}

void FileSelectInfo::appear() {
    f32 animFrame;

    if (!MR::isDead(this) && isNerve(&FileSelectInfoNrvDisappear::sInstance)) {
        animFrame = MR::getAnimFrame(this, 0);
        MR::startAnim(this, "Appear", 0);
        MR::setAnimFrame(this, animFrame, 0);
    } else {
        MR::startAnim(this, "Appear", 0);
    }

    setNerve(&FileSelectInfoNrvAppear::sInstance);
    LayoutActor::appear();
}

void FileSelectInfo::disappear() {
    f32 animFrame;

    if (!MR::isDead(this) && !isNerve(&FileSelectInfoNrvDisappear::sInstance)) {
        if (isNerve(&FileSelectInfoNrvAppear::sInstance)) {
            animFrame = MR::getAnimFrame(this, 0);
        } else {
            MR::startAnim(this, "Appear", 0);
            animFrame = MR::getAnimCtrl(this, 0)->mEnd - 1.0f;
        }

        MR::startAnim(this, "Appear", 0);
        MR::setAnimFrame(this, animFrame, 0);
        MR::setAnimRate(this, 0.0f, 0);
        setNerve(&FileSelectInfoNrvDisappear::sInstance);
    }
}

void FileSelectInfo::slide() {
    mSlideState->setNerve(&FileSelectInfoSub::SlideStateNrvSliding::sInstance);
}

void FileSelectInfo::slideBack() {
    mSlideState->setNerve(&FileSelectInfoSub::SlideStateNrvSlidingBack::sInstance);
}

void FileSelectInfo::setInfo(u16 *pName, s32 number, s32 starNum, s32 starPieceNum, bool isSelectedMario, bool isViewNormalEnding, bool isViewCompleteEnding, const wchar_t *pDateMessage, const wchar_t *pTimeMessage, s32 missNum) {
    mNumber = number;
    mStarNum = starNum;
    mStarPieceNum = starPieceNum;

    copy_u16_to_wchar(mName, mNameBufferSize, pName);

    mIsSelectedMario = isSelectedMario;
    mIsViewNormalEnding = isViewNormalEnding;
    mIsViewCompleteEnding = isViewCompleteEnding;

    copy_wchar(mDateMessage, static_cast<s32>(sizeof(mDateMessage) / sizeof(wchar_t)), pDateMessage);
    copy_wchar(mTimeMessage, static_cast<s32>(sizeof(mTimeMessage) / sizeof(wchar_t)), pTimeMessage);

    mMissNum = missNum;
}

void FileSelectInfo::change() {
    FileSelectInfoSub::CharaState *pCharaState;

    if (mIsSelectedMarioPrev && !mIsSelectedMario) {
        pCharaState = mCharaState;

        if (!pCharaState->isNerve(&FileSelectInfoSub::CharaStateNrvLuigi::sInstance) &&
            !pCharaState->isNerve(&FileSelectInfoSub::CharaStateNrvToLuigi::sInstance)) {
            pCharaState->setNerve(&FileSelectInfoSub::CharaStateNrvToLuigi::sInstance);
        }
    } else if (!mIsSelectedMarioPrev && mIsSelectedMario) {
        pCharaState = mCharaState;

        if (!pCharaState->isNerve(&FileSelectInfoSub::CharaStateNrvMario::sInstance) &&
            !pCharaState->isNerve(&FileSelectInfoSub::CharaStateNrvToMario::sInstance)) {
            pCharaState->setNerve(&FileSelectInfoSub::CharaStateNrvToMario::sInstance);
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

    if (MR::getAnimFrame(this, 0) - sDisappearAnimRate <= 0.0f) {
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

    if (mIsViewNormalEnding) {
        MR::showPane(this, "Complete1");
    } else {
        MR::hidePane(this, "Complete1");
    }

    if (mIsViewCompleteEnding) {
        MR::showPane(this, "Complete2");
    } else {
        MR::hidePane(this, "Complete2");
    }

    bool show_bros = !mIsSelectedMarioPrev || mIsViewCompleteEnding;

    if (show_bros) {
        MR::showPane(this, "BrosIcon");

        if (mIsSelectedMarioPrev) {
            MR::showPane(this, "TxtMario");
            MR::hidePane(this, "TxtLuigi");
        } else {
            MR::hidePane(this, "TxtMario");
            MR::showPane(this, "TxtLuigi");
        }
    } else {
        MR::hidePane(this, "BrosIcon");
    }
}

namespace FileSelectInfoSub {

SlideState::SlideState(FileSelectInfo *pHost)
    : NerveExecutor("スライド状態"), mHost(pHost) {
    initNerve(&SlideStateNrvNormalPos::sInstance);
}

void SlideState::exeNormalPos() {
    if (MR::isFirstStep(this)) {
        MR::startAnim(mHost, "ButtonAppear", 1);
        MR::setAnimFrameAndStop(mHost, 0.0f, 1);
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
        MR::setAnimFrameAndStop(mHost, 0.0f, 1);
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

}  // namespace FileSelectInfoSub

namespace FileSelectInfoSub {

CharaState::CharaState(FileSelectInfo *pHost)
    : NerveExecutor("キャラ選択状態"), mHost(pHost) {
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
