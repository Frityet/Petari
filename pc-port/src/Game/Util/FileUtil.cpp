#include "Game/Util/FileUtil.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Game/compat/RuntimeContext.hpp"

namespace MR {
    namespace {

        std::map< std::string, std::vector< u8 > > sLoadedFiles;
        std::map< std::string, std::unique_ptr< JKRMemArchive > > sMountedArchives;
        std::map< std::string, std::vector< u8 > > sArchiveResources;

        [[nodiscard]] smgpc::game::RuntimeContext* runtime() {
            return smgpc::game::RuntimeContext::try_instance();
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
            auto* context = runtime();
            if (context == nullptr || path.empty()) {
                return false;
            }

            try {
                return context->dvd().exists(path);
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

        [[nodiscard]] std::string mounted_archive_key(const char* path) {
            const auto normalized = normalize_disc_string(path);
            auto* context = runtime();
            if (context == nullptr) {
                return normalized;
            }

            try {
                return context->dvd().resolve(normalized).generic_string();
            } catch (const std::exception&) {
                return normalized;
            }
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

    void* loadToMainRAM(const char* pFilePath, u8* pDst, JKRHeap*, JKRDvdRipper::EAllocDirection) {
        auto* context = runtime();
        if (context == nullptr || pFilePath == nullptr) {
            return nullptr;
        }

        const auto path = path_considering_language(pFilePath, true);
        try {
            auto bytes = context->dvd().read_file(path);
            if (pDst != nullptr) {
                std::memcpy(pDst, bytes.data(), bytes.size());
                return pDst;
            }

            auto it = sLoadedFiles.insert_or_assign(path, std::move(bytes)).first;
            return it->second.empty() ? nullptr : it->second.data();
        } catch (const std::exception&) {
            return nullptr;
        }
    }

    void loadAsyncToMainRAM(const char* pFilePath, u8* pDst, JKRHeap* pHeap, JKRDvdRipper::EAllocDirection allocDir) {
        (void)loadToMainRAM(pFilePath, pDst, pHeap, allocDir);
    }

    JKRMemArchive* mountArchive(const char* pFilePath, JKRHeap*) {
        auto* context = runtime();
        if (context == nullptr || pFilePath == nullptr) {
            return nullptr;
        }

        const auto key = mounted_archive_key(pFilePath);
        if (auto it = sMountedArchives.find(key); it != sMountedArchives.end()) {
            return it->second.get();
        }

        try {
            auto mounted = std::make_unique< JKRMemArchive >(smgpc::game::RarcArchive::from_file(context->dvd().resolve(pFilePath)));
            auto* result = mounted.get();
            sMountedArchives.emplace(key, std::move(mounted));
            return result;
        } catch (const std::exception&) {
            return nullptr;
        }
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
        const auto path = path_considering_language(pFilePath, true);
        if (auto it = sLoadedFiles.find(path); it != sLoadedFiles.end()) {
            return it->second.empty() ? nullptr : it->second.data();
        }

        return nullptr;
    }

    JKRMemArchive* receiveArchive(const char* pFilePath) {
        if (auto it = sMountedArchives.find(mounted_archive_key(pFilePath)); it != sMountedArchives.end()) {
            return it->second.get();
        }

        return nullptr;
    }

    void receiveAllRequestedFile() {
    }

    void createAndAddArchive(void*, JKRHeap*, const char*) {
    }

    void getMountedArchiveAndHeap(const char* pFilePath, JKRArchive** ppArchive, JKRHeap** ppHeap) {
        if (ppArchive != nullptr) {
            *ppArchive = receiveArchive(pFilePath);
        }
        if (ppHeap != nullptr) {
            *ppHeap = nullptr;
        }
    }

    void removeFileConsideringLanguage(const char* pFilePath) {
        sLoadedFiles.erase(path_considering_language(pFilePath, true));
    }

    void removeResourceAndFileHolderIfIsEqualHeap(JKRHeap*) {
    }

    void* decompressFileFromArchive(JKRArchive* pArchive, const char* pFilePath, JKRHeap*, int) {
        if (pArchive == nullptr || pFilePath == nullptr) {
            return nullptr;
        }

        const auto* resource = static_cast< const u8* >(pArchive->getResource(pFilePath));
        const auto size = pArchive->getResSize(resource);
        if (resource == nullptr || size == 0U) {
            return nullptr;
        }

        auto& bytes = sArchiveResources[std::string(pFilePath)];
        bytes.assign(resource, resource + size);
        return bytes.data();
    }

    bool isLoadedFile(const char* pFilePath) {
        return sLoadedFiles.contains(path_considering_language(pFilePath, true));
    }

    bool isMountedArchive(const char* pFilePath) {
        return sMountedArchives.contains(mounted_archive_key(pFilePath));
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
