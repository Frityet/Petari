#include "Game/Map/FileSelector.hpp"
#include "Game/LiveActor/Nerve.hpp"
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
