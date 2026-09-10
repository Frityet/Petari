#include "Game/Util/FileUtil.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "runtime/RuntimeContext.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "resource/Yaz0.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>

namespace MR {
    namespace {

        struct LoadedFile {
            std::vector<u8> bytes;
            JKRHeap* heap;
        };
        std::map<std::string, LoadedFile> sLoadedFiles;
        std::mutex sLoadedFilesMutex;

        [[nodiscard]] smgpc::runtime::RuntimeContext* runtime() {
            return smgpc::runtime::RuntimeContext::try_instance();
        }

        [[nodiscard]] std::string normalize_disc_string(const char* path) {
            if (path == nullptr) {
                return {};
            }

            auto text = std::string(path);
            std::ranges::replace(text, '\\', '/');
            if (text.empty()) {
                return {};
            }
            if (text.front() != '/') {
                text.insert(text.begin(), '/');
            }
            return text;
        }

        [[nodiscard]] bool dvd_exists(std::string_view path) {
            auto* mounts = smgpc::runtime::ArchiveMountService::active();
            if (mounts == nullptr || path.empty()) {
                return false;
            }

            try {
                return mounts->dvd().exists(path);
            } catch (const std::exception&) {
                return false;
            }
        }

        [[nodiscard]] std::string language_path_for(std::string_view path) {
            auto text = std::string(path);
            std::ranges::replace(text, '\\', '/');
            while (!text.empty() && text.front() == '/') {
                text.erase(text.begin());
            }

            if (text.starts_with("LayoutData/")) {
                return "/KrKorean/" + text;
            }

            return "/" + text;
        }

        [[nodiscard]] std::string path_considering_language(const char* path, bool consider_language) {
            const auto normalized = normalize_disc_string(path);
            if (normalized.empty() || !consider_language) {
                return normalized;
            }

            const auto localized = language_path_for(normalized);
            return dvd_exists(localized) ? localized : normalized;
        }

        void copy_path(char* dst, u32 size, std::string_view path) {
            if (dst == nullptr || size == 0U) {
                return;
            }

            const auto count = std::min< std::size_t >(path.size(), static_cast< std::size_t >(size - 1U));
            std::memcpy(dst, path.data(), count);
            dst[count] = '\0';
        }

        [[nodiscard]] std::string with_arc_extension(const char* prefix) {
            auto text = std::string(prefix == nullptr ? "" : prefix);
            if (!text.ends_with(".arc")) {
                text += ".arc";
            }
            return text;
        }

        [[nodiscard]] bool copy_first_existing(char* dst, u32 size, std::initializer_list< std::string_view > candidates) {
            for (const auto candidate : candidates) {
                if (dvd_exists(candidate)) {
                    copy_path(dst, size, candidate);
                    return true;
                }
            }

            return false;
        }

    }  // namespace

    bool isFileExist(const char* pFilePath, bool considerLanguage) {
        return dvd_exists(path_considering_language(pFilePath, considerLanguage));
    }

    u32 getFileSize(const char* pFilePath, bool considerLanguage) {
        auto* context = runtime();
        if (context == nullptr) {
            return 0U;
        }

        try {
            const auto path = context->dvd().resolve(path_considering_language(pFilePath, considerLanguage));
            std::error_code error{};
            const auto size = std::filesystem::file_size(path, error);
            if (error || size > static_cast< std::uintmax_t >(UINT32_MAX)) {
                return 0U;
            }

            return static_cast< u32 >(size);
        } catch (const std::exception&) {
            return 0U;
        }
    }

    s32 convertPathToEntrynumConsideringLanguage(const char* pFilePath) {
        const auto path = path_considering_language(pFilePath, true);
        return DVDConvertPathToEntrynum(path.c_str());
    }

    void* loadToMainRAM(const char* pFilePath, u8* pDst, JKRHeap* pHeap, JKRDvdRipper::EAllocDirection) {
        smgpc::compat::JkrHostAllocationScope host;
        auto* context = runtime();
        if (context == nullptr || pFilePath == nullptr) {
            return nullptr;
        }

        const auto path = path_considering_language(pFilePath, true);
        try {
            if (pDst == nullptr) {
                const std::lock_guard lock(sLoadedFilesMutex);
                if (const auto found = sLoadedFiles.find(path); found != sLoadedFiles.end())
                    return found->second.bytes.empty() ? nullptr : found->second.bytes.data();
            }
            auto bytes = context->dvd().read_file(path);
            if (pDst != nullptr) {
                std::memcpy(pDst, bytes.data(), bytes.size());
                return pDst;
            }

            const std::lock_guard lock(sLoadedFilesMutex);
            auto it = sLoadedFiles.try_emplace(path, LoadedFile{std::move(bytes), pHeap ? pHeap : getCurrentHeap()}).first;
            return it->second.bytes.empty() ? nullptr : it->second.bytes.data();
        } catch (const std::exception&) {
            return nullptr;
        }
    }

    void loadAsyncToMainRAM(const char* pFilePath, u8* pDst, JKRHeap* pHeap, JKRDvdRipper::EAllocDirection allocDir) {
        (void)loadToMainRAM(pFilePath, pDst, pHeap, allocDir);
    }

    JKRMemArchive* mountArchive(const char* pFilePath, JKRHeap* pHeap) {
        smgpc::compat::JkrHostAllocationScope host;
        auto* mounts = smgpc::runtime::ArchiveMountService::active();
        if (mounts == nullptr || pFilePath == nullptr) return nullptr;
        try { return mounts->mount(pFilePath, pHeap); }
        catch (const std::exception&) { return nullptr; }
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
        smgpc::compat::JkrHostAllocationScope host;
        const auto path = path_considering_language(pFilePath, true);
        const std::lock_guard lock(sLoadedFilesMutex);
        if (auto it = sLoadedFiles.find(path); it != sLoadedFiles.end()) {
            return it->second.bytes.empty() ? nullptr : it->second.bytes.data();
        }

        return nullptr;
    }

    JKRMemArchive* receiveArchive(const char* pFilePath) {
        auto* mounts = smgpc::runtime::ArchiveMountService::active();
        if (mounts == nullptr || pFilePath == nullptr) return nullptr;
        return mounts->receive(pFilePath);
    }

    void receiveAllRequestedFile() {
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
        const auto* mounts = smgpc::runtime::ArchiveMountService::active();
        const auto owner = mounts && pFilePath ? mounts->retain(pFilePath) : nullptr;
        if (ppArchive) *ppArchive = owner ? &owner->archive() : nullptr;
        if (ppHeap) *ppHeap = owner ? owner->heap() : nullptr;
    }

    void removeFileConsideringLanguage(const char* pFilePath) {
        smgpc::compat::JkrHostAllocationScope host;
        const auto path = path_considering_language(pFilePath, true);
        const std::lock_guard lock(sLoadedFilesMutex);
        sLoadedFiles.erase(path);
    }

    void removeResourceAndFileHolderIfIsEqualHeap(JKRHeap* heap) {
        smgpc::compat::JkrHostAllocationScope host;
        if (heap == nullptr) return;
        if (auto* resources = smgpc::compat::ResourceHolderService::active()) resources->remove_for_heap(heap);
        if (auto* mounts = smgpc::runtime::ArchiveMountService::active()) mounts->remove_for_heap(heap);
        const std::lock_guard lock(sLoadedFilesMutex);
        std::erase_if(sLoadedFiles, [heap](const auto& entry) { return entry.second.heap == heap; });
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
        smgpc::compat::JkrHostAllocationScope host;
        const auto path = path_considering_language(pFilePath, true);
        const std::lock_guard lock(sLoadedFilesMutex);
        return sLoadedFiles.contains(path);
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
        copy_path(pDst, size, path_considering_language(pFilePath, true));
    }

    bool makeObjectArchiveFileName(char* pDst, u32 size, const char* pFileName) {
        const auto name = std::string(pFileName == nullptr ? "" : pFileName);
        const auto object = "/ObjectData/" + name;
        const auto map_parts = "/MapPartsData/" + name;
        const auto raw = normalize_disc_string(pFileName);
        return copy_first_existing(pDst, size, {object, map_parts, raw});
    }

    bool makeObjectArchiveFileNameFromPrefix(char* pDst, u32 size, const char* pFilePrefix, bool) {
        const auto name = with_arc_extension(pFilePrefix);
        return makeObjectArchiveFileName(pDst, size, name.c_str());
    }

    bool makeLayoutArchiveFileName(char* pDst, u32 size, const char* pFileName) {
        const auto name = std::string(pFileName == nullptr ? "" : pFileName);
        const auto localized = "/KrKorean/LayoutData/" + name;
        const auto base = "/LayoutData/" + name;
        const auto raw = normalize_disc_string(pFileName);
        return copy_first_existing(pDst, size, {localized, base, raw});
    }

    bool makeLayoutArchiveFileNameFromPrefix(char* pDst, u32 size, const char* pFilePrefix, bool fallback) {
        const auto name = with_arc_extension(pFilePrefix);
        if (makeLayoutArchiveFileName(pDst, size, name.c_str())) {
            return true;
        }

        if (fallback) {
            copy_path(pDst, size, "/LayoutData/" + name);
        }
        return false;
    }

    void makeScenarioArchiveFileName(char* pDst, u32 size, const char* pStageName) {
        const auto stage = std::string(pStageName == nullptr ? "" : pStageName);
        copy_path(pDst, size, "/StageData/" + stage + "/" + stage + "Scenario.arc");
    }

}  // namespace MR
