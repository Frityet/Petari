#include "Game/System/NANDManager.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "Game/compat/RuntimeContext.hpp"

namespace {
    enum NANDRequestType {
        NANDRequestType_Move = 0,
        NANDRequestType_Delete = 1,
        NANDRequestType_Write = 2,
        NANDRequestType_Read = 3,
        NANDRequestType_Check = 4,
    };

    [[nodiscard]] std::string normalize_nand_path(const char* pPath) {
        auto path = std::string(pPath != nullptr ? pPath : "");
        while (!path.empty() && path.front() == '/') {
            path.erase(path.begin());
        }

        if (path.starts_with("tmp/")) {
            return path;
        }

        if (const auto slash = path.find_last_of('/'); slash != std::string::npos) {
            path.erase(0, slash + 1U);
        }

        return path;
    }

    [[nodiscard]] smgpc::game::RuntimeContext* active_runtime() {
        return smgpc::game::RuntimeContext::try_instance();
    }

    void complete_request(NANDRequestInfo& request, s32 result) {
        request.mResult = result;
        request._40 = 0U;
    }

    void execute_check(NANDRequestInfo& request) {
        if (request.mCheckAnswer != nullptr) {
            *request.mCheckAnswer = 0U;
        }
        complete_request(request, NAND_RESULT_OK);
    }

    void execute_read(NANDRequestInfo& request) {
        const auto path = normalize_nand_path(request.mPath);
        auto* runtime = active_runtime();
        if (runtime == nullptr || request.mReadBuf == nullptr || path.empty()) {
            complete_request(request, NAND_RESULT_INVALID);
            return;
        }

        auto bytes = runtime->save_data().read_nand_file(path);
        if (!bytes.has_value()) {
            if (request.mReadLength != nullptr) {
                *request.mReadLength = 0U;
            }
            complete_request(request, NAND_RESULT_NOEXISTS);
            return;
        }

        const auto copy_size = std::min<std::size_t>(request.mFsBlock, bytes->size());
        std::memcpy(request.mReadBuf, bytes->data(), copy_size);
        if (request.mReadLength != nullptr) {
            *request.mReadLength = static_cast<u32>(copy_size);
        }
        complete_request(request, NAND_RESULT_OK);
    }

    void execute_write(NANDRequestInfo& request) {
        const auto path = normalize_nand_path(request.mPath);
        auto* runtime = active_runtime();
        if (runtime == nullptr || request.mWriteBuf == nullptr || path.empty()) {
            complete_request(request, NAND_RESULT_INVALID);
            return;
        }

        auto bytes = std::vector<u8>(request.mFsBlock);
        std::memcpy(bytes.data(), request.mWriteBuf, bytes.size());
        runtime->save_data().write_nand_file(path, bytes);
        complete_request(request, NAND_RESULT_OK);
    }

    void execute_delete(NANDRequestInfo& request) {
        const auto path = normalize_nand_path(request.mPath);
        auto* runtime = active_runtime();
        if (runtime == nullptr || path.empty()) {
            complete_request(request, NAND_RESULT_INVALID);
            return;
        }

        complete_request(request, runtime->save_data().erase(path) ? NAND_RESULT_OK : NAND_RESULT_NOEXISTS);
    }

    void execute_move(NANDRequestInfo& request) {
        const auto source = normalize_nand_path(request.mPath);
        auto* runtime = active_runtime();
        if (runtime == nullptr || source.empty()) {
            complete_request(request, NAND_RESULT_INVALID);
            return;
        }

        const auto bytes = runtime->save_data().read_nand_file(source);
        if (!bytes.has_value()) {
            complete_request(request, NAND_RESULT_NOEXISTS);
            return;
        }

        auto destination = std::string(request.mMoveDestDir != nullptr ? request.mMoveDestDir : "");
        while (!destination.empty() && destination.front() == '/') {
            destination.erase(destination.begin());
        }
        if (destination.empty() || destination.ends_with('/')) {
            const auto slash = source.find_last_of('/');
            destination += slash == std::string::npos ? source : source.substr(slash + 1U);
        }
        destination = normalize_nand_path(destination.c_str());
        runtime->save_data().write_nand_file(destination, *bytes);
        runtime->save_data().erase(source);
        complete_request(request, NAND_RESULT_OK);
    }

    void execute_request(NANDRequestInfo& request) {
        switch (request.mType) {
        case NANDRequestType_Move:
            execute_move(request);
            break;
        case NANDRequestType_Delete:
            execute_delete(request);
            break;
        case NANDRequestType_Write:
            execute_write(request);
            break;
        case NANDRequestType_Read:
            execute_read(request);
            break;
        case NANDRequestType_Check:
            execute_check(request);
            break;
        default:
            complete_request(request, NAND_RESULT_INVALID);
            break;
        }
    }
}  // namespace

NANDRequestInfo::NANDRequestInfo() {
    init();
}

void NANDRequestInfo::init() {
    _40 = 0U;
    mType = NANDRequestType_Check;
    mResult = NAND_RESULT_OK;
    mPath[0] = '\0';
    mReadBuf = nullptr;
    mReadLength = nullptr;
    _54 = nullptr;
    mPermission = 0U;
    mAttribute = 0U;
    mFsBlock = 0U;
    mInode = 0U;
}

bool NANDRequestInfo::isDone() const {
    return _40 == 0U;
}

const char* NANDRequestInfo::setMove(const char* pPath, const char* pDestDir) {
    init();
    mType = NANDRequestType_Move;
    std::snprintf(mPath, sizeof(mPath), "%s", pPath != nullptr ? pPath : "");
    mMoveDestDir = pDestDir;
    return mPath;
}

const char* NANDRequestInfo::setWriteSeq(const char* pName, const void* pBuf, u32 fsBlock, u8 permission, u8 attr) {
    init();
    mType = NANDRequestType_Write;
    mWriteBuf = pBuf;
    mFsBlock = fsBlock;
    mPermission = permission;
    mAttribute = attr;
    std::snprintf(mPath, sizeof(mPath), "%s", pName != nullptr ? pName : "");
    return mPath;
}

const char* NANDRequestInfo::setReadSeq(const char* pName, void* pBuf, u32 fsBlock, u32* pLength) {
    init();
    mType = NANDRequestType_Read;
    mReadBuf = pBuf;
    mFsBlock = fsBlock;
    mReadLength = pLength;
    std::snprintf(mPath, sizeof(mPath), "%s", pName != nullptr ? pName : "");
    return mPath;
}

const char* NANDRequestInfo::setCheck(u32 fsBlock, u32 inode, u32* pAnswer) {
    init();
    mType = NANDRequestType_Check;
    mFsBlock = fsBlock;
    mInode = inode;
    mCheckAnswer = pAnswer;
    return mPath;
}

const char* NANDRequestInfo::setDelete(const char* pName) {
    init();
    mType = NANDRequestType_Delete;
    std::snprintf(mPath, sizeof(mPath), "%s", pName != nullptr ? pName : "");
    return mPath;
}

NANDManager::NANDManager() = default;

bool NANDManager::addRequest(NANDRequestInfo* pRequestInfo) {
    if (pRequestInfo == nullptr) {
        return false;
    }

    pRequestInfo->_40 = 1U;
    execute_request(*pRequestInfo);
    return true;
}

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
    void addRequestToNANDManager(NANDRequestInfo* pRequestInfo) {
        auto manager = NANDManager();
        manager.addRequest(pRequestInfo);
    }
}  // namespace MR
