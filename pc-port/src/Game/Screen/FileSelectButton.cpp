#include "Game/Screen/FileSelectButton.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Screen/ButtonPaneController.hpp"
#include "Game/Screen/GalaxyMapGalaxyPlain.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

namespace {
    constexpr auto cButtonCount = 5;

    NEW_NERVE(FileSelectButtonNrvSelect, FileSelectButton, Select);
    NEW_NERVE(FileSelectButtonNrvWait, FileSelectButton, Wait);
    NEW_NERVE(FileSelectButtonNrvDisappear, FileSelectButton, Disappear);
}  // namespace

FileSelectButton::FileSelectButton(const char* pName) : LayoutActor(pName, true), mButtonCtrl{}, mCallbackFunctor{}, _48(nullptr) {
}

FileSelectButton::~FileSelectButton() {
    for (auto* callback : mCallbackFunctor) {
        delete callback;
    }
    for (auto* button : mButtonCtrl) {
        delete button;
    }
    delete _48;
}

void FileSelectButton::init(const JMapInfoIter&) {
    initLayoutManager("FileSelect", 1);
    createPaneControl();
    createButtonController();
    createOthers();
    MR::connectToSceneLayout(this);
    initNerve(&FileSelectButtonNrvSelect::sInstance);
}

void FileSelectButton::appear() {
    LayoutActor::appear();
    for (auto* button : mButtonCtrl) {
        button->appear();
    }
    setNerve(&FileSelectButtonNrvSelect::sInstance);
}

void FileSelectButton::kill() {
    LayoutActor::kill();
}

void FileSelectButton::disappear() {
    for (auto* button : mButtonCtrl) {
        button->disappear();
    }
    setNerve(&FileSelectButtonNrvDisappear::sInstance);
}

void FileSelectButton::setCallbackFunctor(const MR::FunctorBase& rStartFunctor, const MR::FunctorBase& rCopyFunctor,
                                          const MR::FunctorBase& rMiiFunctor, const MR::FunctorBase& rDeleteFunctor,
                                          const MR::FunctorBase& rManualFunctor) {
    const MR::FunctorBase* functors[cButtonCount] = {&rStartFunctor, &rCopyFunctor, &rMiiFunctor, &rDeleteFunctor, &rManualFunctor};
    for (auto i = 0; i < cButtonCount; ++i) {
        delete mCallbackFunctor[i];
        mCallbackFunctor[i] = functors[i]->clone(nullptr);
    }
}

void FileSelectButton::shiftSelect() {
    setNerve(&FileSelectButtonNrvSelect::sInstance);
}

void FileSelectButton::exeSelect() {
    for (auto i = 0; i < cButtonCount; ++i) {
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
    for (auto* button : mButtonCtrl) {
        if (!button->isHidden()) {
            return;
        }
    }
    kill();
}

void FileSelectButton::control() {
    for (auto* button : mButtonCtrl) {
        if (button->isDecidedWait()) {
            button->forceToWait();
        }
        button->update();
    }

    if (_48 != nullptr && mButtonCtrl[4]->isPointing()) {
        _48->show("2PGuidanceIcon", "P2Button");
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

    for (auto* button : mButtonCtrl) {
        button->_22 = false;
    }
}

void FileSelectButton::createOthers() {
    _48 = new GalaxyMapGalaxyPlain(this);
    _48->initWithoutIter();
    MR::connectToSceneLayoutDecoration(_48);
}

const ButtonPaneController* FileSelectButton::getButtonController(s32 index) const {
    if (index < 0 || index >= cButtonCount) {
        return nullptr;
    }
    return mButtonCtrl[index];
}
