#include "Game/System/SaveDataHandler.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/System/NANDManager.hpp"
#include "Game/System/SaveDataBannerCreator.hpp"
#include "Game/System/SysConfigFile.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include <JSystem/JKernel/JKRExpHeap.hpp>
#include <cstdio>
#include <cstring>

namespace {
    static const u32 cSaveDataBufferSize = 0x10000;
    static const char cSaveDataFileName[] = "GameData.bin";
    static const char cBannerFileName[] = "banner.bin";

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

    enum SaveDataFileType { SaveDataFileType_Game = 0, SaveDataFileType_Config = 1, SaveDataFileType_System = 2 };

    static const SaveDataFileSpec cSaveFileSpecTable[] = {
        {"mario", 0xF80},
        {"luigi", 0xF80},
        {"config", 0x60},
    };

    static const SaveDataFileSpec cSaveFileSpecSystem = {
        "sysconf",
        0x80,
    };

    static u32 alignSaveDataSize(u32 size) {
        return (size + 0x1F) & ~0x1F;
    }

    class SaveDataFileAccessor {
    public:
        SaveDataFileAccessor(u8* pSaveData) : mSaveData(pSaveData) {
        }

        SaveDataFileHeader* getHeader() const {
            return reinterpret_cast< SaveDataFileHeader* >(mSaveData);
        }

        SaveDataFileInfo* getFileInfo(int index) const {
            return reinterpret_cast< SaveDataFileInfo* >(mSaveData + sizeof(SaveDataFileHeader)) + index;
        }

        void makeUserFileInfo(SaveDataUserFileInfo* pInfo, const char* pName) const {
            pInfo->mData = nullptr;
            pInfo->mSize = 0;
            pInfo->mType = SaveDataFileType_Config;

            SaveDataFileHeader* pHeader = getHeader();

            for (u32 i = 0; i < pHeader->mFileCount; i++) {
                SaveDataFileInfo* pFileInfo = getFileInfo(i);

                if (strcmp(pFileInfo->mName, pName) != 0) {
                    continue;
                }

                if (i == pHeader->mFileCount - 1) {
                    pInfo->mSize = pHeader->mDataSize - pFileInfo->mDataOffset;
                } else {
                    pInfo->mSize = getFileInfo(i + 1)->mDataOffset - pFileInfo->mDataOffset;
                }

                pInfo->mData = mSaveData + pFileInfo->mDataOffset;

                if (strstr(pFileInfo->mName, "mario") != nullptr || strstr(pFileInfo->mName, "luigi") != nullptr) {
                    pInfo->mType = SaveDataFileType_Game;
                }

                if (strstr(pFileInfo->mName, "sysconf") != nullptr) {
                    pInfo->mType = SaveDataFileType_System;
                }
            }
        }

    private:
        u8* mSaveData;
    };

    class SaveDataHandlerWait : public Nerve {
    public:
        virtual void execute(Spine*) const {
        }

        static SaveDataHandlerWait sInstance;
    };

    class SaveDataHandlerProcessing : public Nerve {
    public:
        virtual void execute(Spine* pSpine) const {
            SaveDataHandler* pHandler = reinterpret_cast< SaveDataHandler* >(pSpine->mExecutor);

            if (pHandler->mNANDRequestInfo->isDone()) {
                pHandler->setNerve(&SaveDataHandlerWait::sInstance);
            }
        }

        static SaveDataHandlerProcessing sInstance;
    };

    class SaveDataHandlerSaveProcessingGameData : public Nerve {
    public:
        virtual void execute(Spine* pSpine) const {
            reinterpret_cast< SaveDataHandler* >(pSpine->mExecutor)->exeSaveProcessingGameData();
        }

        static SaveDataHandlerSaveProcessingGameData sInstance;
    };

    class SaveDataHandlerSaveProcessingBanner : public Nerve {
    public:
        virtual void execute(Spine* pSpine) const {
            reinterpret_cast< SaveDataHandler* >(pSpine->mExecutor)->exeSaveProcessingBanner();
        }

        static SaveDataHandlerSaveProcessingBanner sInstance;
    };

    class SaveDataHandlerRemoveProcessingBanner : public Nerve {
    public:
        virtual void execute(Spine* pSpine) const {
            reinterpret_cast< SaveDataHandler* >(pSpine->mExecutor)->exeRemoveProcessingBanner();
        }

        static SaveDataHandlerRemoveProcessingBanner sInstance;
    };

    class SaveDataHandlerRemoveProcessingGameData : public Nerve {
    public:
        virtual void execute(Spine* pSpine) const {
            SaveDataHandler* pHandler = reinterpret_cast< SaveDataHandler* >(pSpine->mExecutor);
            bool isDone = false;

            if (pHandler->tryRemoveFile(cSaveDataFileName, &isDone)) {
                pHandler->setNerve(&SaveDataHandlerWait::sInstance);
            }
        }

        static SaveDataHandlerRemoveProcessingGameData sInstance;
    };

    SaveDataHandlerWait SaveDataHandlerWait::sInstance;
    SaveDataHandlerProcessing SaveDataHandlerProcessing::sInstance;
    SaveDataHandlerSaveProcessingGameData SaveDataHandlerSaveProcessingGameData::sInstance;
    SaveDataHandlerSaveProcessingBanner SaveDataHandlerSaveProcessingBanner::sInstance;
    SaveDataHandlerRemoveProcessingBanner SaveDataHandlerRemoveProcessingBanner::sInstance;
    SaveDataHandlerRemoveProcessingGameData SaveDataHandlerRemoveProcessingGameData::sInstance;
};  // namespace

SaveDataHandler::SaveDataHandler(const SysConfigFile* pSysConfigFile, const UserFile* pUserFile)
    : NerveExecutor("SaveDataHandler"), mNANDRequestInfo(nullptr), _C(0), _10(0), _14(nullptr), _18(nullptr), mBannerCreator(nullptr) {
    mNANDRequestInfo = new NANDRequestInfo();

    createCommunicationBuffer();
    resetSaveData(_18);
    initializeAllFileInSaveData(_18, pSysConfigFile, pUserFile);

    mBannerCreator = new SaveDataBannerCreator();

    initNerve(&SaveDataHandlerWait::sInstance);
}

void SaveDataHandler::update() {
    updateNerve();
    mBannerCreator->updateNerve();
}

void SaveDataHandler::requestCheckEnableToCreate() {
    mNANDRequestInfo->setCheck(5, 2, &_10);
    MR::addRequestToNANDManager(mNANDRequestInfo);

    setNerve(&SaveDataHandlerProcessing::sInstance);
}

void SaveDataHandler::requestLoadSaveData() {
    MR::zeroMemory(_14, cSaveDataBufferSize);
    mNANDRequestInfo->setReadSeq(cSaveDataFileName, _14, cSaveDataBufferSize, &_C);
    MR::addRequestToNANDManager(mNANDRequestInfo);

    setNerve(&SaveDataHandlerProcessing::sInstance);
}

bool SaveDataHandler::requestVerifyAfterLoadGameDataFile() {
    if (!isCorrectFileHeader(_14)) {
        return false;
    }

    SaveDataFileAccessor accessor(_14);
    SaveDataFileHeader* pHeader = accessor.getHeader();

    if (_C != alignSaveDataSize(pHeader->mDataSize)) {
        return false;
    }

    if (pHeader->mCheckSum != MR::calcCheckSum(_14 + sizeof(u32), pHeader->mDataSize - sizeof(u32))) {
        return false;
    }

    copySaveDataEachFile(_18, _14);

    return true;
}

void SaveDataHandler::initializeUserFileMemory(int index, const UserFile* pUserFile) {
    SaveDataFileAccessor accessor(_18);

    for (u32 i = 0; i < 3; i++) {
        char name[16];
        SaveDataUserFileInfo info;

        snprintf(name, sizeof(name), "%s%1d", cSaveFileSpecTable[i].mPrefix, index);
        accessor.makeUserFileInfo(&info, name);

        if (info.mType == SaveDataFileType_Game) {
            pUserFile->makeGameDataBinary(info.mData, info.mSize);
        } else if (info.mType == SaveDataFileType_Config) {
            pUserFile->makeConfigDataBinary(info.mData, info.mSize);
        }
    }
}

void SaveDataHandler::copyUserFileMemory(int indexDst, int indexSrc) {
    SaveDataFileAccessor accessor(_18);

    for (u32 i = 0; i < 3; i++) {
        char srcName[16];
        char dstName[16];
        SaveDataUserFileInfo srcInfo;
        SaveDataUserFileInfo dstInfo;

        snprintf(srcName, sizeof(srcName), "%s%1d", cSaveFileSpecTable[i].mPrefix, indexSrc);
        accessor.makeUserFileInfo(&srcInfo, srcName);

        snprintf(dstName, sizeof(dstName), "%s%1d", cSaveFileSpecTable[i].mPrefix, indexDst);
        accessor.makeUserFileInfo(&dstInfo, dstName);

        MR::copyMemory(dstInfo.mData, srcInfo.mData, dstInfo.mSize);
    }
}

void SaveDataHandler::restoreGameDataFile(const char* pName, void* pBuffer, u32) {
    SaveDataFileAccessor accessor(_18);
    SaveDataUserFileInfo info;

    accessor.makeUserFileInfo(&info, pName);
    MR::copyMemory(pBuffer, info.mData, info.mSize);
}

void SaveDataHandler::storeUserFile(const UserFile* pUserFile) {
    SaveDataFileAccessor accessor(_18);
    SaveDataUserFileInfo info;

    accessor.makeUserFileInfo(&info, pUserFile->getGameDataName());
    pUserFile->makeGameDataBinary(info.mData, info.mSize);

    accessor.makeUserFileInfo(&info, pUserFile->getConfigDataName());
    pUserFile->makeConfigDataBinary(info.mData, info.mSize);
}

void SaveDataHandler::storeSysConfigFile(const SysConfigFile* pSysConfigFile) {
    SaveDataFileAccessor accessor(_18);
    SaveDataUserFileInfo info;

    accessor.makeUserFileInfo(&info, cSaveFileSpecSystem.mPrefix);
    pSysConfigFile->makeDataBinary(info.mData, info.mSize);
}

void SaveDataHandler::requestSaveSaveData() {
    SaveDataFileAccessor workAccessor(_18);
    SaveDataFileAccessor saveAccessor(_14);
    SaveDataFileHeader* pWorkHeader = workAccessor.getHeader();

    MR::copyMemory(_14, _18, pWorkHeader->mDataSize);

    SaveDataFileHeader* pSaveHeader = saveAccessor.getHeader();
    u32 alignedSize = alignSaveDataSize(pSaveHeader->mDataSize);
    u32 paddingSize = alignedSize - pSaveHeader->mDataSize;

    MR::fillMemory(_14 + pSaveHeader->mDataSize, 0, paddingSize);

    pSaveHeader->mCheckSum = MR::calcCheckSum(_14 + sizeof(u32), pSaveHeader->mDataSize - sizeof(u32));

    mNANDRequestInfo->setWriteSeq(cSaveDataFileName, _14, alignedSize, 0x3C, 0);
    MR::addRequestToNANDManager(mNANDRequestInfo);

    setNerve(&SaveDataHandlerSaveProcessingGameData::sInstance);
}

void SaveDataHandler::requestRemoveSaveData() {
    setNerve(&SaveDataHandlerRemoveProcessingBanner::sInstance);
}

u32 SaveDataHandler::getEnoughtTempBufferSize() {
    return 0xF80;
}

bool SaveDataHandler::isDone() const {
    return isNerve(&SaveDataHandlerWait::sInstance);
}

NANDResultCode SaveDataHandler::getLastResultCode() const {
    return NANDResultCode(mNANDRequestInfo->mResult);
}

void SaveDataHandler::exeSaveProcessingGameData() {
    bool isError = false;

    if (trySave(&isError, mNANDRequestInfo->isDone())) {
        if (isError) {
            setNerve(&SaveDataHandlerWait::sInstance);
        } else {
            setNerve(&SaveDataHandlerSaveProcessingBanner::sInstance);
        }
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

    NANDResultCode resultCode = mBannerCreator->getResultCode();
    mNANDRequestInfo->mResult = resultCode.getCode();

    setNerve(&SaveDataHandlerWait::sInstance);
}

void SaveDataHandler::exeRemoveProcessingBanner() {
    bool isDone = false;

    if (tryRemoveFile(cBannerFileName, &isDone)) {
        if (isDone) {
            setNerve(&SaveDataHandlerRemoveProcessingGameData::sInstance);
        } else {
            setNerve(&SaveDataHandlerWait::sInstance);
        }
    }
}

void SaveDataHandler::resetSaveData(u8* pSaveData) {
    SaveDataFileAccessor accessor(pSaveData);
    SaveDataFileHeader* pHeader = accessor.getHeader();
    u32 dataOffset = sizeof(SaveDataFileHeader) + 19 * sizeof(SaveDataFileInfo);
    u32 fileIndex = 0;

    pHeader->mCheckSum = 0;
    pHeader->mVersion = 2;
    pHeader->mFileCount = 19;
    pHeader->mDataSize = 0;

    for (s32 slotIndex = 1; slotIndex < 7; slotIndex++) {
        for (u32 i = 0; i < 3; i++) {
            SaveDataFileInfo* pFileInfo = accessor.getFileInfo(fileIndex);
            char name[32];

            MR::zeroMemory(pFileInfo->mName, sizeof(pFileInfo->mName));
            snprintf(name, sizeof(name), "%s%1d", cSaveFileSpecTable[i].mPrefix, slotIndex);
            snprintf(pFileInfo->mName, sizeof(pFileInfo->mName), "%s", name);

            pFileInfo->mDataOffset = dataOffset;
            MR::zeroMemory(pSaveData + dataOffset, cSaveFileSpecTable[i].mSize);

            dataOffset += cSaveFileSpecTable[i].mSize;
            fileIndex++;
        }
    }

    SaveDataFileInfo* pFileInfo = accessor.getFileInfo(fileIndex);

    MR::zeroMemory(pFileInfo->mName, sizeof(pFileInfo->mName));
    snprintf(pFileInfo->mName, sizeof(pFileInfo->mName), "%s", cSaveFileSpecSystem.mPrefix);

    pFileInfo->mDataOffset = dataOffset;
    MR::zeroMemory(pSaveData + dataOffset, cSaveFileSpecSystem.mSize);

    pHeader->mDataSize = dataOffset + cSaveFileSpecSystem.mSize;

    u32 alignedSize = alignSaveDataSize(pHeader->mDataSize);
    MR::fillMemory(pSaveData + pHeader->mDataSize, 0, alignedSize - pHeader->mDataSize);
}

void SaveDataHandler::initializeAllFileInSaveData(u8* pSaveData, const SysConfigFile* pSysConfigFile, const UserFile* pUserFile) {
    SaveDataFileAccessor accessor(pSaveData);
    SaveDataFileHeader* pHeader = accessor.getHeader();

    for (u32 i = 0; i < pHeader->mFileCount; i++) {
        SaveDataUserFileInfo info;

        accessor.makeUserFileInfo(&info, accessor.getFileInfo(i)->mName);

        if (info.mType == SaveDataFileType_Game) {
            pUserFile->makeGameDataBinary(info.mData, info.mSize);
        } else if (info.mType == SaveDataFileType_Config) {
            pUserFile->makeConfigDataBinary(info.mData, info.mSize);
        } else {
            pSysConfigFile->makeDataBinary(info.mData, info.mSize);
        }
    }
}

bool SaveDataHandler::isCorrectFileHeader(const u8* pSaveData) {
    SaveDataFileAccessor accessor(const_cast< u8* >(pSaveData));
    SaveDataFileHeader* pHeader = accessor.getHeader();

    if (pHeader->mVersion != 2) {
        return false;
    }

    if (pHeader->mDataSize >= cSaveDataBufferSize) {
        return false;
    }

    return pHeader->mFileCount < 24;
}

void SaveDataHandler::copySaveDataEachFile(u8* pDst, const u8* pSrc) {
    SaveDataFileAccessor srcAccessor(const_cast< u8* >(pSrc));
    SaveDataFileAccessor dstAccessor(pDst);
    SaveDataFileHeader* pSrcHeader = srcAccessor.getHeader();

    for (u32 i = 0; i < pSrcHeader->mFileCount; i++) {
        SaveDataUserFileInfo srcInfo;
        SaveDataUserFileInfo dstInfo;
        const char* pName = srcAccessor.getFileInfo(i)->mName;

        srcAccessor.makeUserFileInfo(&srcInfo, pName);
        dstAccessor.makeUserFileInfo(&dstInfo, pName);

        if (dstInfo.mData != nullptr) {
            MR::copyMemory(dstInfo.mData, srcInfo.mData, dstInfo.mSize);

            s32 extraSize = static_cast< s32 >(dstInfo.mSize) - static_cast< s32 >(srcInfo.mSize);

            if (extraSize > 0) {
                MR::fillMemory(dstInfo.mData + srcInfo.mSize, 0, extraSize);
            }
        }
    }
}

void SaveDataHandler::createCommunicationBuffer() {
    _14 = new (MR::getStationedHeapGDDR3(), 32) u8[cSaveDataBufferSize];
    _18 = new (MR::getStationedHeapGDDR3(), 32) u8[cSaveDataBufferSize];
}

bool SaveDataHandler::tryRemoveFile(const char* pName, bool* pIsDone) {
    if (MR::isFirstStep(this)) {
        mNANDRequestInfo->setDelete(pName);
        MR::addRequestToNANDManager(mNANDRequestInfo);

        return false;
    }

    if (!mNANDRequestInfo->isDone()) {
        return false;
    }

    NANDResultCode resultCode(mNANDRequestInfo->mResult);

    if (resultCode.isSuccess() || resultCode.isNoExistFile()) {
        *pIsDone = true;
    } else {
        *pIsDone = false;
    }

    return true;
}

bool SaveDataHandler::trySave(bool* pIsDone, bool isRequestDone) {
    if (!isRequestDone) {
        return false;
    }

    NANDResultCode resultCode(mNANDRequestInfo->mResult);

    if (resultCode.isSuccess()) {
        *pIsDone = false;
    } else {
        *pIsDone = true;
    }

    return true;
}
