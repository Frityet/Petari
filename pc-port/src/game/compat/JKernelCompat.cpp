#include "JKernelCompat.hpp"

#include <string>

void *JKRArchive::getResource(const char *pFilePath) const {
    const auto span = getResourceSpan(pFilePath);
    if (span.empty()) {
        return nullptr;
    }

    return const_cast<std::byte *>(span.data());
}

JKRMemArchive::JKRMemArchive(std::shared_ptr<const smgpc::assets::MountedArchiveData> archive_data)
    : mArchiveData(std::move(archive_data)) {
    if (mArchiveData == nullptr) {
        return;
    }

    for (const auto &entry : mArchiveData->archive.entries()) {
        const auto bytes = mArchiveData->archive.find_entry(entry.path);
        if (bytes.empty()) {
            continue;
        }

        mResourceSizes[bytes.data()] = static_cast<u32>(bytes.size());
    }
}

std::span<const std::byte> JKRMemArchive::getResourceSpan(const char *pFilePath) const {
    if (mArchiveData == nullptr || pFilePath == nullptr) {
        return {};
    }

    return mArchiveData->archive.find_entry(std::string(pFilePath));
}

void *JKRMemArchive::getResource(const char *pFilePath) const {
    const auto span = getResourceSpan(pFilePath);
    if (span.empty()) {
        return nullptr;
    }

    return const_cast<std::byte *>(span.data());
}

u32 JKRMemArchive::getResSize(const void *pResource) const {
    if (pResource == nullptr) {
        return 0;
    }

    const auto found = mResourceSizes.find(pResource);
    if (found == mResourceSizes.end()) {
        return 0;
    }

    return found->second;
}

const smgpc::assets::layout::RarcArchive *JKRMemArchive::archive() const {
    if (mArchiveData == nullptr) {
        return nullptr;
    }

    return &mArchiveData->archive;
}
