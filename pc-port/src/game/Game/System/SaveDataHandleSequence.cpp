#include "Game/System/SaveDataHandleSequence.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Screen/SaveIcon.hpp"
#include "Game/Screen/SysInfoWindow.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/System/NANDManager.hpp"
#include "Game/System/SaveDataHandler.hpp"
#include "Game/System/SysConfigFile.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/NerveUtil.hpp"

#include <cstdio>

namespace {
    NEW_NERVE(SaveDataHandleSequenceNoOperation, SaveDataHandleSequence, NoOperation);
    NEW_NERVE(SaveDataHandleSequenceCheckEnableToCreate, SaveDataHandleSequence, CheckEnableToCreate);
    NEW_NERVE(SaveDataHandleSequenceSaveAllWithoutKeyWait, SaveDataHandleSequence, SaveAllWithoutKeyWait);
    NEW_NERVE(SaveDataHandleSequenceSaveAllWithoutKeyWaitDisappear, SaveDataHandleSequence, SaveAllWithoutKeyWaitDisappear);
    NEW_NERVE(SaveDataHandleSequencePreLoad, SaveDataHandleSequence, PreLoad);
    NEW_NERVE(SaveDataHandleSequencePreLoadDone, SaveDataHandleSequence, PreLoadDone);

    bool isSaveWindowReady(const SysInfoWindow* pWindow) {
        return pWindow != nullptr && pWindow->getResource() != nullptr;
    }
}  // namespace

SaveDataHandleSequence::SaveDataHandleSequence()
    : NerveExecutor("セーブ/ロード"), mSysConfigFile(nullptr), mCurrentUserFile(nullptr), mBackupUserFile(nullptr), mSaveDataHandler(nullptr),
      mSysInfoWindowSave(nullptr), _24(0), mTempBuffer(new u8[SaveDataHandler::getEnoughtTempBufferSize()]), mSaveIcon(nullptr),
      mInitialized(false) {
    initNerve(&SaveDataHandleSequenceNoOperation::sInstance);
}

SaveDataHandleSequence::~SaveDataHandleSequence() {
    delete mSysConfigFile;
    delete mCurrentUserFile;
    delete mBackupUserFile;
    delete mSaveDataHandler;
    delete mSysInfoWindowSave;
    delete mSaveIcon;
    delete[] mTempBuffer;
}

void SaveDataHandleSequence::initAfterResourceLoaded() {
    if (mInitialized) {
        return;
    }

    mSysConfigFile = new SysConfigFile();
    mCurrentUserFile = new UserFile();
    mBackupUserFile = new UserFile();
    mSaveDataHandler = new SaveDataHandler(mSysConfigFile, mCurrentUserFile);
    mSysInfoWindowSave = MR::createSysInfoWindowMiniExecuteWithChildren();
    mSysInfoWindowSave->kill();
    mSaveIcon = new SaveIcon(mSysInfoWindowSave);
    mSaveIcon->kill();
    mInitialized = true;
}

void SaveDataHandleSequence::update() {
    if (!mInitialized) {
        initAfterResourceLoaded();
    }

    updateNerve();
    if (mSaveDataHandler != nullptr) {
        mSaveDataHandler->update();
    }

    if (mSysInfoWindowSave != nullptr) {
        mSysInfoWindowSave->movement();
        mSysInfoWindowSave->calcAnim();
    }

    if (mSaveIcon != nullptr) {
        mSaveIcon->movement();
        mSaveIcon->calcAnim();
    }
}

void SaveDataHandleSequence::draw() const {
}

void SaveDataHandleSequence::appendDrawCommands(smgpc::render::layout::LayoutDrawList* pDrawList) const {
    if (mSysInfoWindowSave != nullptr) {
        mSysInfoWindowSave->appendDrawCommands(pDrawList);
    }

    if (mSaveIcon != nullptr) {
        mSaveIcon->appendDrawCommands(pDrawList);
    }
}

void SaveDataHandleSequence::startPreLoad() {
    initAfterResourceLoaded();
    setNerve(&SaveDataHandleSequencePreLoad::sInstance);
}

void SaveDataHandleSequence::startCreateUserFile(int index) {
    initAfterResourceLoaded();
    restoreUserFile(mCurrentUserFile, index, true);
    mCurrentUserFile->resetAllData();
    mCurrentUserFile->setCreated();
    mCurrentUserFile->updateLastModified();
    mSaveDataHandler->initializeUserFileMemory(index, mCurrentUserFile);
    setNerve(&SaveDataHandleSequenceSaveAllWithoutKeyWait::sInstance);
}

void SaveDataHandleSequence::startDeleteUserFile(int index) {
    initAfterResourceLoaded();
    restoreUserFile(mCurrentUserFile, index, true);
    mCurrentUserFile->resetAllData();
    mSaveDataHandler->initializeUserFileMemory(index, mCurrentUserFile);
    setNerve(&SaveDataHandleSequenceSaveAllWithoutKeyWait::sInstance);
}

void SaveDataHandleSequence::startSave(bool, bool) {
    initAfterResourceLoaded();
    mSaveDataHandler->storeUserFile(mCurrentUserFile);
    setNerve(&SaveDataHandleSequenceSaveAllWithoutKeyWait::sInstance);
}

void SaveDataHandleSequence::startSaveBackup(bool, bool) {
    initAfterResourceLoaded();
    mSaveDataHandler->storeUserFile(mBackupUserFile);
    setNerve(&SaveDataHandleSequenceSaveAllWithoutKeyWait::sInstance);
}

void SaveDataHandleSequence::startSaveAll() {
    initAfterResourceLoaded();
    setNerve(&SaveDataHandleSequenceSaveAllWithoutKeyWait::sInstance);
}

void SaveDataHandleSequence::startSaveTotalMailSize() {
    startSaveAll();
}

void SaveDataHandleSequence::startLoad(int userFileIndex, bool isPlayerMario) {
    initAfterResourceLoaded();
    restoreUserFile(mCurrentUserFile, userFileIndex, isPlayerMario);
    _24 = 2;
    setNerve(&SaveDataHandleSequenceNoOperation::sInstance);
}

void SaveDataHandleSequence::storeMiiOrIconId(int userFileIndex, const void* pMiiId, const u32* pIconId) {
    initAfterResourceLoaded();
    restoreUserFile(mCurrentUserFile, userFileIndex);
    mCurrentUserFile->setMiiOrIconId(pMiiId, pIconId);
    mCurrentUserFile->updateLastModified();
    mSaveDataHandler->storeUserFile(mCurrentUserFile);
}

void SaveDataHandleSequence::storeCopyUserFile(int indexDst, int indexSrc) {
    initAfterResourceLoaded();
    mSaveDataHandler->copyUserFileMemory(indexDst, indexSrc);
}

bool SaveDataHandleSequence::tryNANDErrorSequence(s32) {
    return false;
}

bool SaveDataHandleSequence::isActive() const {
    return !isNerve(&SaveDataHandleSequenceNoOperation::sInstance);
}

bool SaveDataHandleSequence::isPermitToReset() const {
    return true;
}

void SaveDataHandleSequence::prepareReset() {
}

bool SaveDataHandleSequence::isPreparedReset() const {
    return true;
}

void SaveDataHandleSequence::restoreFromReset() {
    if (mSysInfoWindowSave != nullptr && !MR::isDead(mSysInfoWindowSave)) {
        mSysInfoWindowSave->forceKill();
    }

    if (mSaveIcon != nullptr) {
        mSaveIcon->kill();
    }

    _24 = 3;
    setNerve(&SaveDataHandleSequenceNoOperation::sInstance);
}

bool SaveDataHandleSequence::isInitializedGameDataHolder() const {
    return mCurrentUserFile != nullptr;
}

void SaveDataHandleSequence::restoreUserFile(UserFile* pUserFile, int index) {
    restoreUserFileConfigData(pUserFile, index);
    restoreUserFileGameData(pUserFile, index, pUserFile->isLastLoadedMario());
}

void SaveDataHandleSequence::restoreUserFile(UserFile* pUserFile, int index, bool isPlayerMario) {
    restoreUserFileConfigData(pUserFile, index);
    restoreUserFileGameData(pUserFile, index, isPlayerMario);
    pUserFile->setLastLoadedMario(isPlayerMario);
}

void SaveDataHandleSequence::backupCurrentUserFile() {
    mCurrentUserFile->makeGameDataBinary(mTempBuffer, SaveDataHandler::getEnoughtTempBufferSize());
    mBackupUserFile->loadFromGameDataBinary(mCurrentUserFile->getGameDataName(), mTempBuffer, SaveDataHandler::getEnoughtTempBufferSize());
    mCurrentUserFile->makeConfigDataBinary(mTempBuffer, SaveDataHandler::getEnoughtTempBufferSize());
    mBackupUserFile->loadFromConfigDataBinary(mCurrentUserFile->getConfigDataName(), mTempBuffer, SaveDataHandler::getEnoughtTempBufferSize());
}

void SaveDataHandleSequence::exeNoOperation() {
}

void SaveDataHandleSequence::exeCheckEnableToCreate() {
    if (MR::isFirstStep(this)) {
        mSaveDataHandler->requestCheckEnableToCreate();
        _24 = 1;
    }

    if (!mSaveDataHandler->isDone()) {
        return;
    }

    _24 = mSaveDataHandler->getLastResultCode().isSuccess() ? 2 : 3;
    setNerve(&SaveDataHandleSequenceNoOperation::sInstance);
}

void SaveDataHandleSequence::exeSaveAllWithoutKeyWait() {
    if (MR::isFirstStep(this)) {
        mSaveDataHandler->storeSysConfigFile(mSysConfigFile);
        _24 = 1;
    }

    if (!trySave()) {
        return;
    }

    setNerve(&SaveDataHandleSequenceSaveAllWithoutKeyWaitDisappear::sInstance);
}

void SaveDataHandleSequence::exeSaveAllWithoutKeyWaitDisappear() {
    bool isErr = false;

    if (!trySaveWindowDisappear(&isErr)) {
        return;
    }

    _24 = isErr ? 3 : 2;
    setNerve(&SaveDataHandleSequenceNoOperation::sInstance);
}

void SaveDataHandleSequence::exePreLoad() {
    if (MR::isFirstStep(this)) {
        mSaveDataHandler->requestLoadSaveData();
        _24 = 1;
    }

    if (!mSaveDataHandler->isDone()) {
        return;
    }

    const NANDResultCode resultCode = mSaveDataHandler->getLastResultCode();
    if (resultCode.isSuccess()) {
        setNerve(&SaveDataHandleSequencePreLoadDone::sInstance);
    } else if (resultCode.isNoExistFile()) {
        setNerve(&SaveDataHandleSequenceCheckEnableToCreate::sInstance);
    } else {
        _24 = 3;
        setNerve(&SaveDataHandleSequenceNoOperation::sInstance);
    }
}

void SaveDataHandleSequence::exePreLoadDone() {
    if (mSaveDataHandler->requestVerifyAfterLoadGameDataFile()) {
        restoreSysConfigFile(mSysConfigFile);
        _24 = 2;
    } else {
        _24 = 3;
    }
    setNerve(&SaveDataHandleSequenceNoOperation::sInstance);
}

GameDataHolder* SaveDataHandleSequence::getHolder() {
    return mCurrentUserFile->mGameDataHolder;
}

SysConfigFile* SaveDataHandleSequence::getSysConfigFile() {
    return mSysConfigFile;
}

UserFile* SaveDataHandleSequence::getCurrentUserFile() {
    return mCurrentUserFile;
}

UserFile* SaveDataHandleSequence::getBackupUserFile() {
    return mBackupUserFile;
}

void SaveDataHandleSequence::restoreUserFileConfigData(UserFile* pUserFile, int index) {
    char dataName[16];
    std::snprintf(dataName, sizeof(dataName), "config%1d", index);
    mSaveDataHandler->restoreGameDataFile(dataName, mTempBuffer, SaveDataHandler::getEnoughtTempBufferSize());
    pUserFile->loadFromConfigDataBinary(dataName, mTempBuffer, SaveDataHandler::getEnoughtTempBufferSize());
}

void SaveDataHandleSequence::restoreUserFileGameData(UserFile* pUserFile, int index, bool isPlayerMario) {
    char dataName[16];
    std::snprintf(dataName, sizeof(dataName), "%s%1d", isPlayerMario ? "mario" : "luigi", index);
    mSaveDataHandler->restoreGameDataFile(dataName, mTempBuffer, SaveDataHandler::getEnoughtTempBufferSize());
    pUserFile->loadFromGameDataBinary(dataName, mTempBuffer, SaveDataHandler::getEnoughtTempBufferSize());
    pUserFile->mIsPlayerMario = isPlayerMario;
}

void SaveDataHandleSequence::restoreSysConfigFile(SysConfigFile* pSysConfigFile) {
    mSaveDataHandler->restoreGameDataFile("sysconf", mTempBuffer, SaveDataHandler::getEnoughtTempBufferSize());
    pSysConfigFile->loadFromDataBinary(mTempBuffer, SaveDataHandler::getEnoughtTempBufferSize());
}

bool SaveDataHandleSequence::trySave() {
    const bool isWindowReady = isSaveWindowReady(mSysInfoWindowSave);

    if (MR::isFirstStep(this)) {
        mSaveDataHandler->requestSaveSaveData();
        if (isWindowReady) {
            mSysInfoWindowSave->appear("System_Save01", SysInfoWindow::Type_Blocking, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
        }
    }

    if (!isWindowReady) {
        return mSaveDataHandler->isDone();
    }

    if (mSaveIcon != nullptr && MR::isDead(mSaveIcon) && mSysInfoWindowSave->isWait()) {
        mSaveIcon->appear();
    }

    return getNerveStep() > 120 && mSaveDataHandler->isDone();
}

bool SaveDataHandleSequence::trySaveWindowDisappear(bool* pIsErr) {
    const bool isWindowReady = isSaveWindowReady(mSysInfoWindowSave);

    if (MR::isFirstStep(this)) {
        if (mSaveIcon != nullptr) {
            mSaveIcon->kill();
        }
        if (isWindowReady) {
            mSysInfoWindowSave->disappear();
        }
    }

    if (isWindowReady && !MR::isDead(mSysInfoWindowSave)) {
        return false;
    }

    if (pIsErr != nullptr) {
        *pIsErr = !mSaveDataHandler->getLastResultCode().isSuccess();
    }

    return true;
}
