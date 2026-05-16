#include "game/Game/Util/LayoutUtil.hpp"

#include "assets/AssetLoader.hpp"
#include "assets/GameAssetService.hpp"
#include "assets/layout/RarcArchive.hpp"
#include "assets/layout/Yaz0.hpp"
#include "game/Game/Screen/IconAButton.hpp"
#include "game/Game/Screen/ProloguePictureBook.hpp"
#include "game/compat/LayoutSceneCompat.hpp"
#include "game/compat/RuntimeContext.hpp"
#include "render/layout/LayoutDrawList.hpp"
#include "tests/TestHarness.hpp"

#include <cwchar>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] std::vector< std::byte > load_file_bytes(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    $pc_port_require(stream.is_open());

    const std::vector< char > raw_chars((std::istreambuf_iterator< char >(stream)), std::istreambuf_iterator< char >());
    std::vector< std::byte > bytes {};
    bytes.reserve(raw_chars.size());
    for (const char ch : raw_chars) {
        bytes.push_back(static_cast< std::byte >(static_cast< unsigned char >(ch)));
    }
    return bytes;
}

[[nodiscard]] std::filesystem::path first_existing_path(std::initializer_list< std::filesystem::path > candidates) {
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

class FakeGameAssetService final : public smgpc::assets::IGameAssetService {
public:
    explicit FakeGameAssetService(
        std::shared_ptr< smgpc::assets::MountedArchiveData > message_archive,
        std::shared_ptr< smgpc::assets::MountedArchiveData > icon_a_button_archive = nullptr,
        std::shared_ptr< smgpc::assets::MountedArchiveData > prologue_demo_archive = nullptr)
        : _messageArchive(std::move(message_archive)),
          _iconAButtonArchive(std::move(icon_a_button_archive)),
          _prologueDemoArchive(std::move(prologue_demo_archive)),
          _pathResolver(smgpc::assets::GameAssetPathResolverConfiguration {
              .game_root = first_existing_path({"..", "."}),
              .version = "RMGK01",
              .language = "KrKorean",
              .is_widescreen = true,
          }) {
    }

    void request_load_file(std::string_view) override {
    }

    [[nodiscard]] std::shared_ptr< const std::vector< std::byte > > receive_file(std::string_view) override {
        return nullptr;
    }

    [[nodiscard]] bool is_loaded_file(std::string_view) const override {
        return false;
    }

    void request_mount_archive(std::string_view) override {
    }

    [[nodiscard]] std::shared_ptr< const smgpc::assets::MountedArchiveData > receive_archive(std::string_view archive_path) override {
        if (archive_path.find("Message.arc") != std::string_view::npos) {
            return _messageArchive;
        }
        if (archive_path.find("IconAButton.arc") != std::string_view::npos) {
            return _iconAButtonArchive;
        }
        if (archive_path.find("PrologueDemo.arc") != std::string_view::npos) {
            return _prologueDemoArchive;
        }

        return nullptr;
    }

    [[nodiscard]] bool is_mounted_archive(std::string_view archive_path) const override {
        return (archive_path.find("Message.arc") != std::string_view::npos && _messageArchive != nullptr) ||
               (archive_path.find("IconAButton.arc") != std::string_view::npos && _iconAButtonArchive != nullptr) ||
               (archive_path.find("PrologueDemo.arc") != std::string_view::npos && _prologueDemoArchive != nullptr);
    }

    [[nodiscard]] const smgpc::assets::GameAssetPathResolver& path_resolver() const override {
        return _pathResolver;
    }

private:
    std::shared_ptr< smgpc::assets::MountedArchiveData > _messageArchive {};
    std::shared_ptr< smgpc::assets::MountedArchiveData > _iconAButtonArchive {};
    std::shared_ptr< smgpc::assets::MountedArchiveData > _prologueDemoArchive {};
    smgpc::assets::GameAssetPathResolver _pathResolver;
};

[[nodiscard]] std::shared_ptr< smgpc::assets::MountedArchiveData > load_archive(const std::filesystem::path& archive_path) {
    if (archive_path.empty()) {
        return nullptr;
    }

    auto bytes = load_file_bytes(archive_path);
    if (smgpc::assets::layout::is_yaz0(bytes)) {
        auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
        $pc_port_require(decoded);
        bytes = std::move(*decoded);
    }

    auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
    $pc_port_require(archive);

    auto mounted_archive = std::make_shared< smgpc::assets::MountedArchiveData >();
    mounted_archive->archive = std::move(*archive);
    return mounted_archive;
}

[[nodiscard]] std::shared_ptr< smgpc::assets::MountedArchiveData > load_message_archive() {
    return load_archive(first_existing_path({
        "../orig/RMGK01/files/KrKorean/MessageData/Message.arc",
        "orig/RMGK01/files/KrKorean/MessageData/Message.arc",
    }));
}

}  // namespace

$test("LayoutUtil game message lookup decodes Korean Message.arc strings") {
    auto mounted_archive = load_message_archive();
    if (mounted_archive == nullptr) {
        return;
    }

    FakeGameAssetService asset_service(std::move(mounted_archive));
    smgpc::assets::AssetLoader asset_loader {smgpc::di::DependencyReference< smgpc::assets::IGameAssetService >(asset_service)};
    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {
        .asset_loader = &asset_loader,
    });

    const wchar_t* message = MR::getGameMessageDirect("System_FileSelect008");
    $pc_port_require(message != nullptr);
    $pc_port_require(std::wcscmp(message, L"\uD30C\uC77C\uC744\u0020\uC120\uD0DD\uD574\u0020\uC8FC\uC138\uC694") == 0);

    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {});
}

$test("LayoutUtil game message lookup can use AssetLoader runtime context") {
    auto mounted_archive = load_message_archive();
    if (mounted_archive == nullptr) {
        return;
    }

    FakeGameAssetService asset_service(std::move(mounted_archive));
    smgpc::assets::AssetLoader asset_loader {smgpc::di::DependencyReference< smgpc::assets::IGameAssetService >(asset_service)};
    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {
        .asset_loader = &asset_loader,
    });

    const wchar_t* message = MR::getGameMessageDirect("System_FileSelect008");
    $pc_port_require(message != nullptr);
    $pc_port_require(std::wcscmp(message, L"\uD30C\uC77C\uC744\u0020\uC120\uD0DD\uD574\u0020\uC8FC\uC138\uC694") == 0);

    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {});
}

$test("IconAButton emits picture quads after opening without a message") {
    auto icon_archive = load_archive(first_existing_path({
        "../orig/RMGK01/files/LayoutData/IconAButton.arc",
        "orig/RMGK01/files/LayoutData/IconAButton.arc",
    }));
    if (icon_archive == nullptr) {
        return;
    }

    FakeGameAssetService asset_service(nullptr, std::move(icon_archive));
    smgpc::assets::AssetLoader asset_loader {smgpc::di::DependencyReference< smgpc::assets::IGameAssetService >(asset_service)};
    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {
        .asset_loader = &asset_loader,
        .is_widescreen = true,
    });

    IconAButton button(true, false);
    button.initWithoutIter();
    button.openWithoutMessage();
    for (int i = 0; i < 24; ++i) {
        button.movement();
    }

    smgpc::render::layout::LayoutDrawList draw_list {};
    button.appendDrawCommands(&draw_list);

    $pc_port_require(!draw_list.quads().empty());
    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {});
}

$test("ProloguePictureBook opens A button at the first page wait") {
    auto icon_archive = load_archive(first_existing_path({
        "../orig/RMGK01/files/LayoutData/IconAButton.arc",
        "orig/RMGK01/files/LayoutData/IconAButton.arc",
    }));
    auto prologue_archive = load_archive(first_existing_path({
        "../orig/RMGK01/files/LayoutData/PrologueDemo.arc",
        "orig/RMGK01/files/LayoutData/PrologueDemo.arc",
    }));
    if (icon_archive == nullptr || prologue_archive == nullptr) {
        return;
    }

    FakeGameAssetService asset_service(nullptr, std::move(icon_archive), std::move(prologue_archive));
    smgpc::assets::AssetLoader asset_loader {smgpc::di::DependencyReference< smgpc::assets::IGameAssetService >(asset_service)};
    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {
        .asset_loader = &asset_loader,
        .is_widescreen = true,
    });

    ProloguePictureBook picture_book {};
    picture_book.initWithoutIter();
    picture_book.appear();
    for (int i = 0; i < 380; ++i) {
        picture_book.movement();
        smgpc::game::compat::movement_layout_scene_layer(smgpc::game::compat::LayoutSceneLayer::TalkLayout);
    }

    smgpc::render::layout::LayoutDrawList book_draw_list {};
    picture_book.appendDrawCommands(&book_draw_list);
    const auto book_quad_count = book_draw_list.quads().size();
    smgpc::game::compat::append_layout_scene_layer_draw_commands(smgpc::game::compat::LayoutSceneLayer::TalkLayout, &book_draw_list);

    $pc_port_require(book_draw_list.quads().size() > book_quad_count);
    bool found_prompt_in_layout_bounds = false;
    for (std::size_t quad_index = book_quad_count; quad_index < book_draw_list.quads().size(); ++quad_index) {
        const auto& quad = book_draw_list.quads()[quad_index];
        if (quad.x0 >= 0.0F && quad.x1 <= 608.0F && quad.y0 >= 0.0F && quad.y1 <= 456.0F) {
            found_prompt_in_layout_bounds = true;
        }
    }
    $pc_port_require(found_prompt_in_layout_bounds);
    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {});
}
