#include <aurora/exception.hpp>
#include "DebugPaths.hpp"
#include "capture/ScreenshotService.hpp"
#include "nw4r/ut/ResFont.h"
#include "resource/TplTexture.hpp"
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
    const auto data = archive.file_data(font_name);
    std::vector<u8> bytes(data.begin(), data.end());
    nw4r::ut::ResFont font;
    if (!font.SetResource(bytes.data(), bytes.size())) {
        aurora::throw_host_exception<std::runtime_error>("Original NW4R reader rejected font resource");
    }
    const auto* texture = font.mFontInfo->pGlyph.get();
    std::cout << "font," << font_name << '\n';
    std::cout << "metrics,height=" << font.GetHeight() << ",width=" << font.GetWidth()
              << ",cell=" << font.GetCellWidth() << "x" << font.GetCellHeight()
              << ",sheet=" << u16(texture->sheetWidth) << "x" << u16(texture->sheetHeight)
              << ",sheets=" << u16(texture->sheetNum) << '\n';
    for (auto* map = font.mFontInfo->pMap.get(); map; map = map->pNext) {
        std::cout << "map,begin=0x" << std::hex << u16(map->ccodeBegin) << ",end=0x" << u16(map->ccodeEnd) << std::dec
                  << ",method=" << u16(map->mappingMethod) << '\n';
    }
    for (const auto code : codes) {
        std::cout << "glyph,code=0x" << std::hex << code << std::dec;
        if (!font.HasGlyph(code)) { std::cout << ",missing\n"; continue; }
        nw4r::ut::Glyph glyph{};
        font.GetGlyph(&glyph, code);
        const auto sheet = (static_cast<const u8*>(glyph.pTexture) - texture->sheetImage.get()) / u32(texture->sheetSize);
        std::cout << ",sheet=" << sheet << ",xy=" << glyph.cellX << "," << glyph.cellY
                  << ",size=" << font.GetCellWidth() << "x" << unsigned(glyph.height)
                  << ",widths=" << int(glyph.widths.left) << "/" << unsigned(glyph.widths.glyphWidth)
                  << "/" << int(glyph.widths.charWidth) << '\n';
    }
    const auto output_root = smgpc::debug::pc_port_root() / ".cache" / "font-probes";
    const auto screenshot_service = smgpc::render::capture::create_png_screenshot_service();
    for (u16 sheet = 0; sheet < texture->sheetNum; ++sheet) {
        const auto output = output_root / (font_name + "-sheet" + std::to_string(sheet) + ".png");
        const auto decoded = smgpc::resource::decode_raw_gx_texture(
            {texture->sheetImage + sheet * u32(texture->sheetSize), u32(texture->sheetSize)},
            texture->sheetWidth, texture->sheetHeight, static_cast<smgpc::resource::TplTextureFormat>(u16(texture->sheetFormat)));
        write_texture_png(*screenshot_service, output, decoded);
        std::cout << "sheet_png," << output << '\n';
    }

    return 0;
} catch (const std::exception &e) {
    std::cerr << "BRFNT probe failed: " << e.what() << '\n';
    return 1;
}
