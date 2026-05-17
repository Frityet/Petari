#pragma once

#include "Game/System/NerveExecutor.hpp"

class GameDataHolder;
class SaveIcon;
class SaveDataHandler;
class SysConfigFile;
class SysInfoWindow;
class UserFile;

namespace MR {
    class FunctorBase;
}

class SaveDataHandleSequence : public NerveExecutor {
public:
    SaveDataHandleSequence();
    ~SaveDataHandleSequence() override;

    void initAfterResourceLoaded();
    void registerFunctorOnSaveSuccess(const MR::FunctorBase&);
    void registerFunctorJustBeforeSave(const MR::FunctorBase&);
    void update();
    void draw() const;
    void startPreLoad();
    void startCreateUserFile(int index);
    void startDeleteUserFile(int index);
    void startSave(bool isConfirmRemind, bool isSaveAndQuitMsg);
    void startSaveBackup(bool isConfirmRemind, bool isSaveAndQuitMsg);
    void startSaveAll();
    void startSaveTotalMailSize();
    void startLoad(int userFileIndex, bool isPlayerMario);
    void storeMiiOrIconId(int userFileIndex, const void* pMiiId, const u32* pIconId);
    void storeCopyUserFile(int indexDst, int indexSrc);
    bool tryNANDErrorSequence(s32 code);
    [[nodiscard]] bool isActive() const;
    [[nodiscard]] bool isPermitToReset() const;
    void prepareReset();
    [[nodiscard]] bool isPreparedReset() const;
    void restoreFromReset();
    [[nodiscard]] bool isInitializedGameDataHolder() const;
    void restoreUserFile(UserFile* pUserFile, int index);
    void restoreUserFile(UserFile* pUserFile, int index, bool isPlayerMario);
    void backupCurrentUserFile();
    void exeNoOperation();
    void exeProcessing();
    void exeSaveConfirm();
    void exeSave();
    void exeSaveWindowDisappear();
    void exeSaveAllWithoutKeyWait();
    void exeSaveAllWithoutKeyWaitDisappear();
    void exeSaveAllWithoutWindow();
    void exePreLoad();
    void exePreLoadDone();
    void exeNoSaveConfirmRemind();
    [[nodiscard]] GameDataHolder* getHolder();
    [[nodiscard]] SysConfigFile* getSysConfigFile();
    [[nodiscard]] UserFile* getCurrentUserFile();
    [[nodiscard]] UserFile* getBackupUserFile();
    void restoreUserFileConfigData(UserFile* pUserFile, int index);
    void restoreUserFileGameData(UserFile* pUserFile, int index, bool isPlayerMario);
    void restoreSysConfigFile(SysConfigFile* pSysConfigFile);

    /* 0x08 */ SysConfigFile* mSysConfigFile;
    /* 0x0C */ UserFile* mCurrentUserFile;
    /* 0x10 */ UserFile* mBackupUserFile;
    /* 0x14 */ SaveDataHandler* mSaveDataHandler;
    /* 0x24 */ s32 _24;
    /* 0x28 */ bool mIsConfirmRemind;
    /* 0x29 */ bool mIsSaveAndQuitMsg;
    /* 0x2A */ bool _2A;
    /* 0x2B */ bool _2B;
    /* 0x2C */ bool _2C;
    /* 0x30 */ UserFile* mWorkUserFile;

private:
    void startInstantSequence();
    void completeSequenceSuccess();
    void storeSysConfigToService() const;
    [[nodiscard]] bool trySave();
    [[nodiscard]] bool trySaveWindowDisappear(bool* pIsErr);
    [[nodiscard]] bool trySaveWithoutWindow(bool* pIsErr);
    [[nodiscard]] bool tryConfirm(const char* pSystemMessageId, bool* pIsSelectedYes);
    [[nodiscard]] bool tryProcessDoneKeyWait(const char* pSystemMessageId);
    [[nodiscard]] bool executeSaveFinish(bool* pIsErr);

    bool mIsActive = false;
    SysInfoWindow* mSysInfoWindowConfirm = nullptr;
    SysInfoWindow* mSysInfoWindowSave = nullptr;
    SaveIcon* mSaveIcon = nullptr;
};

namespace smgpc::game {
    SaveDataHandleSequence& save_data_handle_sequence();
}
