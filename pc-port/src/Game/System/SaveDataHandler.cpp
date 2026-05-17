#include "Game/System/SaveDataHandler.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "Game/LiveActor/Nerve.hpp"
#include "Game/System/NANDManager.hpp"
#include "Game/System/SaveDataBannerCreator.hpp"
#include "Game/System/SysConfigFile.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/Util/NerveUtil.hpp"

struct SaveDataFileSpec {
    const char* mPrefix;
    u32 mSize;
};

struct SaveDataFileHeader {
    u32 mCheckSum;
    u32 mVersion;
    u32 mFileCount;
    u32 mDataSize;
};

struct SaveDataFileInfo {
    char mName[12];
    u32 mDataOffset;
};

struct SaveDataUserFileInfo {
    u8* mData;
    u32 mSize;
    u8 mType;
};

enum SaveDataFileType {
    SaveDataFileType_Game = 0,
    SaveDataFileType_Config = 1,
    SaveDataFileType_System = 2,
};

namespace {
    constexpr auto cSaveDataBufferSize = u32{0x10000U};
    constexpr auto cSaveDataVersion = u32{2U};
    constexpr auto cSaveDataFileCount = u32{19U};
    constexpr auto cSaveDataGameFileSize = u32{0xF80U};
    constexpr auto cSaveDataConfigFileSize = u32{0x60U};
    constexpr auto cSaveDataSystemFileSize = u32{0x80U};
    constexpr char cSaveDataFileName[] = "GameData.bin";
    constexpr char cBannerFileName[] = "banner.bin";
    constexpr char cSaveDataSystemFileName[] = "sysconf";
}

    smgpc::game::SaveDataService& backing_save_data() {
        return smgpc::game::RuntimeContext::instance().save_data();
    }

    [[nodiscard]] s32 slot_index_from_name(std::string_view name) {
        if (name.empty()) {
            return 1;
        }

        const auto* header = getHeader();
        if (header->mVersion != cSaveDataVersion || header->mFileCount >= 24U) {
            return;
        }

        for (auto i = u32{}; i < header->mFileCount; ++i) {
            const auto* file_info = getFileInfo(static_cast<int>(i));
            if (std::strncmp(file_info->mName, pName, sizeof(file_info->mName)) != 0) {
                continue;
            }

            pInfo->mData = mSaveData + file_info->mDataOffset;
            if (std::strncmp(pName, "mario", 5U) == 0 || std::strncmp(pName, "luigi", 5U) == 0) {
                pInfo->mType = SaveDataFileType_Game;
                pInfo->mSize = cSaveDataGameFileSize;
            } else if (std::strncmp(pName, "config", 6U) == 0) {
                pInfo->mType = SaveDataFileType_Config;
                pInfo->mSize = cSaveDataConfigFileSize;
            } else if (std::strcmp(pName, cSaveDataSystemFileName) == 0) {
                pInfo->mType = SaveDataFileType_System;
                pInfo->mSize = cSaveDataSystemFileSize;
            }
            return;
        }
    }

private:
    u8* mSaveData;
};

namespace {
    constexpr SaveDataFileSpec cSaveFileSpecTable[] = {
        {"mario", cSaveDataGameFileSize},
        {"luigi", cSaveDataGameFileSize},
        {"config", cSaveDataConfigFileSize},
    };

    constexpr SaveDataFileSpec cSaveFileSpecSystem = {
        cSaveDataSystemFileName,
        cSaveDataSystemFileSize,
    };

    [[nodiscard]] u32 alignSaveDataSize(u32 size) {
        return (size + 0x1FU) & ~0x1FU;
    }

    [[nodiscard]] u32 calcSaveDataCheckSum(const void* pPtr, u32 size) {
        auto sum = u16{};
        auto inv_sum = u16{};
        const auto* bytes = static_cast<const u8*>(pPtr);
        for (auto offset = u32{}; offset + sizeof(u16) <= size; offset += sizeof(u16)) {
            auto word = u16{};
            std::memcpy(&word, bytes + offset, sizeof(word));
            sum = static_cast<u16>(sum + word);
            inv_sum = static_cast<u16>(inv_sum + static_cast<u16>(~word));
        }
        return (static_cast<u32>(sum) << 16U) | inv_sum;
    }

    void copyMemory(void* pDst, const void* pSrc, u32 size) {
        if (pDst != nullptr && pSrc != nullptr && size != 0U) {
            std::memcpy(pDst, pSrc, size);
        }
    }

    void zeroMemory(void* pDst, u32 size) {
        if (pDst != nullptr && size != 0U) {
            std::memset(pDst, 0, size);
        }
    }

    void fillMemory(void* pDst, u8 value, u32 size) {
        if (pDst != nullptr && size != 0U) {
            std::memset(pDst, value, size);
        }
    }

    NEW_NERVE(SaveDataHandlerWait, SaveDataHandler, Wait);
    NEW_NERVE(SaveDataHandlerProcessing, SaveDataHandler, Processing);
    NEW_NERVE(SaveDataHandlerSaveProcessingGameData, SaveDataHandler, SaveProcessingGameData);
    NEW_NERVE(SaveDataHandlerSaveProcessingBanner, SaveDataHandler, SaveProcessingBanner);
    NEW_NERVE(SaveDataHandlerRemoveProcessingBanner, SaveDataHandler, RemoveProcessingBanner);
    NEW_NERVE(SaveDataHandlerRemoveProcessingGameData, SaveDataHandler, RemoveProcessingGameData);
}  // namespace

SaveDataHandler::SaveDataHandler(const SysConfigFile* pSysConfigFile, const UserFile* pUserFile)
    : NerveExecutor("SaveDataHandler"), mNANDRequestInfo(nullptr), _C(0), _10(0), _14(nullptr), _18(nullptr), mBannerCreator(nullptr) {
    mNANDRequestInfo = new NANDRequestInfo();
    createCommunicationBuffer();
    resetSaveData(_18);
    initializeAllFileInSaveData(_18, pSysConfigFile, pUserFile);
    mBannerCreator = new SaveDataBannerCreator();
    mNANDRequestInfo->setReadSeq(cSaveDataFileName, _14, cSaveDataBufferSize, &_C);
    MR::addRequestToNANDManager(mNANDRequestInfo);
    if (mNANDRequestInfo->mResult == NAND_RESULT_OK) {
        mSaveDataDirty = !requestVerifyAfterLoadGameDataFile();
    }
    mNANDRequestInfo->init();
    initNerve(&SaveDataHandlerWait::sInstance);
}

void SaveDataHandler::update() {
    updateNerve();
    if (mBannerCreator != nullptr) {
        mBannerCreator->updateNerve();
    }
}

void SaveDataHandler::requestCheckEnableToCreate() {
    mNANDRequestInfo->setCheck(5, 2, &_10);
    MR::addRequestToNANDManager(mNANDRequestInfo);
    setNerve(&SaveDataHandlerProcessing::sInstance);
}

void SaveDataHandler::requestLoadSaveData() {
    zeroMemory(_14, cSaveDataBufferSize);
    mNANDRequestInfo->setReadSeq(cSaveDataFileName, _14, cSaveDataBufferSize, &_C);
    MR::addRequestToNANDManager(mNANDRequestInfo);
    if (mNANDRequestInfo->mResult == NAND_RESULT_OK) {
        mSaveDataDirty = !requestVerifyAfterLoadGameDataFile();
    }
    setNerve(&SaveDataHandlerProcessing::sInstance);
}

bool SaveDataHandler::requestVerifyAfterLoadGameDataFile() {
    if (!isCorrectFileHeader(_14)) {
        return false;
    }

    const auto* header = reinterpret_cast<const SaveDataFileHeader*>(_14);
    if (_C != alignSaveDataSize(header->mDataSize)) {
        return false;
    }

    if (header->mCheckSum != calcSaveDataCheckSum(_14 + sizeof(u32), header->mDataSize - sizeof(u32))) {
        return false;
    }

    copySaveDataEachFile(_18, _14);
    return true;
}

void SaveDataHandler::initializeUserFileMemory(int index, const UserFile* pUserFile) {
    if (pUserFile == nullptr) {
        mNANDRequestInfo->mResult = NAND_RESULT_INVALID;
        return;
    }

    auto accessor = SaveDataFileAccessor(_18);
    for (u32 i = 0; i < 3; ++i) {
        char name[16]{};
        auto info = SaveDataUserFileInfo{};
        std::snprintf(name, sizeof(name), "%s%1d", cSaveFileSpecTable[i].mPrefix, index);
        accessor.makeUserFileInfo(&info, name);
        if (info.mData == nullptr) {
            continue;
        }

        if (info.mType == SaveDataFileType_Game) {
            pUserFile->makeGameDataBinary(info.mData, info.mSize);
        } else if (info.mType == SaveDataFileType_Config) {
            pUserFile->makeConfigDataBinary(info.mData, info.mSize);
        }
    }
    mNANDRequestInfo->mResult = NAND_RESULT_OK;
    mSaveDataDirty = true;
}

void SaveDataHandler::copyUserFileMemory(int indexDst, int indexSrc) {
    auto accessor = SaveDataFileAccessor(_18);
    for (u32 i = 0; i < 3; ++i) {
        char src_name[16]{};
        char dst_name[16]{};
        auto src_info = SaveDataUserFileInfo{};
        auto dst_info = SaveDataUserFileInfo{};
        std::snprintf(src_name, sizeof(src_name), "%s%1d", cSaveFileSpecTable[i].mPrefix, indexSrc);
        accessor.makeUserFileInfo(&src_info, src_name);
        std::snprintf(dst_name, sizeof(dst_name), "%s%1d", cSaveFileSpecTable[i].mPrefix, indexDst);
        accessor.makeUserFileInfo(&dst_info, dst_name);
        if (src_info.mData != nullptr && dst_info.mData != nullptr) {
            copyMemory(dst_info.mData, src_info.mData, dst_info.mSize);
        }
    }
    mNANDRequestInfo->mResult = NAND_RESULT_OK;
    mSaveDataDirty = true;
}

void SaveDataHandler::restoreGameDataFile(const char* pName, void* pBuffer, u32 size) {
    if (pBuffer == nullptr || size == 0U) {
        mNANDRequestInfo->mResult = NAND_RESULT_INVALID;
        return;
    }

    if (!mSaveDataDirty) {
        mNANDRequestInfo->setReadSeq(cSaveDataFileName, _14, cSaveDataBufferSize, &_C);
        MR::addRequestToNANDManager(mNANDRequestInfo);
        if (mNANDRequestInfo->mResult == NAND_RESULT_OK) {
            mSaveDataDirty = !requestVerifyAfterLoadGameDataFile();
        }
        mNANDRequestInfo->init();
    }

    auto accessor = SaveDataFileAccessor(_18);
    auto info = SaveDataUserFileInfo{};
    accessor.makeUserFileInfo(&info, pName);
    zeroMemory(pBuffer, size);
    if (info.mData == nullptr) {
        mNANDRequestInfo->mResult = NAND_RESULT_NOEXISTS;
        return;
    }

    copyMemory(pBuffer, info.mData, std::min(size, info.mSize));
    mNANDRequestInfo->mResult = NAND_RESULT_OK;
}

void SaveDataHandler::storeUserFile(const UserFile* pUserFile) {
    if (pUserFile == nullptr) {
        mNANDRequestInfo->mResult = NAND_RESULT_INVALID;
        return;
    }

    auto accessor = SaveDataFileAccessor(_18);
    auto info = SaveDataUserFileInfo{};
    accessor.makeUserFileInfo(&info, pUserFile->getGameDataName());
    if (info.mData != nullptr) {
        pUserFile->makeGameDataBinary(info.mData, info.mSize);
    }
    accessor.makeUserFileInfo(&info, pUserFile->getConfigDataName());
    if (info.mData != nullptr) {
        pUserFile->makeConfigDataBinary(info.mData, info.mSize);
    }
    mNANDRequestInfo->mResult = NAND_RESULT_OK;
    mSaveDataDirty = true;
}

void SaveDataHandler::storeSysConfigFile(const SysConfigFile* pSysConfigFile) {
    if (pSysConfigFile == nullptr) {
        mNANDRequestInfo->mResult = NAND_RESULT_INVALID;
        return;
    }

    auto accessor = SaveDataFileAccessor(_18);
    auto info = SaveDataUserFileInfo{};
    accessor.makeUserFileInfo(&info, cSaveFileSpecSystem.mPrefix);
    if (info.mData != nullptr) {
        pSysConfigFile->makeDataBinary(info.mData, info.mSize);
    }
    mNANDRequestInfo->mResult = NAND_RESULT_OK;
    mSaveDataDirty = true;
}

void SaveDataHandler::requestSaveSaveData() {
    auto work_accessor = SaveDataFileAccessor(_18);
    auto save_accessor = SaveDataFileAccessor(_14);
    copyMemory(_14, _18, work_accessor.getHeader()->mDataSize);
    const auto aligned_size = alignSaveDataSize(work_accessor.getHeader()->mDataSize);
    fillMemory(_14 + work_accessor.getHeader()->mDataSize, 0, aligned_size - work_accessor.getHeader()->mDataSize);
    save_accessor.getHeader()->mCheckSum = calcSaveDataCheckSum(_14 + sizeof(u32), save_accessor.getHeader()->mDataSize - sizeof(u32));
    mNANDRequestInfo->setWriteSeq(cSaveDataFileName, _14, aligned_size, 0x3c, 0);
    MR::addRequestToNANDManager(mNANDRequestInfo);
    if (mNANDRequestInfo->mResult == NAND_RESULT_OK) {
        mSaveDataDirty = false;
    }
    setNerve(&SaveDataHandlerSaveProcessingGameData::sInstance);
}

void SaveDataHandler::requestRemoveSaveData() {
    setNerve(&SaveDataHandlerRemoveProcessingBanner::sInstance);
}

u32 SaveDataHandler::getEnoughtTempBufferSize() {
    return cSaveDataGameFileSize;
}

bool SaveDataHandler::isDone() const {
    if (isNerve(&SaveDataHandlerWait::sInstance)) {
        static_cast<void>(mNANDRequestInfo->isDone());
    }
    return isNerve(&SaveDataHandlerWait::sInstance);
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
    auto is_error = false;
    if (trySave(&is_error, mNANDRequestInfo->isDone())) {
        setNerve(is_error ? static_cast<const Nerve*>(&SaveDataHandlerWait::sInstance) :
                            static_cast<const Nerve*>(&SaveDataHandlerSaveProcessingBanner::sInstance));
    }
}

void SaveDataHandler::exeSaveProcessingBanner() {
    if (MR::isFirstStep(this)) {
        mBannerCreator->execute();
        return;
    }

    if (!mBannerCreator->isDone()) {
        return;
    }

    const auto result_code = mBannerCreator->getResultCode();
    mNANDRequestInfo->mResult = result_code.getCode();
    setNerve(&SaveDataHandlerWait::sInstance);
}

void SaveDataHandler::exeRemoveProcessingBanner() {
    auto is_done = false;
    if (tryRemoveFile(cBannerFileName, &is_done)) {
        setNerve(is_done ? static_cast<const Nerve*>(&SaveDataHandlerRemoveProcessingGameData::sInstance) :
                           static_cast<const Nerve*>(&SaveDataHandlerWait::sInstance));
    }
}

void SaveDataHandler::exeRemoveProcessingGameData() {
    auto is_done = false;
    if (tryRemoveFile(cSaveDataFileName, &is_done)) {
        setNerve(&SaveDataHandlerWait::sInstance);
    }
}

void SaveDataHandler::resetSaveData(u8* pSaveData) {
    auto* header = reinterpret_cast<SaveDataFileHeader*>(pSaveData);
    auto* file_infos = reinterpret_cast<SaveDataFileInfo*>(pSaveData + sizeof(SaveDataFileHeader));
    auto data_offset = static_cast<u32>(sizeof(SaveDataFileHeader) + cSaveDataFileCount * sizeof(SaveDataFileInfo));
    auto file_index = u32{};

    header->mCheckSum = 0U;
    header->mVersion = cSaveDataVersion;
    header->mFileCount = cSaveDataFileCount;
    header->mDataSize = 0U;

    for (s32 slot_index = 1; slot_index < 7; ++slot_index) {
        for (u32 i = 0; i < 3; ++i) {
            auto* file_info = &file_infos[file_index];
            zeroMemory(file_info->mName, sizeof(file_info->mName));
            std::snprintf(file_info->mName, sizeof(file_info->mName), "%s%1d", cSaveFileSpecTable[i].mPrefix, slot_index);
            file_info->mDataOffset = data_offset;
            zeroMemory(pSaveData + data_offset, cSaveFileSpecTable[i].mSize);
            data_offset += cSaveFileSpecTable[i].mSize;
            ++file_index;
        }
    }

    auto* file_info = &file_infos[file_index];
    zeroMemory(file_info->mName, sizeof(file_info->mName));
    std::snprintf(file_info->mName, sizeof(file_info->mName), "%s", cSaveFileSpecSystem.mPrefix);
    file_info->mDataOffset = data_offset;
    zeroMemory(pSaveData + data_offset, cSaveFileSpecSystem.mSize);
    header->mDataSize = data_offset + cSaveFileSpecSystem.mSize;
    fillMemory(pSaveData + header->mDataSize, 0, alignSaveDataSize(header->mDataSize) - header->mDataSize);
}

void SaveDataHandler::initializeAllFileInSaveData(u8* pSaveData, const SysConfigFile* pSysConfigFile, const UserFile* pUserFile) {
    auto accessor = SaveDataFileAccessor(pSaveData);
    auto* header = accessor.getHeader();
    for (u32 i = 0; i < header->mFileCount; ++i) {
        auto info = SaveDataUserFileInfo{};
        accessor.makeUserFileInfo(&info, accessor.getFileInfo(static_cast<int>(i))->mName);
        if (info.mData == nullptr) {
            continue;
        }

        if (info.mType == SaveDataFileType_Game) {
            if (pUserFile != nullptr) {
                pUserFile->makeGameDataBinary(info.mData, info.mSize);
            }
        } else if (info.mType == SaveDataFileType_Config) {
            if (pUserFile != nullptr) {
                pUserFile->makeConfigDataBinary(info.mData, info.mSize);
            }
        } else if (pSysConfigFile != nullptr) {
            pSysConfigFile->makeDataBinary(info.mData, info.mSize);
        }
    }
}

bool SaveDataHandler::isCorrectFileHeader(const u8* pSaveData) {
    if (pSaveData == nullptr) {
        return false;
    }

    const auto* header = reinterpret_cast<const SaveDataFileHeader*>(pSaveData);
    return header->mVersion == cSaveDataVersion && header->mDataSize < cSaveDataBufferSize && header->mFileCount < 24U;
}

void SaveDataHandler::copySaveDataEachFile(u8* pDst, const u8* pSrc) {
    auto src_accessor = SaveDataFileAccessor(const_cast<u8*>(pSrc));
    auto dst_accessor = SaveDataFileAccessor(pDst);
    const auto* src_header = src_accessor.getHeader();
    for (u32 i = 0; i < src_header->mFileCount; ++i) {
        auto src_info = SaveDataUserFileInfo{};
        auto dst_info = SaveDataUserFileInfo{};
        const auto* name = src_accessor.getFileInfo(static_cast<int>(i))->mName;
        src_accessor.makeUserFileInfo(&src_info, name);
        dst_accessor.makeUserFileInfo(&dst_info, name);
        if (src_info.mData != nullptr && dst_info.mData != nullptr) {
            copyMemory(dst_info.mData, src_info.mData, std::min(src_info.mSize, dst_info.mSize));
            if (dst_info.mSize > src_info.mSize) {
                fillMemory(dst_info.mData + src_info.mSize, 0, dst_info.mSize - src_info.mSize);
            }
        }
    }
}

void SaveDataHandler::createCommunicationBuffer() {
    _14 = new u8[cSaveDataBufferSize]{};
    _18 = new u8[cSaveDataBufferSize]{};
}

bool SaveDataHandler::tryRemoveFile(const char* pName, bool* pIsDone) {
    const auto* name = pName != nullptr ? pName : "";
    const auto needs_request = mNANDRequestInfo->mType != 1 || std::strcmp(mNANDRequestInfo->mPath, name) != 0;
    if (MR::isFirstStep(this) || needs_request) {
        mNANDRequestInfo->setDelete(pName);
        MR::addRequestToNANDManager(mNANDRequestInfo);
        if (!mNANDRequestInfo->isDone()) {
            return false;
        }
    }

    if (!mNANDRequestInfo->isDone()) {
        return false;
    }

    const auto result_code = NANDResultCode(mNANDRequestInfo->mResult);
    if (pIsDone != nullptr) {
        *pIsDone = result_code.isSuccess() || result_code.isNoExistFile();
    }
    return true;
}

bool SaveDataHandler::trySave(bool* pIsDone, bool isRequestDone) {
    if (!isRequestDone) {
        return false;
    }

    const auto result_code = NANDResultCode(mNANDRequestInfo->mResult);
    if (pIsDone != nullptr) {
        *pIsDone = !result_code.isSuccess();
    }
    return true;
}
