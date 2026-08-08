#include "Game/Util/LayoutUtil.hpp"
#include "JSystem/JKernel/JKRMemArchive.hpp"
#include "layout/BrlytLayout.hpp"
#include "layout/LayoutRuntime.hpp"
#include "resource/RarcArchive.hpp"

#include <nw4r/ut/ResFont.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    struct RetailFontFixture {
        std::filesystem::path mii_font_archive;
        std::filesystem::path file_info_archive;
    };

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    template < typename Exception, typename Operation >
    void require_throws(Operation&& operation, std::string_view message) {
        try {
            operation();
        } catch (const Exception&) {
            return;
        }
        throw std::runtime_error(std::string(message));
    }

    [[nodiscard]] std::optional< RetailFontFixture > find_retail_font_fixture() {
        for (auto root = std::filesystem::current_path(); !root.empty(); root = root.parent_path()) {
            const std::array candidates{
                root / "orig/RMGK02/files/LayoutData",
                root / "orig/RMGK01/files/LayoutData",
                root / "container/orig/RMGK01/files/LayoutData",
                root / "pc-port/container/orig/RMGK01/files/LayoutData",
            };
            for (const auto& directory : candidates) {
                const auto mii_font = directory / "MiiFont.arc";
                const auto file_info = directory / "FileInfo.arc";
                auto error = std::error_code{};
                if (std::filesystem::is_regular_file(mii_font, error) && !error &&
                    std::filesystem::is_regular_file(file_info, error) && !error) {
                    return RetailFontFixture{
                        .mii_font_archive = mii_font,
                        .file_info_archive = file_info,
                    };
                }
            }
            if (root == root.root_path()) {
                break;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] bool pane_descends_from(const smgpc::layout::BrlytLayout& layout, std::size_t pane_index,
                                          std::string_view ancestor_name) {
        for (auto depth = std::size_t{}; pane_index < layout.panes.size() && depth < layout.panes.size(); ++depth) {
            const auto& pane = layout.panes[pane_index];
            if (pane.name == ancestor_name) {
                return true;
            }
            if (pane.parent_index < 0) {
                break;
            }
            pane_index = static_cast< std::size_t >(pane.parent_index);
        }
        return false;
    }

    void test_absent_and_malformed_resources_fail_honestly() {
        auto font = nw4r::ut::ResFont{};
        require(!font.SetResource(nullptr) && !font.SetAlternateChar('?') && !font.HasGlyph('?') && font.IsManaging(nullptr),
                "an empty ResFont must report that no resource or glyph is installed");
        require_throws< std::logic_error >([&] { (void)font.GetWidth(); },
                                           "font metrics without a resource must be unavailable");
#ifndef NDEBUG
        require_throws< std::invalid_argument >([&] { MR::setTextBoxFontRecursive(nullptr, "FileName", nullptr); },
                                                "the MR bridge must reject an absent font before touching a layout");
#endif

        auto malformed = std::array< std::uint8_t, 16U >{
            'R', 'F', 'N', 'T', 0xfeU, 0xffU, 0x01U, 0x04U,
            0x00U, 0x00U, 0x00U, 0x10U, 0x00U, 0x10U, 0x00U, 0x00U,
        };
        require(!font.SetResource(malformed.data(), malformed.size()) && font.IsManaging(nullptr),
                "a header-only BRFNT must fail without installing partial state");
    }

    void test_retail_mii_font_and_layout_binding(const RetailFontFixture& fixture) {
        const auto archive = smgpc::resource::RarcArchive::from_file(fixture.mii_font_archive);
        auto mounted = JKRMemArchive(archive);
        auto* resource = mounted.getResource("/MiiFont26.brfnt");
        const auto resource_size = mounted.getResSize(resource);
        require(resource != nullptr && resource_size > 16U,
                "MiiFont.arc must expose the real MiiFont26.brfnt bytes through JKRMemArchive");

        auto font = nw4r::ut::ResFont{};
        auto unsupported_version = std::vector< std::uint8_t >(
            static_cast< const std::uint8_t* >(resource),
            static_cast< const std::uint8_t* >(resource) + resource_size);
        unsupported_version[6U] = 0x01U;
        unsupported_version[7U] = 0x03U;
        require(!font.SetResource(unsupported_version.data(), unsupported_version.size()) && font.IsManaging(nullptr),
                "an unsupported BRFNT version must fail transactionally instead of becoming a partial font");
        require(!font.SetResource(resource, resource_size - 1U),
                "the size-aware host overload must reject a truncated resource view");
        require(font.SetResource(resource) && font.IsManaging(resource),
                "the retail pointer-only SetResource call used by FileSelector must parse MiiFont26.brfnt");
        require(!font.SetResource(resource),
                "ResFont must reject a second resource while it already manages one");
        const auto default_widths = font.GetDefaultCharWidths();
        require(font.GetType() == nw4r::ut::Font::TYPE_RESOURCE &&
                    font.GetWidth() == 27 && font.GetHeight() == 33 && font.GetAscent() == 26 &&
                    font.GetDescent() == 7 && font.GetBaselinePos() == 26 && font.GetCellWidth() == 27 &&
                    font.GetCellHeight() == 33 && font.GetMaxCharWidth() == 27 && font.GetLineFeed() == 33 &&
                    font.GetTextureFormat() == GX_TF_I4 && font.GetEncoding() == nw4r::ut::FONT_ENCODING_UTF16 &&
                    default_widths.left == 0 && default_widths.glyphWidth == 27U && default_widths.charWidth == 27,
                "the ResFont metric surface must expose the exact retail MiiFont26 FINF/TGLP values");
        require(font.SetAlternateChar('?') && !font.SetAlternateChar(0xffffU) &&
                    font.HasGlyph('?') && !font.HasGlyph(0xffffU),
                "the retail Mii font must install '?' as its real alternate glyph");

        auto question = nw4r::ut::Glyph{};
        auto missing = nw4r::ut::Glyph{};
        font.GetGlyph(&question, '?');
        font.GetGlyph(&missing, 0xffffU);
        require_throws< std::invalid_argument >([&] { font.GetGlyph(nullptr, '?'); },
                                                "GetGlyph must reject absent output storage");
        const auto* resource_begin = static_cast< const std::uint8_t* >(resource);
        const auto* resource_end = resource_begin + resource_size;
        const auto* question_texture = static_cast< const std::uint8_t* >(question.pTexture);
        require(question_texture >= resource_begin && question_texture < resource_end &&
                    question.pTexture == missing.pTexture &&
                    question.cellX == missing.cellX && question.cellY == missing.cellY &&
                    question.widths.charWidth == missing.widths.charWidth &&
                    font.GetCharWidth('?') == font.GetCharWidth(0xffffU),
                "missing characters must resolve to the selected '?' cell and real installed BRFNT sheet storage");
        require(question.texFormat == GX_TF_I4 && question.texWidth == 256U && question.texHeight == 512U &&
                    question.texWidth > question.cellX && question.texHeight > question.cellY && question.height == 33U,
                "GetGlyph must expose real encoded sheet storage and in-bounds retail cell coordinates");

#ifndef NDEBUG
        const auto file_info_archive = smgpc::resource::RarcArchive::from_file(fixture.file_info_archive);
        const auto file_info_layout = smgpc::layout::parse_brlyt_layout(
            file_info_archive.file_data("blyt/fileinfo.brlyt"));
        auto file_name_descendants = std::vector< std::string >{};
        for (const auto& text_box : file_info_layout.text_boxes) {
            if (pane_descends_from(file_info_layout, text_box.pane_index, "FileName")) {
                file_name_descendants.push_back(text_box.name);
            }
        }
        require(file_name_descendants.size() == 2U &&
                    std::ranges::find(file_name_descendants, "ShaName") != file_name_descendants.end() &&
                    std::ranges::find(file_name_descendants, "TxtName") != file_name_descendants.end(),
                "the retail FileName pane topology must contain the ShaName and TxtName text boxes");

        auto layout = smgpc::layout::LayoutRuntime(
            "mii-font-compat-test", "FileInfo", 3U, 0, fixture.file_info_archive);
        require(layout.hasPane("FileName"), "FileInfo.arc must contain the real FileName text box");
        layout.setTextBoxFontRecursive("FileName", font);
        layout.setTextBoxStringRecursive("FileName", std::u16string_view(u"\uffff"));
        const auto missing_rasters = layout.debugTextRasters("FileName");
        std::cout << "FileInfo/FileName recursive text boxes:";
        for (const auto& raster : missing_rasters) {
            std::cout << ' ' << raster.text_box_name;
        }
        std::cout << '\n';
        const auto missing_rasters_are_real = missing_rasters.size() == file_name_descendants.size() &&
            std::ranges::all_of(missing_rasters, [&font](const auto& raster) {
                return raster.external_font && raster.font_width == font.GetWidth() &&
                       raster.font_height == font.GetHeight() && raster.nontransparent_pixel_count > 0U;
            }) && std::ranges::all_of(file_name_descendants, [&missing_rasters](const auto& descendant_name) {
                return std::ranges::find(missing_rasters, descendant_name, &smgpc::layout::LayoutRuntime::DebugTextRasterState::text_box_name) !=
                       missing_rasters.end();
            });
        if (!missing_rasters_are_real) {
            std::cerr << "missing-raster diagnostics: count=" << missing_rasters.size();
            for (const auto& raster : missing_rasters) {
                std::cerr << ",external=" << raster.external_font << ",generation=" << raster.font_generation
                          << ",size=" << raster.width << 'x' << raster.height
                          << ",font=" << raster.font_width << 'x' << raster.font_height
                          << ",opaque=" << raster.nontransparent_pixel_count << ",hash=" << raster.rgba_hash;
            }
            std::cerr << '\n';
        }
        require(missing_rasters_are_real,
                "the bound external BRFNT must rasterize the missing character through its '?' glyph");

        layout.setTextBoxStringRecursive("FileName", std::u16string_view(u"?"));
        const auto question_rasters = layout.debugTextRasters("FileName");
        const auto same_question_rasters = question_rasters.size() == missing_rasters.size() &&
            std::ranges::equal(question_rasters, missing_rasters, [](const auto& question_raster, const auto& missing_raster) {
                return question_raster.external_font == missing_raster.external_font &&
                       question_raster.width == missing_raster.width && question_raster.height == missing_raster.height &&
                       question_raster.font_width == missing_raster.font_width && question_raster.font_height == missing_raster.font_height &&
                       question_raster.nontransparent_pixel_count == missing_raster.nontransparent_pixel_count &&
                       question_raster.rgba_hash == missing_raster.rgba_hash;
            });
        require(same_question_rasters,
                "LayoutRuntime must render the alternate character with the same real glyph bitmap and advance as '?'");

        font.RemoveResource();
        require(font.IsManaging(nullptr) && layout.debugTextRasters("FileName").empty(),
                "RemoveResource must invalidate an existing pane binding instead of preserving a copied fallback font");
        require(font.SetResource(resource) && font.SetAlternateChar('?') &&
                    !layout.debugTextRasters("FileName").empty(),
                "reinstalling a real resource must reactivate bindings to the same live ResFont object");

        {
            auto temporary_font = nw4r::ut::ResFont{};
            require(temporary_font.SetResource(resource) && temporary_font.SetAlternateChar('?'),
                    "the temporary lifetime check requires the retail font");
            layout.setTextBoxFontRecursive("FileName", temporary_font);
            require(!layout.debugTextRasters("FileName").empty(),
                    "a live temporary ResFont must drive its pane binding");
        }
        require(layout.debugTextRasters("FileName").empty(),
                "destroying ResFont must expire the retail-style pointer binding instead of retaining a hidden copy");

        auto empty_font = nw4r::ut::ResFont{};
        require_throws< std::invalid_argument >([&] { layout.setTextBoxFontRecursive("FileName", empty_font); },
                                                "LayoutRuntime must reject a Font without a parsed BRFNT");
#endif
    }

}  // namespace

int main() {
    test_absent_and_malformed_resources_fail_honestly();

    const auto fixture = find_retail_font_fixture();
    if (!fixture.has_value()) {
        std::cout << "[skip] retail MiiFont.arc/FileInfo.arc checks\n";
        std::cout << "Mii font compatibility tests passed: 1/2\n";
        return 0;
    }

    test_retail_mii_font_and_layout_binding(*fixture);
    std::cout << "Mii font compatibility tests passed: 2/2\n";
    return 0;
}
