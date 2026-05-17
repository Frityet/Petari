#pragma once

#include <revolution.h>

class UserFile;

namespace GameSequenceFunction {
    void startPreLoadSaveDataSequence();
    void startTotalMailSizeSaveSequence();
    void startGameDataLoadSequence(int userFileIndex, bool isPlayerMario);
    void startCreateUserFileSequence(int userFileIndex);
    void startDeleteUserFileSequence(int userFileIndex);
    void startCopyUserFileSequence(int userFileIndexDst, int userFileIndexSrc);
    void startSetMiiOrIconIdUserFileSequence(int userFileIndex, const void* pMiiId, const u32* pIconId);
    void storeMiiOrIconIdUserFileSequence(int userFileIndex, const void* pMiiId, const u32* pIconId);
    void storeCopyUserFileSequence(int userFileIndexDst, int userFileIndexSrc);
    void startSaveAllUserFileSequence();
    [[nodiscard]] bool isActiveSaveDataHandleSequence();
    [[nodiscard]] bool isSuccessSaveDataHandleSequence();
    void restoreUserFile(UserFile* pUserFile, int userFileIndex);
    void restoreUserFile(UserFile* pUserFile, int userFileIndex, bool isPlayerMario);
    void tryNANDErrorSequence(s32 code);
    void reserveUserName(const wchar_t* pUserName);
    void startGameDataSaveSequence(bool isConfirmRemind, bool isSaveAndQuitMsg);
}  // namespace GameSequenceFunction
