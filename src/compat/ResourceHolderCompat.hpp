#pragma once

#include "Game/System/ResourceHolder.hpp"
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string_view>
#include <vector>

namespace smgpc::resource {
    class RarcArchive;
    class Mem1ResourceHeap;
}
namespace smgpc::runtime { class DvdFileSystemService; }

namespace smgpc::compat {
    class JkrAllocationDomain;

    // Native retained backing for the actual Game holder, not a replacement
    // Game layout. Archive and raw identity aliases outlive all loaded objects.
    class ResourceArchiveOwner final {
    public:
        ResourceArchiveOwner(std::shared_ptr<const resource::RarcArchive>, std::filesystem::path,
                             std::shared_ptr<JkrAllocationDomain>, std::shared_ptr<resource::Mem1ResourceHeap>);
        ~ResourceArchiveOwner();
        ResourceArchiveOwner(const ResourceArchiveOwner&) = delete;
        ResourceArchiveOwner& operator=(const ResourceArchiveOwner&) = delete;
        [[nodiscard]] ResourceHolder& holder() const noexcept;
        [[nodiscard]] const resource::RarcArchive& archive() const noexcept;
        [[nodiscard]] const std::filesystem::path& resolved_path() const noexcept;

    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };

    class ResourceHolderService final {
    public:
        ResourceHolderService(runtime::DvdFileSystemService&, std::shared_ptr<JkrAllocationDomain>,
                              std::shared_ptr<resource::Mem1ResourceHeap>);
        ~ResourceHolderService();
        ResourceHolderService(const ResourceHolderService&) = delete;
        ResourceHolderService& operator=(const ResourceHolderService&) = delete;
        [[nodiscard]] ResourceHolder* create_and_add(std::string_view archive_name);
        [[nodiscard]] std::vector<ResourceHolder*> create_and_add_stationed(std::int32_t load_type);
        [[nodiscard]] std::shared_ptr<const ResourceArchiveOwner> retain(const ResourceHolder&) const;
        [[nodiscard]] const ResourceArchiveOwner& backing(const ResourceHolder&) const;
        [[nodiscard]] const std::shared_ptr<JkrAllocationDomain>& allocation_domain() const noexcept;
        [[nodiscard]] static ResourceHolderService* active() noexcept;

    private:
        runtime::DvdFileSystemService* _dvd;
        std::shared_ptr<JkrAllocationDomain> _domain;
        std::shared_ptr<resource::Mem1ResourceHeap> _mem1;
        std::map<std::filesystem::path, std::shared_ptr<ResourceArchiveOwner>, std::less<>> _holders;
    };
}
