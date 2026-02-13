#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "AssetServices.hpp"
#include "GameAssetPathResolver.hpp"
#include "layout/RarcArchive.hpp"

namespace smgpc::logging {
class ILogger;
}

namespace smgpc::assets {

struct MountedArchiveData {
    layout::RarcArchive archive {};
};

class IGameAssetService {
public:
    virtual ~IGameAssetService() = default;

    virtual void request_load_file(std::string_view file_path) = 0;
    [[nodiscard]] virtual std::shared_ptr<const std::vector<std::byte>> receive_file(std::string_view file_path) = 0;
    [[nodiscard]] virtual bool is_loaded_file(std::string_view file_path) const = 0;

    virtual void request_mount_archive(std::string_view archive_path) = 0;
    [[nodiscard]] virtual std::shared_ptr<const MountedArchiveData> receive_archive(std::string_view archive_path) = 0;
    [[nodiscard]] virtual bool is_mounted_archive(std::string_view archive_path) const = 0;

    [[nodiscard]] virtual const GameAssetPathResolver &path_resolver() const = 0;
};

class GameAssetService final : public IGameAssetService {
public:
    GameAssetService(
        std::shared_ptr<IAssetManager> asset_manager,
        GameAssetPathResolverConfiguration resolver_configuration,
        std::shared_ptr<logging::ILogger> logger);

    void request_load_file(std::string_view file_path) override;
    [[nodiscard]] std::shared_ptr<const std::vector<std::byte>> receive_file(std::string_view file_path) override;
    [[nodiscard]] bool is_loaded_file(std::string_view file_path) const override;

    void request_mount_archive(std::string_view archive_path) override;
    [[nodiscard]] std::shared_ptr<const MountedArchiveData> receive_archive(std::string_view archive_path) override;
    [[nodiscard]] bool is_mounted_archive(std::string_view archive_path) const override;

    [[nodiscard]] const GameAssetPathResolver &path_resolver() const override;

private:
    struct FileRecord {
        std::shared_ptr<std::vector<std::byte>> bytes {};
        std::optional<AssetError> error {};
    };

    struct ArchiveRecord {
        std::shared_ptr<MountedArchiveData> archive {};
        std::optional<AssetError> error {};
    };

    [[nodiscard]] std::string canonical_path(std::string_view file_path) const;
    [[nodiscard]] static AssetError make_invalid_error(std::string message);

    std::shared_ptr<IAssetManager> _asset_manager {};
    std::shared_ptr<logging::ILogger> _logger {};
    GameAssetPathResolver _path_resolver;

    mutable std::mutex _mutex {};
    std::unordered_map<std::string, FileRecord> _files {};
    std::unordered_map<std::string, ArchiveRecord> _archives {};
};

[[nodiscard]] std::shared_ptr<IGameAssetService> create_default_game_asset_service(
    std::shared_ptr<IAssetManager> asset_manager,
    GameAssetPathResolverConfiguration resolver_configuration,
    std::shared_ptr<logging::ILogger> logger);

}  // namespace smgpc::assets
