#include "capture/ScreenshotService.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/TplTexture.hpp"

#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace {

    [[nodiscard]] std::filesystem::path disc_files_root() {
        const auto cwd = std::filesystem::current_path();
        const std::filesystem::path candidates[]{
            cwd / "orig" / "RMGK01" / "files",
            cwd.parent_path() / "orig" / "RMGK01" / "files",
        };

        for (const auto &candidate : candidates) {
            std::error_code error {};
            const auto canonical = std::filesystem::weakly_canonical(candidate, error);
            if (!error && std::filesystem::is_directory(canonical, error)) {
                return canonical;
            }
        }

        throw std::runtime_error("could not locate orig/RMGK01/files from " + cwd.string());
    }

    [[nodiscard]] std::filesystem::path pc_port_root() {
        const auto cwd = std::filesystem::current_path();
        std::error_code error {};
        if (std::filesystem::is_directory(cwd / "pc-port" / "src", error)) {
            return cwd / "pc-port";
        }
        if (std::filesystem::is_directory(cwd / "src", error) && std::filesystem::is_regular_file(cwd / "xmake.lua", error)) {
            return cwd;
        }

        return cwd / "pc-port";
    }

    [[nodiscard]] bool ends_with(std::string_view text, std::string_view suffix) {
        return text.size() >= suffix.size() && text.substr(text.size() - suffix.size()) == suffix;
    }

    void write_texture_png(const smgpc::render::capture::IScreenshotService &screenshot_service, const std::filesystem::path &output, const smgpc::resource::DecodedTexture &texture) {
        screenshot_service.write_png(
            output,
            smgpc::render::capture::ScreenshotImageView {
                .width = texture.width,
                .height = texture.height,
                .pitch = texture.width * 4U,
                .pixels = std::span<const std::uint8_t>(texture.rgba.data(), texture.rgba.size()),
                .format = smgpc::render::capture::PixelFormat::RGBA8,
                .origin_bottom_left = false,
            });
    }

}  // namespace

int main(int argc, char **argv) try {
    const auto screenshot_service = smgpc::render::capture::create_png_screenshot_service();

    if (argc == 3) {
        const auto archive_path = std::filesystem::path(argv[1]);
        const auto texture_output_root = std::filesystem::path(argv[2]);
        const auto archive = smgpc::resource::RarcArchive::from_file(archive_path);

        for (const auto &entry : archive.entries()) {
            if (!ends_with(entry.path, ".tpl")) {
                continue;
            }

            auto file_name = std::filesystem::path(entry.path).filename();
            file_name.replace_extension(".png");
            const auto texture = smgpc::resource::decode_tpl_texture(archive.file_data(entry));
            const auto texture_output = texture_output_root / file_name;
            write_texture_png(*screenshot_service, texture_output, texture);
            std::cout << texture_output << '\n';
        }

        return 0;
    }

    const auto root = disc_files_root();
    const auto title_logo_archive = smgpc::resource::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "TitleLogo.arc");
    const auto title_logo_texture = smgpc::resource::decode_tpl_texture(title_logo_archive.file_data("timg/mytitlelogokor.tpl"));
    const auto output = pc_port_root() / ".cache" / "decoded-title-logo.png";

    write_texture_png(*screenshot_service, output, title_logo_texture);
    std::cout << output << '\n';

    const auto texture_output_root = pc_port_root() / ".cache" / "title-logo-textures";
    for (const auto &entry : title_logo_archive.entries()) {
        if (!ends_with(entry.path, ".tpl")) {
            continue;
        }

        const auto texture = smgpc::resource::decode_tpl_texture(title_logo_archive.file_data(entry));
        const auto texture_output = texture_output_root / (std::filesystem::path(entry.path).filename().replace_extension(".png"));
        write_texture_png(*screenshot_service, texture_output, texture);
        std::cout << texture_output << '\n';
    }

    return 0;
} catch (const std::exception &e) {
    std::cerr << "tpl dump failed: " << e.what() << '\n';
    return 1;
}
