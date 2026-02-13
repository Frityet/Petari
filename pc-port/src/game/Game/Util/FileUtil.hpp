#pragma once

#include "compat/JKernelCompat.hpp"
#include "compat/Types.hpp"

namespace MR {

[[nodiscard]] bool isFileExist(const char *pFilePath, bool considerLanguage);

void *loadToMainRAM(const char *pFilePath, u8 *pDst, JKRHeap *pHeap, JKRDvdRipper::EAllocDirection allocDir);
void loadAsyncToMainRAM(const char *pFilePath, u8 *pDst, JKRHeap *pHeap, JKRDvdRipper::EAllocDirection allocDir);

JKRMemArchive *mountArchive(const char *pFilePath, JKRHeap *pHeap);
void mountAsyncArchive(const char *pFilePath, JKRHeap *pHeap);
void mountAsyncArchiveByObjectOrLayoutName(const char *pFilePrefix, JKRHeap *pHeap);

void *receiveFile(const char *pFilePath);
JKRMemArchive *receiveArchive(const char *pFilePath);

[[nodiscard]] bool isLoadedFile(const char *pFilePath);
[[nodiscard]] bool isMountedArchive(const char *pFilePath);
[[nodiscard]] bool isLoadedObjectOrLayoutArchive(const char *pFilePrefix);

void makeFileNameConsideringLanguage(char *pDst, u32 size, const char *pFilePath);
bool makeObjectArchiveFileName(char *pDst, u32 size, const char *pFileName);
bool makeObjectArchiveFileNameFromPrefix(char *pDst, u32 size, const char *pFilePrefix, bool unused);
bool makeLayoutArchiveFileName(char *pDst, u32 size, const char *pFileName);
bool makeLayoutArchiveFileNameFromPrefix(char *pDst, u32 size, const char *pFilePrefix, bool fallback);
void makeScenarioArchiveFileName(char *pDst, u32 size, const char *pStageName);

}  // namespace MR
