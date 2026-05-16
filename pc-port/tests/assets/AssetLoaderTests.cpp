#include "assets/AssetLoader.hpp"

#include "tests/TestFilesystem.hpp"
#include "tests/TestHarness.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {

class FakeGameAssetService final : public smgpc::assets::IGameAssetService {
public:
    void request_load_file(std::string_view file_path) override {
        requested_files.push_back(std::string(file_path));
    }

    [[nodiscard]] std::shared_ptr< const std::vector< std::byte > > receive_file(std::string_view file_path) override {
        requested_files.push_back(std::string(file_path));
        if (file_path == "/LayoutData/Test.bin") {
            return std::make_shared< const std::vector< std::byte > >(std::vector< std::byte >{std::byte{0x11}, std::byte{0x22}});
        }
        return nullptr;
    }

    [[nodiscard]] bool is_loaded_file(std::string_view file_path) const override {
        return file_path == "/LayoutData/Test.bin";
    }

    void request_mount_archive(std::string_view archive_path) override {
        requested_archives.push_back(std::string(archive_path));
    }

    [[nodiscard]] std::shared_ptr< const smgpc::assets::MountedArchiveData > receive_archive(std::string_view archive_path) override {
        requested_archives.push_back(std::string(archive_path));
        return nullptr;
    }

    [[nodiscard]] bool is_mounted_archive(std::string_view) const override {
        return false;
    }

    [[nodiscard]] const smgpc::assets::GameAssetPathResolver& path_resolver() const override {
        return resolver;
    }

    smgpc::assets::GameAssetPathResolver resolver{smgpc::assets::GameAssetPathResolverConfiguration{
        .game_root = "/tmp",
        .version = "RMGK01",
        .language = "KrKorean",
        .is_widescreen = true,
    }};
    std::vector< std::string > requested_files{};
    std::vector< std::string > requested_archives{};
};

}  // namespace

$test("AssetLoader centralizes file loading and resolver helpers") {
    FakeGameAssetService service{};
    const smgpc::assets::AssetLoader loader{smgpc::di::DependencyReference< smgpc::assets::IGameAssetService >(service)};

    loader.request_file("/LayoutData/Test.bin");
    const auto bytes = loader.file("/LayoutData/Test.bin");

    $pc_port_require(bytes != nullptr);
    $pc_port_require_eq(bytes->size(), static_cast< std::size_t >(2U));
    $pc_port_require(loader.is_loaded_file("/LayoutData/Test.bin"));
    $pc_port_require_eq(service.requested_files.size(), static_cast< std::size_t >(2U));
    $pc_port_require_eq(service.requested_files[0U], std::string("/LayoutData/Test.bin"));
    $pc_port_require_eq(service.requested_files[1U], std::string("/LayoutData/Test.bin"));
    $pc_port_require_eq(loader.layout_archive_file_name_from_prefix("Hud", true).value_or(std::string{}), std::string("/LayoutData/Hud.arc"));
    $pc_port_require_eq(loader.normalize_path("LayoutData/Hud.arc"), std::string("/LayoutData/Hud.arc"));
    $pc_port_require_eq(loader.language(), std::string_view("KrKorean"));
    $pc_port_require_eq(loader.to_logical_path("/LayoutData/Test.bin"), std::string("LayoutData/Test.bin"));
}

$test("AssetLoader centralizes archive mount requests") {
    FakeGameAssetService service{};
    const smgpc::assets::AssetLoader loader{smgpc::di::DependencyReference< smgpc::assets::IGameAssetService >(service)};

    loader.request_archive("/LayoutData/Missing.arc");
    const auto archive = loader.archive("/LayoutData/Missing.arc");

    $pc_port_require(archive == nullptr);
    $pc_port_require(!loader.is_mounted_archive("/LayoutData/Missing.arc"));
    $pc_port_require_eq(service.requested_archives.size(), static_cast< std::size_t >(2U));
    $pc_port_require_eq(service.requested_archives[0U], std::string("/LayoutData/Missing.arc"));
    $pc_port_require_eq(service.requested_archives[1U], std::string("/LayoutData/Missing.arc"));
}

$test("AssetLoader decodes arbitrary file assets through typed helper") {
    FakeGameAssetService service{};
    const smgpc::assets::AssetLoader loader{smgpc::di::DependencyReference< smgpc::assets::IGameAssetService >(service)};

    const auto decoded = loader.load_file_as< std::string >("/LayoutData/Test.bin", "test payload", [](std::span< const std::byte > bytes) {
        return smgpc::assets::AssetResult< std::string >(smgpc::test::string_from_bytes(bytes));
    });

    $pc_port_require(decoded.has_value());
    $pc_port_require_eq(*decoded, std::string("\x11\x22", 2U));
}

$test("AssetLoader typed helper reports decode failures as empty optionals") {
    FakeGameAssetService service{};
    const smgpc::assets::AssetLoader loader{smgpc::di::DependencyReference< smgpc::assets::IGameAssetService >(service)};

    const auto decoded = loader.load_file_as< std::string >("/LayoutData/Test.bin", "test payload", [](std::span< const std::byte >) {
        return smgpc::assets::AssetResult< std::string >(smgpc::assets::AssetError{
            .code = smgpc::assets::AssetErrorCode::InvalidFormat,
            .message = "intentional test failure",
        });
    });

    $pc_port_require(!decoded.has_value());
}
