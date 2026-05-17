#include "J3dTexture.hpp"

#include <stdexcept>

namespace smgpc::game {
namespace {

constexpr auto J3D1_MAGIC = std::uint32_t {0x4a334431U};
constexpr auto J3D2_MAGIC = std::uint32_t {0x4a334432U};
constexpr auto TEX1_MAGIC = std::uint32_t {0x54455831U};

[[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) {
    if (offset + 2U > data.size()) {
        throw std::runtime_error("J3D read past end of buffer");
    }

    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8U) | static_cast<std::uint16_t>(data[offset + 1U]));
}

[[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
    if (offset + 4U > data.size()) {
        throw std::runtime_error("J3D read past end of buffer");
    }

    return (static_cast<std::uint32_t>(data[offset]) << 24U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) | (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) | static_cast<std::uint32_t>(data[offset + 3U]);
}

[[nodiscard]] std::string read_string(std::span<const std::uint8_t> data, std::size_t offset) {
    if (offset >= data.size()) {
        throw std::runtime_error("J3D string offset outside buffer");
    }

    auto end = offset;
    while (end < data.size() && data[end] != 0U) {
        ++end;
    }
    if (end == data.size()) {
        throw std::runtime_error("J3D string is not null terminated");
    }

    return std::string(reinterpret_cast<const char *>(data.data() + offset), end - offset);
}

[[nodiscard]] std::vector<std::string> read_string_table(std::span<const std::uint8_t> data, std::size_t table_offset) {
    if (table_offset + 4U > data.size()) {
        throw std::runtime_error("J3D string table outside buffer");
    }

    const auto count = read_be16(data, table_offset);
    auto names = std::vector<std::string> {};
    names.reserve(count);
    for (auto i = 0U; i < count; ++i) {
        const auto entry_offset = table_offset + 4U + i * 4U;
        if (entry_offset + 4U > data.size()) {
            throw std::runtime_error("J3D string table entry outside buffer");
        }

        names.push_back(read_string(data, table_offset + read_be16(data, entry_offset + 2U)));
    }

    return names;
}

[[nodiscard]] std::vector<J3dTexture> parse_tex1(std::span<const std::uint8_t> data, std::size_t section_offset, std::size_t section_size) {
    if (section_offset + section_size > data.size() || section_size < 0x20U) {
        throw std::runtime_error("J3D TEX1 section outside buffer");
    }

    const auto texture_count = read_be16(data, section_offset + 0x08U);
    const auto texture_header_offset = section_offset + read_be32(data, section_offset + 0x0CU);
    const auto string_table_offset = section_offset + read_be32(data, section_offset + 0x10U);
    if (texture_header_offset + texture_count * 0x20U > data.size()) {
        throw std::runtime_error("J3D TEX1 texture headers outside buffer");
    }

    const auto names = read_string_table(data, string_table_offset);
    auto textures = std::vector<J3dTexture> {};
    textures.reserve(texture_count);
    for (auto i = 0U; i < texture_count; ++i) {
        const auto header_offset = texture_header_offset + i * 0x20U;
        const auto bti = decode_bti_texture(data.subspan(header_offset));

        textures.push_back(J3dTexture {
            .name = i < names.size() ? names[i] : std::string {},
            .transparency = bti.transparency,
            .wrap_s = bti.wrap_s,
            .wrap_t = bti.wrap_t,
            .palette_format = bti.palette_format,
            .palette_entry_count = bti.palette_entry_count,
            .palette_data_offset = bti.palette_data_offset,
            .mipmap = bti.mipmap,
            .do_edge_lod = bti.do_edge_lod,
            .bias_clamp = bti.bias_clamp,
            .max_anisotropy = bti.max_anisotropy,
            .min_filter = bti.min_filter,
            .mag_filter = bti.mag_filter,
            .min_lod = bti.min_lod,
            .max_lod = bti.max_lod,
            .image_count = bti.image_count,
            .lod_bias = bti.lod_bias,
            .image_data_offset = bti.image_data_offset,
            .image = bti.image,
        });
    }

    return textures;
}

}  // namespace

std::vector<J3dTexture> extract_j3d_textures(std::span<const std::uint8_t> model_data) {
    if (model_data.size() < 0x20U) {
        throw std::runtime_error("J3D model is too small");
    }

    const auto magic = read_be32(model_data, 0U);
    if (magic != J3D1_MAGIC && magic != J3D2_MAGIC) {
        throw std::runtime_error("J3D model has unexpected magic");
    }

    const auto section_count = read_be32(model_data, 0x0CU);
    auto offset = std::size_t {0x20U};
    for (auto i = 0U; i < section_count; ++i) {
        if (offset + 8U > model_data.size()) {
            throw std::runtime_error("J3D section header outside buffer");
        }

        const auto tag = read_be32(model_data, offset);
        const auto section_size = read_be32(model_data, offset + 4U);
        if (section_size < 8U || offset + section_size > model_data.size()) {
            throw std::runtime_error("J3D section size outside buffer");
        }

        if (tag == TEX1_MAGIC) {
            return parse_tex1(model_data, offset, section_size);
        }

        offset += section_size;
    }

    return {};
}

}  // namespace smgpc::game
