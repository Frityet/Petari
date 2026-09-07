#include <aurora/exception.hpp>
#include "DebugPaths.hpp"
#include "capture/ScreenshotService.hpp"
#include "layout/BrfntFont.hpp"
#include "resource/RarcArchive.hpp"

#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {




    [[nodiscard]] std::uint16_t parse_code(std::string_view text) {
        auto value_text = std::string(text);
        const auto base = value_text.starts_with("0x") || value_text.starts_with("0X") ? 0 : 16;
        const auto value = std::stoul(value_text, nullptr, base);
        if (value > UINT16_MAX) {
            aurora::throw_host_exception<std::runtime_error>("glyph code is out of u16 range: " + value_text);
        }

        return static_cast<std::uint16_t>(value);
    }

    void write_texture_png(const smgpc::render::capture::IScreenshotService &screenshot_service, const std::filesystem::path &output,
                           const smgpc::resource::DecodedTexture &texture) {
        screenshot_service.write_png(output,
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
    const auto font_name = argc > 1 ? std::string(argv[1]) : std::string("messagefont26.brfnt");
    auto codes = std::vector<std::uint16_t>{0xff21U, 0x0041U, 0x0042U};
    if (argc > 2) {
        codes.clear();
        for (auto i = 2; i < argc; ++i) {
            codes.push_back(parse_code(argv[i]));
        }
    }

    const auto archive_path = smgpc::debug::disc_files_root() / "KrKorean" / "LayoutData" / "Font.arc";
    const auto archive = smgpc::resource::RarcArchive::from_file(archive_path);
    const auto font = smgpc::layout::parse_brfnt_font(archive.file_data(font_name));

    std::cout << "font," << font_name << '\n';
    std::cout << "metrics,height=" << static_cast<unsigned>(font.height) << ",width=" << static_cast<unsigned>(font.width)
              << ",cell=" << static_cast<unsigned>(font.cell_width) << "x" << static_cast<unsigned>(font.cell_height)
              << ",sheet=" << font.sheet_width << "x" << font.sheet_height << ",sheets=" << font.sheets.size() << '\n';
    for (const auto &map : font.code_maps) {
        std::cout << "map,begin=0x" << std::hex << map.begin << ",end=0x" << map.end << std::dec
                  << ",method=" << static_cast<unsigned>(map.method) << '\n';
    }

    for (const auto code : codes) {
        const auto glyph = font.glyph_for(code);
        std::cout << "glyph,code=0x" << std::hex << code << std::dec;
        if (!glyph.has_value()) {
            std::cout << ",missing\n";
            continue;
        }

        std::cout << ",sheet=" << glyph->sheet_index << ",xy=" << glyph->x << "," << glyph->y << ",size="
                  << static_cast<unsigned>(glyph->width) << "x" << static_cast<unsigned>(glyph->height)
                  << ",widths=" << static_cast<int>(glyph->widths.left) << "/" << static_cast<unsigned>(glyph->widths.glyph_width) << "/"
                  << static_cast<int>(glyph->widths.char_width) << '\n';
    }

    const auto output_root = smgpc::debug::pc_port_root() / ".cache" / "font-probes";
    const auto screenshot_service = smgpc::render::capture::create_png_screenshot_service();
    for (auto sheet = 0U; sheet < font.sheets.size(); ++sheet) {
        const auto output = output_root / (font_name + "-sheet" + std::to_string(sheet) + ".png");
        write_texture_png(*screenshot_service, output, font.sheets[sheet]);
        std::cout << "sheet_png," << output << '\n';
    }

    return 0;
} catch (const std::exception &e) {
    std::cerr << "BRFNT probe failed: " << e.what() << '\n';
    return 1;
}
