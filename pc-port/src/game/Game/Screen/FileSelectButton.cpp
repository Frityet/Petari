#include "Game/Screen/FileSelectButton.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Screen/ButtonPaneController.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

namespace {

NEW_NERVE(FileSelectButtonNrvSelect, FileSelectButton, Select);
NEW_NERVE(FileSelectButtonNrvWait, FileSelectButton, Wait);
NEW_NERVE(FileSelectButtonNrvDisappear, FileSelectButton, Disappear);

}  // namespace

FileSelectButton::FileSelectButton(const char *pName)
    : LayoutActor(pName, true), _48(nullptr) {
    for (int i = 0; i < static_cast<int>(sizeof(mButtonCtrl) / sizeof(*mButtonCtrl)); i++) {
        mButtonCtrl[i] = nullptr;
        mCallbackFunctor[i] = nullptr;
    }
}

FileSelectButton::~FileSelectButton() {
    for (int i = 0; i < static_cast<int>(sizeof(mButtonCtrl) / sizeof(*mButtonCtrl)); i++) {
        delete mButtonCtrl[i];
        delete mCallbackFunctor[i];
    }
}

void FileSelectButton::init(const JMapInfoIter &) {
    initWithoutIter();
}

void FileSelectButton::initWithoutIter() {
    initLayoutManager("FileSelect", 1);
    createPaneControl();
    createButtonController();
    createOthers();
    MR::connectToSceneLayout(this);
    initNerve(&FileSelectButtonNrvSelect::sInstance);
}

void FileSelectButton::appear() {
    LayoutActor::appear();

    int size = sizeof(mButtonCtrl) / sizeof(*mButtonCtrl);
    for (int i = 0; i < size; i++) {
        if (mButtonCtrl[i] != nullptr) {
            mButtonCtrl[i]->appear();
        }
    }

    setNerve(&FileSelectButtonNrvSelect::sInstance);
}

void FileSelectButton::kill() {
    LayoutActor::kill();
}

void FileSelectButton::disappear() {
    int size = sizeof(mButtonCtrl) / sizeof(*mButtonCtrl);
    for (int i = 0; i < size; i++) {
        if (mButtonCtrl[i] != nullptr) {
            mButtonCtrl[i]->disappear();
        }
    }

    setNerve(&FileSelectButtonNrvDisappear::sInstance);
}

void FileSelectButton::setCallbackFunctor(const MR::FunctorBase &rStartFunctor, const MR::FunctorBase &rCopyFunctor, const MR::FunctorBase &rMiiFunctor, const MR::FunctorBase &rDeleteFunctor, const MR::FunctorBase &rManualFunctor) {
    delete mCallbackFunctor[0];
    delete mCallbackFunctor[1];
    delete mCallbackFunctor[2];
    delete mCallbackFunctor[3];
    delete mCallbackFunctor[4];
    mCallbackFunctor[0] = rStartFunctor.clone(nullptr);
    mCallbackFunctor[1] = rCopyFunctor.clone(nullptr);
    mCallbackFunctor[2] = rMiiFunctor.clone(nullptr);
    mCallbackFunctor[3] = rDeleteFunctor.clone(nullptr);
    mCallbackFunctor[4] = rManualFunctor.clone(nullptr);
}

void FileSelectButton::shiftSelect() {
    setNerve(&FileSelectButtonNrvSelect::sInstance);
}

void FileSelectButton::exeSelect() {
    int size = sizeof(mButtonCtrl) / sizeof(*mButtonCtrl);

    for (int i = 0; i < size; i++) {
        if (mButtonCtrl[i] == nullptr) {
            continue;
        }

        if (mButtonCtrl[i]->isPointingTrigger()) {
            MR::startSystemSE("SE_SY_BUTTON_CURSOR_ON", -1, -1);
        }

        if (mButtonCtrl[i]->trySelect()) {
            if (mCallbackFunctor[i] != nullptr) {
                (*mCallbackFunctor[i])();
            }

            setNerve(&FileSelectButtonNrvWait::sInstance);
            break;
        }
    }
}

void FileSelectButton::exeWait() {
}

void FileSelectButton::exeDisappear() {
    int size = sizeof(mButtonCtrl) / sizeof(*mButtonCtrl);

    for (int i = 0; i < size; i++) {
        if (mButtonCtrl[i] != nullptr && !mButtonCtrl[i]->isHidden()) {
            return;
        }
    }

    kill();
}

void FileSelectButton::control() {
    int size = sizeof(mButtonCtrl) / sizeof(*mButtonCtrl);

    for (int i = 0; i < size; i++) {
        if (mButtonCtrl[i] == nullptr) {
            continue;
        }

        if (mButtonCtrl[i]->isDecidedWait()) {
            mButtonCtrl[i]->forceToWait();
        }

        mButtonCtrl[i]->update();
    }
}

void FileSelectButton::createPaneControl() {
    MR::createAndAddPaneCtrl(this, "CopyButton", 1);
    MR::createAndAddPaneCtrl(this, "MiiButton", 1);
    MR::createAndAddPaneCtrl(this, "DeleteButton", 1);
    MR::createAndAddPaneCtrl(this, "StartButton", 1);
    MR::createAndAddPaneCtrl(this, "P2ManualButton", 1);
}

void FileSelectButton::createButtonController() {
    mButtonCtrl[1] = new ButtonPaneController(this, "CopyButton", "PicCopy", 0, true);
    mButtonCtrl[2] = new ButtonPaneController(this, "MiiButton", "PicMii", 0, true);
    mButtonCtrl[3] = new ButtonPaneController(this, "DeleteButton", "PicDelete", 0, true);
    mButtonCtrl[0] = new ButtonPaneController(this, "StartButton", "BoxStartButton", 0, true);
    mButtonCtrl[4] = new ButtonPaneController(this, "P2ManualButton", "BoxP2", 0, true);

    for (int i = 0; i < static_cast<int>(sizeof(mButtonCtrl) / sizeof(*mButtonCtrl)); i++) {
        mButtonCtrl[i]->_22 = false;
    }
}

void FileSelectButton::createOthers() {
    _48 = nullptr;
}
