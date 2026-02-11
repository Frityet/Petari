#include "assets/AssetServices.hpp"
#include "tests/TestFilesystem.hpp"
#include "tests/TestHarness.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

struct PackedAssetHeader {
    std::array<char, 8> magic {};
    std::uint32_t version {};
    std::uint32_t logical_path_size {};
    std::uint64_t source_size {};
    std::uint64_t source_hash {};
};

static_assert(std::is_trivially_copyable_v<PackedAssetHeader>);

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

[[nodiscard]] std::vector<std::byte> bytes_from_values(std::initializer_list<unsigned char> values) {
    std::vector<std::byte> bytes {};
    bytes.reserve(values.size());
    for (const auto value : values) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    return bytes;
}

class FunctionalLoader final : public smgpc::assets::IAssetLoader {
public:
    using Fn = std::function<smgpc::assets::AssetResult<smgpc::assets::LoadedAsset>(const smgpc::assets::AssetId &)>;

    explicit FunctionalLoader(Fn fn)
        : _fn(std::move(fn)) {
    }

    [[nodiscard]] smgpc::assets::AssetResult<smgpc::assets::LoadedAsset> load(const smgpc::assets::AssetId &id) const override {
        ++load_calls;
        requested_ids.push_back(id.logical_path);
        return _fn(id);
    }

    mutable int load_calls {};
    mutable std::vector<std::string> requested_ids {};

private:
    Fn _fn;
};

class FunctionalConverter final : public smgpc::assets::IAssetConverter {
public:
    using Fn = std::function<smgpc::assets::AssetResult<smgpc::assets::ConvertedAsset>(const smgpc::assets::LoadedAsset &)>;

    explicit FunctionalConverter(Fn fn)
        : _fn(std::move(fn)) {
    }

    [[nodiscard]] smgpc::assets::AssetResult<smgpc::assets::ConvertedAsset> convert(const smgpc::assets::LoadedAsset &source) const override {
        ++convert_calls;
        converted_ids.push_back(source.id.logical_path);
        return _fn(source);
    }

    mutable int convert_calls {};
    mutable std::vector<std::string> converted_ids {};

private:
    Fn _fn;
};

[[nodiscard]] smgpc::assets::AssetLocatorConfiguration make_locator_configuration(const std::filesystem::path &game_root) {
    return smgpc::assets::AssetLocatorConfiguration {
        .game_root = game_root, .version = "RMGK01", .language = "KrKorean"
    };
}

[[nodiscard]] smgpc::assets::AssetCacheConfiguration make_cache_configuration(const std::filesystem::path &cache_root) {
    return smgpc::assets::AssetCacheConfiguration {
        .cache_root = cache_root, .version = "RMGK01", .language = "KrKorean"
    };
}

}  // namespace

$test("AssetResult<T> holds success value") {
    smgpc::assets::AssetResult<int> result {123};

    $pc_port_require(result);
    $pc_port_require_eq(result.value(), 123);
    $pc_port_require_eq(*result, 123);
}

$test("AssetResult<T> holds error state") {
    smgpc::assets::AssetResult<int> result {
        smgpc::assets::AssetError {
            .code = smgpc::assets::AssetErrorCode::IoFailure, .message = "bad"
        }
    };

    $pc_port_require(not result);
    $pc_port_require(result.failure().code == smgpc::assets::AssetErrorCode::IoFailure);
    $pc_port_require_eq(result.failure().message, std::string("bad"));
}

$test("AssetResult<void> defaults to success and error state is failure") {
    smgpc::assets::AssetResult<void> success {};
    smgpc::assets::AssetResult<void> failure {
        smgpc::assets::AssetError {
            .code = smgpc::assets::AssetErrorCode::InvalidFormat, .message = "oops"
        }
    };

    $pc_port_require(success);
    $pc_port_require(not failure);
    $pc_port_require(failure.failure().code == smgpc::assets::AssetErrorCode::InvalidFormat);
    $pc_port_require_eq(failure.failure().message, std::string("oops"));
}

$test("FilesystemAssetLocator::locate rejects empty logical path") {
    smgpc::test::TempDirectory temp {};
    smgpc::assets::FilesystemAssetLocator locator(make_locator_configuration(temp.path()));

    const auto result = locator.locate(smgpc::assets::AssetId {});

    $pc_port_require(not result);
    $pc_port_require(result.failure().code == smgpc::assets::AssetErrorCode::NotFound);
}

$test("FilesystemAssetLocator::locate finds asset in disc root then language root") {
    smgpc::test::TempDirectory temp {};
    const auto base = temp.path()/"orig"/"RMGK01"/"files";
    const auto direct_path = base/"LayoutData"/"Direct.arc";
    const auto language_path = base/"KrKorean"/"LayoutData"/"Localized.arc";

    smgpc::test::write_bytes(direct_path, smgpc::test::bytes_from_string("direct"));
    smgpc::test::write_bytes(language_path, smgpc::test::bytes_from_string("localized"));

    smgpc::assets::FilesystemAssetLocator locator(make_locator_configuration(temp.path()));

    const auto direct_result = locator.locate(smgpc::assets::AssetId {.logical_path = "LayoutData/Direct.arc"});
    const auto localized_result = locator.locate(smgpc::assets::AssetId {.logical_path = "LayoutData/Localized.arc"});

    $pc_port_require(direct_result);
    $pc_port_require(localized_result);
    $pc_port_require_eq(*direct_result, direct_path);
    $pc_port_require_eq(*localized_result, language_path);
}

$test("FilesystemAssetLocator::locate returns not found for missing asset") {
    smgpc::test::TempDirectory temp {};
    smgpc::assets::FilesystemAssetLocator locator(make_locator_configuration(temp.path()));

    const auto result = locator.locate(smgpc::assets::AssetId {.logical_path = "LayoutData/Missing.arc"});

    $pc_port_require(not result);
    $pc_port_require(result.failure().code == smgpc::assets::AssetErrorCode::NotFound);
    $pc_port_require(result.failure().message.find("LayoutData/Missing.arc") != std::string::npos);
}

$test("FilesystemAssetLoader requires non-null locator") {
    bool threw = false;

    try {
        smgpc::assets::FilesystemAssetLoader loader(nullptr);
        (void)loader;
    } catch (const std::invalid_argument &) {
        threw = true;
    }

    $pc_port_require(threw);
}

$test("FilesystemAssetLoader::load loads bytes from located file") {
    smgpc::test::TempDirectory temp {};
    const auto source_path = temp.path()/"orig"/"RMGK01"/"files"/"LayoutData"/"Font.arc";
    const auto expected_bytes = bytes_from_values({0x01, 0x02, 0x03, 0x04});
    smgpc::test::write_bytes(source_path, expected_bytes);

    auto locator = std::make_shared<smgpc::assets::FilesystemAssetLocator>(make_locator_configuration(temp.path()));
    smgpc::assets::FilesystemAssetLoader loader(locator);

    const auto result = loader.load(smgpc::assets::AssetId {.logical_path = "LayoutData/Font.arc"});

    $pc_port_require(result);
    $pc_port_require_eq(result->id.logical_path, std::string("LayoutData/Font.arc"));
    $pc_port_require_eq(result->source_path, source_path);
    $pc_port_require_eq(result->bytes.size(), expected_bytes.size());
    $pc_port_require(result->bytes == expected_bytes);
}

$test("FilesystemAssetLoader::load propagates locator failure") {
    smgpc::test::TempDirectory temp {};
    auto locator = std::make_shared<smgpc::assets::FilesystemAssetLocator>(make_locator_configuration(temp.path()));
    smgpc::assets::FilesystemAssetLoader loader(locator);

    const auto result = loader.load(smgpc::assets::AssetId {.logical_path = "LayoutData/Nope.arc"});

    $pc_port_require(not result);
    $pc_port_require(result.failure().code == smgpc::assets::AssetErrorCode::NotFound);
}

$test("PackedAssetConverter::convert rejects empty asset id") {
    smgpc::assets::PackedAssetConverter converter {};
    smgpc::assets::LoadedAsset source {
        .id = {}, .source_path = "ignored", .bytes = bytes_from_values({0xAA})
    };

    const auto result = converter.convert(source);

    $pc_port_require(not result);
    $pc_port_require(result.failure().code == smgpc::assets::AssetErrorCode::InvalidFormat);
}

$test("PackedAssetConverter::convert writes expected header and payload layout") {
    smgpc::assets::PackedAssetConverter converter {};
    const auto source_bytes = smgpc::test::bytes_from_string("ABC");
    smgpc::assets::LoadedAsset source {
        .id = smgpc::assets::AssetId {.logical_path = "LayoutData/Test.arc"}, .source_path = "/tmp/source", .bytes = source_bytes
    };

    const auto result = converter.convert(source);
    $pc_port_require(result);

    const auto &packed = result->bytes;
    $pc_port_require(packed.size() >= sizeof(PackedAssetHeader));

    PackedAssetHeader header {};
    std::memcpy(&header, packed.data(), sizeof(PackedAssetHeader));

    const std::array<char, 8> expected_magic {'S', 'M', 'G', 'P', 'C', 'A', 'S', '1'};
    $pc_port_require(header.magic == expected_magic);
    $pc_port_require_eq(header.version, static_cast<std::uint32_t>(1));
    $pc_port_require_eq(header.logical_path_size, static_cast<std::uint32_t>(source.id.logical_path.size()));
    $pc_port_require_eq(header.source_size, static_cast<std::uint64_t>(source_bytes.size()));
    $pc_port_require_eq(header.source_hash, hash_bytes(source_bytes));
    $pc_port_require_eq(result->source_hash, hash_bytes(source_bytes));
    $pc_port_require_eq(result->conversion_profile, std::string("pack-v1"));

    const auto *logical_begin = reinterpret_cast<const char *>(packed.data() + sizeof(PackedAssetHeader));
    const std::string logical_path(logical_begin, logical_begin + header.logical_path_size);
    $pc_port_require_eq(logical_path, std::string("LayoutData/Test.arc"));

    const auto payload_offset = sizeof(PackedAssetHeader) + header.logical_path_size;
    const auto payload_span = std::span<const std::byte>(packed.data() + payload_offset, packed.size() - payload_offset);
    $pc_port_require_eq(payload_span.size(), source_bytes.size());
    for (std::size_t i = 0; i < payload_span.size(); ++i) {
        $pc_port_require(payload_span[i] == source_bytes[i]);
    }
}

$test("CachingAssetManager requires non-null loader and converter") {
    auto valid_loader = std::make_shared<FunctionalLoader>([](const smgpc::assets::AssetId &) {
            return smgpc::assets::LoadedAsset {};
        });
    auto valid_converter = std::make_shared<FunctionalConverter>([](const smgpc::assets::LoadedAsset &) {
            return smgpc::assets::ConvertedAsset {};
        });
    bool threw_for_loader = false;
    bool threw_for_converter = false;

    try {
        smgpc::assets::CachingAssetManager manager(nullptr, valid_converter, make_cache_configuration("/tmp/cache"));
        (void)manager;
    } catch (const std::invalid_argument &) {
        threw_for_loader = true;
    }

    try {
        smgpc::assets::CachingAssetManager manager(valid_loader, nullptr, make_cache_configuration("/tmp/cache"));
        (void)manager;
    } catch (const std::invalid_argument &) {
        threw_for_converter = true;
    }

    $pc_port_require(threw_for_loader);
    $pc_port_require(threw_for_converter);
}

$test("CachingAssetManager::prepare_asset writes cache and reuses existing record") {
    smgpc::test::TempDirectory temp {};

    auto loader = std::make_shared<FunctionalLoader>([](const smgpc::assets::AssetId &id) {
            return smgpc::assets::LoadedAsset {
                .id = id, .source_path = "/fake/source", .bytes = smgpc::test::bytes_from_string("source")
            };
        });

    auto converter = std::make_shared<FunctionalConverter>([](const smgpc::assets::LoadedAsset &source) {
            return smgpc::assets::ConvertedAsset {
                .id = source.id, .conversion_profile = "unit-converter", .source_hash = 0x1234ULL, .bytes = smgpc::test::bytes_from_string("packed-data")
            };
        });

    smgpc::assets::CachingAssetManager manager(loader, converter, make_cache_configuration(temp.path()/"cache"));
    const smgpc::assets::AssetId id {.logical_path = "LayoutData/Test.arc"};

    const auto first = manager.prepare_asset(id);
    const auto second = manager.prepare_asset(id);

    $pc_port_require(first);
    $pc_port_require(second);
    $pc_port_require_eq(loader->load_calls, 1);
    $pc_port_require_eq(converter->convert_calls, 1);
    $pc_port_require(std::filesystem::exists(first->cached_path));
    $pc_port_require_eq(first->conversion_profile, std::string("unit-converter"));
    $pc_port_require_eq(first->source_hash, static_cast<std::uint64_t>(0x1234ULL));
    $pc_port_require_eq(second->cached_path, first->cached_path);
    $pc_port_require_eq(second->converted_size, first->converted_size);

    const auto expected_path = temp.path()/"cache"/"RMGK01"/"KrKorean"/"LayoutData"/"Test.0000000000001234.smgasset";
    $pc_port_require_eq(first->cached_path, expected_path);
}

$test("CachingAssetManager::prepare_asset recaches when cached file is deleted") {
    smgpc::test::TempDirectory temp {};

    auto loader = std::make_shared<FunctionalLoader>([](const smgpc::assets::AssetId &id) {
            return smgpc::assets::LoadedAsset {
                .id = id, .source_path = "/tmp/source", .bytes = smgpc::test::bytes_from_string("source")
            };
        });
    auto converter = std::make_shared<FunctionalConverter>([](const smgpc::assets::LoadedAsset &source) {
            return smgpc::assets::ConvertedAsset {
                .id = source.id, .conversion_profile = "pack-v1", .source_hash = 0x777ULL, .bytes = smgpc::test::bytes_from_string("content")
            };
        });

    smgpc::assets::CachingAssetManager manager(loader, converter, make_cache_configuration(temp.path()/"cache"));
    const smgpc::assets::AssetId id {.logical_path = "LayoutData/Rebuild.arc"};

    const auto first = manager.prepare_asset(id);
    $pc_port_require(first);

    std::filesystem::remove(first->cached_path);
    const auto second = manager.prepare_asset(id);
    $pc_port_require(second);

    $pc_port_require_eq(loader->load_calls, 2);
    $pc_port_require_eq(converter->convert_calls, 2);
}

$test("CachingAssetManager::prepare_assets stops at first failure") {
    smgpc::test::TempDirectory temp {};

    auto loader = std::make_shared<FunctionalLoader>([](const smgpc::assets::AssetId &id) -> smgpc::assets::AssetResult<smgpc::assets::LoadedAsset> {
            if (id.logical_path == "Bad.arc") {
                return smgpc::assets::AssetError {
                    .code = smgpc::assets::AssetErrorCode::NotFound, .message = "missing"
                };
            }

            return smgpc::assets::LoadedAsset {
                .id = id, .source_path = "/tmp/source", .bytes = smgpc::test::bytes_from_string("src")
            };
        });
    auto converter = std::make_shared<FunctionalConverter>([](const smgpc::assets::LoadedAsset &source) {
            return smgpc::assets::ConvertedAsset {
                .id = source.id, .conversion_profile = "pack-v1", .source_hash = 5, .bytes = smgpc::test::bytes_from_string("packed")
            };
        });

    smgpc::assets::CachingAssetManager manager(loader, converter, make_cache_configuration(temp.path()/"cache"));
    const std::array<smgpc::assets::AssetId, 3> ids {
        smgpc::assets::AssetId {.logical_path = "Good.arc"}, smgpc::assets::AssetId {.logical_path = "Bad.arc"}, smgpc::assets::AssetId {.logical_path = "NeverReached.arc"}
    };

    const auto result = manager.prepare_assets(ids);

    $pc_port_require(not result);
    $pc_port_require(result.failure().code == smgpc::assets::AssetErrorCode::NotFound);
    $pc_port_require_eq(loader->load_calls, 2);
    $pc_port_require_eq(converter->convert_calls, 1);
}

$test("CachingAssetManager::load_cached_asset returns cached bytes") {
    smgpc::test::TempDirectory temp {};

    auto loader = std::make_shared<FunctionalLoader>([](const smgpc::assets::AssetId &id) {
            return smgpc::assets::LoadedAsset {
                .id = id, .source_path = "/tmp/source", .bytes = smgpc::test::bytes_from_string("source")
            };
        });
    auto converter = std::make_shared<FunctionalConverter>([](const smgpc::assets::LoadedAsset &source) {
            return smgpc::assets::ConvertedAsset {
                .id = source.id, .conversion_profile = "pack-v1", .source_hash = 123, .bytes = smgpc::test::bytes_from_string("cached-content")
            };
        });

    smgpc::assets::CachingAssetManager manager(loader, converter, make_cache_configuration(temp.path()/"cache"));
    const auto result = manager.load_cached_asset(smgpc::assets::AssetId {.logical_path = "LayoutData/Font.arc"});

    $pc_port_require(result);
    $pc_port_require_eq(smgpc::test::string_from_bytes(*result), std::string("cached-content"));
}

$test("CachingAssetManager::find_cached_asset reflects cache presence") {
    smgpc::test::TempDirectory temp {};

    auto loader = std::make_shared<FunctionalLoader>([](const smgpc::assets::AssetId &id) {
            return smgpc::assets::LoadedAsset {
                .id = id, .source_path = "/tmp/source", .bytes = smgpc::test::bytes_from_string("data")
            };
        });
    auto converter = std::make_shared<FunctionalConverter>([](const smgpc::assets::LoadedAsset &source) {
            return smgpc::assets::ConvertedAsset {
                .id = source.id, .conversion_profile = "pack-v1", .source_hash = 999, .bytes = smgpc::test::bytes_from_string("result")
            };
        });

    smgpc::assets::CachingAssetManager manager(loader, converter, make_cache_configuration(temp.path()/"cache"));
    const smgpc::assets::AssetId id {.logical_path = "LayoutData/Probe.arc"};

    const auto before = manager.find_cached_asset(id);
    $pc_port_require(not before.has_value());

    const auto prepared = manager.prepare_asset(id);
    $pc_port_require(prepared);

    const auto after = manager.find_cached_asset(id);
    $pc_port_require(after.has_value());
    $pc_port_require_eq(after->cached_path, prepared->cached_path);

    std::filesystem::remove(after->cached_path);
    const auto missing_after_delete = manager.find_cached_asset(id);
    $pc_port_require(not missing_after_delete.has_value());
}

$test("CachingAssetManager::load_cached_asset propagates prepare error") {
    smgpc::test::TempDirectory temp {};

    auto loader = std::make_shared<FunctionalLoader>([](const smgpc::assets::AssetId &) -> smgpc::assets::AssetResult<smgpc::assets::LoadedAsset> {
            return smgpc::assets::AssetError {
                .code = smgpc::assets::AssetErrorCode::IoFailure, .message = "cannot read source"
            };
        });
    auto converter = std::make_shared<FunctionalConverter>([](const smgpc::assets::LoadedAsset &source) {
            return smgpc::assets::ConvertedAsset {
                .id = source.id, .conversion_profile = "pack-v1", .source_hash = 1, .bytes = {}
            };
        });

    smgpc::assets::CachingAssetManager manager(loader, converter, make_cache_configuration(temp.path()/"cache"));
    const auto result = manager.load_cached_asset(smgpc::assets::AssetId {.logical_path = "LayoutData/Fail.arc"});

    $pc_port_require(not result);
    $pc_port_require(result.failure().code == smgpc::assets::AssetErrorCode::IoFailure);
    $pc_port_require(result.failure().message.find("cannot read source") != std::string::npos);
}
