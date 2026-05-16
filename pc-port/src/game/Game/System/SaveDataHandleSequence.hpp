#pragma once

#include "Game/System/NerveExecutor.hpp"

class GameDataHolder;
class SaveDataHandler;
class SaveIcon;
class SysConfigFile;
class SysInfoWindow;
class UserFile;

namespace smgpc::render::layout {
class LayoutDrawList;
}

class SaveDataHandleSequence : public NerveExecutor {
public:
    SaveDataHandleSequence();
    ~SaveDataHandleSequence() override;

    void initAfterResourceLoaded();
    void update();
    void draw() const;
    void appendDrawCommands(smgpc::render::layout::LayoutDrawList*) const;
    void startPreLoad();
    void startCreateUserFile(int);
    void startDeleteUserFile(int);
    void startSave(bool, bool);
    void startSaveBackup(bool, bool);
    void startSaveAll();
    void startSaveTotalMailSize();
    void startLoad(int, bool);
    void storeMiiOrIconId(int, const void*, const u32*);
    void storeCopyUserFile(int, int);
    bool tryNANDErrorSequence(s32);
    bool isActive() const;
    bool isPermitToReset() const;
    void prepareReset();
    bool isPreparedReset() const;
    void restoreFromReset();
    bool isInitializedGameDataHolder() const;
    void restoreUserFile(UserFile*, int);
    void restoreUserFile(UserFile*, int, bool);
    void backupCurrentUserFile();
    void exeNoOperation();
    void exeCheckEnableToCreate();
    void exeSaveAllWithoutKeyWait();
    void exeSaveAllWithoutKeyWaitDisappear();
    void exePreLoad();
    void exePreLoadDone();
    GameDataHolder* getHolder();
    SysConfigFile* getSysConfigFile();
    UserFile* getCurrentUserFile();
    UserFile* getBackupUserFile();
    void restoreUserFileConfigData(UserFile*, int);
    void restoreUserFileGameData(UserFile*, int, bool);
    void restoreSysConfigFile(SysConfigFile*);
    bool trySave();
    bool trySaveWindowDisappear(bool*);

    /* 0x08 */ SysConfigFile* mSysConfigFile;
    /* 0x0C */ UserFile* mCurrentUserFile;
    /* 0x10 */ UserFile* mBackupUserFile;
    /* 0x14 */ SaveDataHandler* mSaveDataHandler;
    /* 0x18 */ SysInfoWindow* mSysInfoWindowSave;
    /* 0x1C */ s32 _24;
    /* 0x20 */ u8* mTempBuffer;
    /* 0x24 */ SaveIcon* mSaveIcon;
    /* 0x28 */ bool mInitialized;
};
