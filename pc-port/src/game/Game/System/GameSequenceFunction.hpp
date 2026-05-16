#pragma once

#include "compat/Types.hpp"

class SaveDataHandleSequence;
class UserFile;

namespace GameSequenceFunction {

void setHostSaveDataHandleSequence(SaveDataHandleSequence *);
void startPreLoadSaveDataSequence();
void startGameDataLoadSequence(int, bool);
void startCreateUserFileSequence(int);
void startDeleteUserFileSequence(int);
void startCopyUserFileSequence(int, int);
void startSetMiiOrIconIdUserFileSequence(int, const void *, const u32 *);
void storeMiiOrIconIdUserFileSequence(int, const void *, const u32 *);
void storeCopyUserFileSequence(int, int);
void startSaveAllUserFileSequence();
[[nodiscard]] bool isActiveSaveDataHandleSequence();
[[nodiscard]] bool isSuccessSaveDataHandleSequence();
void restoreUserFile(UserFile *, int);
void restoreUserFile(UserFile *, int, bool);

}  // namespace GameSequenceFunction
