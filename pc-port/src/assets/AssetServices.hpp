#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace smgpc::assets {

enum class AssetErrorCode {
    NotFound, IoFailure, InvalidFormat
};

struct AssetError {
    AssetErrorCode code {};
    std::string message {};
};

template <typename T>
class AssetResult {
public:
    AssetResult(T value)
        : _state(std::move(value)) {
    }

    AssetResult(AssetError error)
        : _state(std::move(error)) {
    }

    [[nodiscard]] bool has_value() const noexcept {
        return std::holds_alternative<T>(_state);
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return has_value();
    }

    [[nodiscard]] T &value() {
        return std::get<T>(_state);
    }

    [[nodiscard]] const T &value() const {
        return std::get<T>(_state);
    }

    [[nodiscard]] T &operator*() {
        return value();
    }

    [[nodiscard]] const T &operator*() const {
        return value();
    }

    [[nodiscard]] T *operator->() {
        return std::addressof(value());
    }

    [[nodiscard]] const T *operator->() const {
        return std::addressof(value());
    }

    [[nodiscard]] AssetError &failure() {
        return std::get<AssetError>(_state);
    }

    [[nodiscard]] const AssetError &failure() const {
        return std::get<AssetError>(_state);
    }

private:
    std::variant<T, AssetError> _state {};
};

template <>
class AssetResult<void> {
public:
    AssetResult() = default;

    AssetResult(AssetError error)
        : _success(false), _error(std::move(error)) {
    }

    [[nodiscard]] bool has_value() const noexcept {
        return _success;
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return has_value();
    }

    [[nodiscard]] const AssetError &failure() const {
        return _error;
    }

private:
    bool _success {true};
    AssetError _error {};
};

struct AssetId {
    std::string logical_path {};

    [[nodiscard]] bool empty() const noexcept {
        return logical_path.empty();
    }

    friend bool operator==(const AssetId &, const AssetId &) = default;
};

struct AssetLocatorConfiguration {
    std::filesystem::path game_root {};
    std::string version {};
    std::string language {};
};

struct AssetCacheConfiguration {
    std::filesystem::path cache_root {};
    std::string version {};
    std::string language {};
};

struct LoadedAsset {
    AssetId id {};
    std::filesystem::path source_path {};
    std::vector<std::byte> bytes {};
};

struct ConvertedAsset {
    AssetId id {};
    std::string conversion_profile {};
    std::uint64_t source_hash {};
    std::vector<std::byte> bytes {};
};

struct CachedAssetRecord {
    AssetId id {};
    std::filesystem::path cached_path {};
    std::string conversion_profile {};
    std::uint64_t source_hash {};
    std::uint64_t converted_size {};
};

class IAssetLocator {
public:
    virtual ~IAssetLocator() = default;
    [[nodiscard]] virtual AssetResult<std::filesystem::path> locate(const AssetId &id) const = 0;
};

class IAssetLoader {
public:
    virtual ~IAssetLoader() = default;
    [[nodiscard]] virtual AssetResult<LoadedAsset> load(const AssetId &id) const = 0;
};

class IAssetConverter {
public:
    virtual ~IAssetConverter() = default;
    [[nodiscard]] virtual AssetResult<ConvertedAsset> convert(const LoadedAsset &source) const = 0;
};

class IAssetManager {
public:
    virtual ~IAssetManager() = default;
    [[nodiscard]] virtual AssetResult<CachedAssetRecord> prepare_asset(const AssetId &id) = 0;
    [[nodiscard]] virtual AssetResult<void> prepare_assets(std::span<const AssetId> ids) = 0;
    [[nodiscard]] virtual AssetResult<std::vector<std::byte>> load_cached_asset(const AssetId &id) = 0;
    [[nodiscard]] virtual std::optional<CachedAssetRecord> find_cached_asset(const AssetId &id) const = 0;
};

class FilesystemAssetLocator final : public IAssetLocator {
public:
    explicit FilesystemAssetLocator(AssetLocatorConfiguration configuration);
    [[nodiscard]] AssetResult<std::filesystem::path> locate(const AssetId &id) const override;

private:
    AssetLocatorConfiguration _configuration {};
};

class FilesystemAssetLoader final : public IAssetLoader {
public:
    explicit FilesystemAssetLoader(std::shared_ptr<IAssetLocator> locator);
    [[nodiscard]] AssetResult<LoadedAsset> load(const AssetId &id) const override;

private:
    std::shared_ptr<IAssetLocator> _locator {};
};

class PackedAssetConverter final : public IAssetConverter {
public:
    PackedAssetConverter() = default;
    [[nodiscard]] AssetResult<ConvertedAsset> convert(const LoadedAsset &source) const override;
};

class CachingAssetManager final : public IAssetManager {
public:
    CachingAssetManager(std::shared_ptr<IAssetLoader> loader, std::shared_ptr<IAssetConverter> converter, AssetCacheConfiguration configuration);

    [[nodiscard]] AssetResult<CachedAssetRecord> prepare_asset(const AssetId &id) override;
    [[nodiscard]] AssetResult<void> prepare_assets(std::span<const AssetId> ids) override;
    [[nodiscard]] AssetResult<std::vector<std::byte>> load_cached_asset(const AssetId &id) override;
    [[nodiscard]] std::optional<CachedAssetRecord> find_cached_asset(const AssetId &id) const override;

private:
    [[nodiscard]] std::filesystem::path build_cache_path(const AssetId &id, std::uint64_t source_hash) const;

    std::shared_ptr<IAssetLoader> _loader {};
    std::shared_ptr<IAssetConverter> _converter {};
    AssetCacheConfiguration _configuration {};
    mutable std::mutex _mutex {};
    std::unordered_map<std::string, CachedAssetRecord> _records {};
};

}  // namespace smgpc::assets
