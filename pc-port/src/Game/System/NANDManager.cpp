#include "Game/System/NANDManager.hpp"

s32 NANDResultCode::getCode() const {
    return mCode;
}

bool NANDResultCode::isSuccess() const {
    return getCode() == NAND_RESULT_OK;
}

bool NANDResultCode::isSaveDataCorrupted() const {
    return getCode() == NAND_RESULT_ECC_CRIT || getCode() == NAND_RESULT_AUTHENTICATION;
}

bool NANDResultCode::isNANDCorrupted() const {
    return getCode() == NAND_RESULT_CORRUPT;
}

bool NANDResultCode::isMaxBlocks() const {
    return getCode() == NAND_RESULT_MAXBLOCKS;
}

bool NANDResultCode::isMaxFiles() const {
    return getCode() == NAND_RESULT_MAXFILES;
}

bool NANDResultCode::isNoExistFile() const {
    return getCode() == NAND_RESULT_NOEXISTS;
}

bool NANDResultCode::isBusyOrAllocFailed() const {
    return getCode() == NAND_RESULT_BUSY || getCode() == NAND_RESULT_ALLOC_FAILED;
}

bool NANDResultCode::isUnknown() const {
    return getCode() == NAND_RESULT_UNKNOWN;
}

namespace MR {
    void addRequestToNANDManager(NANDRequestInfo*) {
    }
}  // namespace MR
