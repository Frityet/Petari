#include "Game/Screen/YesNoController.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Screen/ButtonPaneController.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

namespace {
    const char* sDefaultCursorSE = "SE_SY_TALK_FOCUS_ITEM";
    const char* sDefaultYesSE = "SE_SY_TALK_SELECT_YES";
    const char* sDefaultNoSE = "SE_SY_TALK_SELECT_NO";
}  // namespace

namespace NrvYesNoController {
    NEW_NERVE(YesNoControllerNrvSelecting, YesNoController, Selecting);
    NEW_NERVE(YesNoControllerNrvDecided, YesNoController, Decided);
    NEW_NERVE(YesNoControllerNrvDisappear, YesNoController, Disappear);
    NEW_NERVE(YesNoControllerNrvSelected, YesNoController, Selected);
    NEW_NERVE(YesNoControllerNrvNotSelected, YesNoController, NotSelected);
}  // namespace NrvYesNoController

YesNoController::YesNoController(LayoutActor* pHost)
    : NerveExecutor("はい／いいえ選択制御"), mHost(pHost), _C(false), mButtonYesPaneCtrl(new ButtonPaneController(mHost, "Right", "BoxRight", 0, true)),
      mButtonNoPaneCtrl(new ButtonPaneController(mHost, "Left", "BoxLeft", 0, true)), mCursorSE(nullptr), mYesSE(nullptr), mNoSE(nullptr) {
    mButtonYesPaneCtrl->_22 = false;
    mButtonNoPaneCtrl->_22 = false;
    initNerve(&NrvYesNoController::YesNoControllerNrvSelecting::sInstance);
}

YesNoController::~YesNoController() {
    delete mButtonYesPaneCtrl;
    delete mButtonNoPaneCtrl;
}

void YesNoController::appear() {
    _C = true;
    setNerve(&NrvYesNoController::YesNoControllerNrvSelecting::sInstance);
}

void YesNoController::kill() {
    _C = false;
    setNerve(&NrvYesNoController::YesNoControllerNrvNotSelected::sInstance);
}

void YesNoController::update() {
    if (!_C) {
        return;
    }

    updateNerve();
    mButtonYesPaneCtrl->update();
    mButtonNoPaneCtrl->update();
}

bool YesNoController::isSelected() const {
    return isNerve(&NrvYesNoController::YesNoControllerNrvSelected::sInstance);
}

bool YesNoController::isSelectedYes() const {
    return mButtonYesPaneCtrl->mIsSelected;
}

bool YesNoController::isDisappearStart() const {
    return isNerve(&NrvYesNoController::YesNoControllerNrvDisappear::sInstance) && MR::isFirstStep(this);
}

void YesNoController::setSE(const char* pCursorSE, const char* pYesSE, const char* pNoSE) {
    mCursorSE = pCursorSE;
    mYesSE = pYesSE;
    mNoSE = pNoSE;
}

bool YesNoController::trySelect() {
    return mButtonYesPaneCtrl->trySelect() || mButtonNoPaneCtrl->trySelect();
}

void YesNoController::exeSelecting() {
    if (MR::isFirstStep(this)) {
        mButtonYesPaneCtrl->appear();
        mButtonNoPaneCtrl->appear();
    }

    if (mButtonYesPaneCtrl->isPointingTrigger() || mButtonNoPaneCtrl->isPointingTrigger()) {
        MR::startSystemSE(mCursorSE != nullptr ? mCursorSE : sDefaultCursorSE, -1, -1);
    }

    if (trySelect()) {
        setNerve(&NrvYesNoController::YesNoControllerNrvDecided::sInstance);
    }
}

void YesNoController::exeDecided() {
    const auto is_selected_yes = mButtonYesPaneCtrl->mIsSelected;
    if (MR::isFirstStep(this)) {
        MR::startCSSound("CS_CLICK_CLOSE", 0, 0);
        MR::startSystemSE(is_selected_yes ? (mYesSE != nullptr ? mYesSE : sDefaultYesSE) : (mNoSE != nullptr ? mNoSE : sDefaultNoSE), -1, -1);
    }

    if ((is_selected_yes && mButtonYesPaneCtrl->isDecidedWait()) || (!is_selected_yes && mButtonNoPaneCtrl->isDecidedWait())) {
        setNerve(&NrvYesNoController::YesNoControllerNrvDisappear::sInstance);
    }
}

void YesNoController::exeDisappear() {
    if (MR::isFirstStep(this)) {
        mButtonYesPaneCtrl->disappear();
        mButtonNoPaneCtrl->disappear();
    }

    if (mButtonYesPaneCtrl->isHidden() && mButtonNoPaneCtrl->isHidden()) {
        const Nerve* next_nerve = (mButtonYesPaneCtrl->mIsSelected || mButtonNoPaneCtrl->mIsSelected) ?
                                      static_cast< const Nerve* >(&NrvYesNoController::YesNoControllerNrvSelected::sInstance) :
                                      static_cast< const Nerve* >(&NrvYesNoController::YesNoControllerNrvNotSelected::sInstance);
        setNerve(next_nerve);
    }
}

void YesNoController::exeSelected() {
    _C = false;
}

void YesNoController::exeNotSelected() {
    _C = false;
}
