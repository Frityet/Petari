#include "assets/AssetServices.hpp"
#include "assets/GameAssetPathResolver.hpp"
#include "assets/GameAssetService.hpp"
#include "assets/PackedAsset.hpp"
#include "common/Logger.hpp"
#include "tests/TestFilesystem.hpp"
#include "tests/TestHarness.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

class FakeAssetManager final : public smgpc::assets::IAssetManager {
public:
    [[nodiscard]] smgpc::assets::AssetResult<smgpc::assets::CachedAssetRecord> prepare_asset(const smgpc::assets::AssetId &id) override {
        return smgpc::assets::CachedAssetRecord {
            .id = id,
        };
    }

    [[nodiscard]] smgpc::assets::AssetResult<void> prepare_assets(std::span<const smgpc::assets::AssetId>) override {
        return {};
    }

    [[nodiscard]] smgpc::assets::AssetResult<std::vector<std::byte>> load_cached_asset(const smgpc::assets::AssetId &id) override {
        ++load_calls;
        requested_paths.push_back(id.logical_path);

        const auto found = packed_by_logical_path.find(id.logical_path);
        if (found == packed_by_logical_path.end()) {
            return smgpc::assets::AssetError {
                .code = smgpc::assets::AssetErrorCode::NotFound,
                .message = "missing test asset",
            };
        }

        return found->second;
    }

    [[nodiscard]] std::optional<smgpc::assets::CachedAssetRecord> find_cached_asset(const smgpc::assets::AssetId &) const override {
        return std::nullopt;
    }

    int load_calls {};
    std::vector<std::string> requested_paths {};
    std::unordered_map<std::string, std::vector<std::byte>> packed_by_logical_path {};
};

class SilentLogger final : public smgpc::logging::ILogger {
public:
    void write(std::FILE *, std::string_view, int, smgpc::logging::Level, smgpc::logging::Category, std::string_view) override {
    }
};

[[nodiscard]] std::vector<std::byte> make_packed_asset(std::string logical_path, std::string payload) {
    smgpc::assets::PackedAssetConverter converter {};
    smgpc::assets::LoadedAsset loaded_asset {
        .id = smgpc::assets::AssetId {.logical_path = std::move(logical_path)},
        .source_path = "/tmp/source",
        .bytes = smgpc::test::bytes_from_string(payload),
    };

    const auto converted = converter.convert(loaded_asset);
    if (not converted) {
        throw std::runtime_error("Failed to create packed test asset");
    }

    return converted->bytes;
}

}  // namespace

$test("GameAssetPathResolver resolves language/object/layout helper paths") {
    smgpc::test::TempDirectory temp {};
    const auto disc_root = temp.path() / "orig" / "RMGK01" / "files";

    smgpc::test::write_bytes(disc_root / "KrKorean" / "LayoutData" / "Localized.arc", smgpc::test::bytes_from_string("a"));
    smgpc::test::write_bytes(disc_root / "ObjectData" / "Obj.arc", smgpc::test::bytes_from_string("a"));
    smgpc::test::write_bytes(disc_root / "MapPartsData" / "Map.arc", smgpc::test::bytes_from_string("a"));
    smgpc::test::write_bytes(disc_root / "Region" / "LayoutData" / "Regional.arc", smgpc::test::bytes_from_string("a"));
    smgpc::test::write_bytes(disc_root / "LayoutData" / "Hud.arc", smgpc::test::bytes_from_string("a"));
    smgpc::test::write_bytes(disc_root / "LayoutData" / "Hud4x3.arc", smgpc::test::bytes_from_string("a"));

    smgpc::assets::GameAssetPathResolver resolver(smgpc::assets::GameAssetPathResolverConfiguration {
        .game_root = temp.path(),
        .version = "RMGK01",
        .language = "KrKorean",
        .is_widescreen = false,
    });

    $pc_port_require_eq(resolver.make_file_name_considering_language("/LayoutData/Localized.arc"), std::string("/KrKorean/LayoutData/Localized.arc"));
    $pc_port_require(resolver.make_object_archive_file_name("Obj.arc").value_or("") == "/ObjectData/Obj.arc");
    $pc_port_require(resolver.make_object_archive_file_name("Map.arc").value_or("") == "/MapPartsData/Map.arc");
    $pc_port_require(resolver.make_layout_archive_file_name("Regional.arc").value_or("") == "/Region/LayoutData/Regional.arc");
    $pc_port_require(resolver.make_layout_archive_file_name_from_prefix("Hud", false).value_or("") == "/LayoutData/Hud4x3.arc");
    $pc_port_require(not resolver.make_layout_archive_file_name_from_prefix("Missing", false).has_value());
    $pc_port_require(resolver.make_layout_archive_file_name_from_prefix("Missing", true).value_or("") == "/LayoutData/Missing.arc");
}

$test("GameAssetService request and receive file semantics are idempotent") {
    auto fake_manager = std::make_shared<FakeAssetManager>();
    fake_manager->packed_by_logical_path.emplace(
        "LayoutData/Test.bin",
        make_packed_asset("LayoutData/Test.bin", "payload"));

    auto logger = std::make_shared<SilentLogger>();
    smgpc::assets::GameAssetService service(
        smgpc::di::DependencyReference<smgpc::assets::IAssetManager>{*fake_manager},
        smgpc::assets::GameAssetPathResolverConfiguration {
            .game_root = "/tmp",
            .version = "RMGK01",
            .language = "KrKorean",
            .is_widescreen = true,
        },
        smgpc::di::DependencyReference<smgpc::logging::ILogger>{*logger});

    service.request_load_file("/LayoutData/Test.bin");
    service.request_load_file("/LayoutData/Test.bin");
    const auto bytes = service.receive_file("/LayoutData/Test.bin");

    $pc_port_require(bytes != nullptr);
    $pc_port_require_eq(smgpc::test::string_from_bytes(*bytes), std::string("payload"));
    $pc_port_require(service.is_loaded_file("/LayoutData/Test.bin"));
    $pc_port_require_eq(fake_manager->load_calls, 1);
}

$test("GameAssetService propagates archive parse failure") {
    auto fake_manager = std::make_shared<FakeAssetManager>();
    fake_manager->packed_by_logical_path.emplace(
        "LayoutData/Bad.arc",
        make_packed_asset("LayoutData/Bad.arc", "not-a-rarc"));

    auto logger = std::make_shared<SilentLogger>();
    smgpc::assets::GameAssetService service(
        smgpc::di::DependencyReference<smgpc::assets::IAssetManager>{*fake_manager},
        smgpc::assets::GameAssetPathResolverConfiguration {
            .game_root = "/tmp",
            .version = "RMGK01",
            .language = "KrKorean",
            .is_widescreen = true,
        },
        smgpc::di::DependencyReference<smgpc::logging::ILogger>{*logger});

    service.request_mount_archive("/LayoutData/Bad.arc");
    const auto archive = service.receive_archive("/LayoutData/Bad.arc");

    $pc_port_require(archive == nullptr);
    $pc_port_require(not service.is_mounted_archive("/LayoutData/Bad.arc"));
}
