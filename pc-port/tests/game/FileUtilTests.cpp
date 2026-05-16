#include "game/Game/Util/FileUtil.hpp"

#include "assets/AssetLoader.hpp"
#include "assets/GameAssetService.hpp"
#include "game/compat/RuntimeContext.hpp"
#include "tests/TestHarness.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {

class RecordingGameAssetService final : public smgpc::assets::IGameAssetService {
public:
    void request_load_file(std::string_view file_path) override {
        requested_files.push_back(std::string(file_path));
    }

    [[nodiscard]] std::shared_ptr<const std::vector<std::byte>> receive_file(std::string_view file_path) override {
        requested_files.push_back(std::string(file_path));
        if (file_path == "/LayoutData/Test.bin") {
            return file_bytes;
        }
        return nullptr;
    }

    [[nodiscard]] bool is_loaded_file(std::string_view file_path) const override {
        return file_path == "/LayoutData/Test.bin";
    }

    void request_mount_archive(std::string_view archive_path) override {
        requested_archives.push_back(std::string(archive_path));
    }

    [[nodiscard]] std::shared_ptr<const smgpc::assets::MountedArchiveData> receive_archive(std::string_view archive_path) override {
        requested_archives.push_back(std::string(archive_path));
        if (archive_path == "/LayoutData/Test.arc") {
            return mounted_archive;
        }
        return nullptr;
    }

    [[nodiscard]] bool is_mounted_archive(std::string_view archive_path) const override {
        return archive_path == "/LayoutData/Test.arc";
    }

    [[nodiscard]] const smgpc::assets::GameAssetPathResolver &path_resolver() const override {
        return resolver;
    }

    smgpc::assets::GameAssetPathResolver resolver {smgpc::assets::GameAssetPathResolverConfiguration {
        .game_root = "/tmp",
        .version = "RMGK01",
        .language = "KrKorean",
        .is_widescreen = true,
    }};
    std::shared_ptr<const std::vector<std::byte>> file_bytes {std::make_shared<const std::vector<std::byte>>(std::vector<std::byte> {std::byte {0x11}, std::byte {0x22}})};
    std::shared_ptr<smgpc::assets::MountedArchiveData> mounted_archive {std::make_shared<smgpc::assets::MountedArchiveData>()};
    std::vector<std::string> requested_files {};
    std::vector<std::string> requested_archives {};
};

}  // namespace

$test("FileUtil delegates file and resolver calls through AssetLoader") {
    RecordingGameAssetService service {};
    smgpc::assets::AssetLoader asset_loader {smgpc::di::DependencyReference<smgpc::assets::IGameAssetService>(service)};
    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {
        .asset_loader = &asset_loader,
    });

    MR::loadAsyncToMainRAM("/LayoutData/Test.bin", nullptr, nullptr, JKRDvdRipper::ALLOC_DIR_TOP);
    auto *bytes = static_cast<std::byte *>(MR::receiveFile("/LayoutData/Test.bin"));

    $pc_port_require(bytes != nullptr);
    $pc_port_require(bytes[0] == std::byte {0x11});
    $pc_port_require(MR::isLoadedFile("/LayoutData/Test.bin"));
    $pc_port_require_eq(service.requested_files.size(), static_cast<std::size_t>(2U));
    $pc_port_require_eq(service.requested_files[0U], std::string("/LayoutData/Test.bin"));
    $pc_port_require_eq(service.requested_files[1U], std::string("/LayoutData/Test.bin"));

    char localized_path[64] {};
    MR::makeFileNameConsideringLanguage(localized_path, sizeof(localized_path), "LayoutData/Test.bin");
    $pc_port_require_eq(std::string(localized_path), std::string("/LayoutData/Test.bin"));

    char layout_path[64] {};
    $pc_port_require(MR::makeLayoutArchiveFileNameFromPrefix(layout_path, sizeof(layout_path), "Hud", true));
    $pc_port_require_eq(std::string(layout_path), std::string("/LayoutData/Hud.arc"));

    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {});
}

$test("FileUtil delegates archive calls through AssetLoader") {
    RecordingGameAssetService service {};
    smgpc::assets::AssetLoader asset_loader {smgpc::di::DependencyReference<smgpc::assets::IGameAssetService>(service)};
    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {
        .asset_loader = &asset_loader,
    });

    MR::mountAsyncArchive("/LayoutData/Test.arc", nullptr);
    JKRMemArchive *archive = MR::receiveArchive("/LayoutData/Test.arc");

    $pc_port_require(archive != nullptr);
    $pc_port_require(archive->archive() != nullptr);
    $pc_port_require(MR::isMountedArchive("/LayoutData/Test.arc"));
    $pc_port_require_eq(service.requested_archives.size(), static_cast<std::size_t>(2U));
    $pc_port_require_eq(service.requested_archives[0U], std::string("/LayoutData/Test.arc"));
    $pc_port_require_eq(service.requested_archives[1U], std::string("/LayoutData/Test.arc"));

    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {});
}
