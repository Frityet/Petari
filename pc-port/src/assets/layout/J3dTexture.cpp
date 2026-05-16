#include "J3dTexture.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "Binary.hpp"

namespace smgpc::assets::layout {
namespace {

[[nodiscard]] AssetError make_error(std::string message) {
    return AssetError {
        .code = AssetErrorCode::InvalidFormat,
        .message = std::move(message),
    };
}

[[nodiscard]] std::size_t gx_texture_level_size(std::uint16_t width, std::uint16_t height, std::uint8_t format) {
    switch (format) {
    case 0U:
        return ((static_cast<std::size_t>(width) + 7U) / 8U) * ((static_cast<std::size_t>(height) + 7U) / 8U) * 32U;
    case 1U:
    case 2U:
        return ((static_cast<std::size_t>(width) + 7U) / 8U) * ((static_cast<std::size_t>(height) + 3U) / 4U) * 32U;
    case 3U:
    case 4U:
    case 5U:
        return ((static_cast<std::size_t>(width) + 3U) / 4U) * ((static_cast<std::size_t>(height) + 3U) / 4U) * 32U;
    case 14U:
        return ((static_cast<std::size_t>(width) + 7U) / 8U) * ((static_cast<std::size_t>(height) + 7U) / 8U) * 32U;
    default:
        return 0U;
    }
}

[[nodiscard]] std::string read_string_table_name(std::span<const std::byte> tex1, std::size_t string_table_offset, std::size_t index) {
    if (!binary::has_bytes(tex1, string_table_offset, 4U)) {
        return {};
    }

    const auto count = static_cast<std::size_t>(binary::read_u16_be(tex1, string_table_offset));
    if (index >= count) {
        return {};
    }

    const auto entry_offset = string_table_offset + 4U + index * 4U;
    if (!binary::has_bytes(tex1, entry_offset, 4U)) {
        return {};
    }

    const auto name_offset = static_cast<std::size_t>(binary::read_u16_be(tex1, entry_offset + 2U));
    if (!binary::has_bytes(tex1, string_table_offset + name_offset, 1U)) {
        return {};
    }

    return binary::read_c_string(tex1, string_table_offset + name_offset);
}

[[nodiscard]] AssetResult<std::span<const std::byte>> find_tex1_section(std::span<const std::byte> bdl_bytes) {
    if (bdl_bytes.size() < 0x20U || !binary::fourcc_equals(bdl_bytes, 0U, "J3D2")) {
        return make_error("J3D file header is invalid.");
    }

    const auto section_count = static_cast<std::size_t>(binary::read_u32_be(bdl_bytes, 0x0CU));
    std::size_t offset = 0x20U;
    for (std::size_t section_index = 0U; section_index < section_count; ++section_index) {
        if (!binary::has_bytes(bdl_bytes, offset, 8U)) {
            return make_error("J3D section header exceeds file bounds.");
        }

        const auto section_size = static_cast<std::size_t>(binary::read_u32_be(bdl_bytes, offset + 4U));
        if (section_size < 8U || !binary::has_bytes(bdl_bytes, offset, section_size)) {
            return make_error("J3D section exceeds file bounds.");
        }

        if (binary::fourcc_equals(bdl_bytes, offset, "TEX1")) {
            return binary::subspan(bdl_bytes, offset, section_size);
        }

        offset += section_size;
    }

    return make_error("J3D file does not contain a TEX1 section.");
}

}  // namespace

AssetResult<std::vector<J3dTexture>> parse_j3d_tex1_textures(std::span<const std::byte> bdl_bytes) {
    const auto tex1_section = find_tex1_section(bdl_bytes);
    if (!tex1_section) {
        return tex1_section.failure();
    }

    const auto tex1 = *tex1_section;
    if (!binary::has_bytes(tex1, 0U, 0x20U) || !binary::fourcc_equals(tex1, 0U, "TEX1")) {
        return make_error("TEX1 header is invalid.");
    }

    const auto texture_count = static_cast<std::size_t>(binary::read_u16_be(tex1, 8U));
    const auto header_table_offset = static_cast<std::size_t>(binary::read_u32_be(tex1, 0x0CU));
    const auto string_table_offset = static_cast<std::size_t>(binary::read_u32_be(tex1, 0x10U));
    if (!binary::has_bytes(tex1, header_table_offset, texture_count * 0x20U)) {
        return make_error("TEX1 texture header table exceeds bounds.");
    }

    std::vector<J3dTexture> textures {};
    textures.reserve(texture_count);
    for (std::size_t texture_index = 0U; texture_index < texture_count; ++texture_index) {
        const auto header_offset = header_table_offset + texture_index * 0x20U;
        const auto format = binary::read_u8(tex1, header_offset + 0U);
        const auto width = binary::read_u16_be(tex1, header_offset + 2U);
        const auto height = binary::read_u16_be(tex1, header_offset + 4U);
        const auto image_offset = static_cast<std::size_t>(binary::read_u32_be(tex1, header_offset + 0x1CU));
        const auto image_size = gx_texture_level_size(width, height, format);
        if (image_size == 0U) {
            return make_error("TEX1 texture uses unsupported GX format.");
        }
        if (!binary::has_bytes(tex1, image_offset, image_size)) {
            return make_error("TEX1 image data exceeds bounds.");
        }

        auto decoded = tpl::decode_gx_tiled_texture(binary::subspan(tex1, image_offset, image_size), width, height, format);
        if (!decoded) {
            return decoded.failure();
        }

        textures.push_back(J3dTexture {
            .name = read_string_table_name(tex1, string_table_offset, texture_index),
            .image = std::move(*decoded),
            .wrap_s = binary::read_u8(tex1, header_offset + 6U),
            .wrap_t = binary::read_u8(tex1, header_offset + 7U),
        });
    }

    return textures;
}

}  // namespace smgpc::assets::layout
