#include "Game/Screen/IconAButton.hpp"

#include <cstdio>

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

namespace NrvIconAButton {
    NEW_NERVE(IconAButtonNrvOpen, IconAButton, Open);
    NEW_NERVE(IconAButtonNrvWait, IconAButton, Wait);
    NEW_NERVE(IconAButtonNrvTerm, IconAButton, Term);
}  // namespace NrvIconAButton

IconAButton::IconAButton(bool connectToScene, bool connectToPause)
    : LayoutActor("Aボタンアイコン", true), mFollowPos{}, mFollowActor(nullptr), mFollowPaneName{} {
    if (connectToScene) {
        if (connectToPause) {
            MR::connectToSceneLayoutOnPause(this);
        } else {
            MR::connectToSceneTalkLayout(this);
        }
    }
}

void IconAButton::init(const JMapInfoIter&) {
    initLayoutManager("IconAButton", 1);
    initNerve(&NrvIconAButton::IconAButtonNrvOpen::sInstance);
    MR::startAnim(this, "Appear", 0);
    MR::setAnimFrameAndStop(this, 0.0F, 0);
    kill();
}

void IconAButton::control() {
    updateFollowPos();
}

void IconAButton::setFollowActorPane(LayoutActor* pActor, const char* pName) {
    mFollowActor = pActor;
    std::snprintf(mFollowPaneName, sizeof(mFollowPaneName), "%s", pName != nullptr ? pName : "");
    MR::setFollowPos(&mFollowPos, this, nullptr);
}

bool IconAButton::isOpen() const {
    return !MR::isDead(this);
}

bool IconAButton::isWait() const {
    return isNerve(&NrvIconAButton::IconAButtonNrvWait::sInstance);
}

void IconAButton::openWithTalk() {
    appear();
    updateFollowPos();
    MR::showPane(this, "PicPlate");
    setNerve(&NrvIconAButton::IconAButtonNrvOpen::sInstance);
    MR::setTextBoxLayoutMessageRecursive(this, "IconAButton", "Layout_SystemTalk");
}

void IconAButton::openWithRead() {
    appear();
    updateFollowPos();
    MR::showPane(this, "PicPlate");
    setNerve(&NrvIconAButton::IconAButtonNrvOpen::sInstance);
    MR::setTextBoxLayoutMessageRecursive(this, "IconAButton", "Layout_SystemRead");
}

void IconAButton::openWithTurn() {
    appear();
    updateFollowPos();
    MR::showPane(this, "PicPlate");
    setNerve(&NrvIconAButton::IconAButtonNrvOpen::sInstance);
    MR::setTextBoxLayoutMessageRecursive(this, "IconAButton", "Layout_SystemTurn");
}

void IconAButton::openWithoutMessage() {
    appear();
    updateFollowPos();
    MR::showPane(this, "PicPlate");
    setNerve(&NrvIconAButton::IconAButtonNrvOpen::sInstance);
    MR::hidePane(this, "PicPlate");
    MR::clearTextBoxMessageRecursive(this, "IconAButton");
}

void IconAButton::term() {
    if (!MR::isDead(this) &&
        (isNerve(&NrvIconAButton::IconAButtonNrvWait::sInstance) || isNerve(&NrvIconAButton::IconAButtonNrvOpen::sInstance))) {
        setNerve(&NrvIconAButton::IconAButtonNrvTerm::sInstance);
    }
}

void IconAButton::exeOpen() {
    if (MR::isFirstStep(this)) {
        MR::startAnim(this, "Appear", 0);
    }

    if (MR::isAnimStopped(this, 0)) {
        setNerve(&NrvIconAButton::IconAButtonNrvWait::sInstance);
    }
}

void IconAButton::exeWait() {
    if (MR::isFirstStep(this)) {
        MR::startAnim(this, "Press", 0);
    }
}

void IconAButton::exeTerm() {
    if (MR::isFirstStep(this)) {
        MR::startAnim(this, "End", 0);
    }

    if (MR::isAnimStopped(this, 0)) {
        kill();
    }
}

void IconAButton::updateFollowPos() {
    if (mFollowActor != nullptr) {
        MR::copyPaneTrans(&mFollowPos, mFollowActor, mFollowPaneName);
        MR::setFollowPos(&mFollowPos, this, nullptr);
    }
}
