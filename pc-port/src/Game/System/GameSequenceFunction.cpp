#include "Game/System/GameSequenceFunction.hpp"

#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/System/UserFile.hpp"

namespace {
    SaveDataHandleSequence& getSaveDataHandleSequence() {
        return smgpc::game::save_data_handle_sequence();
    }
}  // namespace

namespace GameSequenceFunction {
    void startPreLoadSaveDataSequence() {
        getSaveDataHandleSequence().startPreLoad();
    }

    void startTotalMailSizeSaveSequence() {
        getSaveDataHandleSequence().startSaveTotalMailSize();
    }

    void startGameDataLoadSequence(int userFileIndex, bool isPlayerMario) {
        getSaveDataHandleSequence().startLoad(userFileIndex, isPlayerMario);
    }

    void startCreateUserFileSequence(int userFileIndex) {
        getSaveDataHandleSequence().startCreateUserFile(userFileIndex);
    }

    void startDeleteUserFileSequence(int userFileIndex) {
        getSaveDataHandleSequence().startDeleteUserFile(userFileIndex);
    }

    void startCopyUserFileSequence(int userFileIndexDst, int userFileIndexSrc) {
        storeCopyUserFileSequence(userFileIndexDst, userFileIndexSrc);
        startSaveAllUserFileSequence();
    }

    void startSetMiiOrIconIdUserFileSequence(int userFileIndex, const void* pMiiId, const u32* pIconId) {
        storeMiiOrIconIdUserFileSequence(userFileIndex, pMiiId, pIconId);
        startSaveAllUserFileSequence();
    }

    void storeMiiOrIconIdUserFileSequence(int userFileIndex, const void* pMiiId, const u32* pIconId) {
        getSaveDataHandleSequence().storeMiiOrIconId(userFileIndex, pMiiId, pIconId);
    }

    void storeCopyUserFileSequence(int userFileIndexDst, int userFileIndexSrc) {
        getSaveDataHandleSequence().storeCopyUserFile(userFileIndexDst, userFileIndexSrc);
    }

    void startSaveAllUserFileSequence() {
        getSaveDataHandleSequence().startSaveAll();
    }

    bool isActiveSaveDataHandleSequence() {
        return getSaveDataHandleSequence().isActive();
    }

    bool isSuccessSaveDataHandleSequence() {
        return getSaveDataHandleSequence()._24 == 2;
    }

    void restoreUserFile(UserFile* pUserFile, int userFileIndex) {
        getSaveDataHandleSequence().restoreUserFile(pUserFile, userFileIndex);
    }

    void restoreUserFile(UserFile* pUserFile, int userFileIndex, bool isPlayerMario) {
        getSaveDataHandleSequence().restoreUserFile(pUserFile, userFileIndex, isPlayerMario);
    }

    void tryNANDErrorSequence(s32 code) {
        getSaveDataHandleSequence().tryNANDErrorSequence(code);
    }

    void reserveUserName(const wchar_t* pUserName) {
        if (auto* file = getSaveDataHandleSequence().getCurrentUserFile()) {
            file->setUserName(pUserName);
        }
    }

    void startGameDataSaveSequence(bool isConfirmRemind, bool isSaveAndQuitMsg) {
        getSaveDataHandleSequence().startSave(isConfirmRemind, isSaveAndQuitMsg);
    }
}  // namespace GameSequenceFunction
