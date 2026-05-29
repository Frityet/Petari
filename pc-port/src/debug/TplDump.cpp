#include "resource/RarcArchive.hpp"
#include "resource/TplTexture.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
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

    void write_texture_ppm(const std::filesystem::path &output, const smgpc::resource::DecodedTexture &texture) {
        std::filesystem::create_directories(output.parent_path());
        auto file = std::ofstream(output, std::ios::binary);
        if (!file) {
            throw std::runtime_error("cannot write texture dump " + output.string());
        }
        file << "P6\n" << texture.width << ' ' << texture.height << "\n255\n";
        for (auto offset = std::size_t{}; offset + 3U < texture.rgba.size(); offset += 4U) {
            const auto alpha = static_cast<std::uint16_t>(texture.rgba[offset + 3U]);
            const auto rgb = std::array< char, 3U >{
                static_cast<char>((static_cast<std::uint16_t>(texture.rgba[offset]) * alpha) / 255U),
                static_cast<char>((static_cast<std::uint16_t>(texture.rgba[offset + 1U]) * alpha) / 255U),
                static_cast<char>((static_cast<std::uint16_t>(texture.rgba[offset + 2U]) * alpha) / 255U),
            };
            file.write(rgb.data(), static_cast<std::streamsize>(rgb.size()));
        }
    }

    void print_texture_stats(std::string_view name, const smgpc::resource::DecodedTexture &texture) {
        auto min_x = texture.width;
        auto min_y = texture.height;
        auto max_x = std::uint16_t{};
        auto max_y = std::uint16_t{};
        auto visible = std::uint64_t{};
        auto red = std::uint64_t{};
        auto green = std::uint64_t{};
        auto blue = std::uint64_t{};
        auto alpha = std::uint64_t{};
        for (auto y = std::uint16_t{}; y < texture.height; ++y) {
            for (auto x = std::uint16_t{}; x < texture.width; ++x) {
                const auto offset = (static_cast<std::size_t>(y) * texture.width + x) * 4U;
                red += texture.rgba[offset];
                green += texture.rgba[offset + 1U];
                blue += texture.rgba[offset + 2U];
                alpha += texture.rgba[offset + 3U];
                if (texture.rgba[offset + 3U] == 0U) {
                    continue;
                }
                min_x = std::min(min_x, x);
                min_y = std::min(min_y, y);
                max_x = std::max(max_x, x);
                max_y = std::max(max_y, y);
                ++visible;
            }
        }
        const auto pixels = std::max<std::uint64_t>(1U, static_cast<std::uint64_t>(texture.width) * texture.height);
        std::cout << name << " size=" << texture.width << 'x' << texture.height << " format=" << static_cast<std::uint32_t>(texture.format)
                  << " visible=" << visible << " bbox=" << min_x << ',' << min_y << '-' << max_x << ',' << max_y << " avg_rgba="
                  << (red / pixels) << ',' << (green / pixels) << ',' << (blue / pixels) << ',' << (alpha / pixels) << '\n';
    }

}  // namespace

int main(int argc, char **argv) try {
    if (argc == 3) {
        const auto archive_path = std::filesystem::path(argv[1]);
        const auto texture_output_root = std::filesystem::path(argv[2]);
        const auto archive = smgpc::resource::RarcArchive::from_file(archive_path);

        for (const auto &entry : archive.entries()) {
            if (!ends_with(entry.path, ".tpl")) {
                continue;
            }

            auto file_name = std::filesystem::path(entry.path).filename();
            file_name.replace_extension(".ppm");
            const auto texture = smgpc::resource::decode_tpl_texture(archive.file_data(entry));
            const auto texture_output = texture_output_root / file_name;
            write_texture_ppm(texture_output, texture);
            print_texture_stats(entry.path, texture);
            std::cout << texture_output << '\n';
        }

        return 0;
    }

    const auto root = disc_files_root();
    const auto title_logo_archive = smgpc::resource::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "TitleLogo.arc");
    const auto title_logo_texture = smgpc::resource::decode_tpl_texture(title_logo_archive.file_data("timg/mytitlelogokor.tpl"));
    const auto output = pc_port_root() / ".cache" / "decoded-title-logo.ppm";

    write_texture_ppm(output, title_logo_texture);
    print_texture_stats("timg/mytitlelogokor.tpl", title_logo_texture);
    std::cout << output << '\n';

    const auto texture_output_root = pc_port_root() / ".cache" / "title-logo-textures";
    for (const auto &entry : title_logo_archive.entries()) {
        if (!ends_with(entry.path, ".tpl")) {
            continue;
        }

        const auto texture = smgpc::resource::decode_tpl_texture(title_logo_archive.file_data(entry));
        const auto texture_output = texture_output_root / (std::filesystem::path(entry.path).filename().replace_extension(".ppm"));
        write_texture_ppm(texture_output, texture);
        print_texture_stats(entry.path, texture);
        std::cout << texture_output << '\n';
    }

    return 0;
} catch (const std::exception &e) {
    std::cerr << "tpl dump failed: " << e.what() << '\n';
    return 1;
}
