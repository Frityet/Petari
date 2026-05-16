#include "assets/AssetServices.hpp"
#include "assets/PackedAsset.hpp"
#include "assets/layout/Bmg.hpp"
#include "assets/layout/Brfnt.hpp"
#include "assets/layout/Brlan.hpp"
#include "assets/layout/Brlyt.hpp"
#include "assets/layout/J3dModel.hpp"
#include "assets/layout/J3dTexture.hpp"
#include "assets/layout/J3dThumbnail.hpp"
#include "assets/layout/RarcArchive.hpp"
#include "assets/layout/Tpl.hpp"
#include "assets/layout/Yaz0.hpp"
#include "game/compat/FileSelectSkyJ3d.hpp"
#include "tests/TestHarness.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    [[nodiscard]] std::vector< std::byte > make_bytes(std::initializer_list< unsigned char > values) {
        std::vector< std::byte > bytes{};
        bytes.reserve(values.size());
        for (const auto value : values) {
            bytes.push_back(static_cast< std::byte >(value));
        }
        return bytes;
    }

    void append_u8(std::vector< std::byte >* bytes, std::uint8_t value) {
        bytes->push_back(static_cast< std::byte >(value));
    }

    void append_u16_be(std::vector< std::byte >* bytes, std::uint16_t value) {
        append_u8(bytes, static_cast< std::uint8_t >((value >> 8U) & 0xFFU));
        append_u8(bytes, static_cast< std::uint8_t >(value & 0xFFU));
    }

    void append_u32_be(std::vector< std::byte >* bytes, std::uint32_t value) {
        append_u8(bytes, static_cast< std::uint8_t >((value >> 24U) & 0xFFU));
        append_u8(bytes, static_cast< std::uint8_t >((value >> 16U) & 0xFFU));
        append_u8(bytes, static_cast< std::uint8_t >((value >> 8U) & 0xFFU));
        append_u8(bytes, static_cast< std::uint8_t >(value & 0xFFU));
    }

    void append_ascii(std::vector< std::byte >* bytes, std::string_view text) {
        for (const char ch : text) {
            append_u8(bytes, static_cast< std::uint8_t >(ch));
        }
    }

    void patch_u32_be(std::vector< std::byte >* bytes, std::size_t offset, std::uint32_t value) {
        (*bytes)[offset + 0U] = static_cast< std::byte >((value >> 24U) & 0xFFU);
        (*bytes)[offset + 1U] = static_cast< std::byte >((value >> 16U) & 0xFFU);
        (*bytes)[offset + 2U] = static_cast< std::byte >((value >> 8U) & 0xFFU);
        (*bytes)[offset + 3U] = static_cast< std::byte >(value & 0xFFU);
    }

    [[nodiscard]] std::vector< std::byte > make_test_brfnt() {
        std::vector< std::byte > bytes{};
        bytes.reserve(256U);

        append_ascii(&bytes, "RFNT");
        append_u16_be(&bytes, 0xFEFFU);
        append_u16_be(&bytes, 0x0104U);
        const auto file_size_offset = bytes.size();
        append_u32_be(&bytes, 0U);
        append_u16_be(&bytes, 0x0010U);
        append_u16_be(&bytes, 4U);

        append_ascii(&bytes, "FINF");
        append_u32_be(&bytes, 0x20U);
        append_u8(&bytes, 1U);
        append_u8(&bytes, 8U);
        append_u16_be(&bytes, 0U);
        append_u8(&bytes, 0U);
        append_u8(&bytes, 8U);
        append_u8(&bytes, 8U);
        append_u8(&bytes, 1U);
        const auto tglp_offset_patch = bytes.size();
        append_u32_be(&bytes, 0U);
        const auto cwdh_offset_patch = bytes.size();
        append_u32_be(&bytes, 0U);
        const auto cmap_offset_patch = bytes.size();
        append_u32_be(&bytes, 0U);
        append_u8(&bytes, 8U);
        append_u8(&bytes, 8U);
        append_u8(&bytes, 7U);
        append_u8(&bytes, 0U);

        append_ascii(&bytes, "TGLP");
        append_u32_be(&bytes, 0xA4U);
        const auto tglp_payload_offset = bytes.size();
        append_u8(&bytes, 8U);
        append_u8(&bytes, 8U);
        append_u8(&bytes, 7U);
        append_u8(&bytes, 8U);
        append_u32_be(&bytes, 128U);
        append_u16_be(&bytes, 1U);
        append_u16_be(&bytes, 0U);
        append_u16_be(&bytes, 2U);
        append_u16_be(&bytes, 1U);
        append_u16_be(&bytes, 32U);
        append_u16_be(&bytes, 8U);
        append_u32_be(&bytes, static_cast< std::uint32_t >(tglp_payload_offset + 28U));
        append_u32_be(&bytes, 0U);
        bytes.insert(bytes.end(), 128U, static_cast< std::byte >(0xFFU));

        append_ascii(&bytes, "CWDH");
        append_u32_be(&bytes, 0x18U);
        const auto cwdh_payload_offset = bytes.size();
        append_u16_be(&bytes, 0U);
        append_u16_be(&bytes, 1U);
        append_u32_be(&bytes, 0U);
        append_u8(&bytes, 0U);
        append_u8(&bytes, 0U);
        append_u8(&bytes, 4U);
        append_u8(&bytes, 1U);
        append_u8(&bytes, 5U);
        append_u8(&bytes, 7U);
        append_u16_be(&bytes, 0U);

        append_ascii(&bytes, "CMAP");
        append_u32_be(&bytes, 0x20U);
        const auto cmap_payload_offset = bytes.size();
        append_u16_be(&bytes, 0x20U);
        append_u16_be(&bytes, 0x41U);
        append_u16_be(&bytes, 2U);
        append_u16_be(&bytes, 0U);
        append_u32_be(&bytes, 0U);
        append_u16_be(&bytes, 2U);
        append_u16_be(&bytes, 0x20U);
        append_u16_be(&bytes, 0U);
        append_u16_be(&bytes, 0x41U);
        append_u16_be(&bytes, 1U);
        append_u16_be(&bytes, 0U);

        patch_u32_be(&bytes, file_size_offset, static_cast< std::uint32_t >(bytes.size()));
        patch_u32_be(&bytes, tglp_offset_patch, static_cast< std::uint32_t >(tglp_payload_offset));
        patch_u32_be(&bytes, cwdh_offset_patch, static_cast< std::uint32_t >(cwdh_payload_offset));
        patch_u32_be(&bytes, cmap_offset_patch, static_cast< std::uint32_t >(cmap_payload_offset));

        return bytes;
    }

    [[nodiscard]] std::vector< std::byte > load_file_bytes(const std::filesystem::path& path) {
        std::ifstream stream(path, std::ios::binary);
        $pc_port_require(stream.is_open());

        const std::vector< char > raw_chars((std::istreambuf_iterator< char >(stream)), std::istreambuf_iterator< char >());
        std::vector< std::byte > bytes{};
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

    std::filesystem::path dev_find_first(const std::filesystem::path& file) {
        return first_existing_path({
            "../orig/RMGK01/files/KrKorean/MessageData/" / file,
            "orig/RMGK01/files/KrKorean/MessageData/" / file,
        });
    }

    [[nodiscard]] std::array< std::uint8_t, 0x14U > make_j3d_tev_stage_raw(std::initializer_list< unsigned int > values) {
        $pc_port_require_eq(values.size(), static_cast< std::size_t >(0x14U));

        std::array< std::uint8_t, 0x14U > raw{};
        std::size_t index = 0U;
        for (const unsigned int value : values) {
            raw[index] = static_cast< std::uint8_t >(value);
            ++index;
        }
        return raw;
    }

    void require_j3d_tev_stage_raw_and_decoded(const smgpc::assets::layout::J3dTevStageInfoRaw& stage, const std::array< std::uint8_t, 0x14U >& raw) {
        $pc_port_require(stage.valid);
        for (std::size_t byte_index = 0U; byte_index < raw.size(); ++byte_index) {
            $pc_port_require_eq(stage.bytes[byte_index], raw[byte_index]);
        }

        $pc_port_require_eq(stage.color_args.a, raw[1U]);
        $pc_port_require_eq(stage.color_args.b, raw[2U]);
        $pc_port_require_eq(stage.color_args.c, raw[3U]);
        $pc_port_require_eq(stage.color_args.d, raw[4U]);
        $pc_port_require_eq(stage.color_op.op, raw[5U]);
        $pc_port_require_eq(stage.color_op.bias, raw[6U]);
        $pc_port_require_eq(stage.color_op.scale, raw[7U]);
        $pc_port_require_eq(stage.color_op.clamp, raw[8U]);
        $pc_port_require_eq(stage.color_op.output_register, raw[9U]);
        $pc_port_require_eq(stage.alpha_args.a, raw[10U]);
        $pc_port_require_eq(stage.alpha_args.b, raw[11U]);
        $pc_port_require_eq(stage.alpha_args.c, raw[12U]);
        $pc_port_require_eq(stage.alpha_args.d, raw[13U]);
        $pc_port_require_eq(stage.alpha_op.op, raw[14U]);
        $pc_port_require_eq(stage.alpha_op.bias, raw[15U]);
        $pc_port_require_eq(stage.alpha_op.scale, raw[16U]);
        $pc_port_require_eq(stage.alpha_op.clamp, raw[17U]);
        $pc_port_require_eq(stage.alpha_op.output_register, raw[18U]);
    }

}  // namespace

$test("PackedAsset unpack returns original payload bytes") {
    smgpc::assets::PackedAssetConverter converter{};
    smgpc::assets::LoadedAsset source{
        .id = smgpc::assets::AssetId{.logical_path = "LayoutData/TitleLogo.arc"},
        .source_path = "/tmp/source.arc",
        .bytes = make_bytes({0x01, 0x23, 0x45, 0x67, 0x89}),
    };

    const auto converted = converter.convert(source);
    $pc_port_require(converted);

    const auto unpacked = smgpc::assets::unpack_packed_asset(converted->bytes);
    $pc_port_require(unpacked);
    $pc_port_require_eq(unpacked->size(), source.bytes.size());
    $pc_port_require(*unpacked == source.bytes);
}

$test("Yaz0 decode expands literal stream") {
    const auto encoded = make_bytes({
        0x59, 0x61, 0x7A, 0x30,  // Yaz0
        0x00, 0x00, 0x00, 0x04,  // decompressed size
        0x00, 0x00, 0x00, 0x00,  // reserved
        0x00, 0x00, 0x00, 0x00,  // reserved
        0xF0,                    // four literals
        0x41, 0x42, 0x43, 0x44,  // A B C D
    });

    const auto decoded = smgpc::assets::layout::decode_yaz0(encoded);
    $pc_port_require(decoded);
    $pc_port_require_eq(decoded->size(), static_cast< std::size_t >(4));
    $pc_port_require((*decoded)[0] == static_cast< std::byte >(0x41));
    $pc_port_require((*decoded)[1] == static_cast< std::byte >(0x42));
    $pc_port_require((*decoded)[2] == static_cast< std::byte >(0x43));
    $pc_port_require((*decoded)[3] == static_cast< std::byte >(0x44));
}

$test("GX CMPR texture decode expands 8x8 compressed blocks") {
    std::vector< std::byte > encoded{};
    encoded.reserve(32U);
    for (int subblock = 0; subblock < 4; ++subblock) {
        const auto bytes = make_bytes({
            0xF8,
            0x00,  // RGB565 red
            0x00,
            0x1F,  // RGB565 blue
            0x00,
            0x00,
            0x00,
            0x00,  // all selectors use color 0
        });
        encoded.insert(encoded.end(), bytes.begin(), bytes.end());
    }

    const auto decoded = smgpc::assets::layout::tpl::decode_gx_tiled_texture(encoded, 8U, 8U, 14U);
    $pc_port_require(decoded);
    $pc_port_require_eq(decoded->width, static_cast< std::uint16_t >(8U));
    $pc_port_require_eq(decoded->height, static_cast< std::uint16_t >(8U));
    $pc_port_require_eq(decoded->rgba8[0U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(decoded->rgba8[1U], static_cast< std::uint8_t >(0U));
    $pc_port_require_eq(decoded->rgba8[2U], static_cast< std::uint8_t >(0U));
    $pc_port_require_eq(decoded->rgba8[3U], static_cast< std::uint8_t >(255U));
}

$test("GX IA4 texture decode reads high nibble alpha and low nibble intensity") {
    std::vector< std::byte > encoded(32U, static_cast< std::byte >(0x00U));
    encoded[0U] = static_cast< std::byte >(0xF1U);

    const auto decoded = smgpc::assets::layout::tpl::decode_gx_tiled_texture(encoded, 8U, 4U, 2U);
    $pc_port_require(decoded);
    $pc_port_require_eq(decoded->rgba8[0U], static_cast< std::uint8_t >(17U));
    $pc_port_require_eq(decoded->rgba8[1U], static_cast< std::uint8_t >(17U));
    $pc_port_require_eq(decoded->rgba8[2U], static_cast< std::uint8_t >(17U));
    $pc_port_require_eq(decoded->rgba8[3U], static_cast< std::uint8_t >(255U));
}

$test("GX IA8 texture decode reads intensity before alpha") {
    std::vector< std::byte > encoded(32U, static_cast< std::byte >(0x00U));
    encoded[0U] = static_cast< std::byte >(0x40U);
    encoded[1U] = static_cast< std::byte >(0xC0U);

    const auto decoded = smgpc::assets::layout::tpl::decode_gx_tiled_texture(encoded, 4U, 4U, 3U);
    $pc_port_require(decoded);
    $pc_port_require_eq(decoded->rgba8[0U], static_cast< std::uint8_t >(0x40U));
    $pc_port_require_eq(decoded->rgba8[1U], static_cast< std::uint8_t >(0x40U));
    $pc_port_require_eq(decoded->rgba8[2U], static_cast< std::uint8_t >(0x40U));
    $pc_port_require_eq(decoded->rgba8[3U], static_cast< std::uint8_t >(0xC0U));
}

$test("BRFNT compiler round-trips parsed font bytes") {
    const auto bytes = make_test_brfnt();
    const auto parsed = smgpc::assets::layout::parse_brfnt(bytes, "TestFont");
    $pc_port_require(parsed);

    const auto recompiled = smgpc::assets::layout::compile_brfnt(*parsed);
    $pc_port_require(recompiled);
    $pc_port_require(*recompiled == bytes);
}

$test("Korean game BRFNTs recompile byte-for-byte when font cache is available") {
    const auto font_dir = first_existing_path({
        "pc-port/.cache/assets/RMGK01/KrKorean/Font",
        ".cache/assets/RMGK01/KrKorean/Font",
    });
    if (font_dir.empty()) {
        return;
    }

    std::size_t checked_count = 0U;
    for (const auto& entry : std::filesystem::directory_iterator(font_dir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".brfnt") {
            continue;
        }

        const auto bytes = load_file_bytes(entry.path());
        const auto parsed = smgpc::assets::layout::parse_brfnt(bytes, entry.path().stem().string());
        $pc_port_require(parsed);

        const auto recompiled = smgpc::assets::layout::compile_brfnt(*parsed);
        $pc_port_require(recompiled);
        $pc_port_require(*recompiled == bytes);
        ++checked_count;
    }

    $pc_port_require(checked_count > 0U);
}

$test("Korean message BRFNT maps Hangul glyphs when game font cache is available") {
    const auto font_path = first_existing_path({
        "pc-port/.cache/assets/RMGK01/KrKorean/Font/messagefont26.brfnt",
        ".cache/assets/RMGK01/KrKorean/Font/messagefont26.brfnt",
    });
    if (font_path.empty()) {
        return;
    }

    const auto bytes = load_file_bytes(font_path);

    const auto parsed = smgpc::assets::layout::parse_brfnt(bytes, "messagefont26");
    $pc_port_require(parsed);
    $pc_port_require(parsed->font_width() > 0U);
    $pc_port_require(parsed->font_height() > 0U);
    $pc_port_require(parsed->baseline_position() > 0U);
    $pc_port_require(parsed->has_codepoint(static_cast< std::uint16_t >(0xD558U)));
    $pc_port_require(parsed->has_codepoint(static_cast< std::uint16_t >(0xE000U)));
    $pc_port_require(parsed->has_codepoint(static_cast< std::uint16_t >(0xE001U)));
    $pc_port_require(parsed->has_codepoint(static_cast< std::uint16_t >(0xE00CU)));
    $pc_port_require(parsed->has_codepoint(static_cast< std::uint16_t >(0xE00DU)));
    $pc_port_require(parsed->has_codepoint(static_cast< std::uint16_t >(0xE016U)));

    smgpc::assets::layout::BrfntGlyph glyph{};
    $pc_port_require(parsed->get_glyph(static_cast< std::uint16_t >(0xD558U), &glyph));
    $pc_port_require(parsed->get_glyph(static_cast< std::uint16_t >(0xE00DU), &glyph));
}

$test("Korean Message.arc maps PrologueDemo BMG text") {
    const auto message_path = first_existing_path({
        "../orig/RMGK01/files/KrKorean/MessageData/Message.arc",
        "orig/RMGK01/files/KrKorean/MessageData/Message.arc",
    });
    if (message_path.empty()) {
        return;
    }

    auto bytes = load_file_bytes(message_path);
    if (smgpc::assets::layout::is_yaz0(bytes)) {
        auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
        $pc_port_require(decoded);
        bytes = std::move(*decoded);
    }

    const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
    $pc_port_require(archive);

    const auto bmg_bytes = archive->find_entry("message.bmg");
    const auto table_bytes = archive->find_entry("messageid.tbl");
    $pc_port_require(not bmg_bytes.empty());
    $pc_port_require(not table_bytes.empty());

    const auto messages = smgpc::assets::layout::parse_bmg_messages(bmg_bytes, table_bytes);
    $pc_port_require(messages);

    const auto found = messages->find("Layout_PrologueDemoPro01");
    $pc_port_require(found != messages->end());
    $pc_port_require(found->second ==
                     u"\uADF8\uD574\u0020\uBC84\uC12F\uC655\uAD6D\uC5D0\uB294\u000A\uD558\uB298\uC774\u0020\uC548\u0020\uBCF4\uC77C\u0020\uC815\uB3C4"
                     u"\uB85C\u0020\uCEE4\uB2E4\uB780\u000A\u300C\uBCC4\uB625\uBCC4\u300D\uC774\u0020\uB5A8\uC5B4\uC84C\uC2B5\uB2C8\uB2E4\u002E");
}

$test("Korean Message.arc preserves BMG picture font command escapes") {
    const auto message_path = first_existing_path({
        "../orig/RMGK01/files/KrKorean/MessageData/Message.arc",
        "orig/RMGK01/files/KrKorean/MessageData/Message.arc",
    });
    if (message_path.empty()) {
        return;
    }

    auto bytes = load_file_bytes(message_path);
    if (smgpc::assets::layout::is_yaz0(bytes)) {
        auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
        $pc_port_require(decoded);
        bytes = std::move(*decoded);
    }

    const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
    $pc_port_require(archive);

    const auto bmg_bytes = archive->find_entry("message.bmg");
    const auto table_bytes = archive->find_entry("messageid.tbl");
    $pc_port_require(not bmg_bytes.empty());
    $pc_port_require(not table_bytes.empty());

    const auto messages = smgpc::assets::layout::parse_bmg_messages(bmg_bytes, table_bytes);
    $pc_port_require(messages);

    const auto found = messages->find("2PGuidance001");
    $pc_port_require(found != messages->end());
    for (const char16_t code_unit : found->second) {
        $pc_port_require(code_unit != u'\u001A');
    }
    $pc_port_require(found->second.find(smgpc::assets::layout::make_bmg_picture_font_tag(0x0000U)) != std::u16string::npos);
    $pc_port_require(found->second.find(smgpc::assets::layout::make_bmg_picture_font_tag(0x001DU)) != std::u16string::npos);

    const auto star_bit_message = messages->find("2PGuidance002");
    $pc_port_require(star_bit_message != messages->end());
    $pc_port_require(star_bit_message->second.find(smgpc::assets::layout::make_bmg_picture_font_tag(0x0001U)) != std::u16string::npos);
    $pc_port_require(star_bit_message->second.find(smgpc::assets::layout::make_bmg_picture_font_tag(0x000BU)) != std::u16string::npos);

    const auto coin_message = messages->find("2PGuidance003");
    $pc_port_require(coin_message != messages->end());
    $pc_port_require(coin_message->second.find(smgpc::assets::layout::make_bmg_picture_font_tag(0x0011U)) != std::u16string::npos);

    const auto copy_message = messages->find("System_FileSelect016");
    $pc_port_require(copy_message != messages->end());
    $pc_port_require(copy_message->second.find(u"{0}") != std::u16string::npos);
    $pc_port_require(copy_message->second.find(u"{1}") != std::u16string::npos);
}

$test("Korean PictureFont maps BMG picture tag codepoints when game font cache is available") {
    const auto font_path = first_existing_path({
        "pc-port/.cache/assets/RMGK01/KrKorean/Font/picturefont.brfnt",
        ".cache/assets/RMGK01/KrKorean/Font/picturefont.brfnt",
    });
    if (font_path.empty()) {
        return;
    }

    const auto bytes = load_file_bytes(font_path);

    const auto parsed = smgpc::assets::layout::parse_brfnt(bytes, "picturefont");
    $pc_port_require(parsed);
    $pc_port_require(parsed->has_codepoint(smgpc::assets::layout::bmg_picture_font_codepoint(0x0000U)));
    $pc_port_require(parsed->has_codepoint(smgpc::assets::layout::bmg_picture_font_codepoint(0x0001U)));
    $pc_port_require(parsed->has_codepoint(smgpc::assets::layout::bmg_picture_font_codepoint(0x000AU)));
    $pc_port_require(parsed->has_codepoint(smgpc::assets::layout::bmg_picture_font_codepoint(0x000BU)));
    $pc_port_require(parsed->has_codepoint(smgpc::assets::layout::bmg_picture_font_codepoint(0x0011U)));
    $pc_port_require(parsed->has_codepoint(smgpc::assets::layout::bmg_picture_font_codepoint(0x001DU)));
}

$test("MapGalaxyBg BRLYT parses shorter wnd1 blocks") {
    const auto archive_path = first_existing_path({
        "../orig/RMGK01/files/LayoutData/MapGalaxyBg.arc",
        "orig/RMGK01/files/LayoutData/MapGalaxyBg.arc",
    });
    if (archive_path.empty()) {
        return;
    }

    auto bytes = load_file_bytes(archive_path);
    if (smgpc::assets::layout::is_yaz0(bytes)) {
        auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
        $pc_port_require(decoded);
        bytes = std::move(*decoded);
    }

    const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
    $pc_port_require(archive);

    const auto brlyt_bytes = archive->find_entry("blyt/mapgalaxybg.brlyt");
    $pc_port_require(!brlyt_bytes.empty());

    const auto layout = smgpc::assets::layout::parse_brlyt(brlyt_bytes);
    $pc_port_require(layout);
    $pc_port_require(!layout->panes.empty());
}

$test("SysInfoWindow and MiiSelect BRLYT parse wnd1 content and frame tables") {
    const auto sys_info_path = first_existing_path({
        "../orig/RMGK01/files/LayoutData/SysInfoWindow.arc",
        "orig/RMGK01/files/LayoutData/SysInfoWindow.arc",
    });
    const auto mii_select_path = first_existing_path({
        "../orig/RMGK01/files/LayoutData/MiiSelect.arc",
        "orig/RMGK01/files/LayoutData/MiiSelect.arc",
    });
    if (sys_info_path.empty() || mii_select_path.empty()) {
        return;
    }

    for (const auto& archive_path : {sys_info_path, mii_select_path}) {
        auto bytes = load_file_bytes(archive_path);
        if (smgpc::assets::layout::is_yaz0(bytes)) {
            auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
            $pc_port_require(decoded);
            bytes = std::move(*decoded);
        }

        const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
        $pc_port_require(archive);

        std::size_t window_count = 0U;
        for (const auto& entry : archive->entries()) {
            if (!entry.path.ends_with(".brlyt")) {
                continue;
            }

            const auto brlyt_bytes = archive->find_entry(entry.path);
            $pc_port_require(!brlyt_bytes.empty());
            const auto layout = smgpc::assets::layout::parse_brlyt(brlyt_bytes);
            $pc_port_require(layout);

            for (const auto& pane : layout->panes) {
                if (pane.type != smgpc::assets::layout::PaneType::Window) {
                    continue;
                }

                ++window_count;
                $pc_port_require(pane.material_index >= 0);
                $pc_port_require(!pane.window_frames.empty());
                $pc_port_require(pane.window_frames.front().material_index >= 0);
                $pc_port_require_eq(pane.window_frame_material_index, pane.window_frames.front().material_index);
            }
        }

        $pc_port_require(window_count > 0U);
    }
}

$test("MiiIcon character BRLAN preserves named texture pattern table") {
    const auto archive_path = first_existing_path({
        "../orig/RMGK01/files/LayoutData/MiiIcon.arc",
        "orig/RMGK01/files/LayoutData/MiiIcon.arc",
    });
    if (archive_path.empty()) {
        return;
    }

    auto bytes = load_file_bytes(archive_path);
    if (smgpc::assets::layout::is_yaz0(bytes)) {
        auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
        $pc_port_require(decoded);
        bytes = std::move(*decoded);
    }

    const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
    $pc_port_require(archive);

    const auto brlan_bytes = archive->find_entry("anim/character.brlan");
    $pc_port_require(!brlan_bytes.empty());

    const auto animation = smgpc::assets::layout::parse_brlan(brlan_bytes, "Character");
    $pc_port_require(animation);
    $pc_port_require_eq(animation->texture_names.size(), static_cast< std::size_t >(6U));
    $pc_port_require_eq(animation->texture_names[0U], std::string("MyMiiKinopio.tpl"));
    $pc_port_require_eq(animation->texture_names[2U], std::string("MyMiiMario.tpl"));
    $pc_port_require_eq(animation->texture_names[5U], std::string("MyMiiYoshi.tpl"));
}

$test("TitleLogo wait BRLAN scrolls PicLogoGalaxy at original rate") {
    const auto archive_path = first_existing_path({
        "../orig/RMGK01/files/KrKorean/LayoutData/TitleLogo.arc",
        "orig/RMGK01/files/KrKorean/LayoutData/TitleLogo.arc",
    });
    if (archive_path.empty()) {
        return;
    }

    auto bytes = load_file_bytes(archive_path);
    if (smgpc::assets::layout::is_yaz0(bytes)) {
        auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
        $pc_port_require(decoded);
        bytes = std::move(*decoded);
    }

    const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
    $pc_port_require(archive);

    const auto brlan_bytes = archive->find_entry("anim/wait.brlan");
    $pc_port_require(!brlan_bytes.empty());

    const auto animation = smgpc::assets::layout::parse_brlan(brlan_bytes, "Wait");
    $pc_port_require(animation);
    $pc_port_require_eq(animation->frame_size, static_cast< std::uint16_t >(10000U));

    const auto found = std::find_if(animation->tracks.begin(), animation->tracks.end(), [](const smgpc::assets::layout::BrlanTrack& track) {
        return track.pane_name == "PicLogoGalaxy" && track.kind == "RLTS" && track.target == 0U;
    });
    $pc_port_require(found != animation->tracks.end());
    $pc_port_require_eq(found->keys.size(), static_cast< std::size_t >(2U));
    $pc_port_require(std::fabs(found->keys[0U].value - 0.0F) < 0.0001F);
    $pc_port_require(std::fabs(found->keys[1U].frame - 10000.0F) < 0.0001F);
    $pc_port_require(std::fabs(found->keys[1U].value - 1.0F) < 0.0001F);
}

$test("TitleLogo appear BRLAN does not scroll PicLogoGalaxy before Wait") {
    const auto archive_path = first_existing_path({
        "../orig/RMGK01/files/KrKorean/LayoutData/TitleLogo.arc",
        "orig/RMGK01/files/KrKorean/LayoutData/TitleLogo.arc",
    });
    if (archive_path.empty()) {
        return;
    }

    auto bytes = load_file_bytes(archive_path);
    if (smgpc::assets::layout::is_yaz0(bytes)) {
        auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
        $pc_port_require(decoded);
        bytes = std::move(*decoded);
    }

    const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
    $pc_port_require(archive);

    const auto brlan_bytes = archive->find_entry("anim/appear.brlan");
    $pc_port_require(!brlan_bytes.empty());

    const auto animation = smgpc::assets::layout::parse_brlan(brlan_bytes, "Appear");
    $pc_port_require(animation);
    $pc_port_require_eq(animation->frame_size, static_cast< std::uint16_t >(201U));

    const auto found = std::find_if(animation->tracks.begin(), animation->tracks.end(), [](const smgpc::assets::layout::BrlanTrack& track) {
        return track.pane_name == "PicLogoGalaxy" && track.kind == "RLTS";
    });
    $pc_port_require(found == animation->tracks.end());
}

$test("P2Manual BRLYT preserves MAT1 TEV alpha blend data") {
    const auto archive_path = first_existing_path({
        "../orig/RMGK01/files/LayoutData/P2Manual.arc",
        "orig/RMGK01/files/LayoutData/P2Manual.arc",
    });
    if (archive_path.empty()) {
        return;
    }

    auto bytes = load_file_bytes(archive_path);
    if (smgpc::assets::layout::is_yaz0(bytes)) {
        auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
        $pc_port_require(decoded);
        bytes = std::move(*decoded);
    }

    const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
    $pc_port_require(archive);

    const auto brlyt_bytes = archive->find_entry("blyt/p2manual.brlyt");
    $pc_port_require(!brlyt_bytes.empty());

    const auto layout = smgpc::assets::layout::parse_brlyt(brlyt_bytes);
    $pc_port_require(layout);

    const auto find_material = [&](std::string_view name) -> const smgpc::assets::layout::MaterialDefinition* {
        const auto found =
            std::find_if(layout->materials.begin(), layout->materials.end(), [&](const auto& material) { return material.name == name; });
        return found == layout->materials.end() ? nullptr : &*found;
    };

    const auto* left_button = find_material("PicLButton");
    $pc_port_require(left_button != nullptr);
    $pc_port_require_eq(left_button->texture_index, 1);
    $pc_port_require_eq(left_button->texture_color[0U], static_cast< std::uint8_t >(0U));
    $pc_port_require_eq(left_button->texture_color[1U], static_cast< std::uint8_t >(160U));
    $pc_port_require_eq(left_button->texture_color[2U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(left_button->texture_color[3U], static_cast< std::uint8_t >(0U));
    $pc_port_require_eq(left_button->font_color[0U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(left_button->font_color[1U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(left_button->font_color[2U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(left_button->font_color[3U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(left_button->tev_stage_count, 2);
    $pc_port_require_eq(left_button->tev_stages.size(), static_cast< std::size_t >(2U));
    $pc_port_require(left_button->has_alpha_compare);
    $pc_port_require_eq(left_button->alpha_compare[0U], static_cast< std::uint8_t >(0x77U));
    $pc_port_require_eq(left_button->alpha_compare[1U], static_cast< std::uint8_t >(0x00U));
    $pc_port_require(left_button->blend.enabled);
    $pc_port_require_eq(left_button->blend.type, static_cast< std::uint8_t >(1U));
    $pc_port_require_eq(left_button->blend.source_factor, static_cast< std::uint8_t >(4U));
    $pc_port_require_eq(left_button->blend.destination_factor, static_cast< std::uint8_t >(5U));
    $pc_port_require_eq(left_button->blend.operation, static_cast< std::uint8_t >(3U));
    $pc_port_require(left_button->has_tev_swap_mode);
    $pc_port_require_eq(left_button->tev_swap_mode[0U], static_cast< std::uint8_t >(0xE4U));
    $pc_port_require_eq(left_button->tev_stages[0U].raw[0U], static_cast< std::uint8_t >(0x00U));
    $pc_port_require_eq(left_button->tev_stages[0U].raw[4U], static_cast< std::uint8_t >(0x42U));
    $pc_port_require_eq(left_button->tev_stages[1U].raw[0U], static_cast< std::uint8_t >(0xFFU));
    $pc_port_require_eq(left_button->tev_stages[1U].raw[8U], static_cast< std::uint8_t >(0x07U));

    const auto* win_info_frame = find_material("WinInfoLT");
    $pc_port_require(win_info_frame != nullptr);
    $pc_port_require_eq(win_info_frame->texture_index, 6);
    $pc_port_require_eq(layout->texture_names[static_cast< std::size_t >(win_info_frame->texture_index)], std::string("MySysInfoWindow.tpl"));
    $pc_port_require_eq(win_info_frame->texture_color[0U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(win_info_frame->texture_color[1U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(win_info_frame->texture_color[2U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(win_info_frame->texture_color[3U], static_cast< std::uint8_t >(0U));
    $pc_port_require_eq(win_info_frame->font_color[0U], static_cast< std::uint8_t >(200U));
    $pc_port_require_eq(win_info_frame->font_color[1U], static_cast< std::uint8_t >(220U));
    $pc_port_require_eq(win_info_frame->font_color[2U], static_cast< std::uint8_t >(245U));
    $pc_port_require_eq(win_info_frame->font_color[3U], static_cast< std::uint8_t >(255U));
    $pc_port_require(!win_info_frame->has_alpha_compare);
    $pc_port_require(!win_info_frame->blend.enabled);
}

$test("FileSelectDataPlanet BDL exposes real TEX1 texture data") {
    const auto archive_path = first_existing_path({
        "../orig/RMGK01/files/ObjectData/FileSelectDataPlanet.arc",
        "orig/RMGK01/files/ObjectData/FileSelectDataPlanet.arc",
    });
    if (archive_path.empty()) {
        return;
    }

    auto bytes = load_file_bytes(archive_path);
    if (smgpc::assets::layout::is_yaz0(bytes)) {
        auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
        $pc_port_require(decoded);
        bytes = std::move(*decoded);
    }

    const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
    $pc_port_require(archive);

    const auto bdl_bytes = archive->find_entry("fileselectdataplanet.bdl");
    $pc_port_require(not bdl_bytes.empty());

    const auto textures = smgpc::assets::layout::parse_j3d_tex1_textures(bdl_bytes);
    $pc_port_require(textures);
    $pc_port_require(!textures->empty());
    $pc_port_require_eq(textures->front().name, std::string("grnd03L2"));
    $pc_port_require_eq(textures->front().image.width, static_cast< std::uint16_t >(256U));
    $pc_port_require_eq(textures->front().image.height, static_cast< std::uint16_t >(256U));
    $pc_port_require(!textures->front().image.rgba8.empty());
}

$test("FileSelectDataPlanet BDL renders a visible thumbnail") {
    const auto archive_path = first_existing_path({
        "../orig/RMGK01/files/ObjectData/FileSelectDataPlanet.arc",
        "orig/RMGK01/files/ObjectData/FileSelectDataPlanet.arc",
    });
    if (archive_path.empty()) {
        return;
    }

    auto bytes = load_file_bytes(archive_path);
    if (smgpc::assets::layout::is_yaz0(bytes)) {
        auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
        $pc_port_require(decoded);
        bytes = std::move(*decoded);
    }

    const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
    $pc_port_require(archive);

    const auto bdl_bytes = archive->find_entry("fileselectdataplanet.bdl");
    $pc_port_require(not bdl_bytes.empty());

    const auto thumbnail = smgpc::assets::layout::render_j3d_thumbnail(
        bdl_bytes, smgpc::assets::layout::J3dThumbnailOptions{.width = 96U, .height = 96U, .pitch_degrees = -8.0F});
    if (!thumbnail) {
        throw std::runtime_error(thumbnail.failure().message);
    }
    $pc_port_require_eq(thumbnail->width, static_cast< std::uint16_t >(96U));
    $pc_port_require_eq(thumbnail->height, static_cast< std::uint16_t >(96U));

    std::size_t covered_pixels = 0U;
    std::size_t colored_pixels = 0U;
    for (std::size_t index = 0U; index + 3U < thumbnail->rgba8.size(); index += 4U) {
        if (thumbnail->rgba8[index + 3U] == 0U) {
            continue;
        }

        ++covered_pixels;
        if (thumbnail->rgba8[index + 0U] != 0U || thumbnail->rgba8[index + 1U] != 0U || thumbnail->rgba8[index + 2U] != 0U) {
            ++colored_pixels;
        }
    }

    $pc_port_require(covered_pixels > 1800U);
    $pc_port_require(colored_pixels > 1800U);
}

$test("FileSelectData fellow BDLs render visible thumbnails") {
    struct FellowModelFixture {
        std::string_view archive_name;
        std::string_view model_entry;
    };
    constexpr std::array< FellowModelFixture, 5 > fixtures{{
        {.archive_name = "FileSelectDataMario", .model_entry = "fileselectdatamario.bdl"},
        {.archive_name = "FileSelectDataLuigi", .model_entry = "fileselectdataluigi.bdl"},
        {.archive_name = "FileSelectDataYoshi", .model_entry = "fileselectdatayoshi.bdl"},
        {.archive_name = "FileSelectDataKinopio", .model_entry = "fileselectdatakinopio.bdl"},
        {.archive_name = "FileSelectDataPeach", .model_entry = "fileselectdatapeach.bdl"},
    }};

    for (const auto& fixture : fixtures) {
        const auto archive_path = first_existing_path({
            std::filesystem::path("../orig/RMGK01/files/ObjectData") / (std::string(fixture.archive_name) + ".arc"),
            std::filesystem::path("orig/RMGK01/files/ObjectData") / (std::string(fixture.archive_name) + ".arc"),
        });
        if (archive_path.empty()) {
            return;
        }

        auto bytes = load_file_bytes(archive_path);
        if (smgpc::assets::layout::is_yaz0(bytes)) {
            auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
            $pc_port_require(decoded);
            bytes = std::move(*decoded);
        }

        const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
        $pc_port_require(archive);

        const auto bdl_bytes = archive->find_entry(fixture.model_entry);
        $pc_port_require(not bdl_bytes.empty());

        const auto thumbnail = smgpc::assets::layout::render_j3d_thumbnail(
            bdl_bytes, smgpc::assets::layout::J3dThumbnailOptions{
                           .width = 128U, .height = 128U, .pitch_degrees = -8.0F, .margin = 0.92F, .ambient_light = 0.80F, .diffuse_light = 0.20F});
        if (!thumbnail) {
            throw std::runtime_error(thumbnail.failure().message);
        }
        $pc_port_require_eq(thumbnail->width, static_cast< std::uint16_t >(128U));
        $pc_port_require_eq(thumbnail->height, static_cast< std::uint16_t >(128U));

        std::size_t covered_pixels = 0U;
        std::size_t colored_pixels = 0U;
        for (std::size_t index = 0U; index + 3U < thumbnail->rgba8.size(); index += 4U) {
            if (thumbnail->rgba8[index + 3U] == 0U) {
                continue;
            }

            ++covered_pixels;
            if (thumbnail->rgba8[index + 0U] != 0U || thumbnail->rgba8[index + 1U] != 0U || thumbnail->rgba8[index + 2U] != 0U) {
                ++colored_pixels;
            }
        }

        $pc_port_require(covered_pixels > 3000U);
        $pc_port_require(colored_pixels > 2500U);
    }
}

$test("CometNearOrbitSky BDL renders a visible thumbnail with F32 positions") {
    const auto archive_path = first_existing_path({
        "../orig/RMGK01/files/ObjectData/CometNearOrbitSky.arc",
        "orig/RMGK01/files/ObjectData/CometNearOrbitSky.arc",
    });
    if (archive_path.empty()) {
        return;
    }

    auto bytes = load_file_bytes(archive_path);
    if (smgpc::assets::layout::is_yaz0(bytes)) {
        auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
        $pc_port_require(decoded);
        bytes = std::move(*decoded);
    }

    const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
    $pc_port_require(archive);

    const auto bdl_bytes = archive->find_entry("cometnearorbitsky.bdl");
    $pc_port_require(not bdl_bytes.empty());

    const auto thumbnail = smgpc::assets::layout::render_j3d_thumbnail(
        bdl_bytes, smgpc::assets::layout::J3dThumbnailOptions{.width = 128U, .height = 96U, .pitch_degrees = -10.0F, .margin = 0.98F});
    if (!thumbnail) {
        throw std::runtime_error(thumbnail.failure().message);
    }
    $pc_port_require_eq(thumbnail->width, static_cast< std::uint16_t >(128U));
    $pc_port_require_eq(thumbnail->height, static_cast< std::uint16_t >(96U));

    std::size_t covered_pixels = 0U;
    std::size_t colored_pixels = 0U;
    for (std::size_t index = 0U; index + 3U < thumbnail->rgba8.size(); index += 4U) {
        if (thumbnail->rgba8[index + 3U] == 0U) {
            continue;
        }

        ++covered_pixels;
        if (thumbnail->rgba8[index + 0U] != 0U || thumbnail->rgba8[index + 1U] != 0U || thumbnail->rgba8[index + 2U] != 0U) {
            ++colored_pixels;
        }
    }

    $pc_port_require(covered_pixels > 2000U);
    $pc_port_require(colored_pixels > 2000U);
}

$test("CometNearOrbitSky BDL exposes reusable J3D model data") {
    const auto archive_path = first_existing_path({
        "../orig/RMGK01/files/ObjectData/CometNearOrbitSky.arc",
        "orig/RMGK01/files/ObjectData/CometNearOrbitSky.arc",
    });
    if (archive_path.empty()) {
        return;
    }

    auto bytes = load_file_bytes(archive_path);
    if (smgpc::assets::layout::is_yaz0(bytes)) {
        auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
        $pc_port_require(decoded);
        bytes = std::move(*decoded);
    }

    const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
    $pc_port_require(archive);

    const auto bdl_bytes = archive->find_entry("cometnearorbitsky.bdl");
    $pc_port_require(not bdl_bytes.empty());

    const auto model = smgpc::assets::layout::parse_j3d_model(bdl_bytes);
    if (!model) {
        throw std::runtime_error(model.failure().message);
    }

    $pc_port_require_eq(model->shapes.size(), static_cast< std::size_t >(9U));
    $pc_port_require_eq(model->materials.size(), static_cast< std::size_t >(9U));
    $pc_port_require_eq(model->textures.size(), static_cast< std::size_t >(12U));
    $pc_port_require_eq(model->joints.size(), static_cast< std::size_t >(8U));
    $pc_port_require_eq(model->draw_matrices.size(), static_cast< std::size_t >(5U));

    std::size_t triangle_count = 0U;
    for (const auto& shape : model->shapes) {
        triangle_count += shape.triangles.size();
        $pc_port_require(!shape.matrix_groups.empty());
        $pc_port_require(!shape.triangles.empty());
    }
    $pc_port_require(triangle_count > 0U);

    $pc_port_require_eq(model->materials[0U].name, std::string("ACometHalo_v"));
    $pc_port_require_eq(model->materials[0U].texture_indices[0U], static_cast< std::uint16_t >(1U));
    $pc_port_require(model->materials[0U].texture_coord_generators[0U].valid);
    $pc_port_require_eq(model->materials[0U].texture_coord_generators[0U].source, static_cast< std::uint8_t >(4U));
    $pc_port_require_eq(model->materials[0U].texture_coord_generators[0U].matrix, static_cast< std::uint8_t >(30U));
    $pc_port_require(model->materials[0U].texture_matrices[0U].valid);
    $pc_port_require_eq(model->materials[0U].texture_matrices[0U].srt.rotation, static_cast< std::int16_t >(2767));
    $pc_port_require(std::fabs(model->materials[0U].texture_matrices[0U].srt.scale_x - 4.0F) < 0.001F);
    $pc_port_require(std::fabs(model->materials[0U].texture_matrices[0U].srt.scale_y + 3.0F) < 0.001F);
    $pc_port_require(model->materials[0U].blend.valid);
    $pc_port_require_eq(model->materials[0U].blend.type, static_cast< std::uint8_t >(1U));
    $pc_port_require_eq(model->materials[0U].blend.source_factor, static_cast< std::uint8_t >(1U));
    $pc_port_require_eq(model->materials[0U].blend.destination_factor, static_cast< std::uint8_t >(1U));
    $pc_port_require_eq(model->materials[2U].name, std::string("CometHalo_v"));
    $pc_port_require_eq(model->materials[2U].indirect_texture_stage_count, static_cast< std::uint8_t >(1U));
    $pc_port_require(model->materials[2U].indirect_texture_orders[0U].valid);
    $pc_port_require_eq(model->materials[2U].indirect_texture_orders[0U].texture_coordinate, static_cast< std::uint8_t >(1U));
    $pc_port_require_eq(model->materials[2U].indirect_texture_orders[0U].texture_map, static_cast< std::uint8_t >(1U));
    $pc_port_require(model->materials[2U].indirect_texture_matrices[0U].valid);
    $pc_port_require(std::fabs(model->materials[2U].indirect_texture_matrices[0U].values[0U] - 0.5F) < 0.0001F);
    $pc_port_require(std::fabs(model->materials[2U].indirect_texture_matrices[0U].values[4U] - 0.5F) < 0.0001F);
    $pc_port_require_eq(model->materials[2U].indirect_texture_matrices[0U].scale_exponent, static_cast< std::int8_t >(-2));
    $pc_port_require(model->materials[2U].indirect_texture_coord_scales[0U].valid);
    $pc_port_require_eq(model->materials[2U].indirect_texture_coord_scales[0U].scale_s, static_cast< std::uint8_t >(0U));
    $pc_port_require_eq(model->materials[2U].indirect_texture_coord_scales[0U].scale_t, static_cast< std::uint8_t >(0U));
    $pc_port_require(model->materials[2U].indirect_tev_stages[0U].valid);
    $pc_port_require_eq(model->materials[2U].indirect_tev_stages[0U].format, static_cast< std::uint8_t >(0U));
    $pc_port_require_eq(model->materials[2U].indirect_tev_stages[0U].bias, static_cast< std::uint8_t >(7U));
    $pc_port_require_eq(model->materials[2U].indirect_tev_stages[0U].matrix, static_cast< std::uint8_t >(1U));
    $pc_port_require(model->materials[6U].blend.valid);
    $pc_port_require_eq(model->materials[6U].blend.type, static_cast< std::uint8_t >(0U));
    $pc_port_require_eq(model->materials[7U].indirect_texture_stage_count, static_cast< std::uint8_t >(0U));
    $pc_port_require_eq(model->materials[7U].tev_stage_count, static_cast< std::uint8_t >(3U));
    $pc_port_require(model->materials[7U].tev_orders[0U].valid);
    $pc_port_require_eq(model->materials[7U].tev_orders[0U].texture_coordinate, static_cast< std::uint8_t >(1U));
    $pc_port_require_eq(model->materials[7U].tev_orders[0U].texture_map, static_cast< std::uint8_t >(1U));
    $pc_port_require(model->materials[7U].z_mode.valid);
    $pc_port_require_eq(model->materials[7U].z_mode.update_enable, static_cast< std::uint8_t >(0U));
    $pc_port_require(model->materials[7U].tev_k_colors[0U].valid);
    $pc_port_require_eq(model->materials[7U].tev_k_colors[0U].g, static_cast< std::uint8_t >(0x39U));
    $pc_port_require_eq(model->materials[7U].tev_k_colors[0U].b, static_cast< std::uint8_t >(0x58U));
    $pc_port_require(model->materials[7U].tev_stages[0U].valid);
    $pc_port_require_eq(model->materials[7U].tev_stages[0U].bytes[1U], static_cast< std::uint8_t >(0x0FU));
    $pc_port_require_eq(model->materials[7U].tev_stages[0U].bytes[2U], static_cast< std::uint8_t >(0x08U));
    $pc_port_require_eq(model->materials[7U].tev_stages[0U].bytes[3U], static_cast< std::uint8_t >(0x0AU));
    $pc_port_require(model->materials[7U].tev_stages[1U].valid);
    $pc_port_require_eq(model->materials[7U].tev_stages[1U].bytes[2U], static_cast< std::uint8_t >(0x0AU));
    $pc_port_require_eq(model->materials[7U].tev_stages[1U].bytes[3U], static_cast< std::uint8_t >(0x08U));
    $pc_port_require(model->materials[7U].tev_swap_modes[1U].valid);
    $pc_port_require_eq(model->materials[7U].tev_swap_modes[1U].raw[0U], static_cast< std::uint8_t >(0x01U));
    $pc_port_require_eq(model->materials[7U].tev_swap_modes[1U].raw[1U], static_cast< std::uint8_t >(0x00U));
    $pc_port_require(model->materials[7U].tev_swap_mode_tables[0U].valid);
    $pc_port_require_eq(model->materials[7U].tev_swap_mode_tables[0U].channels[0U], static_cast< std::uint8_t >(0U));
    $pc_port_require_eq(model->materials[7U].tev_swap_mode_tables[0U].channels[1U], static_cast< std::uint8_t >(1U));
    $pc_port_require_eq(model->materials[7U].tev_swap_mode_tables[0U].channels[2U], static_cast< std::uint8_t >(2U));
    $pc_port_require_eq(model->materials[7U].tev_swap_mode_tables[0U].channels[3U], static_cast< std::uint8_t >(3U));
    $pc_port_require(model->materials[7U].tev_swap_mode_tables[1U].valid);
    $pc_port_require_eq(model->materials[7U].tev_swap_mode_tables[1U].channels[0U], static_cast< std::uint8_t >(2U));
    $pc_port_require_eq(model->materials[7U].tev_swap_mode_tables[1U].channels[1U], static_cast< std::uint8_t >(2U));
    $pc_port_require_eq(model->materials[7U].tev_swap_mode_tables[1U].channels[2U], static_cast< std::uint8_t >(2U));
    $pc_port_require_eq(model->materials[7U].tev_swap_mode_tables[1U].channels[3U], static_cast< std::uint8_t >(3U));
    $pc_port_require(model->materials[7U].alpha_compare.valid);
    $pc_port_require_eq(model->materials[7U].alpha_compare.comp0, static_cast< std::uint8_t >(7U));
    $pc_port_require_eq(model->materials[7U].alpha_compare.ref0, static_cast< std::uint8_t >(0U));
    $pc_port_require_eq(model->materials[7U].alpha_compare.op, static_cast< std::uint8_t >(1U));
    $pc_port_require_eq(model->materials[7U].alpha_compare.comp1, static_cast< std::uint8_t >(7U));
    $pc_port_require_eq(model->materials[7U].alpha_compare.ref1, static_cast< std::uint8_t >(0U));
    $pc_port_require(model->materials[5U].tev_k_colors[0U].valid);
    $pc_port_require_eq(model->materials[5U].tev_k_colors[0U].g, static_cast< std::uint8_t >(0x19U));
    $pc_port_require_eq(model->materials[5U].tev_k_colors[0U].b, static_cast< std::uint8_t >(0x34U));
    $pc_port_require(model->materials[5U].tev_stages[0U].valid);
    $pc_port_require_eq(model->materials[5U].tev_stages[0U].bytes[1U], static_cast< std::uint8_t >(0x02U));
    $pc_port_require_eq(model->materials[5U].name, std::string("EarthNightMat_v"));
    $pc_port_require(model->materials[5U].texture_coord_generators[0U].valid);
    $pc_port_require_eq(model->materials[5U].texture_coord_generators[0U].source, static_cast< std::uint8_t >(0U));
    $pc_port_require_eq(model->materials[5U].texture_coord_generators[0U].matrix, static_cast< std::uint8_t >(30U));
    $pc_port_require(model->materials[5U].texture_matrices[0U].valid);
    $pc_port_require_eq(static_cast< std::uint8_t >(model->materials[5U].texture_matrices[0U].info & 0x3FU), static_cast< std::uint8_t >(8U));
    $pc_port_require_eq(model->materials[5U].texture_matrices[0U].projection, static_cast< std::uint8_t >(0U));
    $pc_port_require(std::fabs(model->materials[5U].texture_matrices[0U].effect_matrix[0U] - 0.70710677F) < 0.0001F);
    $pc_port_require(std::fabs(model->materials[5U].texture_matrices[0U].effect_matrix[2U] + 0.70710677F) < 0.0001F);
    $pc_port_require(std::fabs(model->materials[5U].texture_matrices[0U].effect_matrix[7U] + 0.000937939F) < 0.0001F);
    $pc_port_require(std::fabs(model->materials[5U].texture_matrices[0U].effect_matrix[11U] - 7000.00049F) < 0.01F);
    $pc_port_require(model->materials[5U].texture_coord_generators[1U].valid);
    $pc_port_require_eq(model->materials[5U].texture_coord_generators[1U].source, static_cast< std::uint8_t >(0U));
    $pc_port_require_eq(model->materials[5U].texture_coord_generators[1U].matrix, static_cast< std::uint8_t >(33U));
    $pc_port_require(model->materials[5U].texture_matrices[1U].valid);
    $pc_port_require_eq(static_cast< std::uint8_t >(model->materials[5U].texture_matrices[1U].info & 0x3FU), static_cast< std::uint8_t >(8U));
    $pc_port_require(std::fabs(model->materials[5U].texture_matrices[1U].effect_matrix[11U] - 8000.00049F) < 0.01F);
    $pc_port_require(model->materials[7U].texture_matrices[0U].valid);
    $pc_port_require(std::fabs(model->materials[7U].texture_matrices[0U].srt.scale_x - 0.5F) < 0.001F);
    $pc_port_require_eq(model->materials[3U].material_color.r, static_cast< std::uint8_t >(144U));
    $pc_port_require_eq(model->joints[0U].name, std::string("world_root"));
    $pc_port_require_eq(model->joints[7U].name, std::string("Obit"));
    $pc_port_require_eq(model->joints[1U].parent_index, static_cast< std::uint16_t >(0U));
    $pc_port_require_eq(model->joints[2U].parent_index, static_cast< std::uint16_t >(1U));
    $pc_port_require_eq(model->joints[3U].parent_index, static_cast< std::uint16_t >(2U));
    $pc_port_require_eq(model->joints[7U].parent_index, static_cast< std::uint16_t >(0U));
    $pc_port_require(!model->draw_matrices[0U].weighted);
    $pc_port_require_eq(model->draw_matrices[0U].index, static_cast< std::uint16_t >(2U));
    $pc_port_require_eq(model->shapes[0U].material_index, static_cast< std::uint16_t >(1U));
    $pc_port_require(model->shapes[0U].triangles.front().v0.position_index != smgpc::assets::layout::J3D_NO_VERTEX_INDEX);
}

$test("CometNearOrbitSky BDL decodes MAT3 TEV stage fields") {
    const auto archive_path = first_existing_path({
        "../orig/RMGK01/files/ObjectData/CometNearOrbitSky.arc",
        "orig/RMGK01/files/ObjectData/CometNearOrbitSky.arc",
    });
    if (archive_path.empty()) {
        return;
    }

    auto bytes = load_file_bytes(archive_path);
    if (smgpc::assets::layout::is_yaz0(bytes)) {
        auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
        $pc_port_require(decoded);
        bytes = std::move(*decoded);
    }

    const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
    $pc_port_require(archive);

    const auto bdl_bytes = archive->find_entry("cometnearorbitsky.bdl");
    $pc_port_require(not bdl_bytes.empty());

    const auto model = smgpc::assets::layout::parse_j3d_model(bdl_bytes);
    if (!model) {
        throw std::runtime_error(model.failure().message);
    }

    const auto& earth = model->materials[5U];
    $pc_port_require_eq(earth.name, std::string("EarthNightMat_v"));
    $pc_port_require_eq(earth.tev_stage_count, static_cast< std::uint8_t >(2U));
    const auto earth_stage0 = make_j3d_tev_stage_raw({
        0xFFU, 0x02U, 0x08U, 0x0AU, 0x0EU, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U,
        0x07U, 0x04U, 0x05U, 0x07U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0xFFU,
    });
    const auto earth_stage1 = make_j3d_tev_stage_raw({
        0xFFU, 0x0FU, 0x08U, 0x0AU, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U,
        0x05U, 0x07U, 0x07U, 0x07U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU,
    });
    require_j3d_tev_stage_raw_and_decoded(earth.tev_stages[0U], earth_stage0);
    require_j3d_tev_stage_raw_and_decoded(earth.tev_stages[1U], earth_stage1);

    const auto& space = model->materials[7U];
    $pc_port_require_eq(space.name, std::string("Space_Mat_v"));
    $pc_port_require_eq(space.tev_stage_count, static_cast< std::uint8_t >(3U));
    const auto space_stage0 = make_j3d_tev_stage_raw({
        0xFFU, 0x0FU, 0x08U, 0x0AU, 0x0EU, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U,
        0x07U, 0x04U, 0x05U, 0x07U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0xFFU,
    });
    const auto space_stage1 = make_j3d_tev_stage_raw({
        0xFFU, 0x0FU, 0x0AU, 0x08U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U,
        0x05U, 0x07U, 0x07U, 0x07U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU,
    });
    require_j3d_tev_stage_raw_and_decoded(space.tev_stages[0U], space_stage0);
    require_j3d_tev_stage_raw_and_decoded(space.tev_stages[1U], space_stage1);
    require_j3d_tev_stage_raw_and_decoded(space.tev_stages[2U], space_stage1);
}

$test("CometNearOrbitSky live J3D adapter emits custom textured sky triangles") {
    const auto archive_path = first_existing_path({
        "../orig/RMGK01/files/ObjectData/CometNearOrbitSky.arc",
        "orig/RMGK01/files/ObjectData/CometNearOrbitSky.arc",
    });
    if (archive_path.empty()) {
        return;
    }

    auto bytes = load_file_bytes(archive_path);
    if (smgpc::assets::layout::is_yaz0(bytes)) {
        auto decoded = smgpc::assets::layout::decode_yaz0(bytes);
        $pc_port_require(decoded);
        bytes = std::move(*decoded);
    }

    const auto archive = smgpc::assets::layout::RarcArchive::parse(std::move(bytes));
    $pc_port_require(archive);

    const auto bdl_bytes = archive->find_entry("cometnearorbitsky.bdl");
    $pc_port_require(not bdl_bytes.empty());
    const auto bck_bytes = archive->find_entry("cometnearorbitsky.bck");
    $pc_port_require(not bck_bytes.empty());
    const auto btk_bytes = archive->find_entry("cometnearorbitsky.btk");
    $pc_port_require(not btk_bytes.empty());

    auto sky = smgpc::game::compat::FileSelectSkyJ3d::parse(bdl_bytes, bck_bytes, btk_bytes);
    if (!sky) {
        throw std::runtime_error(sky.failure().message);
    }
    $pc_port_require(!sky->empty());
    $pc_port_require(sky->triangleCount() > 1000U);

    smgpc::render::layout::LayoutDrawList frame0{};
    smgpc::render::layout::LayoutDrawList frame300{};
    unsetenv("SMGPC_FILE_SELECT_LIVE_J3D_SKY_SECONDARY_MODE");
    sky->appendDrawCommands(&frame0, 0.0F);
    sky->appendDrawCommands(&frame300, 300.0F);

    $pc_port_require(frame0.quads().empty());
    $pc_port_require(frame300.quads().empty());
    $pc_port_require(!frame0.triangle_batches().empty());
    $pc_port_require(!frame300.triangle_batches().empty());
    std::size_t frame0_vertex_count = 0U;
    std::size_t frame300_vertex_count = 0U;
    bool has_textured_batch = false;
    bool has_secondary_textured_batch = false;
    bool has_j3d_tev_batch = false;
    for (const auto& batch : frame0.triangle_batches()) {
        frame0_vertex_count += batch.vertices.size();
        has_textured_batch = has_textured_batch || (batch.texture.rgba8 != nullptr && batch.texture.width > 0U && batch.texture.height > 0U);
        has_secondary_textured_batch =
            has_secondary_textured_batch ||
            (batch.secondary_texture.rgba8 != nullptr && batch.secondary_texture.width > 0U && batch.secondary_texture.height > 0U &&
             batch.secondary_texture_mode != smgpc::render::layout::TriangleTextureCombineMode::None);
        has_j3d_tev_batch =
            has_j3d_tev_batch ||
            (batch.secondary_texture_mode == smgpc::render::layout::TriangleTextureCombineMode::J3dTevColorStages &&
             batch.tev_stage_count >= 2U && batch.tev_stages[0U].color_op.clamp == 1U &&
             batch.tev_stages[1U].color_op.clamp == 1U);
    }
    for (const auto& batch : frame300.triangle_batches()) {
        frame300_vertex_count += batch.vertices.size();
    }
    $pc_port_require(frame0_vertex_count > 100U);
    $pc_port_require(frame300_vertex_count > 0U);
    $pc_port_require(has_textured_batch);
    $pc_port_require(has_secondary_textured_batch);
    $pc_port_require(has_j3d_tev_batch);
    $pc_port_require(
        std::fabs(frame0.triangle_batches().front().vertices.front().x - frame300.triangle_batches().front().vertices.front().x) > 0.001F ||
        std::fabs(frame0.triangle_batches().front().vertices.front().y - frame300.triangle_batches().front().vertices.front().y) > 0.001F);
}
