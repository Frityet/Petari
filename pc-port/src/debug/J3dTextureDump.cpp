#include "capture/ScreenshotService.hpp"
#include "render/J3dTexture.hpp"
#include "resource/RarcArchive.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

    [[nodiscard]] std::filesystem::path disc_files_root() {
        const auto cwd = std::filesystem::current_path();
        const std::filesystem::path candidates[]{
            cwd / "orig" / "RMGK01" / "files",
            cwd.parent_path() / "orig" / "RMGK01" / "files",
        };

        for (const auto &candidate : candidates) {
            std::error_code error{};
            const auto canonical = std::filesystem::weakly_canonical(candidate, error);
            if (!error && std::filesystem::is_directory(canonical, error)) {
                return canonical;
            }
        }

        throw std::runtime_error("could not locate orig/RMGK01/files from " + cwd.string());
    }

    [[nodiscard]] std::filesystem::path pc_port_root() {
        const auto cwd = std::filesystem::current_path();
        std::error_code error{};
        if (std::filesystem::is_directory(cwd / "pc-port" / "src", error)) {
            return cwd / "pc-port";
        }
        if (std::filesystem::is_directory(cwd / "src", error) && std::filesystem::is_regular_file(cwd / "xmake.lua", error)) {
            return cwd;
        }

        return cwd / "pc-port";
    }

    [[nodiscard]] std::string lowercase(std::string_view text) {
        auto lowered = std::string(text);
        std::ranges::transform(lowered, lowered.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return lowered;
    }

    [[nodiscard]] bool ends_with_ignore_case(std::string_view text, std::string_view suffix) {
        if (text.size() < suffix.size()) {
            return false;
        }

        return lowercase(text.substr(text.size() - suffix.size())) == lowercase(suffix);
    }

    [[nodiscard]] std::string sanitize_filename(std::string_view text) {
        auto sanitized = std::string{};
        sanitized.reserve(text.size());

        for (const auto c : text) {
            const auto value = static_cast<unsigned char>(c);
            if (std::isalnum(value) != 0 || c == '-' || c == '_') {
                sanitized.push_back(c);
            } else {
                sanitized.push_back('_');
            }
        }

        return sanitized.empty() ? "texture" : sanitized;
    }

    [[nodiscard]] const char *texture_format_name(smgpc::resource::TplTextureFormat format) {
        switch (format) {
        case smgpc::resource::TplTextureFormat::I4:
            return "I4";
        case smgpc::resource::TplTextureFormat::I8:
            return "I8";
        case smgpc::resource::TplTextureFormat::IA4:
            return "IA4";
        case smgpc::resource::TplTextureFormat::IA8:
            return "IA8";
        case smgpc::resource::TplTextureFormat::RGB565:
            return "RGB565";
        case smgpc::resource::TplTextureFormat::RGB5A3:
            return "RGB5A3";
        case smgpc::resource::TplTextureFormat::RGBA8:
            return "RGBA8";
        case smgpc::resource::TplTextureFormat::C4:
            return "C4";
        case smgpc::resource::TplTextureFormat::C8:
            return "C8";
        case smgpc::resource::TplTextureFormat::C14X2:
            return "C14X2";
        case smgpc::resource::TplTextureFormat::CMPR:
            return "CMPR";
        }

        return "unknown";
    }

    void write_texture_png(const smgpc::render::capture::IScreenshotService &screenshot_service, const std::filesystem::path &output, const smgpc::resource::DecodedTexture &texture) {
        screenshot_service.write_png(
            output,
            smgpc::render::capture::ScreenshotImageView{
                .width = texture.width,
                .height = texture.height,
                .pitch = texture.width * 4U,
                .pixels = std::span<const std::uint8_t>(texture.rgba.data(), texture.rgba.size()),
                .format = smgpc::render::capture::PixelFormat::RGBA8,
                .origin_bottom_left = false,
            });
    }

    [[nodiscard]] bool has_translucent_pixels(const smgpc::resource::DecodedTexture &texture) {
        for (auto i = std::size_t{3U}; i < texture.rgba.size(); i += 4U) {
            if (texture.rgba[i] != 0xffU) {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] smgpc::resource::DecodedTexture make_opaque_preview(smgpc::resource::DecodedTexture texture) {
        for (auto i = std::size_t{3U}; i < texture.rgba.size(); i += 4U) {
            texture.rgba[i] = 0xffU;
        }

        return texture;
    }

    [[nodiscard]] std::filesystem::path texture_output_path(const std::filesystem::path &model_output_root, std::size_t index, std::string_view name, std::string_view suffix) {
        auto stem = std::ostringstream{};
        stem << std::setw(2) << std::setfill('0') << index << '-' << sanitize_filename(name) << suffix << ".png";
        return model_output_root / stem.str();
    }

    void dump_model_textures(
        const smgpc::render::capture::IScreenshotService &screenshot_service,
        const std::filesystem::path &output_root,
        std::ofstream &manifest,
        std::string_view object_name,
        const smgpc::resource::RarcArchive &archive,
        const smgpc::resource::RarcEntry &entry) {
        const auto model_path = std::filesystem::path(entry.path);
        const auto model_output_root = output_root / sanitize_filename(object_name) / sanitize_filename(model_path.stem().string());
        const auto textures = smgpc::render::extract_j3d_textures(archive.file_data(entry));

        std::cout << entry.path << ": " << textures.size() << " textures\n";
        for (auto i = std::size_t{}; i < textures.size(); ++i) {
            const auto &texture = textures[i];
            const auto output = texture_output_path(model_output_root, i, texture.name, "");
            write_texture_png(screenshot_service, output, texture.image);
            std::cout << output << '\n';

            manifest << object_name << ',' << entry.path << ',' << i << ',' << texture.name << ','
                     << texture.image.width << 'x' << texture.image.height << ','
                     << texture_format_name(texture.image.format) << ','
                     << static_cast<int>(texture.wrap_s) << ','
                     << static_cast<int>(texture.wrap_t) << ','
                     << output.string() << '\n';

            if (has_translucent_pixels(texture.image)) {
                const auto opaque_output = texture_output_path(model_output_root, i, texture.name, "-opaque-preview");
                write_texture_png(screenshot_service, opaque_output, make_opaque_preview(texture.image));
                std::cout << opaque_output << '\n';
            }
        }
    }

}  // namespace

int main(int argc, char **argv) try {
    const auto object_name = argc > 1 ? std::string_view(argv[1]) : std::string_view("CometNearOrbitSky");
    const auto archive_path = disc_files_root() / "ObjectData" / (std::string(object_name) + ".arc");
    const auto archive = smgpc::resource::RarcArchive::from_file(archive_path);
    const auto output_root = pc_port_root() / ".cache" / "j3d-textures";
    const auto manifest_path = output_root / sanitize_filename(object_name) / "manifest.csv";
    std::filesystem::create_directories(manifest_path.parent_path());

    auto manifest = std::ofstream(manifest_path);
    if (!manifest) {
        throw std::runtime_error("cannot write J3D texture manifest " + manifest_path.string());
    }

    manifest << "object,model,index,name,dimensions,format,wrap_s,wrap_t,path\n";

    const auto screenshot_service = smgpc::render::capture::create_png_screenshot_service();
    auto model_count = 0U;
    for (const auto &entry : archive.entries()) {
        if (!ends_with_ignore_case(entry.path, ".bdl") && !ends_with_ignore_case(entry.path, ".bmd")) {
            continue;
        }

        dump_model_textures(*screenshot_service, output_root, manifest, object_name, archive, entry);
        ++model_count;
    }

    if (model_count == 0U) {
        throw std::runtime_error("object archive contains no J3D model files: " + archive_path.string());
    }

    std::cout << manifest_path << '\n';
    return 0;
} catch (const std::exception &e) {
    std::cerr << "J3D texture dump failed: " << e.what() << '\n';
    return 1;
}
