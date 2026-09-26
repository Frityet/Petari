#include "OriginalStageResourceProcessFixture.hpp"
#include "runtime/RuntimeServices.hpp"
#include <fstream>
#include "Game/Util/LayoutUtil.hpp"
#include "JSystem/JKernel/JKRMemArchive.hpp"
#include "layout/BrlytLayout.hpp"
#include "layout/LayoutRuntime.hpp"
#include "layout/Nw4rLayoutRecords.hpp"
#include <nw4r/lyt/textBox.h>
#include <nw4r/ut/Rect.h>
#include "resource/RarcArchive.hpp"

#include <nw4r/ut/ResFont.h>
#include <nw4r/ut/binaryFileFormat.h>
#include <aurora/endian.hpp>

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

    struct RetailFontFiles {
        std::filesystem::path directory = std::filesystem::temp_directory_path() /
            ("petari-mii-font-resources-" + std::to_string(getpid()));
        RetailFontFixture fixture{directory / "MiiFont.arc", directory / "FileInfo.arc"};
        RetailFontFiles() {
            require(std::filesystem::create_directory(directory), "font fixture requires a fresh temporary directory");
            try {
                smgpc::runtime::DvdFileSystemService dvd{"/"};
                for (const auto& path : {fixture.mii_font_archive, fixture.file_info_archive}) {
                    const auto bytes = dvd.read_file("/LayoutData/" + path.filename().string());
                    std::ofstream output(path, std::ios::binary);
                    output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                    require(bool(output), "actual font fixture archive must be written completely");
                }
            } catch (...) {
                std::filesystem::remove_all(directory);
                throw;
            }
        }
        ~RetailFontFiles() { std::error_code error; std::filesystem::remove_all(directory, error); }
    };

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

    void test_original_font_tables() {
        // Literal Wii bytes cover all three CMAP methods, a backward-linked
        // width block, original fallback rules, and in-place RFNT -> RFNU reuse.
        std::array<u8, 264> bytes{};
        const auto u16at = [&](size_t offset, u16 value) { aurora::endian::write_big(bytes.data() + offset, value); };
        const auto u32at = [&](size_t offset, u32 value) { aurora::endian::write_big(bytes.data() + offset, value); };
        const auto block = [&](size_t offset, u32 kind, u32 size) { u32at(offset, kind); u32at(offset + 4, size); };
        u32at(0, 'RFNT'); u16at(4, 0xfeff); u16at(6, 0x104); u32at(8, bytes.size()); u16at(12, 16); u16at(14, 7);
        block(16, 'FINF', 32); bytes[24] = 1; bytes[25] = 2; u16at(26, 1);
        bytes[29] = 1; bytes[30] = 7; bytes[31] = nw4r::ut::FONT_ENCODING_UTF16;
        u32at(32, 56); u32at(36, 160); u32at(40, 184); bytes[44] = bytes[45] = bytes[46] = 1;
        block(48, 'TGLP', 80); bytes[56] = bytes[57] = bytes[58] = bytes[59] = 1;
        u32at(60, 32); u16at(64, 1); u16at(66, GX_TF_I4); u16at(68, 4); u16at(70, 4);
        u16at(72, 8); u16at(74, 8); u32at(76, 96);
        block(128, 'CWDH', 24); u16at(136, 0); u16at(138, 1);
        bytes[145] = bytes[148] = 1; bytes[146] = 2; bytes[149] = 3;
        block(152, 'CWDH', 24); u16at(160, 4); u16at(162, 5); u32at(164, 136);
        bytes[168] = 1; bytes[169] = 1; bytes[170] = 4;
        bytes[171] = 0xff; bytes[172] = 1; bytes[173] = 5;
        block(176, 'CMAP', 24); u16at(184, 'A'); u16at(186, 'B'); u32at(192, 208);
        block(200, 'CMAP', 28); u16at(208, 0x80); u16at(210, 0x82); u16at(212, 1); u32at(216, 236);
        u16at(220, 2); u16at(222, 0xffff); u16at(224, 3);
        block(228, 'CMAP', 36); u16at(236, 0x1000); u16at(238, 0x3000); u16at(240, 2); u16at(248, 3);
        u16at(250, 0x1000); u16at(252, 4); u16at(254, 0x2000); u16at(256, 5); u16at(258, 0x3000); u16at(260, 6);

        nw4r::ut::ResFont font;
        require(font.SetResource(bytes.data(), bytes.size()) && aurora::endian::read_u32(bytes.data()) == 'RFNU',
                "original loader relocates the actual resource in place and marks it RFNU");
        const std::array<u16, 7> codes{'A', 'B', 0x80, 0x82, 0x1000, 0x2000, 0x3000};
        const std::array<int, 7> widths{2, 3, 7, 7, 4, 5, 7};
        for (size_t i = 0; i < codes.size(); ++i) {
            require(font.FindGlyphIndex(codes[i]) == i && font.GetCharWidth(codes[i]) == widths[i],
                    "original direct, table and binary-scan maps resolve exact indices and linked/default widths");
        }
        require(!font.HasGlyph(0x81) && !font.HasGlyph(0x1800) && font.GetCharWidth(0x81) == 3 &&
                    font.SetAlternateChar(0x2000) && !font.SetAlternateChar(0x1800) && font.GetCharWidth(0x81) == 5,
                "map holes and failed binary searches use the current original alternate glyph");
        nw4r::ut::Glyph glyph{};
        font.GetGlyph(&glyph, 0x2000);
        require(glyph.pTexture == bytes.data() + 96 && glyph.cellX == 3 && glyph.cellY == 3 && glyph.widths.left == -1,
                "original glyph construction uses the borrowed sheet and exact authored cell/width values");
        nw4r::ut::ResFont shared;
        require(shared.SetResource(bytes.data()), "a second original font can borrow the already-relocated resource");
        font.SetLineFeed(11);
        font.SetDefaultCharWidths({-2, 3, 9});
        require(shared.GetLineFeed() == 11 && shared.GetCharWidth(0x80) == 9 && shared.GetDefaultCharWidths().left == -2,
                "font mutations update shared FINF bytes, with no substitute per-font cache");
        auto copied = bytes;
        nw4r::ut::ResFont relocated_copy;
        require(relocated_copy.SetResource(copied.data()), "relocating the complete RFNU buffer preserves its internal pointers");
        relocated_copy.GetGlyph(&glyph, 0x2000);
        require(glyph.pTexture == copied.data() + 96 && relocated_copy.GetCharWidth('A') == 2,
                "copied resources resolve both forward and backward pointers within their new allocation");
        font.RemoveResource();
        require(font.IsManaging(nullptr) && shared.GetLineFeed() == 11,
                "original resource removal releases only this borrow and leaves other fonts intact");
        std::cout << "Original NW4R font tables, shared mutations and 64-bit resource relocation passed\n";
    }

    void test_absent_and_malformed_resources_fail_honestly() {
        auto font = nw4r::ut::ResFont{};
        require(!font.SetResource(nullptr, 0) && font.IsManaging(nullptr),
                "the bounded native entry rejects absent storage without installing a resource");


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
        auto& records = layout.native_records();
        std::vector<nw4r::lyt::TextBox*> boxes;
        std::vector<nw4r::ut::Rect> missing_rects;
        for (const auto& descendant : file_name_descendants) {
            auto* box = nw4r::ut::DynamicCast<nw4r::lyt::TextBox*>(records.pane(descendant.c_str()));
            require(box, "FileName descendants must be the actual original SDK TextBox objects");
            const auto original_size = box->mFontSize;
            TextBoxRecursiveSetFont(&font).execute(box);
            require(box->mpFont == &font && box->mFontSize == original_size,
                    "the original font operation borrows the live font and preserves authored display size");
            box->AllocStringBuffer(2);
            require(box->SetString(L"\uffff", 0, 1) == 1, "actual SDK buffer accepts one missing glyph");
            missing_rects.push_back(box->GetTextDrawRect());
            require(missing_rects.back().GetWidth() > 0 && missing_rects.back().GetHeight() > 0,
                    "original text writer measures the selected alternate glyph through the live font");
            boxes.push_back(box);
        }
        for (size_t i = 0; i < boxes.size(); ++i) {
            auto* box = boxes[i]; box->SetString(L"?", 0, 1);
            const auto rect = box->GetTextDrawRect();
            require(rect.left == missing_rects[i].left && rect.top == missing_rects[i].top &&
                    rect.right == missing_rects[i].right && rect.bottom == missing_rects[i].bottom,
                    "the actual TextBox writer gives fallback and literal '?' identical bounds and advance");
        }
        font.RemoveResource();
        require(font.IsManaging(nullptr) && std::ranges::all_of(boxes, [&font](const auto* box) { return box->mpFont == &font; }),
                "font removal preserves the original borrowed object identity without hidden copied resources");
        require(font.SetResource(resource) && font.SetAlternateChar('?'), "the same live font can reinstall its real resource");
        require(boxes.front()->GetTextDrawRect().GetWidth() == missing_rects.front().GetWidth(),
                "existing actual TextBox font pointers observe reinstalled resources");
        {
            nw4r::ut::ResFont temporary_font;
            require(temporary_font.SetResource(resource) && temporary_font.SetAlternateChar('?'),
                    "the temporary borrowing check requires the real font");
            for (auto* box : boxes) {
                TextBoxRecursiveSetFont(&temporary_font).execute(box);
                require(box->mpFont == &temporary_font && box->GetTextDrawRect().GetWidth() > 0,
                        "actual TextBox borrows temporary font storage while the caller keeps it alive");
                box->SetFont(nullptr);
                require(!box->mpFont, "the caller detaches its borrowed font before destroying it");
            }
        }
#endif
    }

}  // namespace

int main() {
    return smgpc::test::run_stage_resource_process("original-mii-font", [] {
        test_absent_and_malformed_resources_fail_honestly();
        test_original_font_tables();
        const RetailFontFiles files;
        test_retail_mii_font_and_layout_binding(files.fixture);
        std::cout << "Mii font resource and layout binding tests passed: 2/2\n";
    });
}
