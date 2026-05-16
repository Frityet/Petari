#include "Game/Screen/ProloguePictureBook.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Screen/IconAButton.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

namespace {
static const s32 sBookPageInfo[] = {0, 350, 700, 1050, 1400, 1748, -1};
};

namespace NrvProloguePictureBook {
NEW_NERVE(ProloguePictureBookActive, ProloguePictureBook, Active);
NEW_NERVE(ProloguePictureBookPlaying, ProloguePictureBook, Playing);
NEW_NERVE(ProloguePictureBookKeyWait, ProloguePictureBook, KeyWait);
NEW_NERVE(ProloguePictureBookEnd, ProloguePictureBook, End);
};  // namespace NrvProloguePictureBook

ProloguePictureBook::ProloguePictureBook()
    : LayoutActor("プロローグの絵本", true), mAButtonIcon(nullptr), mPage(0) {
}

ProloguePictureBook::~ProloguePictureBook() {
    delete mAButtonIcon;
}

void ProloguePictureBook::init(const JMapInfoIter &) {
    initWithoutIter();
}

void ProloguePictureBook::initWithoutIter() {
    MR::connectToSceneLayout(this);
    initLayoutManager("PrologueDemo", 1);
    initNerve(&NrvProloguePictureBook::ProloguePictureBookPlaying::sInstance);

    mAButtonIcon = MR::createAndSetupIconAButton(this, true, false);

    kill();
}

void ProloguePictureBook::appear() {
    LayoutActor::appear();
    mPage = 0;
    MR::requestMovementOn(mAButtonIcon);
    MR::startAnim(this, "Prologue", 0);
    setNerve(&NrvProloguePictureBook::ProloguePictureBookPlaying::sInstance);
}

void ProloguePictureBook::kill() {
    LayoutActor::kill();
    if (mAButtonIcon != nullptr) {
        mAButtonIcon->kill();
    }
}

void ProloguePictureBook::exeActive() {
    if (MR::isAnimStopped(this, 0)) {
        setNerve(&NrvProloguePictureBook::ProloguePictureBookEnd::sInstance);
    }
}

void ProloguePictureBook::exePlaying() {
    if (MR::isAnimStopped(this, 0)) {
        setNerve(&NrvProloguePictureBook::ProloguePictureBookEnd::sInstance);
    } else {
        int index = static_cast<int>(mPage) + 1;

        if (sBookPageInfo[index] < 0) {
            return;
        }

        static_cast<void>(MR::testSystemPadTriggerDecide());

        if (sBookPageInfo[index] > static_cast<s32>(MR::getAnimFrame(this, 0))) {
            return;
        }

        MR::setAnimFrame(this, static_cast<f32>(sBookPageInfo[index]) - 1.0f, 0);

        setNerve(&NrvProloguePictureBook::ProloguePictureBookKeyWait::sInstance);
    }
}

void ProloguePictureBook::exeKeyWait() {
    if (MR::isFirstStep(this)) {
        mAButtonIcon->openWithoutMessage();
        MR::setAnimRate(this, 0.0f, 0);
    }

    if (MR::testCorePadTriggerA(WPAD_CHAN0)) {
        MR::startSystemSE("SE_SY_TALK_FOCUS_ITEM", -1, -1);
        mAButtonIcon->term();

        mPage++;
        MR::setAnimRate(this, 1.0f, 0);

        setNerve(&NrvProloguePictureBook::ProloguePictureBookPlaying::sInstance);
    }
}

void ProloguePictureBook::exeEnd() {
}

bool ProloguePictureBook::isEnd() const {
    return isNerve(&NrvProloguePictureBook::ProloguePictureBookEnd::sInstance);
}

void ProloguePictureBook::control() {
    MR::setLayoutScalePosAtPaneScaleTrans(mAButtonIcon, this, "AButtonPosition");
}
