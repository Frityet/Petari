#include "Game/Map/FileSelector.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Map/FIleSelectItem.hpp"
#include "Game/Map/FileSelectCameraController.hpp"
#include "Game/Map/FileSelectEffect.hpp"
#include "Game/Map/FileSelectFunc.hpp"
#include "Game/Map/FileSelectItemDelegator.hpp"
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
#include "Game/System/GameSequenceFunction.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "JSystem/JKernel/JKRMemArchive.hpp"
#include <RVLFaceLib.h>

namespace {
    static const f32 sItemThetaOffset[] = {10.0f, -10.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    static const s32 sIndexOrder[] = {1, 2, 4, 6, 5, 3};
    static s32 sBgmFarState = 5;
    static u32 sBgmFarStateChangeFrames = 60;
    static s32 sBgmNearState = 6;
    static u32 sBgmNearStateChangeFrames = 60;
    static const char* cLuigiNameMessageID = "System_FileSelect_Icon001";
    static const char* cMarioNameMessageID = "System_FileSelect_Icon000";

    s32 getItemArrayIndex(s32 fileNo) NO_INLINE;

    s32 getItemArrayIndex(s32 fileNo) {
        for (s32 i = 0; i < 6; i++) {
            if (fileNo == sIndexOrder[i]) {
                return i;
            }
        }

        return -1;
    }
};  // namespace

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

FileSelector::FileSelector(const char* pName)
    : LiveActor(pName),
      mCameraController(nullptr),
      mSky(nullptr),
      mItems(nullptr),
      mSelectButton(nullptr),
      mBackButton(nullptr),
      mBrosButton(nullptr),
      mInfoMessage(nullptr),
      mSysInfoWindow(nullptr),
      _B4(nullptr),
      _B8(nullptr),
      _BC(nullptr),
      _C0(nullptr),
      mFiles(nullptr),
      _CC(nullptr),
      mTitleSeq(nullptr),
      mMiiSelect(nullptr),
      _DC(new RFLCreateID),
      mManual2P(nullptr),
      _E4(0),
      _E8(0),
      mSelectEffects(nullptr) {}

void FileSelector::init(const JMapInfoIter& rIter) {
    MR::connectToScene(this, 0x21, -1, -1, -1);
    initHitSensor(1);
    MR::addHitSensorPriorBinder(this, "body", 8, 500.0f, TVec3f(0.0f, 0.0f, 0.0f));
    initUserFileArray();
    MR::invalidateClipping(this);
    createCameraController();
    createSky();
    createFileItems();
    createOperationButton();
    createBackButton();
    createBrosButton();
    createInfoMessage();
    createSysInfoWindow();
    createFileInfo();
    createTitle();
    createMiiSelect();
    createMiiConfirmIcon();
    createManual();
    createSelectEffect();
    initNerve(&NrvFileSelector::FileSelectorNrvWaitBind::sInstance);
    MR::tryRegisterDemoCast(this, rIter);
    createMiiFont();
    appear();
}

void FileSelector::appear() {
    LiveActor::appear();
    mCameraController->appear();
    setNerve(&NrvFileSelector::FileSelectorNrvWaitBind::sInstance);
}

void FileSelector::kill() {
    mBackButton->kill();
    LiveActor::kill();
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
    disappearAllLayout();
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

void FileSelector::notifyItem(FileSelectItem* pItem, s32 action) {
    switch (action) {
    case 0:
        onPoint(pItem);
        break;
    case 1:
        onSelect(pItem);
        break;
    case 2:
        if (isNerve(&NrvFileSelector::FileSelectorNrvCopySelect::sInstance)) {
            onPoint(pItem);
        }
        break;
    }
}

void FileSelector::control() {
    _C0 = nullptr;

    for (s32 i = 0; i < mItems->getObjectCount(); i++) {
        LiveActor* pItem = mItems->getActor(i);
        pItem->mPosition.x = pItem->mPosition.x * 0.95f + _98[i].x * 0.05f;
        pItem->mPosition.y = pItem->mPosition.y * 0.95f + _98[i].y * 0.05f;
        pItem->mPosition.z = pItem->mPosition.z * 0.95f + _98[i].z * 0.05f;
    }

    for (s32 i = 0; i < mItems->getObjectCount(); i++) {
        LiveActor* pItem = mItems->getActor(i);
        mSelectEffects[i].mPosition.set(TVec3f(pItem->mPosition.x, pItem->mPosition.y + 1000.0f, pItem->mPosition.z));
    }

    updateBgm();
}

void FileSelector::initUserFileArray() {
    mFiles = new UserFile[6];
    _CC = new u8[6];
    initUserFile();
}

void FileSelector::createCameraController() {
    mCameraController = new FileSelectCameraController("ファイル選択カメラ");
    mCameraController->initWithoutIter();
}

void FileSelector::createFileItems() {
    mItems = new DeriveActorGroup< FileSelectItem >("ルイージ切り替えボタン", 6);
    _98 = new TVec3f[6];
    calcBasePos(0.0f);
    MR::createSceneObj(SceneObj_MiiFacePartsHolder);

    FileSelectItemDelegator< FileSelector >* pDelegator = new FileSelectItemDelegator< FileSelector >(this, &FileSelector::notifyItem);

    for (s32 i = 0; i < 6; i++) {
        FileSelectIconID iconID;
        FileSelectItem* pItem = new FileSelectItem(sIndexOrder[i], true, iconID, "ファイル情報");
        pItem->initWithoutIter();
        pItem->setSelectDelegator(pDelegator);
        pItem->mPosition.set(_98[i]);
        mItems->registerActor(pItem);
    }
}

void FileSelector::createOperationButton() {
    mSelectButton = new FileSelectButton("FileSelectButton");
    mSelectButton->initWithoutIter();

    MR::FunctorV0M< FileSelector*, void (FileSelector::*)() > startFunctor = MR::Functor(this, &FileSelector::callbackStart);
    MR::FunctorV0M< FileSelector*, void (FileSelector::*)() > copyFunctor = MR::Functor(this, &FileSelector::callbackCopy);
    MR::FunctorV0M< FileSelector*, void (FileSelector::*)() > miiFunctor = MR::Functor(this, &FileSelector::callbackMii);
    MR::FunctorV0M< FileSelector*, void (FileSelector::*)() > deleteFunctor = MR::Functor(this, &FileSelector::callbackDelete);
    MR::FunctorV0M< FileSelector*, void (FileSelector::*)() > manualFunctor = MR::Functor(this, &FileSelector::callbackManual);

    mSelectButton->setCallbackFunctor(startFunctor, copyFunctor, miiFunctor, deleteFunctor, manualFunctor);
}

void FileSelector::appearAllItems() {
    for (s32 i = 0; i < mItems->getObjectCount(); i++) {
        mItems->getActor(i)->appear();
    }
}

void FileSelector::disappearAllLayout() {
    if (!MR::isDead(mSelectButton)) {
        mSelectButton->disappear();

        if (!MR::isDead(mSelectInfo)) {
            mSelectInfo->slideBack();
        }
    }

    if (!MR::isDead(mSelectInfo)) {
        mSelectInfo->disappear();
    }

    if (!mBackButton->isHidden() && !mBackButton->_24) {
        mBackButton->disappear();
    }

    if (!MR::isDead(mBrosButton)) {
        mBrosButton->disappear();
    }
}

bool FileSelector::isHiddenAllLayout() const {
    return MR::isDead(mSelectButton) && MR::isDead(mSelectInfo) && mBackButton->isHidden() && MR::isDead(mBrosButton);
}

void FileSelector::updateFileInfo() {
    if (_C0) {
        if (_BC != _C0) {
            if (_BC) {
                clearPointing();
                mSelectInfo->disappear();
            }

            if (_C0->isExist()) {
                setFileInfo(_C0->_140);
                mSelectInfo->appear();
                mSelectInfo->forceChange();
            }

            _BC = _C0;
            _C0->onPointing();
        }
    } else if (_BC) {
        clearPointing();
        mSelectInfo->disappear();
    }
}

void FileSelector::appearAllIndex() {
    for (s32 i = 0; i < mItems->getObjectCount(); i++) {
        reinterpret_cast< FileSelectItem* >(mItems->getActor(i))->appearIndex();
    }
}

void FileSelector::disappearAllIndex() {
    for (s32 i = 0; i < mItems->getObjectCount(); i++) {
        reinterpret_cast< FileSelectItem* >(mItems->getActor(i))->disappearIndex();
    }
}

void FileSelector::invalidateSelectAll() {
    for (s32 i = 0; i < mItems->getObjectCount(); i++) {
        reinterpret_cast< FileSelectItem* >(mItems->getActor(i))->invalidateSelect();
    }
}

void FileSelector::validateSelectAll() {
    for (s32 i = 0; i < mItems->getObjectCount(); i++) {
        reinterpret_cast< FileSelectItem* >(mItems->getActor(i))->validateSelect();
    }
}

void FileSelector::initUserFile() {
    for (s32 i = 0; i < 6; i++) {
        GameSequenceFunction::restoreUserFile(&mFiles[i], i + 1);
    }

    checkAllComplete();
}

void FileSelector::restoreUserFile() {
    for (s32 i = 0; i < 6; i++) {
        GameSequenceFunction::restoreUserFile(&mFiles[i], i + 1, mFiles[i].mIsPlayerMario);
    }

    checkAllComplete();
}

void FileSelector::checkAllComplete() {
    for (s32 i = 0; i < 6; i++) {
        bool isMario = mFiles[i].mIsPlayerMario;
        _CC[i] = 0;

        if (!isMario) {
            GameSequenceFunction::restoreUserFile(&mFiles[i], i + 1, true);
        }

        if (mFiles[i].isPowerStarGetFinalChallengeGalaxy()) {
            GameSequenceFunction::restoreUserFile(&mFiles[i], i + 1, false);

            if (mFiles[i].isPowerStarGetFinalChallengeGalaxy()) {
                _CC[i] = 1;
            }
        }

        if (isMario != mFiles[i].mIsPlayerMario) {
            GameSequenceFunction::restoreUserFile(&mFiles[i], i + 1, isMario);
        }
    }
}

void FileSelector::onPoint(FileSelectItem* pItem) {
    if (mBackButton->isHidden() || !mBackButton->isPointing()) {
        if (!_C0 || _C0->_140 > pItem->_140) {
            _C0 = pItem;
        }
    }
}

void FileSelector::onSelect(FileSelectItem* pItem) {
    if (isNerve(&NrvFileSelector::FileSelectorNrvCopySelect::sInstance)) {
        setNerve(&NrvFileSelector::FileSelectorNrvCopyConfirmStart::sInstance);
    } else if (isNerve(&NrvFileSelector::FileSelectorNrvFileSelect::sInstance)) {
        if (pItem->isNew()) {
            setNerve(&NrvFileSelector::FileSelectorNrvCreateConfirmStart::sInstance);
        } else {
            setNerve(&NrvFileSelector::FileSelectorNrvFileConfirmStart::sInstance);
        }
    } else {
        return;
    }

    _B4 = pItem;
    MR::startSystemSE("SE_SY_GALAXY_SELECTED", -1, -1);
    playSelectedME();
}

void FileSelector::clearPointing() {
    if (_BC) {
        _BC->offPointing();
        _BC = nullptr;
    }
}

void FileSelector::setFileInfo(s32 fileNo) {
    FileSelectIconID iconID;
    FileSelectItem* pItem = static_cast< FileSelectItem* >(mItems->getActor(getItemArrayIndex(fileNo)));
    u16 name[RFL_NAME_LEN + 1];

    if (pItem->_146) {
        FileSelectFunc::copyMiiName(name, iconID);
    } else {
        getIconId(&iconID, fileNo);
        FileSelectFunc::copyMiiName(name, iconID);
    }

    OSCalendarTime calendar;
    OSTicksToCalendarTime(mFiles[fileNo - 1].getLastModified(), &calendar);

    wchar_t dateMessage[32];
    wchar_t timeMessage[32];
    MR::makeDateString(dateMessage, 32, calendar.year, calendar.mon + 1, calendar.mday);
    MR::makeTimeString(timeMessage, 32, calendar.hour, calendar.min);

    bool isViewCompleteEnding = mFiles[fileNo - 1].isViewCompleteEnding();
    bool isViewNormalEnding = mFiles[fileNo - 1].isViewNormalEnding();
    s32 starPieceNum = mFiles[fileNo - 1].getStarPieceNum();
    s32 powerStarNum = mFiles[fileNo - 1].getPowerStarNum();
    s32 missCount = getMissCount(fileNo);

    mSelectInfo->setInfo(name, fileNo, powerStarNum, starPieceNum, !isUserFileLuigi(fileNo), isViewNormalEnding, isViewCompleteEnding, dateMessage, timeMessage, missCount);
}

void FileSelector::goToNearPoint() {
    calcBasePos(-16000.0f);
    mCameraController->goToNearPoint(_98[getItemArrayIndex(_B4->_140)]);
}

void FileSelector::calcBasePos(f32 y) {
    const f32 rotateSin = MR::sin(0.34906587f);
    const f32 rotateCos = MR::cos(0.34906587f);

    for (s32 i = 0; i < 6; i++) {
        if (_B4 && _B4 == static_cast< FileSelectItem* >(mItems->getActor(i))) {
            continue;
        }

        f32 theta = -static_cast< f32 >(i + 4) * 1.0471976f - sItemThetaOffset[i] * 3.1415927f / 180.0f;
        f32 x = MR::cos(theta) * 5000.0f;
        f32 z = MR::sin(theta) * 5000.0f;

        _98[i].x = x;
        _98[i].y = y - z * rotateSin;
        _98[i].z = z * rotateCos;
    }
}

void FileSelector::initAllItems() {
    for (s32 i = 0; i < 6; i++) {
        FileSelectItem* pItem = static_cast< FileSelectItem* >(mItems->getActor(i));
        s32 fileNo = pItem->_140;

        if (mFiles[fileNo - 1].isCreated()) {
            FileSelectIconID iconID;
            u32 iconId;

            if (mFiles[fileNo - 1].getIconId(&iconId)) {
                iconID.setFellowID(getUserFileFellowID(fileNo));
            } else {
                RFLCreateID createID;

                if (mFiles[fileNo - 1].getMiiId(&createID)) {
                    if (MR::getSceneObj< MiiFacePartsHolder >(SceneObj_MiiFacePartsHolder)->isError()) {
                        pItem->_146 = 1;
                    } else if (isUserFileMiiIdValid(fileNo)) {
                        iconID.setMiiIndex(getUserFileMiiIndex(fileNo));
                    } else {
                        pItem->_146 = 1;
                    }
                }
            }

            pItem->forceChange(iconID, _CC[fileNo - 1]);
        }
    }
}

void FileSelector::validateRotateAllItems() {
    for (s32 i = 0; i < mItems->getObjectCount(); i++) {
        reinterpret_cast< FileSelectItem* >(mItems->getActor(i))->validateRotate();
    }
}

FileSelectIconID::EFellowID FileSelector::getUserFileFellowID(s32 fileNo) const {
    u32 iconId = 0;

    if (mFiles[fileNo - 1].getIconId(&iconId) && iconId <= 5) {
        return static_cast< FileSelectIconID::EFellowID >(iconId - 1);
    }

    return FileSelectIconID::Mario;
}

bool FileSelector::isUserFileMiiIdValid(s32 fileNo) const {
    RFLCreateID createID;
    u16 index;

    if (mFiles[fileNo - 1].getMiiId(&createID) && RFLSearchOfficialData(&createID, &index) == TRUE) {
        return RFLIsAvailableOfficialData(index) == TRUE;
    }

    return false;
}

s32 FileSelector::getUserFileMiiIndex(s32 fileNo) const {
    RFLCreateID createID;
    u16 index;

    if (mFiles[fileNo - 1].getMiiId(&createID) && RFLSearchOfficialData(&createID, &index)) {
        return index;
    }

    return 0;
}

bool FileSelector::isUserFileCorrupted(s32 fileNo) const {
    const UserFile* pFile = &mFiles[fileNo - 1];
    return pFile->mIsGameDataCorrupted || pFile->mIsConfigDataCorrupted;
}

bool FileSelector::isUserFileAppearLuigi(s32 fileNo) const {
    const UserFile* pFile = &mFiles[fileNo - 1];

    if (pFile->mIsPlayerMario) {
        return pFile->isViewCompleteEnding();
    }

    return true;
}

bool FileSelector::isUserFileLuigi(s32 fileNo) const {
    return !mFiles[fileNo - 1].mIsPlayerMario;
}

void FileSelector::setUserFileMario(s32 fileNo, bool isMario) {
    mFiles[fileNo - 1].mIsPlayerMario = isMario;
}

void FileSelector::storeSetMiiIdUserFile(s32 fileNo, const FileSelectIconID& rIconID) {
    if (rIconID.isFellow()) {
        u32 iconID = rIconID.getFellowID() + 1;
        GameSequenceFunction::storeMiiOrIconIdUserFileSequence(fileNo, nullptr, &iconID);
    } else {
        getMiiId(_DC, rIconID);
        GameSequenceFunction::storeMiiOrIconIdUserFileSequence(fileNo, _DC, nullptr);
    }

    GameSequenceFunction::startSaveAllUserFileSequence();
}

void FileSelector::getMiiId(RFLCreateID* pCreateID, const FileSelectIconID& rIconID) const {
    RFLAdditionalInfo info;

    if (RFLGetAdditionalInfo(&info, RFLDataSource_Official, nullptr, rIconID.getMiiIndex()) == RFLErrcode_Success) {
        *pCreateID = info.createID;
    }
}

void FileSelector::getIconId(FileSelectIconID* pIconId, s32 fileNo) const {
    u32 iconId = 0;

    if (mFiles[fileNo - 1].getIconId(&iconId)) {
        pIconId->setFellowID(getUserFileFellowID(fileNo));
    } else {
        RFLCreateID createID;

        if (mFiles[fileNo - 1].getMiiId(&createID) && isUserFileMiiIdValid(fileNo)) {
            pIconId->setMiiIndex(getUserFileMiiIndex(fileNo));
        } else {
            pIconId->setFellowID(FileSelectIconID::Mario);
        }
    }
}

s32 FileSelector::getMissCount(s32 fileNo) const {
    if (_CC[fileNo - 1]) {
        return mFiles[fileNo - 1].getPlayerMissNum();
    }

    return -1;
}

bool FileSelector::checkSelectedBackButton() {
    if (mBackButton->isHidden()) {
        return false;
    }

    if (mBackButton->_24) {
        return true;
    }

    if (MR::testSystemTriggerB()) {
        MR::startSystemSE("SE_SY_GALAXY_DECIDE_CANCEL", -1, -1);
        mBackButton->disappear();
        return true;
    }

    return false;
}

void FileSelector::playSelectedME() {
    switch (MR::getRandom(0l, 4l)) {
    case 0:
        MR::startSystemME("ME_ASTRO_DOME_SELECT1");
        break;
    case 1:
        MR::startSystemME("ME_ASTRO_DOME_SELECT2");
        break;
    case 2:
        MR::startSystemME("ME_ASTRO_DOME_SELECT3");
        break;
    case 3:
        MR::startSystemME("ME_ASTRO_DOME_SELECT4");
        break;
    }
}

void FileSelector::updateBgm() {
    if (!mCameraController) {
        return;
    }

    s32 state = 0;
    u32 frames = 0;

    if (mCameraController->isToOrAtFarPoint()) {
        state = sBgmFarState;
        frames = sBgmFarStateChangeFrames;
    } else if (mCameraController->isToOrAtNearPoint()) {
        state = sBgmNearState;
        frames = sBgmNearStateChangeFrames;
    }

    if (_E8 != state) {
        MR::setStageBGMState(state, frames);
        _E8 = state;
    }
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

void FileSelector::createSelectEffect() {
    mSelectEffects = new FileSelectEffect[6];

    for (s32 i = 0; i < 6; i++) {
        mSelectEffects[i].initWithoutIter();
        mSelectEffects[i].mPosition.set(_98[i]);
        mSelectEffects[i].mScale.x = 0.4f;
        mSelectEffects[i].mScale.y = 0.4f;
        mSelectEffects[i].mScale.z = 0.4f;
    }
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

void FileSelector::exeRFLError() {
    if (MR::isFirstStep(this)) {
        mSysInfoWindow->appear("RFL_02", SysInfoWindow::Type_Key, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
    }

    if (MR::isDead(mSysInfoWindow)) {
        setNerve(&NrvFileSelector::FileSelectorNrvTitleEnd::sInstance);
    }
}

void FileSelector::exeRFLWait() {
    if (MR::isFirstStep(this)) {
        mSysInfoWindowMini->appear("RFL_01", SysInfoWindow::Type_Blocking, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
    }

    if (MR::getSceneObj< MiiFacePartsHolder >(SceneObj_MiiFacePartsHolder)->isInitEnd() && mSysInfoWindowMini->isWait()) {
        setNerve(&NrvFileSelector::FileSelectorNrvRFLWaitEnd::sInstance);
    }
}

void FileSelector::exeRFLWaitEnd() {
    if (MR::isFirstStep(this)) {
        mSysInfoWindowMini->disappear();
    }

    if (MR::isDead(mSysInfoWindowMini)) {
        setNerve(&NrvFileSelector::FileSelectorNrvTitleEnd::sInstance);
    }
}

void FileSelector::exeFileSelectStart() {
    if (MR::isFirstStep(this)) {
        mCameraController->goToFarPoint();
        calcBasePos(0.0f);
    }

    if (mCameraController->isAtFarPoint()) {
        setNerve(&NrvFileSelector::FileSelectorNrvFileSelect::sInstance);
    }
}

void FileSelector::exeFileSelect() {
    if (MR::isFirstStep(this)) {
        validateSelectAll();
        appearAllIndex();
        MR::activeStarPointerGuidance();
    } else {
        updateFileInfo();
    }

    MR::requestFileSelectGuidance();
}

void FileSelector::exeFileConfirmStart() {
    if (MR::isFirstStep(this)) {
        goToNearPoint();
        invalidateSelectAll();
        disappearAllIndex();
        _B4->turnToFront(40);
    }

    if (mCameraController->isAtNearPoint()) {
        if (isUserFileCorrupted(_B4->_140)) {
            setNerve(&NrvFileSelector::FileSelectorNrvFileBroken::sInstance);
        } else if (_B4->_146) {
            setNerve(&NrvFileSelector::FileSelectorNrvFileConfirmMiiDeleteWarningStart::sInstance);
        } else {
            setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
        }
    }
}

void FileSelector::exeFileConfirmMiiDeleteWarningStart() {
    if (MR::isFirstStep(this)) {
        disappearAllLayout();
    }

    if (isHiddenAllLayout()) {
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirmMiiDeleteWarning::sInstance);
    }
}

void FileSelector::exeFileConfirmMiiDeleteWarning() {
    if (MR::isFirstStep(this)) {
        mSysInfoWindowMini->appear("System_FileSelect009", SysInfoWindow::Type_Key, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
    }

    if (MR::isDead(mSysInfoWindowMini)) {
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirmMiiDeleteSave::sInstance);
    }
}

void FileSelector::exeFileConfirmMiiDeleteSave() {
    if (MR::isFirstStep(this)) {
        u32 iconID = 1;
        GameSequenceFunction::startSetMiiOrIconIdUserFileSequence(_B4->_140, nullptr, &iconID);
    }

    if (!GameSequenceFunction::isActiveSaveDataHandleSequence()) {
        _B4->_146 = 0;
        restoreUserFile();
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
    }
}

void FileSelector::exeFileConfirm() {
    if (MR::isFirstStep(this)) {
        mSelectButton->appear();
        mBackButton->appear();

        s32 fileNo = _B4->_140;

        if (isUserFileAppearLuigi(fileNo)) {
            mBrosButton->appear(!isUserFileLuigi(fileNo));
        }

        if (MR::isDead(mSelectInfo)) {
            setFileInfo(fileNo);
            mSelectInfo->appear();
            mSelectInfo->forceChange();
        }

        mSelectInfo->slide();
    }

    if (!MR::isDead(mBrosButton) && mBrosButton->isSelected()) {
        s32 fileNo = _B4->_140;

        setUserFileMario(fileNo, mBrosButton->isSelectedMario());
        restoreUserFile();
        setFileInfo(fileNo);
        mSelectInfo->change();
        mBrosButton->resume();
    }

    if (checkSelectedBackButton()) {
        disappearAllLayout();
        clearPointing();
        setNerve(&NrvFileSelector::FileSelectorNrvFileSelectStart::sInstance);
    }
}

void FileSelector::exeDemoStartWait() {
    if (MR::isFirstStep(this)) {
        MR::startSystemSE("SE_SY_FILE_SELECTED", -1, -1);
        MR::stopStageBGM(90);
        disappearAllLayout();
        MR::closeWipeFade(60);
    }

    if (MR::isWipeBlank()) {
        setNerve(&NrvFileSelector::FileSelectorNrvDemo::sInstance);
    }
}

void FileSelector::exeDemo() {
    if (MR::isFirstStep(this)) {
        s32 fileNo = _B4->_140;
        GameSequenceFunction::startGameDataLoadSequence(fileNo, !isUserFileLuigi(fileNo));
        MR::stopStageBGM(90);
    }

    if (!GameSequenceFunction::isActiveSaveDataHandleSequence()) {
        s32 fileNo = _B4->_140;
        FileSelectIconID iconID;
        u16 name[RFL_NAME_LEN + 1];
        getIconId(&iconID, fileNo);

        if (iconID.isMii()) {
            FileSelectFunc::copyMiiName(name, iconID);
        } else if (isUserFileLuigi(fileNo)) {
            MR::copyString(reinterpret_cast< wchar_t* >(name), MR::getGameMessageDirect(cLuigiNameMessageID), RFL_NAME_LEN + 1);
        } else {
            MR::copyString(reinterpret_cast< wchar_t* >(name), MR::getGameMessageDirect(cMarioNameMessageID), RFL_NAME_LEN + 1);
        }

        GameSequenceFunction::reserveUserName(reinterpret_cast< const wchar_t* >(name));
        MR::requestChangeStageInGameAfterLoadingGameData();
    }
}

void FileSelector::exeCreateConfirmStart() {
    if (MR::isFirstStep(this)) {
        goToNearPoint();
        invalidateSelectAll();
        disappearAllIndex();
    }

    if (mCameraController->isAtNearPoint()) {
        setNerve(&NrvFileSelector::FileSelectorNrvCreateConfirm::sInstance);
    }
}

void FileSelector::exeCreateConfirm() {
    if (MR::isFirstStep(this)) {
        mSysInfoWindow->appear("System_FileSelect001", SysInfoWindow::Type_YesNo, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
        mSysInfoWindow->setYesNoSelectorSE("SE_SY_BUTTON_CURSOR_ON", "SE_SY_FILE_SEL_NEW_FILE", "SE_SY_TALK_SELECT_NO");
    }

    if (MR::isDead(mSysInfoWindow)) {
        mSysInfoWindow->resetYesNoSelectorSE();

        if (mSysInfoWindow->isSelectedYes()) {
            setNerve(&NrvFileSelector::FileSelectorNrvCreate::sInstance);
        } else {
            clearPointing();
            setNerve(&NrvFileSelector::FileSelectorNrvFileSelectStart::sInstance);
        }
    }
}

void FileSelector::exeCreate() {
    if (MR::isFirstStep(this)) {
        GameSequenceFunction::startCreateUserFileSequence(_B4->_140);
    }

    if (!GameSequenceFunction::isActiveSaveDataHandleSequence()) {
        if (GameSequenceFunction::isSuccessSaveDataHandleSequence()) {
            restoreUserFile();
            setNerve(&NrvFileSelector::FileSelectorNrvMiiSelectStartFirst::sInstance);
        } else {
            clearPointing();
            setNerve(&NrvFileSelector::FileSelectorNrvFileSelectStart::sInstance);
        }
    }
}

void FileSelector::exeCopyWait() {
    if (MR::isFirstStep(this)) {
        MR::startSystemSE("SE_SY_FILE_SEL_UPPER_DECIDE", -1, -1);
        _B8 = _B4;
        disappearAllLayout();
        clearPointing();
        mCameraController->goToFarPoint();
        calcBasePos(0.0f);
    }

    if (mCameraController->isAtFarPoint()) {
        setNerve(&NrvFileSelector::FileSelectorNrvCopySelect::sInstance);
    }
}

void FileSelector::exeCopySelect() {
    if (MR::isFirstStep(this)) {
        validateSelectAll();
        _B8->invalidateSelect();
        mBackButton->appear();
        appearAllIndex();
        MR::activeStarPointerGuidance();
        mSelectEffects[getItemArrayIndex(_B8->_140)].appear();
    }

    MR::requestFileSelectCopyGuidance();
    updateFileInfo();

    if (mBackButton->isPointing()) {
        clearPointing();
        mSelectInfo->disappear();
    }

    if (checkSelectedBackButton()) {
        invalidateSelectAll();
        mSelectEffects[getItemArrayIndex(_B8->_140)].disappear();
        mInfoMessage->disappear();
        MR::deactiveStarPointerGuidance();
        setFileInfo(_B4->_140);
        mSelectInfo->appear();
        mSelectInfo->forceChange();
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirmStart::sInstance);
    }
}

void FileSelector::exeCopyConfirmStart() {
    if (MR::isFirstStep(this)) {
        disappearAllLayout();
        invalidateSelectAll();
        mSelectEffects[getItemArrayIndex(_B8->_140)].disappear();
    }

    if (isHiddenAllLayout()) {
        setNerve(&NrvFileSelector::FileSelectorNrvCopyConfirm::sInstance);
    }
}

void FileSelector::exeCopyConfirm() {
    if (MR::isFirstStep(this)) {
        clearPointing();

        if (_B4->isNew()) {
            mSysInfoWindow->appear("System_FileSelect016", SysInfoWindow::Type_YesNo, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
        } else {
            mSysInfoWindow->appear("System_FileSelect014", SysInfoWindow::Type_YesNo, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
        }

        mSysInfoWindow->setTextBoxArgNumber(_B8->_140, 0);
        mSysInfoWindow->setTextBoxArgNumber(_B4->_140, 1);
    }

    if (MR::isDead(mSysInfoWindow)) {
        if (mSysInfoWindow->isSelectedYes()) {
            setNerve(&NrvFileSelector::FileSelectorNrvCopySave::sInstance);
        } else {
            _B4 = _B8;
            setNerve(&NrvFileSelector::FileSelectorNrvCopySelect::sInstance);
        }
    }
}

void FileSelector::exeCopySave() {
    if (MR::isFirstStep(this)) {
        setUserFileMario(_B4->_140, !isUserFileLuigi(_B8->_140));

        if (_B4->isNew()) {
            GameSequenceFunction::startCopyUserFileSequence(_B4->_140, _B8->_140);
        } else {
            GameSequenceFunction::storeCopyUserFileSequence(_B4->_140, _B8->_140);
            setNerve(&NrvFileSelector::FileSelectorNrvCopySaveMii::sInstance);
            return;
        }
    }

    if (!GameSequenceFunction::isActiveSaveDataHandleSequence()) {
        setNerve(&NrvFileSelector::FileSelectorNrvCopyDemo::sInstance);
    }
}

void FileSelector::exeCopySaveMii() {
    if (MR::isFirstStep(this)) {
        RFLCreateID createID;

        if (_B4->_146 && mFiles[_B4->_140 - 1].getMiiId(&createID)) {
            GameSequenceFunction::storeMiiOrIconIdUserFileSequence(_B4->_140, &createID, nullptr);
            GameSequenceFunction::startSaveAllUserFileSequence();
        } else {
            FileSelectIconID iconID;
            _B4->copyIconID(&iconID);
            storeSetMiiIdUserFile(_B4->_140, iconID);
        }
    }

    if (!GameSequenceFunction::isActiveSaveDataHandleSequence()) {
        setNerve(&NrvFileSelector::FileSelectorNrvCopyDemo::sInstance);
    }
}

void FileSelector::exeCopyDemo() {
    if (MR::isFirstStep(this)) {
        MR::startSystemSE("SE_SY_FILE_SEL_COPY", -1, -1);
        restoreUserFile();

        FileSelectIconID iconID;
        getIconId(&iconID, _B4->_140);
        _B4->change(iconID, _CC[_B4->_140 - 1]);

        mBackButton->disappear();
        mInfoMessage->disappear();
        invalidateSelectAll();
    }

    if (_B4->isExist()) {
        clearPointing();
        setNerve(&NrvFileSelector::FileSelectorNrvFileSelect::sInstance);
    }
}

void FileSelector::exeCopyRejectStart() {
    if (MR::isFirstStep(this)) {
        disappearAllLayout();
    }

    if (isHiddenAllLayout()) {
        setNerve(&NrvFileSelector::FileSelectorNrvCopyReject::sInstance);
    }
}

void FileSelector::exeCopyReject() {
    if (MR::isFirstStep(this)) {
        mSysInfoWindowMini->appear("System_FileSelect003", SysInfoWindow::Type_Key, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
        MR::startSystemSE("SE_SY_FILE_SEL_NG", -1, -1);
    }

    if (MR::isDead(mSysInfoWindowMini)) {
        mSelectButton->shiftSelect();
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
    }
}

void FileSelector::exeDeleteConfirmStart() {
    if (MR::isFirstStep(this)) {
        MR::startSystemSE("SE_SY_FILE_SEL_UPPER_DECIDE", -1, -1);
        disappearAllLayout();
    }

    if (isHiddenAllLayout()) {
        setNerve(&NrvFileSelector::FileSelectorNrvDeleteConfirm::sInstance);
    }
}

void FileSelector::exeDeleteConfirm() {
    if (MR::isFirstStep(this)) {
        mSysInfoWindow->setYesNoSelectorSE("SE_SY_BUTTON_CURSOR_ON", "SE_SY_FILE_SEL_DELETE", "SE_SY_TALK_SELECT_NO");
        mSysInfoWindow->appear("System_FileSelect007", SysInfoWindow::Type_YesNo, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
    }

    if (MR::isDead(mSysInfoWindow)) {
        mSysInfoWindow->resetYesNoSelectorSE();

        if (mSysInfoWindow->isSelectedYes()) {
            setNerve(&NrvFileSelector::FileSelectorNrvDelete::sInstance);
        } else {
            mSelectButton->shiftSelect();
            setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
        }
    }
}

void FileSelector::exeDelete() {
    if (MR::isFirstStep(this)) {
        setUserFileMario(_B4->_140, true);
        GameSequenceFunction::startDeleteUserFileSequence(_B4->_140);
    }

    if (!GameSequenceFunction::isActiveSaveDataHandleSequence()) {
        if (GameSequenceFunction::isSuccessSaveDataHandleSequence()) {
            setNerve(&NrvFileSelector::FileSelectorNrvDeleteDemo::sInstance);
        } else {
            clearPointing();
            setNerve(&NrvFileSelector::FileSelectorNrvFileSelectStart::sInstance);
        }
    }
}

void FileSelector::exeDeleteDemo() {
    if (MR::isFirstStep(this)) {
        _B4->format();
    }

    if (_B4->isNew()) {
        initUserFile();
        clearPointing();
        setNerve(&NrvFileSelector::FileSelectorNrvFileSelectStart::sInstance);
    }
}

void FileSelector::exeFileBroken() {
    if (MR::isFirstStep(this)) {
        mSysInfoWindow->appear("System_FileSelect012", SysInfoWindow::Type_Key, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
        MR::startSystemSE("SE_SY_FILE_SEL_NG", -1, -1);
    }

    if (MR::isDead(mSysInfoWindow)) {
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
    }
}

void FileSelector::exeManualStart() {
    if (MR::isFirstStep(this)) {
        disappearAllLayout();
    }

    if (isHiddenAllLayout()) {
        setNerve(&NrvFileSelector::FileSelectorNrvManual::sInstance);
    }
}

void FileSelector::exeManual() {
    if (MR::isFirstStep(this)) {
        mManual2P->appear();
    }

    if (mManual2P->isClosed()) {
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
    }
}

void FileSelector::exeMiiWait() {
    if (MR::isFirstStep(this)) {
        MR::startSystemSE("SE_SY_FILE_SEL_UPPER_DECIDE", -1, -1);
    }

    if (mBackButton->isHidden() && MR::isDead(mSelectInfo)) {
        setNerve(&NrvFileSelector::FileSelectorNrvMiiSelectStart::sInstance);
    }
}

void FileSelector::exeMiiTip() {
    if (MR::isFirstStep(this)) {
        mSysInfoWindow->appear("System_FileSelect004", SysInfoWindow::Type_Key, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
        clearPointing();
        mSelectInfo->disappear();
    }

    if (MR::isDead(mSysInfoWindow)) {
        setNerve(&NrvFileSelector::FileSelectorNrvMiiSelectStart::sInstance);
    }
}

void FileSelector::exeMiiSelectStart() {
    if (MR::isFirstStep(this)) {
        if (isNerve(&NrvFileSelector::FileSelectorNrvMiiSelectStart::sInstance)) {
            FileSelectIconID iconID;
            _B4->copyIconID(&iconID);
            mMiiSelect->prohibitIcon(iconID);
        } else {
            _E4 = true;
            mMiiSelect->admitIcon();
        }

        if (isUserFileAppearLuigi(_B4->_140)) {
            mMiiSelect->validateAllSpecialMii();
        } else {
            mMiiSelect->invalidateSpecialMii(FileSelectIconID::Luigi);
        }

        mMiiSelect->appear();
    }

    if (!mMiiSelect->isAppearing()) {
        setNerve(&NrvFileSelector::FileSelectorNrvMiiSelect::sInstance);
    }
}

void FileSelector::exeMiiSelect() {
    if (MR::isFirstStep(this) && !_E4) {
        mBackButton->appear();
    }

    if (mMiiSelect->isDummySelected()) {
        MR::startSystemSE("SE_SY_FILE_SEL_MII_SELECTED", -1, -1);
        mMiiSelect->disappear();

        if (!mBackButton->isHidden()) {
            mBackButton->disappear();
        }

        setNerve(&NrvFileSelector::FileSelectorNrvMiiInfoStart::sInstance);
    } else if (mMiiSelect->isSelected()) {
        MR::startSystemSE("SE_SY_FILE_SEL_MII_SELECTED", -1, -1);
        mMiiSelect->disappear();

        if (!mBackButton->isHidden()) {
            mBackButton->disappear();
        }

        setNerve(&NrvFileSelector::FileSelectorNrvMiiConfirmWait::sInstance);
    } else if (checkSelectedBackButton()) {
        mMiiSelect->disappear();
        mBackButton->disappear();
        setNerve(&NrvFileSelector::FileSelectorNrvMiiCancel::sInstance);
    }
}

void FileSelector::exeMiiCancel() {
    if (MR::isDead(mMiiSelect) && mBackButton->isHidden()) {
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
    }
}

void FileSelector::exeMiiConfirmWait() {
    if (MR::isDead(mMiiSelect) && mBackButton->isHidden()) {
        setNerve(&NrvFileSelector::FileSelectorNrvMiiConfirm::sInstance);
    }
}

void FileSelector::exeMiiConfirm() {
    if (MR::isFirstStep(this)) {
        if (_E4) {
            mSysInfoWindow->appear("System_FileSelect013", SysInfoWindow::Type_YesNo, SysInfoWindow::TextPos_Bottom, SysInfoWindow::MessageType_System);
        } else {
            mSysInfoWindow->appear("System_FileSelect005", SysInfoWindow::Type_YesNo, SysInfoWindow::TextPos_Bottom, SysInfoWindow::MessageType_System);
        }

        FileSelectIconID iconID;
        mMiiSelect->getSelectedID(&iconID);

        u16 name[RFL_NAME_LEN + 1];
        FileSelectFunc::copyMiiName(name, iconID);
        mMiiConfirmIcon->appear(mMiiSelect->getSelectedMiiTexMap(), reinterpret_cast< const wchar_t* >(name));
        mSysInfoWindow->setYesNoSelectorSE("SE_SY_BUTTON_CURSOR_ON", "SE_SY_FILE_SEL_MII_CHANGE", "SE_SY_TALK_SELECT_NO");
    }

    if (!mMiiConfirmIcon->isDisappear() && mSysInfoWindow->isDisappear()) {
        mMiiConfirmIcon->disappear();
    }

    if (MR::isDead(mSysInfoWindow)) {
        mSysInfoWindow->resetYesNoSelectorSE();

        if (mSysInfoWindow->isSelectedYes()) {
            setNerve(&NrvFileSelector::FileSelectorNrvMiiCreateWait::sInstance);
        } else if (_E4) {
            setNerve(&NrvFileSelector::FileSelectorNrvMiiSelectStartFirst::sInstance);
        } else {
            setNerve(&NrvFileSelector::FileSelectorNrvMiiSelectStart::sInstance);
        }
    }
}

void FileSelector::exeMiiCreateWait() {
    if (MR::isFirstStep(this)) {
        FileSelectIconID iconID;
        mMiiSelect->getSelectedID(&iconID);
        storeSetMiiIdUserFile(_B4->_140, iconID);
        _E4 = false;
    }

    if (!GameSequenceFunction::isActiveSaveDataHandleSequence()) {
        setNerve(&NrvFileSelector::FileSelectorNrvMiiCreateDemo::sInstance);
    }
}

void FileSelector::exeMiiCreateDemo() {
    if (MR::isFirstStep(this)) {
        restoreUserFile();

        s32 fileNo = _B4->_140;
        FileSelectIconID iconID;
        mMiiSelect->getSelectedID(&iconID);
        _B4->change(iconID, _CC[fileNo - 1]);
        setFileInfo(fileNo);
    }

    if (_B4->isExist()) {
        FileSelectIconID iconID;
        mMiiSelect->getSelectedID(&iconID);

        if (iconID.isMii()) {
            setNerve(&NrvFileSelector::FileSelectorNrvMiiCaution::sInstance);
        } else {
            setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
        }
    }
}

void FileSelector::exeMiiCaution() {
    if (MR::isFirstStep(this)) {
        mSysInfoWindowMini->appear("System_FileSelect006", SysInfoWindow::Type_Key, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
    }

    if (MR::isDead(mSysInfoWindowMini)) {
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
    }
}

void FileSelector::exeMiiInfoStart() {
    if (MR::isDead(mMiiSelect) && mBackButton->isHidden()) {
        setNerve(&NrvFileSelector::FileSelectorNrvMiiInfo::sInstance);
    }
}

void FileSelector::exeMiiInfo() {
    if (MR::isFirstStep(this)) {
        mSysInfoWindowMini->appear("System_FileSelect015", SysInfoWindow::Type_Key, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
    }

    if (MR::isDead(mSysInfoWindowMini)) {
        if (_E4) {
            setNerve(&NrvFileSelector::FileSelectorNrvMiiSelectStartFirst::sInstance);
        } else {
            setNerve(&NrvFileSelector::FileSelectorNrvMiiSelectStart::sInstance);
        }
    }
}
