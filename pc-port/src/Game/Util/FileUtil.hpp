#pragma once

#include <JSystem/JKernel/JKRArchive.hpp>
#include <JSystem/JKernel/JKRDvdRipper.hpp>
#include <JSystem/JKernel/JKRHeap.hpp>
#include <revolution.h>

namespace MR {
    bool isFileExist(const char* pFilePath, bool considerLanguage) NO_INLINE;
    u32 getFileSize(const char* pFilePath, bool considerLanguage);
    s32 convertPathToEntrynumConsideringLanguage(const char* pFilePath);
    void* loadToMainRAM(const char* pFilePath, u8* pDst, JKRHeap* pHeap, JKRDvdRipper::EAllocDirection allocDir);
    void loadAsyncToMainRAM(const char* pFilePath, u8* pDst, JKRHeap* pHeap, JKRDvdRipper::EAllocDirection allocDir);
    JKRMemArchive* mountArchive(const char* pFilePath, JKRHeap* pHeap);
    void mountAsyncArchive(const char* pFilePath, JKRHeap* pHeap);
    void mountAsyncArchiveByObjectOrLayoutName(const char* pFilePrefix, JKRHeap* pHeap);
    void* receiveFile(const char* pFilePath);
    JKRMemArchive* receiveArchive(const char* pFilePath);
    void receiveAllRequestedFile();
    void createAndAddArchive(void* pArcData, JKRHeap* pHeap, const char* pFilePath);
    void getMountedArchiveAndHeap(const char* pFilePath, JKRArchive** ppArchive, JKRHeap** ppHeap);
    void removeFileConsideringLanguage(const char* pFilePath);
    void removeResourceAndFileHolderIfIsEqualHeap(JKRHeap* pHeap);
    void* decompressFileFromArchive(JKRArchive* pArchive, const char* pFilePath, JKRHeap* pHeap, int align);
    bool isLoadedFile(const char* pFilePath);
    bool isMountedArchive(const char* pFilePath);
    bool isLoadedObjectOrLayoutArchive(const char* pFilePrefix);
    void makeFileNameConsideringLanguage(char* pDst, u32 size, const char* pFilePath);
    bool makeObjectArchiveFileName(char* pDst, u32 size, const char* pFileName);
    bool makeObjectArchiveFileNameFromPrefix(char* pDst, u32 size, const char* pFilePrefix, bool unused) NO_INLINE;
    bool makeLayoutArchiveFileName(char* pDst, u32 size, const char* pFileName);
    bool makeLayoutArchiveFileNameFromPrefix(char* pDst, u32 size, const char* pFilePrefix, bool fallback);
    void makeScenarioArchiveFileName(char* pDst, u32 size, const char* pStageName);
}  // namespace MR
