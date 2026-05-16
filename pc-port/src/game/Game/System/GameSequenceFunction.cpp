#include "Game/System/GameSequenceFunction.hpp"

#include "Game/System/SaveDataHandleSequence.hpp"

namespace {

SaveDataHandleSequence *sSaveDataHandleSequence = nullptr;

[[nodiscard]] SaveDataHandleSequence *getSaveDataHandleSequence() {
    return sSaveDataHandleSequence;
}

}  // namespace

namespace GameSequenceFunction {

void setHostSaveDataHandleSequence(SaveDataHandleSequence *pSequence) {
    sSaveDataHandleSequence = pSequence;
}

void startPreLoadSaveDataSequence() {
    if (auto *sequence = getSaveDataHandleSequence()) {
        sequence->startPreLoad();
    }
}

void startGameDataLoadSequence(int userFileIndex, bool isPlayerMario) {
    if (auto *sequence = getSaveDataHandleSequence()) {
        sequence->startLoad(userFileIndex, isPlayerMario);
    }
}

void startCreateUserFileSequence(int index) {
    if (auto *sequence = getSaveDataHandleSequence()) {
        sequence->startCreateUserFile(index);
    }
}

void startDeleteUserFileSequence(int index) {
    if (auto *sequence = getSaveDataHandleSequence()) {
        sequence->startDeleteUserFile(index);
    }
}

void startCopyUserFileSequence(int indexDst, int indexSrc) {
    if (auto *sequence = getSaveDataHandleSequence()) {
        sequence->storeCopyUserFile(indexDst, indexSrc);
        sequence->startSaveAll();
    }
}

void startSetMiiOrIconIdUserFileSequence(int userFileIndex, const void *pMiiId, const u32 *pIconId) {
    if (auto *sequence = getSaveDataHandleSequence()) {
        sequence->storeMiiOrIconId(userFileIndex, pMiiId, pIconId);
        sequence->startSaveAll();
    }
}

void storeMiiOrIconIdUserFileSequence(int userFileIndex, const void *pMiiId, const u32 *pIconId) {
    if (auto *sequence = getSaveDataHandleSequence()) {
        sequence->storeMiiOrIconId(userFileIndex, pMiiId, pIconId);
    }
}

void storeCopyUserFileSequence(int indexDst, int indexSrc) {
    if (auto *sequence = getSaveDataHandleSequence()) {
        sequence->storeCopyUserFile(indexDst, indexSrc);
    }
}

void startSaveAllUserFileSequence() {
    if (auto *sequence = getSaveDataHandleSequence()) {
        sequence->startSaveAll();
    }
}

bool isActiveSaveDataHandleSequence() {
    if (auto *sequence = getSaveDataHandleSequence()) {
        return sequence->isActive();
    }
    return false;
}

bool isSuccessSaveDataHandleSequence() {
    if (auto *sequence = getSaveDataHandleSequence()) {
        return sequence->_24 == 2;
    }
    return false;
}

void restoreUserFile(UserFile *pUserFile, int index) {
    if (auto *sequence = getSaveDataHandleSequence()) {
        sequence->restoreUserFile(pUserFile, index);
    }
}

void restoreUserFile(UserFile *pUserFile, int index, bool isPlayerMario) {
    if (auto *sequence = getSaveDataHandleSequence()) {
        sequence->restoreUserFile(pUserFile, index, isPlayerMario);
    }
}

}  // namespace GameSequenceFunction
