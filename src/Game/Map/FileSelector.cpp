#include "Game/Map/FileSelector.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Map/FileSelectCameraController.hpp"
#include "Game/Map/FileSelectSky.hpp"
#include "Game/NPC/MiiFacePartsHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/BackButton.hpp"
#include "Game/Screen/BrosButton.hpp"
#include "Game/Screen/FileSelectButton.hpp"
#include "Game/Screen/FileSelectInfo.hpp"
#include "Game/Screen/InformationMessage.hpp"
#include "Game/Screen/Manual2P.hpp"
#include "Game/Screen/MiiConfirmIcon.hpp"
#include "Game/Screen/MiiSelect.hpp"
#include "Game/Screen/MiiSelectIcon.hpp"
#include "Game/Screen/SysInfoWindow.hpp"
#include "Game/Screen/TitleSequenceProduct.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "JSystem/JKernel/JKRMemArchive.hpp"

namespace NrvFileSelector {
    class FileSelectorNrvWaitBind : public Nerve {
    public:
        virtual void execute(Spine*) const;
        static FileSelectorNrvWaitBind sInstance;
    };

    FileSelectorNrvWaitBind FileSelectorNrvWaitBind::sInstance;
    NEW_NERVE(FileSelectorNrvTitle, FileSelector, Title);
    NEW_NERVE(FileSelectorNrvTitleEnd, FileSelector, TitleEnd);
    NEW_NERVE(FileSelectorNrvRFLError, FileSelector, RFLError);
    NEW_NERVE(FileSelectorNrvRFLWait, FileSelector, RFLWait);
    NEW_NERVE(FileSelectorNrvRFLWaitEnd, FileSelector, RFLWaitEnd);
    NEW_NERVE(FileSelectorNrvFileSelectStart, FileSelector, FileSelectStart);
    NEW_NERVE(FileSelectorNrvFileSelect, FileSelector, FileSelect);
    NEW_NERVE(FileSelectorNrvFileConfirmStart, FileSelector, FileConfirmStart);
    NEW_NERVE(FileSelectorNrvFileConfirmMiiDeleteWarningStart, FileSelector, FileConfirmMiiDeleteWarningStart);
    NEW_NERVE(FileSelectorNrvFileConfirmMiiDeleteWarning, FileSelector, FileConfirmMiiDeleteWarning);
    NEW_NERVE(FileSelectorNrvFileConfirmMiiDeleteSave, FileSelector, FileConfirmMiiDeleteSave);
    NEW_NERVE(FileSelectorNrvFileConfirm, FileSelector, FileConfirm);
    NEW_NERVE(FileSelectorNrvDemoStartWait, FileSelector, DemoStartWait);
    NEW_NERVE(FileSelectorNrvDemo, FileSelector, Demo);
    NEW_NERVE(FileSelectorNrvCreateConfirmStart, FileSelector, CreateConfirmStart);
    NEW_NERVE(FileSelectorNrvCreateConfirm, FileSelector, CreateConfirm);
    NEW_NERVE(FileSelectorNrvCreate, FileSelector, Create);
    NEW_NERVE(FileSelectorNrvCopyWait, FileSelector, CopyWait);
    NEW_NERVE(FileSelectorNrvCopySelect, FileSelector, CopySelect);
    NEW_NERVE(FileSelectorNrvCopyConfirmStart, FileSelector, CopyConfirmStart);
    NEW_NERVE(FileSelectorNrvCopyConfirm, FileSelector, CopyConfirm);
    NEW_NERVE(FileSelectorNrvCopySave, FileSelector, CopySave);
    NEW_NERVE(FileSelectorNrvCopySaveMii, FileSelector, CopySaveMii);
    NEW_NERVE(FileSelectorNrvCopyDemo, FileSelector, CopyDemo);
    NEW_NERVE(FileSelectorNrvCopyRejectStart, FileSelector, CopyRejectStart);
    NEW_NERVE(FileSelectorNrvCopyReject, FileSelector, CopyReject);
    NEW_NERVE(FileSelectorNrvMiiWait, FileSelector, MiiWait);
    NEW_NERVE(FileSelectorNrvMiiTip, FileSelector, MiiTip);
    NEW_NERVE(FileSelectorNrvMiiSelectStartFirst, FileSelector, MiiSelectStart);
    NEW_NERVE(FileSelectorNrvMiiSelectStart, FileSelector, MiiSelectStart);
    NEW_NERVE(FileSelectorNrvMiiSelect, FileSelector, MiiSelect);
    NEW_NERVE(FileSelectorNrvMiiCancel, FileSelector, MiiCancel);
    NEW_NERVE(FileSelectorNrvMiiConfirmWait, FileSelector, MiiConfirmWait);
    NEW_NERVE(FileSelectorNrvMiiConfirm, FileSelector, MiiConfirm);
    NEW_NERVE(FileSelectorNrvMiiCreateWait, FileSelector, MiiCreateWait);
    NEW_NERVE(FileSelectorNrvMiiCreateDemo, FileSelector, MiiCreateDemo);
    NEW_NERVE(FileSelectorNrvMiiCaution, FileSelector, MiiCaution);
    NEW_NERVE(FileSelectorNrvMiiInfoStart, FileSelector, MiiInfoStart);
    NEW_NERVE(FileSelectorNrvMiiInfo, FileSelector, MiiInfo);
    NEW_NERVE(FileSelectorNrvDeleteConfirmStart, FileSelector, DeleteConfirmStart);
    NEW_NERVE(FileSelectorNrvDeleteConfirm, FileSelector, DeleteConfirm);
    NEW_NERVE(FileSelectorNrvDelete, FileSelector, Delete);
    NEW_NERVE(FileSelectorNrvDeleteDemo, FileSelector, DeleteDemo);
    NEW_NERVE(FileSelectorNrvFileBroken, FileSelector, FileBroken);
    NEW_NERVE(FileSelectorNrvManualStart, FileSelector, ManualStart);
    NEW_NERVE(FileSelectorNrvManual, FileSelector, Manual);
};  // namespace NrvFileSelector

void NrvFileSelector::FileSelectorNrvWaitBind::execute(Spine* pSpine) const {
    FileSelector* actor = reinterpret_cast< FileSelector* >(pSpine->mExecutor);
    actor->mPosition = *MR::getPlayerPos();
}

bool FileSelector::receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (MR::isMsgAutoRushBegin(msg) && isNerve(&NrvFileSelector::FileSelectorNrvWaitBind::sInstance)) {
        MR::hidePlayer();
        setNerve(&NrvFileSelector::FileSelectorNrvTitle::sInstance);
        return true;
    }

    if (MR::isMsgUpdateBaseMtx(msg)) {
        TPos3f baseMtx;
        baseMtx.identity();
        MR::setPlayerBaseMtx(baseMtx);
        return true;
    }

    return false;
}

void FileSelector::callbackStart() {
    setNerve(&NrvFileSelector::FileSelectorNrvDemoStartWait::sInstance);
}

void FileSelector::callbackCopy() {
    setNerve(&NrvFileSelector::FileSelectorNrvCopyWait::sInstance);
}

void FileSelector::callbackMii() {
    disappaerAllLayout();
    invalidateSelectAll();
    setNerve(&NrvFileSelector::FileSelectorNrvMiiWait::sInstance);
}

void FileSelector::callbackDelete() {
    setNerve(&NrvFileSelector::FileSelectorNrvDeleteConfirmStart::sInstance);
}

void FileSelector::callbackManual() {
    MR::startSystemSE("SE_SY_FILE_SEL_TIPS_OPEN", -1, -1);
    setNerve(&NrvFileSelector::FileSelectorNrvManualStart::sInstance);
}

void FileSelector::createBackButton() {
    mBackButton = new BackButton("戻るボタン", false);
    mBackButton->initWithoutIter();
    MR::connectToScene(mBackButton, 0xE, 0xD, -1, 0x3D);
}

void FileSelector::createBrosButton() {
    mBrosButton = new BrosButton("ルイージ切り替えボタン");
    mBrosButton->initWithoutIter();
}

void FileSelector::createInfoMessage() {
    mInfoMessage = new InformationMessage();
    mInfoMessage->initWithoutIter();
}

void FileSelector::createSysInfoWindow() {
    mSysInfoWindow = MR::createSysInfoWindow();
    MR::connectToSceneLayout(mSysInfoWindow);
    mSysInfoWindowMini = MR::createSysInfoWindowMiniExecuteWithChildren();
    MR::connectToSceneLayout(mSysInfoWindowMini);
}

void FileSelector::createFileInfo() {
    mSelectInfo = new FileSelectInfo(0xB, "ファイル情報");
    mSelectInfo->initWithoutIter();
}

void FileSelector::createTitle() {
    mTitleSeq = new TitleSequenceProduct();
    mTitleSeq->kill();
}

void FileSelector::createSky() {
    mSky = new FileSelectSky("ファイル選択空");
    mSky->initWithoutIter();
    mSky->appear();
}

void FileSelector::createMiiSelect() {
    mMiiSelect = new MiiSelect("MiiSelect");
    mMiiSelect->initWithoutIter();
}

void FileSelector::createMiiConfirmIcon() {
    mMiiConfirmIcon = new MiiConfirmIcon("Mii確認用アイコン");
    mMiiConfirmIcon->initWithoutIter();
    MR::connectToScene(mMiiConfirmIcon, 0xE, 0xD, -1, 0x3D);
}

void FileSelector::createMiiFont() {
    JKRMemArchive* archive = MR::receiveArchive("/LayoutData/MiiFont.arc");
    mFont = new nw4r::ut::ResFont();
    mFont->SetResource(archive->getResource("/MiiFont26.brfnt"));
    mFont->SetAlternateChar('?');
    MR::setTextBoxFontRecursive(mSelectInfo, "FileName", mFont);
    MR::setTextBoxFontRecursive(mMiiSelect, "TxtName", mFont);
    MR::setTextBoxFontRecursive(mMiiConfirmIcon, "MiiName", mFont);
}

void FileSelector::createManual() {
    mManual2P = new Manual2P("２Ｐマニュアル");
    mManual2P->initWithoutIter();
}

void FileSelector::exeTitle() {
    if (MR::isFirstStep(this)) {
        mTitleSeq->appear();
        MR::deactivateDefaultGameLayout();
        MR::startStarPointerModeTitle(this);
        MR::resetCameraMan();
    }

    mTitleSeq->updateNerve();

    if (!mTitleSeq->isActive()) {
        if (MR::getSceneObj< MiiFacePartsHolder >(SceneObj_MiiFacePartsHolder)->isInitEnd()) {
            if (MR::getSceneObj< MiiFacePartsHolder >(SceneObj_MiiFacePartsHolder)->isError()) {
                setNerve(&NrvFileSelector::FileSelectorNrvRFLError::sInstance);
            } else {
                setNerve(&NrvFileSelector::FileSelectorNrvTitleEnd::sInstance);
            }
        } else {
            setNerve(&NrvFileSelector::FileSelectorNrvRFLWait::sInstance);
        }
    }
}

void FileSelector::exeTitleEnd() {
    if (MR::isFirstStep(this)) {
        mCameraController->goToFarPoint();
        calcBasePos(0.0f);
        appearAllItems();
        initAllItems();
        mMiiSelect->collectValidMiiIndex();
        invalidateSelectAll();
        MR::startStarPointerModeFileSelect(this);
        MR::startStageBGM("MBGM_FILE_SELECT", false);
    }

    if (mCameraController->isAtFarPoint()) {
        validateRotateAllItems();
        setNerve(&NrvFileSelector::FileSelectorNrvFileSelect::sInstance);
    }
}
