#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::resource {
    class RarcArchive;
}

namespace smgpc::runtime {
    class DvdFileSystemService;
}

namespace smgpc::compat {
    class ResourceHolderService;
}

class ResourceHolder final {
public:
    [[nodiscard]] const smgpc::resource::RarcArchive &archive() const noexcept;
    [[nodiscard]] std::span<const std::uint8_t> resource_data(std::string_view resource_name) const;
    [[nodiscard]] const std::filesystem::path &resolved_path() const noexcept;

private:
    friend class smgpc::compat::ResourceHolderService;

    ResourceHolder(const smgpc::resource::RarcArchive &archive, std::filesystem::path resolved_path);

    const smgpc::resource::RarcArchive *_archive;
    std::filesystem::path _resolved_path;
};

namespace smgpc::compat {

    // Runtime owner for the retail ResourceHolderManager boundary. Requests
    // are resolved from their exact archive name and retained for the active
    // runtime; no placement or actor-name inference is performed here.
    class ResourceHolderService final {
    public:
        explicit ResourceHolderService(smgpc::runtime::DvdFileSystemService &dvd);
        ~ResourceHolderService();

        ResourceHolderService(const ResourceHolderService &) = delete;
        ResourceHolderService &operator=(const ResourceHolderService &) = delete;

        [[nodiscard]] ResourceHolder *create_and_add(std::string_view archive_name);
        [[nodiscard]] std::vector<ResourceHolder *> create_and_add_stationed(std::int32_t load_type);
        [[nodiscard]] static ResourceHolderService *active() noexcept;

    private:
        smgpc::runtime::DvdFileSystemService *_dvd;
        std::map<std::filesystem::path, std::unique_ptr<ResourceHolder>, std::less<>> _holders;
    };

}  // namespace smgpc::compat
