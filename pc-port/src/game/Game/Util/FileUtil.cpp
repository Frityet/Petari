#include "Game/Util/FileUtil.hpp"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>

#include "compat/RuntimeAssetLoader.hpp"

namespace {

std::unordered_map<std::string, std::unique_ptr<JKRMemArchive>> sMountedArchives {};

void copy_path(char *pDst, u32 size, const std::string &path) {
    if (pDst == nullptr || size == 0U) {
        return;
    }

    std::snprintf(pDst, size, "%s", path.c_str());
}

[[nodiscard]] std::string canonical_path(const smgpc::assets::AssetLoader &assetLoader, const char *pFilePath) {
    if (pFilePath == nullptr || pFilePath[0] == '\0') {
        return "/";
    }

    return assetLoader.file_name_considering_language(pFilePath);
}

}  // namespace

namespace MR {

bool isFileExist(const char *pFilePath, bool considerLanguage) {
    const smgpc::game::compat::RuntimeAssetLoaderScope assetLoader {};
    if (!assetLoader || pFilePath == nullptr) {
        return false;
    }

    return assetLoader->is_file_exist(pFilePath, considerLanguage);
}

void *loadToMainRAM(const char *pFilePath, u8 *pDst, JKRHeap *pHeap, JKRDvdRipper::EAllocDirection allocDir) {
    loadAsyncToMainRAM(pFilePath, pDst, pHeap, allocDir);
    return receiveFile(pFilePath);
}

void loadAsyncToMainRAM(const char *pFilePath, u8 *pDst, JKRHeap *pHeap, JKRDvdRipper::EAllocDirection allocDir) {
    (void)pDst;
    (void)pHeap;
    (void)allocDir;

    const smgpc::game::compat::RuntimeAssetLoaderScope assetLoader {};
    if (!assetLoader || pFilePath == nullptr) {
        return;
    }

    assetLoader->request_file(pFilePath);
}

JKRMemArchive *mountArchive(const char *pFilePath, JKRHeap *pHeap) {
    mountAsyncArchive(pFilePath, pHeap);
    return receiveArchive(pFilePath);
}

void mountAsyncArchive(const char *pFilePath, JKRHeap *pHeap) {
    (void)pHeap;

    const smgpc::game::compat::RuntimeAssetLoaderScope assetLoader {};
    if (!assetLoader || pFilePath == nullptr) {
        return;
    }

    assetLoader->request_archive(pFilePath);
}

void mountAsyncArchiveByObjectOrLayoutName(const char *pFilePrefix, JKRHeap *pHeap) {
    (void)pHeap;

    const smgpc::game::compat::RuntimeAssetLoaderScope assetLoader {};
    if (!assetLoader || pFilePrefix == nullptr) {
        return;
    }

    auto object_path = assetLoader->object_archive_file_name_from_prefix(pFilePrefix, false);
    if (object_path.has_value()) {
        mountAsyncArchive(object_path->c_str(), nullptr);
        return;
    }

    auto layout_path = assetLoader->layout_archive_file_name_from_prefix(pFilePrefix, false);
    if (layout_path.has_value()) {
        mountAsyncArchive(layout_path->c_str(), nullptr);
    }
}

void *receiveFile(const char *pFilePath) {
    const smgpc::game::compat::RuntimeAssetLoaderScope assetLoader {};
    if (!assetLoader || pFilePath == nullptr) {
        return nullptr;
    }

    const auto bytes = assetLoader->file(pFilePath);
    if (bytes == nullptr || bytes->empty()) {
        return nullptr;
    }

    return const_cast<std::byte *>(bytes->data());
}

JKRMemArchive *receiveArchive(const char *pFilePath) {
    const smgpc::game::compat::RuntimeAssetLoaderScope assetLoader {};
    if (!assetLoader || pFilePath == nullptr) {
        return nullptr;
    }

    const auto mounted = assetLoader->archive(pFilePath);
    if (mounted == nullptr) {
        return nullptr;
    }

    const auto key = canonical_path(*assetLoader, pFilePath);
    const auto existing = sMountedArchives.find(key);
    if (existing != sMountedArchives.end()) {
        return existing->second.get();
    }

    auto archive = std::make_unique<JKRMemArchive>(mounted);
    auto *archive_ptr = archive.get();
    sMountedArchives.emplace(key, std::move(archive));
    return archive_ptr;
}

bool isLoadedFile(const char *pFilePath) {
    const smgpc::game::compat::RuntimeAssetLoaderScope assetLoader {};
    if (!assetLoader || pFilePath == nullptr) {
        return false;
    }

    return assetLoader->is_loaded_file(pFilePath);
}

bool isMountedArchive(const char *pFilePath) {
    const smgpc::game::compat::RuntimeAssetLoaderScope assetLoader {};
    if (!assetLoader || pFilePath == nullptr) {
        return false;
    }

    return assetLoader->is_mounted_archive(pFilePath);
}

bool isLoadedObjectOrLayoutArchive(const char *pFilePrefix) {
    const smgpc::game::compat::RuntimeAssetLoaderScope assetLoader {};
    if (!assetLoader || pFilePrefix == nullptr) {
        return false;
    }

    char object_path[256] {};
    const bool has_object = makeObjectArchiveFileNameFromPrefix(object_path, sizeof(object_path), pFilePrefix, false);

    char layout_path[256] {};
    const bool has_layout = makeLayoutArchiveFileNameFromPrefix(layout_path, sizeof(layout_path), pFilePrefix, false);

    if (has_object) {
        return isLoadedFile(object_path);
    }

    if (has_layout) {
        return isLoadedFile(layout_path);
    }

    return false;
}

void makeFileNameConsideringLanguage(char *pDst, u32 size, const char *pFilePath) {
    const smgpc::game::compat::RuntimeAssetLoaderScope assetLoader {};
    if (!assetLoader || pFilePath == nullptr) {
        copy_path(pDst, size, pFilePath == nullptr ? std::string {} : std::string(pFilePath));
        return;
    }

    copy_path(pDst, size, assetLoader->file_name_considering_language(pFilePath));
}

bool makeObjectArchiveFileName(char *pDst, u32 size, const char *pFileName) {
    const smgpc::game::compat::RuntimeAssetLoaderScope assetLoader {};
    if (!assetLoader || pFileName == nullptr) {
        return false;
    }

    const auto path = assetLoader->object_archive_file_name(pFileName);
    if (not path.has_value()) {
        return false;
    }

    copy_path(pDst, size, *path);
    return true;
}

bool makeObjectArchiveFileNameFromPrefix(char *pDst, u32 size, const char *pFilePrefix, bool unused) {
    const smgpc::game::compat::RuntimeAssetLoaderScope assetLoader {};
    if (!assetLoader || pFilePrefix == nullptr) {
        return false;
    }

    const auto path = assetLoader->object_archive_file_name_from_prefix(pFilePrefix, unused);
    if (not path.has_value()) {
        return false;
    }

    copy_path(pDst, size, *path);
    return true;
}

bool makeLayoutArchiveFileName(char *pDst, u32 size, const char *pFileName) {
    const smgpc::game::compat::RuntimeAssetLoaderScope assetLoader {};
    if (!assetLoader || pFileName == nullptr) {
        return false;
    }

    const auto path = assetLoader->layout_archive_file_name(pFileName);
    if (not path.has_value()) {
        return false;
    }

    copy_path(pDst, size, *path);
    return true;
}

bool makeLayoutArchiveFileNameFromPrefix(char *pDst, u32 size, const char *pFilePrefix, bool fallback) {
    const smgpc::game::compat::RuntimeAssetLoaderScope assetLoader {};
    if (!assetLoader || pFilePrefix == nullptr) {
        return false;
    }

    const auto path = assetLoader->layout_archive_file_name_from_prefix(pFilePrefix, fallback);
    if (not path.has_value()) {
        return false;
    }

    copy_path(pDst, size, *path);
    return true;
}

void makeScenarioArchiveFileName(char *pDst, u32 size, const char *pStageName) {
    if (pDst == nullptr || size == 0U || pStageName == nullptr) {
        return;
    }

    std::snprintf(pDst, size, "/StageData/%s/%sScenario.arc", pStageName, pStageName);
}

}  // namespace MR
