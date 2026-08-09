#include "compat/ResourceHolderCompat.hpp"

#include "Game/System/StationedFileInfo.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"

#include <algorithm>
#include <stdexcept>

namespace {

    smgpc::compat::ResourceHolderService *sActiveResourceHolderService = nullptr;

    [[nodiscard]] std::filesystem::path normalize_archive_request(std::string_view archive_name) {
        auto normalized = std::string(archive_name);
        std::ranges::replace(normalized, '\\', '/');
        while (!normalized.empty() && normalized.front() == '/') {
            normalized.erase(normalized.begin());
        }
        return std::filesystem::path(normalized).lexically_normal();
    }

}  // namespace

ResourceHolder::ResourceHolder(const smgpc::resource::RarcArchive &archive,
                               std::filesystem::path resolved_path)
    : _archive(&archive), _resolved_path(std::move(resolved_path)) {
}

const smgpc::resource::RarcArchive &ResourceHolder::archive() const noexcept {
    return *_archive;
}

std::span<const std::uint8_t> ResourceHolder::resource_data(std::string_view resource_name) const {
    return _archive->resource_data(resource_name);
}

const std::filesystem::path &ResourceHolder::resolved_path() const noexcept {
    return _resolved_path;
}

namespace smgpc::compat {

    ResourceHolderService::ResourceHolderService(smgpc::runtime::DvdFileSystemService &dvd)
        : _dvd(&dvd) {
        if (sActiveResourceHolderService != nullptr) {
            throw std::logic_error("Only one ResourceHolder service may be active.");
        }
        sActiveResourceHolderService = this;
    }

    ResourceHolderService::~ResourceHolderService() {
        if (sActiveResourceHolderService == this) {
            sActiveResourceHolderService = nullptr;
        }
    }

    ResourceHolder *ResourceHolderService::create_and_add(std::string_view archive_name) {
        const auto requested = normalize_archive_request(archive_name);
        if (requested.empty() || requested == "." || requested.filename().empty()) {
            throw std::invalid_argument("ResourceHolder requires an exact archive name.");
        }

        const auto resolved = _dvd->find_first({
            std::filesystem::path("ObjectData") / requested,
            std::filesystem::path("MapPartsData") / requested,
            requested,
        });
        if (!resolved.has_value()) {
            throw std::runtime_error("Required ResourceHolder archive is unavailable: " +
                                     requested.generic_string());
        }

        const auto key = _dvd->resolve(resolved->generic_string());
        if (const auto found = _holders.find(key); found != _holders.end()) {
            return found->second.get();
        }

        auto &archive = _dvd->archive_for_path(*resolved);
        auto holder = std::unique_ptr<ResourceHolder>(new ResourceHolder(archive, key));
        auto *result = holder.get();
        _holders.emplace(key, std::move(holder));
        return result;
    }

    std::vector<ResourceHolder *> ResourceHolderService::create_and_add_stationed(std::int32_t load_type) {
        auto resources = std::vector<ResourceHolder *>{};
        for (auto *info = MR::getStationedFileInfoTable(); info->mArchive != nullptr; ++info) {
            if (info->mLoadType == load_type) {
                resources.push_back(create_and_add(info->mArchive));
            }
        }
        return resources;
    }

    ResourceHolderService *ResourceHolderService::active() noexcept {
        return sActiveResourceHolderService;
    }

}  // namespace smgpc::compat
