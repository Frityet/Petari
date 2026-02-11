#include "AssetServices.hpp"

#include <array>
#include <fstream>
#include <iomanip>
#include <ios>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace smgpc::assets {
namespace {

struct PackedAssetHeader {
    std::array<char, 8> magic {};
    std::uint32_t version {};
    std::uint32_t logical_path_size {};
    std::uint64_t source_size {};
    std::uint64_t source_hash {};
};

static_assert(std::is_trivially_copyable_v<PackedAssetHeader>);

[[nodiscard]] AssetError make_error(AssetErrorCode code, std::string message) {
    return AssetError {.code = code, .message = std::move(message)};
}

[[nodiscard]] std::filesystem::path normalize_logical_path(const std::string &logical_path) {
    auto normalized = std::filesystem::path(logical_path).lexically_normal();
    if (normalized.is_absolute()) {
        normalized = normalized.relative_path();
    }

    return normalized;
}

[[nodiscard]] AssetResult<std::vector<std::byte>> read_all_bytes(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    if (not stream.is_open()) {
        return make_error(AssetErrorCode::IoFailure, "Failed to open file " + path.string());
    }

    stream.seekg(0, std::ios::end);
    const auto file_size_position = stream.tellg();
    if (file_size_position < 0) {
        return make_error(AssetErrorCode::IoFailure, "Failed to query file size " + path.string());
    }
    const auto file_size = static_cast<std::size_t>(file_size_position);

    std::vector<std::byte> bytes(file_size);
    stream.seekg(0, std::ios::beg);
    if (file_size > 0) {
        stream.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(file_size));
        if (not stream) {
            return make_error(AssetErrorCode::IoFailure, "Failed to read file " + path.string());
        }
    }

    return bytes;
}

[[nodiscard]] AssetResult<void> write_all_bytes(const std::filesystem::path &path, std::span<const std::byte> bytes) {
    const auto parent_path = path.parent_path();
    if (not parent_path.empty()) {
        std::error_code create_error {};
        std::filesystem::create_directories(parent_path, create_error);
        if (create_error) {
            return AssetResult<void>(make_error(AssetErrorCode::IoFailure, "Failed to create cache directory " + parent_path.string()));
        }
    }

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (not stream.is_open()) {
        return AssetResult<void>(make_error(AssetErrorCode::IoFailure, "Failed to open output file " + path.string()));
    }

    if (not bytes.empty()) {
        stream.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (not stream) {
            return AssetResult<void>(make_error(AssetErrorCode::IoFailure, "Failed to write output file " + path.string()));
        }
    }

    return {};
}

[[nodiscard]] std::uint64_t hash_bytes(std::span<const std::byte> bytes) {
    constexpr std::uint64_t OFFSET_BASIS = 14695981039346656037ULL;
    constexpr std::uint64_t PRIME = 1099511628211ULL;

    std::uint64_t hash = OFFSET_BASIS;
    for (const std::byte byte_value : bytes) {
        hash ^= static_cast<std::uint64_t>(std::to_integer<unsigned char>(byte_value));
        hash *= PRIME;
    }

    return hash;
}

template <typename T>
void append_value(std::vector<std::byte> &output, const T &value) {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto *begin = reinterpret_cast<const std::byte *>(std::addressof(value));
    output.insert(output.end(), begin, begin + sizeof(T));
}

void append_bytes(std::vector<std::byte> &output, std::span<const std::byte> bytes) {
    output.insert(output.end(), bytes.begin(), bytes.end());
}

[[nodiscard]] std::string to_hex(std::uint64_t value) {
    std::ostringstream stream;
    stream << std::hex << std::setw(16) << std::setfill('0') << value;
    return stream.str();
}

[[nodiscard]] std::filesystem::path make_relative_cache_asset_path(const AssetId &id, std::uint64_t hash_value) {
    const auto logical_path = normalize_logical_path(id.logical_path);
    auto filename = logical_path.filename().string();
    if (filename.empty()) {
        filename = "asset";
    }

    const auto stem = std::filesystem::path(filename).stem().string();
    const auto cache_filename = (stem.empty() ? filename : stem) + "." + to_hex(hash_value) + ".smgasset";
    return logical_path.parent_path()/cache_filename;
}

}  // namespace

FilesystemAssetLocator::FilesystemAssetLocator(AssetLocatorConfiguration configuration)
    : _configuration(std::move(configuration)) {
}

AssetResult<std::filesystem::path> FilesystemAssetLocator::locate(const AssetId &id) const {
    if (id.empty()) {
        return make_error(AssetErrorCode::NotFound, "Asset logical path cannot be empty.");
    }

    const auto normalized_logical = normalize_logical_path(id.logical_path);
    const auto disc_files_root = _configuration.game_root/"orig"/_configuration.version/"files";

    std::array<std::filesystem::path, 2> candidate_paths {
        disc_files_root/normalized_logical, disc_files_root/_configuration.language/normalized_logical
    };

    for (const auto &candidate_path : candidate_paths) {
        if (std::filesystem::exists(candidate_path)) {
            return candidate_path;
        }
    }

    return make_error(AssetErrorCode::NotFound, "Asset not found in disc root for " + id.logical_path);
}

FilesystemAssetLoader::FilesystemAssetLoader(std::shared_ptr<IAssetLocator> locator)
    : _locator(std::move(locator)) {
    if (not _locator) {
        throw std::invalid_argument("FilesystemAssetLoader requires a non-null locator.");
    }
}

AssetResult<LoadedAsset> FilesystemAssetLoader::load(const AssetId &id) const {
    const auto located_path = _locator->locate(id);
    if (not located_path) {
        return located_path.failure();
    }

    const auto bytes = read_all_bytes(*located_path);
    if (not bytes) {
        return bytes.failure();
    }

    return LoadedAsset {
        .id = id, .source_path = *located_path, .bytes = *bytes
    };
}

AssetResult<ConvertedAsset> PackedAssetConverter::convert(const LoadedAsset &source) const {
    if (source.id.empty()) {
        return make_error(AssetErrorCode::InvalidFormat, "Cannot convert an unnamed asset.");
    }

    if (source.id.logical_path.size() > std::numeric_limits<std::uint32_t>::max()) {
        return make_error(AssetErrorCode::InvalidFormat, "Asset logical path is too large.");
    }

    const auto source_hash = hash_bytes(source.bytes);

    const PackedAssetHeader header {
        .magic = {'S', 'M', 'G', 'P', 'C', 'A', 'S', '1'}, .version = 1U, .logical_path_size = static_cast<std::uint32_t>(source.id.logical_path.size()), .source_size = static_cast<std::uint64_t>(source.bytes.size()), .source_hash = source_hash
    };

    const std::span<const char> logical_characters(source.id.logical_path.data(), source.id.logical_path.size());
    const auto logical_bytes = std::as_bytes(logical_characters);

    std::vector<std::byte> packed_bytes {};
    packed_bytes.reserve(sizeof(PackedAssetHeader) + logical_bytes.size() + source.bytes.size());

    append_value(packed_bytes, header);
    append_bytes(packed_bytes, logical_bytes);
    append_bytes(packed_bytes, source.bytes);

    return ConvertedAsset {
        .id = source.id, .conversion_profile = "pack-v1", .source_hash = source_hash, .bytes = std::move(packed_bytes)
    };
}

CachingAssetManager::CachingAssetManager(std::shared_ptr<IAssetLoader> loader, std::shared_ptr<IAssetConverter> converter, AssetCacheConfiguration configuration)
    : _loader(std::move(loader)), _converter(std::move(converter)), _configuration(std::move(configuration)) {
    if (not _loader or not _converter) {
        throw std::invalid_argument("CachingAssetManager requires loader and converter services.");
    }
}

AssetResult<CachedAssetRecord> CachingAssetManager::prepare_asset(const AssetId &id) {
    if (id.empty()) {
        return make_error(AssetErrorCode::NotFound, "Asset logical path cannot be empty.");
    }

    {
        auto lock = std::scoped_lock(_mutex);
        const auto existing = _records.find(id.logical_path);
        if (existing != _records.end() and std::filesystem::exists(existing->second.cached_path)) {
            return existing->second;
        }
    }

    const auto loaded_asset = _loader->load(id);
    if (not loaded_asset) {
        return loaded_asset.failure();
    }

    const auto converted_asset = _converter->convert(*loaded_asset);
    if (not converted_asset) {
        return converted_asset.failure();
    }

    const auto cache_path = build_cache_path(id, converted_asset->source_hash);
    const auto write_result = write_all_bytes(cache_path, converted_asset->bytes);
    if (not write_result) {
        return write_result.failure();
    }

    CachedAssetRecord record {
        .id = id, .cached_path = cache_path, .conversion_profile = converted_asset->conversion_profile, .source_hash = converted_asset->source_hash, .converted_size = static_cast<std::uint64_t>(converted_asset->bytes.size())
    };

    {
        auto lock = std::scoped_lock(_mutex);
        _records[id.logical_path] = record;
    }

    return record;
}

AssetResult<void> CachingAssetManager::prepare_assets(std::span<const AssetId> ids) {
    for (const auto &id : ids) {
        const auto prepared = prepare_asset(id);
        if (not prepared) {
            return AssetResult<void>(prepared.failure());
        }
    }

    return {};
}

AssetResult<std::vector<std::byte>> CachingAssetManager::load_cached_asset(const AssetId &id) {
    const auto prepared = prepare_asset(id);
    if (not prepared) {
        return prepared.failure();
    }

    return read_all_bytes(prepared->cached_path);
}

std::optional<CachedAssetRecord> CachingAssetManager::find_cached_asset(const AssetId &id) const {
    auto lock = std::scoped_lock(_mutex);
    const auto found = _records.find(id.logical_path);
    if (found == _records.end()) {
        return std::nullopt;
    }
    if (not std::filesystem::exists(found->second.cached_path)) {
        return std::nullopt;
    }

    return found->second;
}

std::filesystem::path CachingAssetManager::build_cache_path(const AssetId &id, std::uint64_t source_hash) const {
    return _configuration.cache_root/_configuration.version/_configuration.language/make_relative_cache_asset_path(id, source_hash);
}

}  // namespace smgpc::assets
