#include "Game/Util/FileUtil.hpp"
#include "Game/System/FileLoader.hpp"
#include "Game/System/Language.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/StringUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "resource/Yaz0.hpp"
#include <aurora/exception.hpp>
#include <cstdio>
#include <cstring>
#include <exception>
#include <stdexcept>

namespace MR {
    bool isFileExist(const char* pFilePath, bool considerLanguage) {
        s32 entryNum;

        if (considerLanguage) {
            entryNum = convertPathToEntrynumConsideringLanguage(pFilePath);
        } else {
            entryNum = DVDConvertPathToEntrynum(pFilePath);
        }

        return entryNum >= 0;
    }

    u32 getFileSize(const char* pFilePath, bool considerLanguage) {
        s32 entryNum;

        if (considerLanguage) {
            entryNum = convertPathToEntrynumConsideringLanguage(pFilePath);
        } else {
            entryNum = DVDConvertPathToEntrynum(pFilePath);
        }

        DVDFileInfo fileInfo;
        DVDFastOpen(entryNum, &fileInfo);
        u32 size = fileInfo.length;
        DVDClose(&fileInfo);

        return size;
    }

    s32 convertPathToEntrynumConsideringLanguage(const char* pFilePath) {
        char filePath[256];
        makeFileNameConsideringLanguage(filePath, sizeof(filePath), pFilePath);

        return DVDConvertPathToEntrynum(filePath);
    }

    void* loadToMainRAM(const char* pFilePath, u8* pDst, JKRHeap* pHeap, JKRDvdRipper::EAllocDirection allocDir) {
        loadAsyncToMainRAM(pFilePath, pDst, pHeap, allocDir);

        return receiveFile(pFilePath);
    }

    void loadAsyncToMainRAM(const char* pFilePath, u8* pDst, JKRHeap* pHeap, JKRDvdRipper::EAllocDirection allocDir) {
        char filePath[256];
        makeFileNameConsideringLanguage(filePath, sizeof(filePath), pFilePath);

        SingletonHolder< FileLoader >::get()->requestLoadToMainRAM(filePath, pDst, pHeap, allocDir, false);
    }

    JKRMemArchive* mountArchive(const char* pFilePath, JKRHeap* pHeap) {
        char filePath[256];
        makeFileNameConsideringLanguage(filePath, sizeof(filePath), pFilePath);
        if (pHeap == nullptr) pHeap = getCurrentHeap();
        smgpc::compat::JkrHostAllocationScope host;
        auto* mounts = smgpc::runtime::ArchiveMountService::active();
        if (mounts == nullptr)
            aurora::throw_host_exception<std::logic_error>("Archive mounting requires its native mount owner");
        return mounts->mount(filePath, pHeap);
    }

    void mountAsyncArchive(const char* pFilePath, JKRHeap* pHeap) {
        (void)mountArchive(pFilePath, pHeap);
    }

    void mountAsyncArchiveByObjectOrLayoutName(const char* pFilePrefix, JKRHeap* pHeap) {
        char path[256]{};
        if (makeObjectArchiveFileNameFromPrefix(path, sizeof(path), pFilePrefix, false) ||
            makeLayoutArchiveFileNameFromPrefix(path, sizeof(path), pFilePrefix, false)) {
            (void)mountArchive(path, pHeap);
        }
    }

    void* receiveFile(const char* pFilePath) {
        char filePath[256];
        makeFileNameConsideringLanguage(filePath, sizeof(filePath), pFilePath);

        return SingletonHolder< FileLoader >::get()->receiveFile(filePath);
    }

    JKRMemArchive* receiveArchive(const char* pFilePath) {
        char filePath[256];
        makeFileNameConsideringLanguage(filePath, sizeof(filePath), pFilePath);
        auto* mounts = smgpc::runtime::ArchiveMountService::active();
        return mounts ? mounts->receive(filePath) : nullptr;
    }

    void receiveAllRequestedFile() {
        SingletonHolder< FileLoader >::get()->receiveAllRequestedFile();
    }

    void createAndAddArchive(void* pArcData, JKRHeap* pHeap, const char* pFilePath) {
        auto* mounts = smgpc::runtime::ArchiveMountService::active();
        if (mounts == nullptr)
            aurora::throw_host_exception<std::logic_error>("Archive registration requires its native mount owner");
        if (pArcData == nullptr || pFilePath == nullptr)
            aurora::throw_host_exception<std::invalid_argument>("Archive registration requires source bytes and a name");
        // Like the SDK's mountFixed(void*), the original API trusts its caller
        // to provide the complete buffer described by this big-endian header.
        const auto* bytes = static_cast<const u8*>(pArcData);
        if (std::memcmp(bytes, "RARC", 4) != 0)
            aurora::throw_host_exception<std::invalid_argument>("Fixed archive registration requires decompressed RARC bytes");
        const u32 size = (u32(bytes[4]) << 24) | (u32(bytes[5]) << 16) | (u32(bytes[6]) << 8) | bytes[7];
        (void)mounts->mount_memory_fixed(pFilePath, {bytes, size}, pHeap);
    }

    void getMountedArchiveAndHeap(const char* pFilePath, JKRArchive** ppArchive, JKRHeap** ppHeap) {
        char filePath[256];
        makeFileNameConsideringLanguage(filePath, sizeof(filePath), pFilePath);
        const auto* mounts = smgpc::runtime::ArchiveMountService::active();
        const auto owner = mounts ? mounts->retain(filePath) : nullptr;
        if (ppArchive) *ppArchive = owner ? &owner->archive() : nullptr;
        if (ppHeap) *ppHeap = owner ? owner->heap() : nullptr;
    }

    void removeFileConsideringLanguage(const char* pFilePath) {
        char filePath[256];
        makeFileNameConsideringLanguage(filePath, sizeof(filePath), pFilePath);

        SingletonHolder< FileLoader >::get()->removeFile(filePath);
    }

    void removeResourceAndFileHolderIfIsEqualHeap(JKRHeap* heap) {
        smgpc::compat::JkrHostAllocationScope host;
        if (heap == nullptr) return;
        if (auto* resources = smgpc::compat::ResourceHolderService::active()) resources->remove_for_heap(heap);
        if (auto* mounts = smgpc::runtime::ArchiveMountService::active()) mounts->remove_for_heap(heap);
        if (auto* loader = SingletonHolder<FileLoader>::get()) loader->removeHolderIfIsEqualHeap(heap);
    }

    void* decompressFileFromArchive(JKRArchive* pArchive, const char* pFilePath, JKRHeap* pHeap, int align) {
        if (pHeap == nullptr) pHeap = getCurrentHeap();
        smgpc::compat::JkrHostAllocationScope host;
        if (pArchive == nullptr || pFilePath == nullptr) return nullptr;
        const auto* data = static_cast<const u8*>(pArchive->getResource(pFilePath));
        if (data == nullptr) return nullptr;
        const auto size = pArchive->getResSize(data);
        if (size == 0 || size == UINT32_MAX) return nullptr;
        if (size >= 4 && std::memcmp(data, "Yay0", 4) == 0)
            aurora::throw_host_exception<std::logic_error>("Yay0 archive resource decoding is not implemented");
        const auto bytes = smgpc::resource::decompress_yaz0({data, size});
        // Preserve original caller ownership, heap choice and signed alignment.
        // Returning a cached vector prevented ordinary delete[]/heap retirement.
        auto* result = new (pHeap, align) u8[bytes.size()];
        std::memcpy(result, bytes.data(), bytes.size());
        return result;
    }

    bool isLoadedFile(const char* pFilePath) {
        char filePath[256];
        makeFileNameConsideringLanguage(filePath, sizeof(filePath), pFilePath);

        return SingletonHolder< FileLoader >::get()->isLoaded(filePath);
    }

    bool isMountedArchive(const char* pFilePath) {
        return receiveArchive(pFilePath) != nullptr;
    }

    bool isLoadedObjectOrLayoutArchive(const char* pFilePrefix) {
        char path[256]{};
        return (makeObjectArchiveFileNameFromPrefix(path, sizeof(path), pFilePrefix, false) && isMountedArchive(path)) ||
               (makeLayoutArchiveFileNameFromPrefix(path, sizeof(path), pFilePrefix, false) && isMountedArchive(path));
    }

    void makeFileNameConsideringLanguage(char* pDst, u32 size, const char* pFilePath) {
        addFilePrefix(pDst, size, pFilePath, getCurrentLanguagePrefix());

        if (!isFileExist(pDst, false)) {
            snprintf(pDst, size, "%s", pFilePath);
        }
    }

    bool makeObjectArchiveFileName(char* pDst, u32 size, const char* pFileName) {
        snprintf(pDst, size, "/ObjectData/%s", pFileName);

        if (isFileExist(pDst, true)) {
            return true;
        }

        snprintf(pDst, size, "/MapPartsData/%s", pFileName);

        if (isFileExist(pDst, false)) {
            return true;
        }

        snprintf(pDst, size, "%s", pFileName);

        return isFileExist(pDst, true);
    }

    bool makeObjectArchiveFileNameFromPrefix(char* pDst, u32 size, const char* pFilePrefix, bool) {
        char fileName[256];
        snprintf(fileName, sizeof(fileName), "%s.arc", pFilePrefix);

        return makeObjectArchiveFileName(pDst, size, fileName);
    }

    bool makeLayoutArchiveFileName(char* pDst, u32 size, const char* pFileName) {
        snprintf(pDst, size, "/Region/LayoutData/%s", pFileName);

        if (isFileExist(pDst, false)) {
            return true;
        }

        snprintf(pDst, size, "/LayoutData/%s", pFileName);

        if (isFileExist(pDst, true)) {
            return true;
        }

        snprintf(pDst, size, "%s", pFileName);

        return isFileExist(pDst, false);
    }

    bool makeLayoutArchiveFileNameFromPrefix(char* pDst, u32 size, const char* pFilePrefix, bool fallback) {
        char fileName[64];
        snprintf(fileName, sizeof(fileName), "%s.arc", pFilePrefix);
        bool isExistArc = makeLayoutArchiveFileName(pDst, size, fileName);

        const char* pAspectSuffix = isScreen16Per9() ? "16x9" : "4x3";
        snprintf(fileName, sizeof(fileName), "%s%s.arc", pFilePrefix, pAspectSuffix);
        bool isExistAspectArc = makeLayoutArchiveFileName(pDst, size, fileName);

        snprintf(fileName, sizeof(fileName), "%sReplace.arc", pFilePrefix);
        bool isExistReplaceArc = makeLayoutArchiveFileName(pDst, size, fileName);

        bool fileFound = isExistArc || isExistAspectArc || isExistReplaceArc;

        if (!fileFound && !fallback) {
            return false;
        }

        if (isExistAspectArc) {
            pAspectSuffix = isScreen16Per9() ? "16x9" : "4x3";
            snprintf(fileName, sizeof(fileName), "%s%s.arc", pFilePrefix, pAspectSuffix);
        } else if (isExistReplaceArc) {
            snprintf(fileName, sizeof(fileName), "%sReplace.arc", pFilePrefix);
        } else {
            snprintf(fileName, sizeof(fileName), "%s.arc", pFilePrefix);
        }

        makeLayoutArchiveFileName(pDst, size, fileName);

        return true;
    }

    void makeScenarioArchiveFileName(char* pDst, u32 size, const char* pStageName) {
        snprintf(pDst, size, "/StageData/%s/%sScenario.arc", pStageName, pStageName);
    }

}  // namespace MR
