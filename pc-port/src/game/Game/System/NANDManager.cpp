#include "Game/System/NANDManager.hpp"

#include "compat/HostNandStorage.hpp"

#include <cstdio>

NANDRequestInfo::NANDRequestInfo() {
    init();
}

void NANDRequestInfo::init() {
    _40 = 0U;
    mType = 4;
    mResult = NAND_RESULT_OK;
    mPath[0] = '\0';
    mFsBlock = 0U;
    mReadBuf = nullptr;
    mReadLength = nullptr;
    _54 = nullptr;
    mPermission = 0U;
    mAttribute = 0U;
    mInode = 0U;
}

bool NANDRequestInfo::isDone() const {
    return _40 == 0U;
}

const char *NANDRequestInfo::setMove(const char *pPath, const char *pDestDir) {
    init();
    mType = 0;
    std::snprintf(mPath, sizeof(mPath), "%s", pPath);
    mMoveDestDir = pDestDir;
    return mPath;
}

const char *NANDRequestInfo::setWriteSeq(const char *pName, const void *pBuf, u32 fsBlock, u8 permission, u8 attr) {
    init();
    mWriteBuf = pBuf;
    mType = 2;
    mFsBlock = fsBlock;
    mPermission = permission;
    mAttribute = attr;
    std::snprintf(mPath, sizeof(mPath), "%s", pName);
    return mPath;
}

const char *NANDRequestInfo::setReadSeq(const char *pName, void *pBuf, u32 fsBlock, u32 *pLength) {
    init();
    mReadBuf = pBuf;
    mType = 3;
    mFsBlock = fsBlock;
    mReadLength = pLength;
    std::snprintf(mPath, sizeof(mPath), "%s", pName);
    return mPath;
}

const char *NANDRequestInfo::setCheck(u32 fsBlock, u32 inode, u32 *pAnswer) {
    init();
    mFsBlock = fsBlock;
    mType = 4;
    mInode = inode;
    mCheckAnswer = pAnswer;
    return mPath;
}

const char *NANDRequestInfo::setDelete(const char *pName) {
    init();
    mType = 1;
    std::snprintf(mPath, sizeof(mPath), "%s", pName);
    return mPath;
}

NANDManager::NANDManager() = default;

bool NANDManager::addRequest(NANDRequestInfo *pRequestInfo) {
    MR::addRequestToNANDManager(pRequestInfo);
    return true;
}

s32 NANDResultCode::getCode() const {
    return mCode;
}

bool NANDResultCode::isSuccess() const {
    return mCode == NAND_RESULT_OK;
}

bool NANDResultCode::isSaveDataCorrupted() const {
    return mCode == NAND_RESULT_ECC_CRIT || mCode == NAND_RESULT_AUTHENTICATION;
}

bool NANDResultCode::isNANDCorrupted() const {
    return mCode == NAND_RESULT_CORRUPT;
}

bool NANDResultCode::isMaxBlocks() const {
    return mCode == NAND_RESULT_MAXBLOCKS;
}

bool NANDResultCode::isMaxFiles() const {
    return mCode == NAND_RESULT_MAXFILES;
}

bool NANDResultCode::isNoExistFile() const {
    return mCode == NAND_RESULT_NOEXISTS;
}

bool NANDResultCode::isBusyOrAllocFailed() const {
    return mCode == NAND_RESULT_BUSY || mCode == NAND_RESULT_ALLOC_FAILED;
}

bool NANDResultCode::isUnknown() const {
    return mCode == NAND_RESULT_UNKNOWN;
}

namespace MR {

void addRequestToNANDManager(NANDRequestInfo *pRequestInfo) {
    if (pRequestInfo == nullptr) {
        return;
    }

    auto &storage = smgpc::game::compat::HostNandStorage::instance();
    pRequestInfo->_40 = 2U;
    switch (pRequestInfo->mType) {
    case 0:
        pRequestInfo->mResult = storage.move(pRequestInfo->mPath, pRequestInfo->mMoveDestDir);
        break;
    case 1:
        pRequestInfo->mResult = storage.remove(pRequestInfo->mPath);
        break;
    case 2:
        pRequestInfo->mResult = storage.write(pRequestInfo->mPath, pRequestInfo->mWriteBuf, pRequestInfo->mFsBlock);
        break;
    case 3:
        pRequestInfo->mResult = storage.read(pRequestInfo->mPath, pRequestInfo->mReadBuf, pRequestInfo->mFsBlock, pRequestInfo->mReadLength);
        break;
    case 4:
        pRequestInfo->mResult = storage.check(pRequestInfo->mFsBlock, pRequestInfo->mInode, pRequestInfo->mCheckAnswer);
        break;
    default:
        pRequestInfo->mResult = NAND_RESULT_INVALID;
        break;
    }
    pRequestInfo->_40 = 0U;

    if (pRequestInfo->_54 != nullptr) {
        pRequestInfo->_54(pRequestInfo);
    }
}

}  // namespace MR
