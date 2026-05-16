#include "Game/Screen/InformationMessage.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Screen/IconAButton.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

namespace NrvInformationMessage {
NEW_NERVE(InformationMessageNrvAppear, InformationMessage, Appear);
NEW_NERVE(InformationMessageNrvWait, InformationMessage, Wait);
NEW_NERVE(InformationMessageNrvDisappear, InformationMessage, Disappear);
}  // namespace NrvInformationMessage

InformationMessage::InformationMessage()
    : LayoutActor("インフォメーションメッセージ", true), mAButtonIcon(nullptr), mIsCenter(false) {
}

InformationMessage::~InformationMessage() {
    delete mAButtonIcon;
}

void InformationMessage::init(const JMapInfoIter &) {
    initWithoutIter();
}

void InformationMessage::initWithoutIter() {
    initLayoutManager("InformationWindow", 2);
    MR::createAndAddPaneCtrl(this, "AButtonPosC", 1);
    MR::createAndAddPaneCtrl(this, "AButtonPosU", 1);
    MR::connectToSceneLayoutDecoration(this);

    mAButtonIcon = new IconAButton(true, false);
    mAButtonIcon->initWithoutIter();

    initNerve(&NrvInformationMessage::InformationMessageNrvAppear::sInstance);
    kill();
}

void InformationMessage::appear() {
    if (mIsCenter) {
        MR::hidePaneRecursive(this, "InfoWindowUp");
        MR::showPaneRecursive(this, "InfoWindowCenter");
    } else {
        MR::showPaneRecursive(this, "InfoWindowUp");
        MR::hidePaneRecursive(this, "InfoWindowCenter");
    }

    MR::startAnim(this, "Appear", 0);
    MR::startAnim(this, "Line", 1);
    setNerve(&NrvInformationMessage::InformationMessageNrvAppear::sInstance);
    LayoutActor::appear();
}

void InformationMessage::appearWithButtonLayout() {
    appear();
    mAButtonIcon->setFollowActorPane(this, mIsCenter ? "AButtonPosC" : "AButtonPosU");
    mAButtonIcon->openWithoutMessage();
}

void InformationMessage::disappear() {
    setNerve(&NrvInformationMessage::InformationMessageNrvDisappear::sInstance);
}

void InformationMessage::setMessage(const char *pMessageId) {
    MR::setTextBoxGameMessageRecursive(this, nullptr, pMessageId);
}

void InformationMessage::setMessage(const wchar_t *pMessage) {
    MR::setTextBoxMessageRecursive(this, nullptr, pMessage);
}

void InformationMessage::setReplaceString(const wchar_t *pMessage, s32 index) {
    MR::setTextBoxArgStringRecursive(this, nullptr, pMessage, index);
}

void InformationMessage::exeAppear() {
    MR::setNerveAtAnimStopped(this, &NrvInformationMessage::InformationMessageNrvWait::sInstance, 0);
}

void InformationMessage::exeWait() {
    MR::startAnimAtFirstStep(this, "Wait", 0);
}

void InformationMessage::exeDisappear() {
    if (MR::isFirstStep(this)) {
        MR::startAnim(this, "End", 0);

        if (!MR::isDead(mAButtonIcon)) {
            mAButtonIcon->term();
        }
    }

    MR::killAtAnimStopped(this, 0);
}
