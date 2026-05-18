#include "Game/System/SaveDataHandleSequence.hpp"

#include <array>
#include <cstdio>
#include <vector>

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
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"

namespace {
    NEW_NERVE(SaveDataHandleSequenceNoOperation, SaveDataHandleSequence, NoOperation);
    NEW_NERVE(SaveDataHandleSequenceProcessing, SaveDataHandleSequence, Processing);
    NEW_NERVE(SaveDataHandleSequenceSaveConfirm, SaveDataHandleSequence, SaveConfirm);
    NEW_NERVE(SaveDataHandleSequenceSave, SaveDataHandleSequence, Save);
    NEW_NERVE(SaveDataHandleSequenceSaveWindowDisappear, SaveDataHandleSequence, SaveWindowDisappear);
    NEW_NERVE(SaveDataHandleSequenceSaveAllWithoutKeyWait, SaveDataHandleSequence, SaveAllWithoutKeyWait);
    NEW_NERVE(SaveDataHandleSequenceSaveAllWithoutKeyWaitDisappear, SaveDataHandleSequence, SaveAllWithoutKeyWaitDisappear);
    NEW_NERVE(SaveDataHandleSequenceSaveAllWithoutWindow, SaveDataHandleSequence, SaveAllWithoutWindow);
    NEW_NERVE(SaveDataHandleSequencePreLoad, SaveDataHandleSequence, PreLoad);
    NEW_NERVE(SaveDataHandleSequencePreLoadDone, SaveDataHandleSequence, PreLoadDone);
    NEW_NERVE(SaveDataHandleSequenceNoSaveConfirmRemind, SaveDataHandleSequence, NoSaveConfirmRemind);
}  // namespace

SaveDataHandleSequence::SaveDataHandleSequence()
    : NerveExecutor("セーブ/ロード"), mSysConfigFile(nullptr), mCurrentUserFile(nullptr), mBackupUserFile(nullptr), mSaveDataHandler(nullptr),
      _24(0), mIsConfirmRemind(false), mIsSaveAndQuitMsg(false), _2A(false), _2B(false), _2C(false), mWorkUserFile(nullptr) {
    initNerve(&SaveDataHandleSequenceNoOperation::sInstance);
}

SaveDataHandleSequence::~SaveDataHandleSequence() {
    delete mSaveDataHandler;
    delete mSaveIcon;
    delete mSysInfoWindowSave;
    delete mSysInfoWindowConfirm;
    delete mSysConfigFile;
    delete mCurrentUserFile;
    delete mBackupUserFile;
}

void SaveDataHandleSequence::initAfterResourceLoaded() {
    if (mSysConfigFile == nullptr) {
        mSysConfigFile = new SysConfigFile();
    }
    if (mCurrentUserFile == nullptr) {
        mCurrentUserFile = new UserFile();
    }
    if (mBackupUserFile == nullptr) {
        mBackupUserFile = new UserFile();
    }
    if (mSaveDataHandler == nullptr) {
        mSaveDataHandler = new SaveDataHandler(mSysConfigFile, mCurrentUserFile);
    }
    if (mSysInfoWindowConfirm == nullptr) {
        mSysInfoWindowConfirm = MR::createSysInfoWindowExecuteWithChildren();
        mSysInfoWindowConfirm->kill();
    }
    if (mSysInfoWindowSave == nullptr) {
        mSysInfoWindowSave = MR::createSysInfoWindowMiniExecuteWithChildren();
        mSysInfoWindowSave->kill();
    }
    if (mSaveIcon == nullptr) {
        mSaveIcon = new SaveIcon(mSysInfoWindowSave);
        mSaveIcon->kill();
    }

    restoreSysConfigFile(mSysConfigFile);
}

void SaveDataHandleSequence::registerFunctorOnSaveSuccess(const MR::FunctorBase&) {
}

void SaveDataHandleSequence::registerFunctorJustBeforeSave(const MR::FunctorBase&) {
}

void SaveDataHandleSequence::update() {
    updateNerve();
    if (mSaveDataHandler != nullptr) {
        mSaveDataHandler->update();
    }

    if (mSysInfoWindowConfirm != nullptr) {
        mSysInfoWindowConfirm->movement();
        mSysInfoWindowConfirm->calcAnim();
    }
    if (mSysInfoWindowSave != nullptr) {
        mSysInfoWindowSave->movement();
        mSysInfoWindowSave->calcAnim();
    }
    if (mSaveIcon != nullptr) {
        mSaveIcon->movement();
        mSaveIcon->calcAnim();
    }

    if (!isNerve(&SaveDataHandleSequenceNoOperation::sInstance) && !isNerve(&SaveDataHandleSequencePreLoad::sInstance) &&
        !isNerve(&SaveDataHandleSequencePreLoadDone::sInstance)) {
        MR::requestStarPointerModeSaveLoad(this);
    }
}

void SaveDataHandleSequence::draw() const {
    if (mSysInfoWindowConfirm != nullptr) {
        mSysInfoWindowConfirm->draw();
    }
    if (mSysInfoWindowSave != nullptr) {
        mSysInfoWindowSave->draw();
    }
    if (mSaveIcon != nullptr) {
        mSaveIcon->draw();
    }
}

void SaveDataHandleSequence::startPreLoad() {
    mIsActive = true;
    setNerve(&SaveDataHandleSequencePreLoad::sInstance);
}

void SaveDataHandleSequence::startCreateUserFile(int index) {
    restoreUserFile(mCurrentUserFile, index, true);
    mCurrentUserFile->resetAllData();
    mCurrentUserFile->setCreated();
    mCurrentUserFile->setLastLoadedMario(true);
    mCurrentUserFile->updateLastModified();
    mSaveDataHandler->initializeUserFileMemory(index, mCurrentUserFile);
    mIsActive = true;
    setNerve(&SaveDataHandleSequenceSaveAllWithoutKeyWait::sInstance);
}

void SaveDataHandleSequence::startDeleteUserFile(int index) {
    restoreUserFile(mCurrentUserFile, index, true);
    mCurrentUserFile->resetAllData();
    mSaveDataHandler->initializeUserFileMemory(index, mCurrentUserFile);
    mIsActive = true;
    setNerve(&SaveDataHandleSequenceSaveAllWithoutKeyWait::sInstance);
}

void SaveDataHandleSequence::startSave(bool isConfirmRemind, bool isSaveAndQuitMsg) {
    mIsConfirmRemind = isConfirmRemind;
    mIsSaveAndQuitMsg = isSaveAndQuitMsg;
    mWorkUserFile = mCurrentUserFile;
    mIsActive = true;
    setNerve(&SaveDataHandleSequenceSaveConfirm::sInstance);
}

void SaveDataHandleSequence::startSaveBackup(bool isConfirmRemind, bool isSaveAndQuitMsg) {
    mIsConfirmRemind = isConfirmRemind;
    mIsSaveAndQuitMsg = isSaveAndQuitMsg;
    mWorkUserFile = mBackupUserFile;
    mIsActive = true;
    setNerve(&SaveDataHandleSequenceSaveConfirm::sInstance);
}

void SaveDataHandleSequence::startSaveAll() {
    mIsActive = true;
    setNerve(&SaveDataHandleSequenceSaveAllWithoutKeyWait::sInstance);
}

void SaveDataHandleSequence::startSaveTotalMailSize() {
    storeSysConfigToService();
    mIsActive = true;
    setNerve(&SaveDataHandleSequenceSaveAllWithoutWindow::sInstance);
}

void SaveDataHandleSequence::startLoad(int userFileIndex, bool isPlayerMario) {
    restoreUserFile(mCurrentUserFile, userFileIndex, isPlayerMario);
    if (mSaveDataHandler != nullptr) {
        mSaveDataHandler->requestVerifyAfterLoadGameDataFile();
    }
    completeSequenceSuccess();
}

void SaveDataHandleSequence::storeMiiOrIconId(int userFileIndex, const void* pMiiId, const u32* pIconId) {
    restoreUserFile(mCurrentUserFile, userFileIndex);
    mCurrentUserFile->setMiiOrIconId(pMiiId, pIconId);
    mCurrentUserFile->updateLastModified();
    mSaveDataHandler->storeUserFile(mCurrentUserFile);
}

void SaveDataHandleSequence::storeCopyUserFile(int indexDst, int indexSrc) {
    mSaveDataHandler->copyUserFileMemory(indexDst, indexSrc);
}

bool SaveDataHandleSequence::tryNANDErrorSequence(s32) {
    return false;
}

bool SaveDataHandleSequence::isActive() const {
    return mIsActive || !isNerve(&SaveDataHandleSequenceNoOperation::sInstance);
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
    if (mSysInfoWindowConfirm != nullptr) {
        mSysInfoWindowConfirm->forceKill();
    }
    if (mSysInfoWindowSave != nullptr) {
        mSysInfoWindowSave->forceKill();
    }
    if (mSaveIcon != nullptr) {
        mSaveIcon->kill();
    }
    completeSequenceSuccess();
}

bool SaveDataHandleSequence::isInitializedGameDataHolder() const {
    return mCurrentUserFile != nullptr;
}

void SaveDataHandleSequence::restoreUserFile(UserFile* pUserFile, int index) {
    if (pUserFile != nullptr) {
        restoreUserFileConfigData(pUserFile, index);
        restoreUserFileGameData(pUserFile, index, pUserFile->isLastLoadedMario());
    }
}

void SaveDataHandleSequence::restoreUserFile(UserFile* pUserFile, int index, bool isPlayerMario) {
    if (pUserFile != nullptr) {
        restoreUserFileConfigData(pUserFile, index);
        restoreUserFileGameData(pUserFile, index, isPlayerMario);
        pUserFile->setLastLoadedMario(isPlayerMario);
    }
}

void SaveDataHandleSequence::backupCurrentUserFile() {
    if (mCurrentUserFile != nullptr && mBackupUserFile != nullptr) {
        *mBackupUserFile = *mCurrentUserFile;
    }
}

void SaveDataHandleSequence::exeNoOperation() {
    mIsActive = false;
    mIsConfirmRemind = false;
    mIsSaveAndQuitMsg = false;
    _2B = false;
}

void SaveDataHandleSequence::exeProcessing() {
    completeSequenceSuccess();
}

void SaveDataHandleSequence::exeSaveConfirm() {
    auto is_selected_yes = false;
    const auto* message_id = mIsSaveAndQuitMsg ? "System_Save07" : "System_Save00";
    if (!tryConfirm(message_id, &is_selected_yes)) {
        return;
    }

    if (is_selected_yes) {
        if (mWorkUserFile != nullptr) {
            mWorkUserFile->setCreated();
            mWorkUserFile->updateLastModified();
        }
        setNerve(&SaveDataHandleSequenceSave::sInstance);
    } else if (mIsConfirmRemind) {
        setNerve(&SaveDataHandleSequenceNoSaveConfirmRemind::sInstance);
    } else {
        _24 = 3;
        setNerve(&SaveDataHandleSequenceNoOperation::sInstance);
    }
}

void SaveDataHandleSequence::exeSave() {
    if (MR::isFirstStep(this)) {
        storeSysConfigToService();
        if (mWorkUserFile != nullptr) {
            mSaveDataHandler->storeUserFile(mWorkUserFile);
        }
    }

    if (trySave()) {
        setNerve(&SaveDataHandleSequenceSaveWindowDisappear::sInstance);
    }
}

void SaveDataHandleSequence::exeSaveWindowDisappear() {
    auto is_err = false;
    if (!trySaveWindowDisappear(&is_err) || is_err) {
        return;
    }

    _24 = 2;
    setNerve(&SaveDataHandleSequenceNoOperation::sInstance);
}

void SaveDataHandleSequence::exeSaveAllWithoutKeyWait() {
    if (MR::isFirstStep(this)) {
        storeSysConfigToService();
    }

    if (trySave()) {
        setNerve(&SaveDataHandleSequenceSaveAllWithoutKeyWaitDisappear::sInstance);
    }
}

void SaveDataHandleSequence::exeSaveAllWithoutKeyWaitDisappear() {
    auto is_err = false;
    if (!trySaveWindowDisappear(&is_err) || is_err) {
        return;
    }

    _24 = 2;
    setNerve(&SaveDataHandleSequenceNoOperation::sInstance);
}

void SaveDataHandleSequence::exeSaveAllWithoutWindow() {
    if (MR::isFirstStep(this)) {
        storeSysConfigToService();
    }

    auto is_err = false;
    if (!trySaveWithoutWindow(&is_err) || is_err) {
        return;
    }

    _24 = 2;
    setNerve(&SaveDataHandleSequenceNoOperation::sInstance);
}

void SaveDataHandleSequence::exePreLoad() {
    if (MR::isFirstStep(this)) {
        mSaveDataHandler->requestLoadSaveData();
    }

    if (mSaveDataHandler->isDone()) {
        setNerve(&SaveDataHandleSequencePreLoadDone::sInstance);
    }
}

void SaveDataHandleSequence::exePreLoadDone() {
    restoreSysConfigFile(mSysConfigFile);
    _24 = 2;
    setNerve(&SaveDataHandleSequenceNoOperation::sInstance);
}

void SaveDataHandleSequence::exeNoSaveConfirmRemind() {
    auto is_selected_yes = false;
    if (!tryConfirm("ConfirmEndGame", &is_selected_yes)) {
        return;
    }

    if (is_selected_yes) {
        _24 = 2;
        setNerve(&SaveDataHandleSequenceNoOperation::sInstance);
    } else {
        setNerve(&SaveDataHandleSequenceSaveConfirm::sInstance);
    }
}

GameDataHolder* SaveDataHandleSequence::getHolder() {
    return mCurrentUserFile != nullptr ? mCurrentUserFile->mGameDataHolder : nullptr;
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
    if (pUserFile != nullptr) {
        auto buffer = std::vector<u8>(SaveDataHandler::getEnoughtTempBufferSize());
        auto dataName = std::array<char, 16U>{};
        std::snprintf(dataName.data(), dataName.size(), "config%1d", index);
        mSaveDataHandler->restoreGameDataFile(dataName.data(), buffer.data(), static_cast<u32>(buffer.size()));
        pUserFile->loadFromConfigDataBinary(dataName.data(), buffer.data(), static_cast<u32>(buffer.size()));
    }
}

void SaveDataHandleSequence::restoreUserFileGameData(UserFile* pUserFile, int index, bool isPlayerMario) {
    if (pUserFile != nullptr) {
        auto buffer = std::vector<u8>(SaveDataHandler::getEnoughtTempBufferSize());
        auto dataName = std::array<char, 16U>{};
        std::snprintf(dataName.data(), dataName.size(), "%s%1d", isPlayerMario ? "mario" : "luigi", index);
        mSaveDataHandler->restoreGameDataFile(dataName.data(), buffer.data(), static_cast<u32>(buffer.size()));
        pUserFile->loadFromGameDataBinary(dataName.data(), buffer.data(), static_cast<u32>(buffer.size()));
        pUserFile->mIsPlayerMario = isPlayerMario;
    }
}

void SaveDataHandleSequence::restoreSysConfigFile(SysConfigFile* pSysConfigFile) {
    if (pSysConfigFile == nullptr) {
        return;
    }

    auto buffer = std::vector<u8>(SaveDataHandler::getEnoughtTempBufferSize());
    mSaveDataHandler->restoreGameDataFile("sysconf", buffer.data(), static_cast<u32>(buffer.size()));
    pSysConfigFile->loadFromDataBinary(buffer.data(), static_cast<u32>(buffer.size()));
}

void SaveDataHandleSequence::startInstantSequence() {
    _24 = 0;
    mIsActive = true;
    setNerve(&SaveDataHandleSequenceProcessing::sInstance);
}

void SaveDataHandleSequence::completeSequenceSuccess() {
    _24 = 2;
    mIsActive = false;
    setNerve(&SaveDataHandleSequenceNoOperation::sInstance);
}

void SaveDataHandleSequence::storeSysConfigToService() const {
    if (mSysConfigFile == nullptr) {
        return;
    }

    mSaveDataHandler->storeSysConfigFile(mSysConfigFile);
}

bool SaveDataHandleSequence::trySave() {
    if (MR::isFirstStep(this)) {
        mSaveDataHandler->requestSaveSaveData();
        if (mSysInfoWindowSave != nullptr) {
            mSysInfoWindowSave->appear("System_Save01", SysInfoWindow::Type_Blocking, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
        }
    }

    if (mSaveIcon != nullptr && MR::isDead(mSaveIcon) && (mSysInfoWindowSave == nullptr || mSysInfoWindowSave->isWait())) {
        mSaveIcon->appear();
    }

    if (MR::isGreaterStep(this, 20)) {
        MR::startSystemSE("SE_SY_LV_SAVING", -1, -1);
    }

    return MR::isGreaterStep(this, 120) && mSaveDataHandler->isDone();
}

bool SaveDataHandleSequence::trySaveWindowDisappear(bool* pIsErr) {
    if (MR::isFirstStep(this)) {
        if (mSaveIcon != nullptr) {
            mSaveIcon->kill();
        }
        if (mSysInfoWindowSave != nullptr) {
            mSysInfoWindowSave->disappear();
        }
    }

    if (mSysInfoWindowSave == nullptr || MR::isDead(mSysInfoWindowSave)) {
        return executeSaveFinish(pIsErr);
    }

    return false;
}

bool SaveDataHandleSequence::trySaveWithoutWindow(bool* pIsErr) {
    if (MR::isFirstStep(this)) {
        mSaveDataHandler->requestSaveSaveData();
    }

    if (mSaveDataHandler->isDone()) {
        return executeSaveFinish(pIsErr);
    }

    return false;
}

bool SaveDataHandleSequence::tryConfirm(const char* pSystemMessageId, bool* pIsSelectedYes) {
    if (MR::isFirstStep(this)) {
        _24 = 1;
        if (mSysInfoWindowConfirm != nullptr) {
            mSysInfoWindowConfirm->appear(pSystemMessageId, SysInfoWindow::Type_YesNo, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
        }
        MR::startSystemSE("SE_SY_SAVE_CONFIRM_INFO", -1, -1);
    }

    if (mSysInfoWindowConfirm == nullptr || MR::isDead(mSysInfoWindowConfirm)) {
        if (pIsSelectedYes != nullptr) {
            *pIsSelectedYes = mSysInfoWindowConfirm != nullptr && mSysInfoWindowConfirm->isSelectedYes();
        }
        return true;
    }

    return false;
}

bool SaveDataHandleSequence::tryProcessDoneKeyWait(const char* pSystemMessageId) {
    if (MR::isFirstStep(this) && mSysInfoWindowSave != nullptr) {
        mSysInfoWindowSave->appear(pSystemMessageId, SysInfoWindow::Type_Key, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
    }

    return mSysInfoWindowSave == nullptr || MR::isDead(mSysInfoWindowSave);
}

bool SaveDataHandleSequence::executeSaveFinish(bool* pIsErr) {
    if (mSaveDataHandler->getLastResultCode().isSuccess()) {
        if (pIsErr != nullptr) {
            *pIsErr = false;
        }
        MR::startSystemSE("SE_SY_SAVE_SUCCESS", -1, -1);
        return true;
    }

    if (pIsErr != nullptr) {
        *pIsErr = true;
    }
    _24 = 3;
    MR::startSystemSE("SE_SY_SAVE_FAILURE", -1, -1);
    setNerve(&SaveDataHandleSequenceNoOperation::sInstance);
    return true;
}

namespace smgpc::game {
    SaveDataHandleSequence& save_data_handle_sequence() {
        static auto sequence = SaveDataHandleSequence();
        static auto initialized = false;
        if (!initialized) {
            sequence.initAfterResourceLoaded();
            initialized = true;
        }
        return sequence;
    }
}  // namespace smgpc::game
