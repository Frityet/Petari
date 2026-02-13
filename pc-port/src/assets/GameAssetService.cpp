#include "GameAssetService.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Logger.hpp"
#include "PackedAsset.hpp"
#include "layout/Yaz0.hpp"

namespace smgpc::assets {

GameAssetService::GameAssetService(
    di::DependencyReference<IAssetManager> asset_manager,
    GameAssetPathResolverConfiguration resolver_configuration,
    di::DependencyReference<logging::ILogger> logger)
    : _asset_manager(std::move(asset_manager)),
      _logger(std::move(logger)),
      _path_resolver(std::move(resolver_configuration)) {
}

void GameAssetService::request_load_file(std::string_view file_path) {
    const auto canonical = canonical_path(file_path);

    {
        const auto lock = std::scoped_lock(_mutex);
        const auto found = _files.find(canonical);
        if (found != _files.end() && (found->second.bytes != nullptr || found->second.error.has_value())) {
            return;
        }
    }

    FileRecord record {};
    const auto cached = _asset_manager->load_cached_asset(AssetId {
        .logical_path = _path_resolver.to_logical_path(canonical)
    });
    if (not cached) {
        record.error = cached.failure();
    } else {
        auto unpacked = unpack_packed_asset(*cached);
        if (not unpacked) {
            record.error = unpacked.failure();
        } else {
            record.bytes = std::make_shared<std::vector<std::byte>>(std::move(*unpacked));
        }
    }

    {
        const auto lock = std::scoped_lock(_mutex);
        _files[canonical] = std::move(record);
    }
}

std::shared_ptr<const std::vector<std::byte>> GameAssetService::receive_file(std::string_view file_path) {
    const auto canonical = canonical_path(file_path);
    request_load_file(canonical);

    const auto lock = std::scoped_lock(_mutex);
    const auto found = _files.find(canonical);
    if (found == _files.end() || found->second.bytes == nullptr) {
        return nullptr;
    }
    return found->second.bytes;
}

bool GameAssetService::is_loaded_file(std::string_view file_path) const {
    const auto canonical = canonical_path(file_path);

    const auto lock = std::scoped_lock(_mutex);
    const auto found = _files.find(canonical);
    return found != _files.end() && found->second.bytes != nullptr;
}

void GameAssetService::request_mount_archive(std::string_view archive_path) {
    const auto canonical = canonical_path(archive_path);

    {
        const auto lock = std::scoped_lock(_mutex);
        const auto found = _archives.find(canonical);
        if (found != _archives.end() && (found->second.archive != nullptr || found->second.error.has_value())) {
            return;
        }
    }

    request_load_file(canonical);

    ArchiveRecord record {};
    auto file_bytes = receive_file(canonical);
    if (file_bytes == nullptr) {
        const auto lock = std::scoped_lock(_mutex);
        const auto file_record = _files.find(canonical);
        if (file_record != _files.end() && file_record->second.error.has_value()) {
            record.error = file_record->second.error;
        } else {
            record.error = make_invalid_error("Archive request failed before file bytes were available: " + canonical);
        }
    } else {
        std::vector<std::byte> archive_bytes(*file_bytes);

        if (layout::is_yaz0(archive_bytes)) {
            auto decoded = layout::decode_yaz0(archive_bytes);
            if (not decoded) {
                record.error = decoded.failure();
            } else {
                archive_bytes = std::move(*decoded);
            }
        }

        if (not record.error.has_value()) {
            auto parsed_archive = layout::RarcArchive::parse(std::move(archive_bytes));
            if (not parsed_archive) {
                record.error = parsed_archive.failure();
            } else {
                auto mounted = std::make_shared<MountedArchiveData>();
                mounted->archive = std::move(*parsed_archive);
                record.archive = std::move(mounted);
            }
        }
    }

    {
        const auto lock = std::scoped_lock(_mutex);
        _archives[canonical] = std::move(record);
    }
}

std::shared_ptr<const MountedArchiveData> GameAssetService::receive_archive(std::string_view archive_path) {
    const auto canonical = canonical_path(archive_path);
    request_mount_archive(canonical);

    const auto lock = std::scoped_lock(_mutex);
    const auto found = _archives.find(canonical);
    if (found == _archives.end() || found->second.archive == nullptr) {
        return nullptr;
    }
    return found->second.archive;
}

bool GameAssetService::is_mounted_archive(std::string_view archive_path) const {
    const auto canonical = canonical_path(archive_path);

    const auto lock = std::scoped_lock(_mutex);
    const auto found = _archives.find(canonical);
    return found != _archives.end() && found->second.archive != nullptr;
}

const GameAssetPathResolver &GameAssetService::path_resolver() const {
    return _path_resolver;
}

std::string GameAssetService::canonical_path(std::string_view file_path) const {
    return _path_resolver.make_file_name_considering_language(file_path);
}

AssetError GameAssetService::make_invalid_error(std::string message) {
    return AssetError {
        .code = AssetErrorCode::InvalidFormat,
        .message = std::move(message),
    };
}

std::unique_ptr<IGameAssetService> create_default_game_asset_service(
    di::DependencyReference<IAssetManager> asset_manager,
    GameAssetPathResolverConfiguration resolver_configuration,
    di::DependencyReference<logging::ILogger> logger) {
    return std::make_unique<GameAssetService>(std::move(asset_manager), std::move(resolver_configuration), std::move(logger));
}

}  // namespace smgpc::assets
