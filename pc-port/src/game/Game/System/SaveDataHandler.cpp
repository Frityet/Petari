#include "Game/System/SaveDataHandler.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/System/SaveDataBannerCreator.hpp"
#include "compat/SaveDataEndian.hpp"
#include "Game/System/SysConfigFile.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/NerveUtil.hpp"

#include <array>
#include <cstdio>
#include <cstring>

namespace {

NEW_NERVE(SaveDataHandlerWait, SaveDataHandler, Wait);
NEW_NERVE(SaveDataHandlerProcessing, SaveDataHandler, Processing);
NEW_NERVE(SaveDataHandlerSaveProcessingGameData, SaveDataHandler, SaveProcessingGameData);
NEW_NERVE(SaveDataHandlerSaveProcessingBanner, SaveDataHandler, SaveProcessingBanner);
NEW_NERVE(SaveDataHandlerRemoveProcessingBanner, SaveDataHandler, RemoveProcessingBanner);
NEW_NERVE(SaveDataHandlerRemoveProcessingGameData, SaveDataHandler, RemoveProcessingGameData);

constexpr const char *GAME_DATA_FILE = "GameData.bin";
constexpr const char *BANNER_FILE = "banner.bin";
constexpr u32 SAVE_IMAGE_BUFFER_SIZE = 0x10000U;
constexpr u32 SAVE_FILE_HEADER_SIZE = 0x10U;
constexpr u32 SAVE_FILE_INFO_SIZE = 0x10U;
constexpr u32 SAVE_FILE_INFO_NAME_SIZE = 0x0CU;
constexpr u32 SAVE_FILE_DATA_START = 0x140U;
constexpr u32 SAVE_FILE_COUNT = 19U;
constexpr u32 SAVE_FILE_VERSION = 2U;
constexpr u32 SAVE_GAME_DATA_SIZE = 0xF80U;
constexpr u32 SAVE_CONFIG_DATA_SIZE = 0x60U;
constexpr u32 SAVE_SYSCONF_DATA_SIZE = 0x80U;

struct SaveFileSpec {
    const char *name;
    u32 size;
};

constexpr std::array<SaveFileSpec, 3> USER_FILE_SPECS {{
    {"mario", SAVE_GAME_DATA_SIZE},
    {"luigi", SAVE_GAME_DATA_SIZE},
    {"config", SAVE_CONFIG_DATA_SIZE},
}};
constexpr SaveFileSpec SYSTEM_FILE_SPEC {"sysconf", SAVE_SYSCONF_DATA_SIZE};

[[nodiscard]] u32 align32(u32 value) {
    return (value + 0x1FU) & ~0x1FU;
}

[[nodiscard]] u32 read_header_checksum(const u8 *pData) {
    return SaveDataEndian::read_u32(pData);
}

void write_header_checksum(u8 *pData, u32 value) {
    SaveDataEndian::write_u32(pData, value);
}

[[nodiscard]] u32 read_header_version(const u8 *pData) {
    return SaveDataEndian::read_u32(pData + 4U);
}

void write_header_version(u8 *pData, u32 value) {
    SaveDataEndian::write_u32(pData + 4U, value);
}

[[nodiscard]] u32 read_header_file_count(const u8 *pData) {
    return SaveDataEndian::read_u32(pData + 8U);
}

void write_header_file_count(u8 *pData, u32 value) {
    SaveDataEndian::write_u32(pData + 8U, value);
}

[[nodiscard]] u32 read_header_size(const u8 *pData) {
    return SaveDataEndian::read_u32(pData + 0x0CU);
}

void write_header_size(u8 *pData, u32 value) {
    SaveDataEndian::write_u32(pData + 0x0CU, value);
}

[[nodiscard]] u8 *file_info(u8 *pData, u32 index) {
    return pData + SAVE_FILE_HEADER_SIZE + index * SAVE_FILE_INFO_SIZE;
}

[[nodiscard]] const u8 *file_info(const u8 *pData, u32 index) {
    return pData + SAVE_FILE_HEADER_SIZE + index * SAVE_FILE_INFO_SIZE;
}

[[nodiscard]] u32 read_file_offset(const u8 *pInfo) {
    return SaveDataEndian::read_u32(pInfo + 0x0CU);
}

void write_file_offset(u8 *pInfo, u32 value) {
    SaveDataEndian::write_u32(pInfo + 0x0CU, value);
}

void write_file_name(u8 *pInfo, const char *pName) {
    MR::zeroMemory(pInfo, SAVE_FILE_INFO_NAME_SIZE);
    std::snprintf(reinterpret_cast<char *>(pInfo), SAVE_FILE_INFO_NAME_SIZE, "%s", pName);
}

[[nodiscard]] bool file_name_equals(const u8 *pInfo, const char *pName) {
    return std::strncmp(reinterpret_cast<const char *>(pInfo), pName, SAVE_FILE_INFO_NAME_SIZE) == 0;
}

class SaveDataFileAccessor {
public:
    explicit SaveDataFileAccessor(u8 *pData)
        : mData(pData) {
    }

    [[nodiscard]] u8 *getHeader() const {
        return mData;
    }

    [[nodiscard]] u8 *getFileInfo(int index) const {
        return file_info(mData, static_cast<u32>(index));
    }

    struct UserFileInfo {
        u8 *data {};
        u32 size {};
        u8 type {1U};
    };

    void makeUserFileInfo(UserFileInfo *pInfo, const char *pName) const {
        pInfo->data = nullptr;
        pInfo->size = 0U;
        pInfo->type = 1U;

        const u32 fileCount = read_header_file_count(mData);
        for (u32 index = 0; index < fileCount; ++index) {
            const u8 *info = file_info(mData, index);
            if (!file_name_equals(info, pName)) {
                continue;
            }

            const u32 offset = read_file_offset(info);
            const u32 endOffset = index + 1U == fileCount
                ? read_header_size(mData)
                : read_file_offset(file_info(mData, index + 1U));
            pInfo->data = mData + offset;
            pInfo->size = endOffset - offset;

            const auto *name = reinterpret_cast<const char *>(info);
            if (std::strstr(name, "mario") != nullptr || std::strstr(name, "luigi") != nullptr) {
                pInfo->type = 0U;
            }
            if (std::strstr(name, "sysconf") != nullptr) {
                pInfo->type = 2U;
            }
            return;
        }
    }

private:
    u8 *mData;
};

}  // namespace

SaveDataHandler::SaveDataHandler(const SysConfigFile *pSysConfigFile, const UserFile *pUserFile)
    : NerveExecutor("SaveDataHandler"),
      mNANDRequestInfo(new NANDRequestInfo()),
      mReadLength(0U),
      mCheckAnswer(0U),
      mNANDCommunicationBuffer(nullptr),
      mSaveDataBuffer(nullptr),
      mBannerCreator(nullptr) {
    createCommunicationBuffer();
    resetSaveData(mSaveDataBuffer);
    initializeAllFileInSaveData(mSaveDataBuffer, pSysConfigFile, pUserFile);
    mBannerCreator = new SaveDataBannerCreator();
    initNerve(&SaveDataHandlerWait::sInstance);
}

SaveDataHandler::~SaveDataHandler() {
    delete mNANDRequestInfo;
    delete[] mNANDCommunicationBuffer;
    delete[] mSaveDataBuffer;
    delete mBannerCreator;
}

void SaveDataHandler::update() {
    updateNerve();
    mBannerCreator->updateNerve();
}

void SaveDataHandler::requestCheckEnableToCreate() {
    mNANDRequestInfo->setCheck(5U, 2U, &mCheckAnswer);
    MR::addRequestToNANDManager(mNANDRequestInfo);
    setNerve(&SaveDataHandlerProcessing::sInstance);
}

void SaveDataHandler::requestLoadSaveData() {
    MR::zeroMemory(mNANDCommunicationBuffer, SAVE_IMAGE_BUFFER_SIZE);
    mNANDRequestInfo->setReadSeq(GAME_DATA_FILE, mNANDCommunicationBuffer, SAVE_IMAGE_BUFFER_SIZE, &mReadLength);
    MR::addRequestToNANDManager(mNANDRequestInfo);
    setNerve(&SaveDataHandlerProcessing::sInstance);
}

bool SaveDataHandler::requestVerifyAfterLoadGameDataFile() {
    if (!isCorrectFileHeader(mNANDCommunicationBuffer)) {
        return false;
    }

    const u32 fileSize = read_header_size(mNANDCommunicationBuffer);
    if (mReadLength != align32(fileSize)) {
        return false;
    }

    const u32 checksum = MR::calcCheckSum(mNANDCommunicationBuffer + 4U, fileSize - 4U);
    if (read_header_checksum(mNANDCommunicationBuffer) != checksum) {
        return false;
    }

    copySaveDataEachFile(mSaveDataBuffer, mNANDCommunicationBuffer);
    return true;
}

void SaveDataHandler::initializeUserFileMemory(int index, const UserFile *pUserFile) {
    SaveDataFileAccessor accessor(mSaveDataBuffer);
    for (const auto &spec : USER_FILE_SPECS) {
        char name[16];
        std::snprintf(name, sizeof(name), "%s%1d", spec.name, index);

        SaveDataFileAccessor::UserFileInfo info {};
        accessor.makeUserFileInfo(&info, name);
        if (info.data == nullptr) {
            continue;
        }

        if (info.type == 0U) {
            pUserFile->makeGameDataBinary(info.data, info.size);
        } else if (info.type == 1U) {
            pUserFile->makeConfigDataBinary(info.data, info.size);
        }
    }
}

void SaveDataHandler::copyUserFileMemory(int indexDst, int indexSrc) {
    SaveDataFileAccessor accessor(mSaveDataBuffer);
    for (const auto &spec : USER_FILE_SPECS) {
        char dstName[16];
        char srcName[16];
        std::snprintf(dstName, sizeof(dstName), "%s%1d", spec.name, indexDst);
        std::snprintf(srcName, sizeof(srcName), "%s%1d", spec.name, indexSrc);

        SaveDataFileAccessor::UserFileInfo dst {};
        SaveDataFileAccessor::UserFileInfo src {};
        accessor.makeUserFileInfo(&dst, dstName);
        accessor.makeUserFileInfo(&src, srcName);
        if (dst.data != nullptr && src.data != nullptr) {
            MR::copyMemory(dst.data, src.data, std::min(dst.size, src.size));
        }
    }
}

void SaveDataHandler::restoreGameDataFile(const char *pName, void *pDst, u32 size) {
    SaveDataFileAccessor accessor(mSaveDataBuffer);
    SaveDataFileAccessor::UserFileInfo info {};
    accessor.makeUserFileInfo(&info, pName);
    if (info.data == nullptr || pDst == nullptr) {
        return;
    }
    MR::copyMemory(pDst, info.data, std::min(size, info.size));
}

void SaveDataHandler::storeUserFile(const UserFile *pUserFile) {
    SaveDataFileAccessor accessor(mSaveDataBuffer);

    SaveDataFileAccessor::UserFileInfo gameInfo {};
    accessor.makeUserFileInfo(&gameInfo, pUserFile->getGameDataName());
    if (gameInfo.data != nullptr) {
        pUserFile->makeGameDataBinary(gameInfo.data, gameInfo.size);
    }

    SaveDataFileAccessor::UserFileInfo configInfo {};
    accessor.makeUserFileInfo(&configInfo, pUserFile->getConfigDataName());
    if (configInfo.data != nullptr) {
        pUserFile->makeConfigDataBinary(configInfo.data, configInfo.size);
    }
}

void SaveDataHandler::storeSysConfigFile(const SysConfigFile *pSysConfigFile) {
    SaveDataFileAccessor accessor(mSaveDataBuffer);
    SaveDataFileAccessor::UserFileInfo info {};
    accessor.makeUserFileInfo(&info, SYSTEM_FILE_SPEC.name);
    if (info.data != nullptr) {
        pSysConfigFile->makeDataBinary(info.data, info.size);
    }
}

void SaveDataHandler::requestSaveSaveData() {
    const u32 fileSize = read_header_size(mSaveDataBuffer);
    const u32 alignedFileSize = align32(fileSize);

    MR::copyMemory(mNANDCommunicationBuffer, mSaveDataBuffer, fileSize);
    if (alignedFileSize > fileSize) {
        MR::zeroMemory(mNANDCommunicationBuffer + fileSize, alignedFileSize - fileSize);
    }

    write_header_checksum(mNANDCommunicationBuffer, MR::calcCheckSum(mNANDCommunicationBuffer + 4U, fileSize - 4U));
    mNANDRequestInfo->setWriteSeq(GAME_DATA_FILE, mNANDCommunicationBuffer, alignedFileSize, 0x3CU, 0U);
    MR::addRequestToNANDManager(mNANDRequestInfo);
    setNerve(&SaveDataHandlerSaveProcessingGameData::sInstance);
}

void SaveDataHandler::requestRemoveSaveData() {
    setNerve(&SaveDataHandlerRemoveProcessingBanner::sInstance);
}

u32 SaveDataHandler::getEnoughtTempBufferSize() {
    return SAVE_GAME_DATA_SIZE;
}

bool SaveDataHandler::isDone() const {
    return isNerve(&SaveDataHandlerWait::sInstance) && mNANDRequestInfo->isDone();
}

NANDResultCode SaveDataHandler::getLastResultCode() const {
    return NANDResultCode(mNANDRequestInfo->mResult);
}

void SaveDataHandler::exeWait() {
}

void SaveDataHandler::exeProcessing() {
    if (mNANDRequestInfo->isDone()) {
        setNerve(&SaveDataHandlerWait::sInstance);
    }
}

void SaveDataHandler::exeSaveProcessingGameData() {
    bool isErr = false;
    if (!trySave(&isErr, mNANDRequestInfo->isDone())) {
        return;
    }

    if (isErr) {
        setNerve(&SaveDataHandlerWait::sInstance);
    } else {
        setNerve(&SaveDataHandlerSaveProcessingBanner::sInstance);
    }
}

void SaveDataHandler::exeSaveProcessingBanner() {
    if (MR::isFirstStep(this)) {
        mBannerCreator->execute();
    }
    if (mBannerCreator->isDone()) {
        mNANDRequestInfo->mResult = mBannerCreator->getResultCode().getCode();
        setNerve(&SaveDataHandlerWait::sInstance);
    }
}

void SaveDataHandler::exeRemoveProcessingBanner() {
    bool removed = false;
    if (!tryRemoveFile(BANNER_FILE, &removed)) {
        return;
    }
    if (removed) {
        setNerve(&SaveDataHandlerRemoveProcessingGameData::sInstance);
    } else {
        setNerve(&SaveDataHandlerWait::sInstance);
    }
}

void SaveDataHandler::exeRemoveProcessingGameData() {
    bool removed = false;
    if (tryRemoveFile(GAME_DATA_FILE, &removed)) {
        setNerve(&SaveDataHandlerWait::sInstance);
    }
}

void SaveDataHandler::resetSaveData(u8 *pData) {
    MR::zeroMemory(pData, SAVE_IMAGE_BUFFER_SIZE);
    write_header_checksum(pData, 0U);
    write_header_version(pData, SAVE_FILE_VERSION);
    write_header_file_count(pData, SAVE_FILE_COUNT);

    u32 fileIndex = 0U;
    u32 dataOffset = SAVE_FILE_DATA_START;
    for (int userIndex = 1; userIndex <= 6; ++userIndex) {
        for (const auto &spec : USER_FILE_SPECS) {
            u8 *info = file_info(pData, fileIndex);
            char name[32];
            std::snprintf(name, sizeof(name), "%s%1d", spec.name, userIndex);
            write_file_name(info, name);
            write_file_offset(info, dataOffset);
            dataOffset += spec.size;
            ++fileIndex;
        }
    }

    u8 *info = file_info(pData, fileIndex);
    write_file_name(info, SYSTEM_FILE_SPEC.name);
    write_file_offset(info, dataOffset);
    dataOffset += SYSTEM_FILE_SPEC.size;
    write_header_size(pData, align32(dataOffset));
}

void SaveDataHandler::initializeAllFileInSaveData(u8 *pData, const SysConfigFile *pSysConfigFile, const UserFile *pUserFile) {
    SaveDataFileAccessor accessor(pData);
    const u32 fileCount = read_header_file_count(accessor.getHeader());
    for (u32 index = 0U; index < fileCount; ++index) {
        const char *name = reinterpret_cast<const char *>(accessor.getFileInfo(static_cast<int>(index)));
        SaveDataFileAccessor::UserFileInfo info {};
        accessor.makeUserFileInfo(&info, name);
        if (info.data == nullptr) {
            continue;
        }

        if (info.type == 0U) {
            pUserFile->makeGameDataBinary(info.data, info.size);
        } else if (info.type == 1U) {
            pUserFile->makeConfigDataBinary(info.data, info.size);
        } else {
            pSysConfigFile->makeDataBinary(info.data, info.size);
        }
    }
}

bool SaveDataHandler::isCorrectFileHeader(const u8 *pData) {
    if (pData == nullptr) {
        return false;
    }
    if (read_header_version(pData) != SAVE_FILE_VERSION) {
        return false;
    }
    if (read_header_size(pData) >= SAVE_IMAGE_BUFFER_SIZE) {
        return false;
    }
    return read_header_file_count(pData) < 0x18U;
}

void SaveDataHandler::copySaveDataEachFile(u8 *pDst, const u8 *pSrc) {
    SaveDataFileAccessor dstAccessor(pDst);
    SaveDataFileAccessor srcAccessor(const_cast<u8 *>(pSrc));

    const u32 fileCount = read_header_file_count(pSrc);
    for (u32 index = 0U; index < fileCount; ++index) {
        const char *name = reinterpret_cast<const char *>(file_info(pSrc, index));
        SaveDataFileAccessor::UserFileInfo dst {};
        SaveDataFileAccessor::UserFileInfo src {};
        dstAccessor.makeUserFileInfo(&dst, name);
        srcAccessor.makeUserFileInfo(&src, name);
        if (dst.data == nullptr || src.data == nullptr) {
            continue;
        }

        const u32 copySize = std::min(dst.size, src.size);
        MR::copyMemory(dst.data, src.data, copySize);
        if (dst.size > copySize) {
            MR::zeroMemory(dst.data + copySize, dst.size - copySize);
        }
    }
}

void SaveDataHandler::createCommunicationBuffer() {
    mNANDCommunicationBuffer = new u8[SAVE_IMAGE_BUFFER_SIZE];
    mSaveDataBuffer = new u8[SAVE_IMAGE_BUFFER_SIZE];
}

bool SaveDataHandler::tryRemoveFile(const char *pName, bool *pRemoved) {
    if (MR::isFirstStep(this)) {
        mNANDRequestInfo->setDelete(pName);
        MR::addRequestToNANDManager(mNANDRequestInfo);
        return false;
    }

    if (!mNANDRequestInfo->isDone()) {
        return false;
    }

    const NANDResultCode resultCode(mNANDRequestInfo->mResult);
    *pRemoved = resultCode.isSuccess() || resultCode.isNoExistFile();
    return true;
}

bool SaveDataHandler::trySave(bool *pIsErr, bool isDone) {
    if (!isDone) {
        return false;
    }

    const NANDResultCode resultCode(mNANDRequestInfo->mResult);
    *pIsErr = !resultCode.isSuccess();
    return true;
}
