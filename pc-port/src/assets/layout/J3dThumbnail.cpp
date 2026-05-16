#include "J3dThumbnail.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Binary.hpp"
#include "J3dTexture.hpp"

namespace smgpc::assets::layout {
namespace {

constexpr std::uint32_t GX_VA_POS = 0x09U;
constexpr std::uint32_t GX_VA_NRM = 0x0AU;
constexpr std::uint32_t GX_VA_CLR0 = 0x0BU;
constexpr std::uint32_t GX_VA_TEX0 = 0x0DU;
constexpr std::uint32_t GX_VA_NULL = 0xFFU;
constexpr std::uint32_t GX_INDEX16 = 3U;
constexpr std::uint32_t GX_S16 = 3U;
constexpr std::uint32_t GX_F32 = 4U;
constexpr std::uint32_t GX_RGB565 = 0U;
constexpr std::uint32_t GX_RGBA8 = 5U;
constexpr std::uint8_t GX_TRIANGLES = 0x90U;
constexpr std::uint8_t GX_TRIANGLESTRIP = 0x98U;
constexpr std::uint8_t GX_TRIANGLEFAN = 0xA0U;

struct Section {
    std::size_t offset{};
    std::size_t size{};
};

struct Vec2f {
    float x{};
    float y{};
};

struct Vec3f {
    float x{};
    float y{};
    float z{};
};

struct Rgba {
    float r{1.0F};
    float g{1.0F};
    float b{1.0F};
    float a{1.0F};
};

struct VertexFormat {
    std::uint32_t position_type{GX_S16};
    std::uint8_t position_frac{};
    std::uint8_t position_stride{6U};
    std::uint8_t normal_frac{};
    std::uint8_t texcoord_frac{};
    std::uint32_t color_type{GX_RGB565};
    std::uint8_t color_stride{2U};
};

struct VertexArrays {
    std::span<const std::byte> bytes{};
    std::size_t position_offset{};
    std::size_t normal_offset{};
    std::size_t color_offset{};
    std::size_t texcoord_offset{};
    VertexFormat format{};
};

struct VtxDesc {
    std::uint32_t attr{};
    std::uint32_t type{};
};

struct Vertex {
    Vec3f position{};
    Vec3f normal{};
    Vec2f texcoord{};
    Rgba color{};
    Rgba material_color{};
    std::size_t texture_index{};
};

struct Triangle {
    Vertex v0{};
    Vertex v1{};
    Vertex v2{};
};

struct ShapeInfo {
    std::uint16_t material_index{};
    std::uint16_t shape_init_index{};
};

struct MaterialInfo {
    std::size_t texture_index{};
    Rgba color{};
};

[[nodiscard]] AssetError make_error(std::string message) {
    return AssetError{
        .code = AssetErrorCode::InvalidFormat,
        .message = std::move(message),
    };
}

[[nodiscard]] std::optional<Section> find_section(std::span<const std::byte> bytes, std::string_view name) {
    if (bytes.size() < 0x20U || !binary::fourcc_equals(bytes, 0U, "J3D2")) {
        return std::nullopt;
    }

    const auto section_count = static_cast<std::size_t>(binary::read_u32_be(bytes, 0x0CU));
    std::size_t offset = 0x20U;
    for (std::size_t section_index = 0U; section_index < section_count; ++section_index) {
        if (!binary::has_bytes(bytes, offset, 8U)) {
            return std::nullopt;
        }

        const auto section_size = static_cast<std::size_t>(binary::read_u32_be(bytes, offset + 4U));
        if (section_size < 8U || !binary::has_bytes(bytes, offset, section_size)) {
            return std::nullopt;
        }

        if (binary::fourcc_equals(bytes, offset, name)) {
            return Section{.offset = offset, .size = section_size};
        }
        offset += section_size;
    }

    return std::nullopt;
}

[[nodiscard]] float decode_s16(std::span<const std::byte> bytes, std::size_t offset, std::uint8_t frac) {
    const auto raw = static_cast<std::int16_t>(binary::read_u16_be(bytes, offset));
    return static_cast<float>(raw) / static_cast<float>(1U << frac);
}

[[nodiscard]] Vec3f read_vec3_s16(std::span<const std::byte> bytes, std::size_t offset, std::uint8_t frac) {
    return Vec3f{
        .x = decode_s16(bytes, offset + 0U, frac),
        .y = decode_s16(bytes, offset + 2U, frac),
        .z = decode_s16(bytes, offset + 4U, frac),
    };
}

[[nodiscard]] Vec3f read_vec3_f32(std::span<const std::byte> bytes, std::size_t offset) {
    return Vec3f{
        .x = binary::read_f32_be(bytes, offset + 0U),
        .y = binary::read_f32_be(bytes, offset + 4U),
        .z = binary::read_f32_be(bytes, offset + 8U),
    };
}

[[nodiscard]] Vec3f read_position(std::span<const std::byte> bytes, std::size_t offset, const VertexFormat &format) {
    if (format.position_type == GX_F32) {
        return read_vec3_f32(bytes, offset);
    }

    return read_vec3_s16(bytes, offset, format.position_frac);
}

[[nodiscard]] Vec2f read_vec2_s16(std::span<const std::byte> bytes, std::size_t offset, std::uint8_t frac) {
    return Vec2f{
        .x = decode_s16(bytes, offset + 0U, frac),
        .y = decode_s16(bytes, offset + 2U, frac),
    };
}

[[nodiscard]] Rgba read_rgb565(std::span<const std::byte> bytes, std::size_t offset) {
    const auto packed = binary::read_u16_be(bytes, offset);
    const auto expand5 = [](std::uint16_t value) {
        return static_cast<float>((value << 3U) | (value >> 2U)) / 255.0F;
    };
    const auto expand6 = [](std::uint16_t value) {
        return static_cast<float>((value << 2U) | (value >> 4U)) / 255.0F;
    };

    return Rgba{
        .r = expand5(static_cast<std::uint16_t>((packed >> 11U) & 0x1FU)),
        .g = expand6(static_cast<std::uint16_t>((packed >> 5U) & 0x3FU)),
        .b = expand5(static_cast<std::uint16_t>(packed & 0x1FU)),
        .a = 1.0F,
    };
}

[[nodiscard]] Rgba read_rgba8(std::span<const std::byte> bytes, std::size_t offset) {
    return Rgba{
        .r = static_cast<float>(binary::read_u8(bytes, offset + 0U)) / 255.0F,
        .g = static_cast<float>(binary::read_u8(bytes, offset + 1U)) / 255.0F,
        .b = static_cast<float>(binary::read_u8(bytes, offset + 2U)) / 255.0F,
        .a = static_cast<float>(binary::read_u8(bytes, offset + 3U)) / 255.0F,
    };
}

[[nodiscard]] Rgba read_vertex_color(std::span<const std::byte> bytes, std::size_t offset, const VertexFormat &format) {
    if (format.color_type == GX_RGBA8) {
        return read_rgba8(bytes, offset);
    }

    return read_rgb565(bytes, offset);
}

[[nodiscard]] AssetResult<VertexArrays> parse_vertex_arrays(std::span<const std::byte> bytes, const Section &section) {
    if (!binary::has_bytes(bytes, section.offset, 0x40U)) {
        return make_error("VTX1 section is too small.");
    }

    VertexArrays arrays{
        .bytes = bytes,
        .position_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x0CU)),
        .normal_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x10U)),
        .color_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x18U)),
        .texcoord_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x20U)),
    };

    const auto format_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x08U));
    if (!binary::has_bytes(bytes, format_offset, 16U)) {
        return make_error("VTX1 format table exceeds file bounds.");
    }

    for (std::size_t index = 0U; binary::has_bytes(bytes, format_offset + index * 16U, 16U); ++index) {
        const auto entry_offset = format_offset + index * 16U;
        const auto attr = binary::read_u32_be(bytes, entry_offset + 0U);
        if (attr == GX_VA_NULL) {
            break;
        }

        const auto count = binary::read_u32_be(bytes, entry_offset + 4U);
        const auto type = binary::read_u32_be(bytes, entry_offset + 8U);
        const auto frac = binary::read_u8(bytes, entry_offset + 12U);

        if (attr == GX_VA_POS) {
            if ((type != GX_S16 && type != GX_F32) || count != 1U) {
                return make_error("J3D thumbnail renderer only supports S16/F32 XYZ positions.");
            }
            arrays.format.position_type = type;
            arrays.format.position_frac = frac;
            arrays.format.position_stride = type == GX_F32 ? 12U : 6U;
        } else if (attr == GX_VA_NRM) {
            if (type != GX_S16 || count != 0U) {
                return make_error("J3D thumbnail renderer only supports S16 XYZ normals.");
            }
            arrays.format.normal_frac = frac;
        } else if (attr == GX_VA_CLR0) {
            if (type != GX_RGB565 && type != GX_RGBA8) {
                return make_error("J3D thumbnail renderer only supports RGB565/RGBA8 vertex colors, got type " + std::to_string(type) + ".");
            }
            arrays.format.color_type = type;
            arrays.format.color_stride = type == GX_RGBA8 ? 4U : 2U;
        } else if (attr == GX_VA_TEX0) {
            if (type != GX_S16 || count != 1U) {
                return make_error("J3D thumbnail renderer only supports S16 ST texture coordinates.");
            }
            arrays.format.texcoord_frac = frac;
        }
    }

    if (!binary::has_bytes(bytes, arrays.position_offset, arrays.format.position_stride) || !binary::has_bytes(bytes, arrays.normal_offset, 6U) ||
        !binary::has_bytes(bytes, arrays.color_offset, arrays.format.color_stride) || !binary::has_bytes(bytes, arrays.texcoord_offset, 4U)) {
        return make_error("VTX1 array offsets exceed file bounds.");
    }

    return arrays;
}

[[nodiscard]] AssetResult<std::vector<MaterialInfo>> parse_material_infos(std::span<const std::byte> bytes, const Section &section) {
    if (!binary::has_bytes(bytes, section.offset, 0x4CU)) {
        return make_error("MAT3 section is too small.");
    }

    const auto material_count = static_cast<std::size_t>(binary::read_u16_be(bytes, section.offset + 0x08U));
    const auto material_init_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x0CU));
    const auto material_id_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x10U));
    const auto mat_color_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x20U));
    const auto tex_no_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x48U));

    if (!binary::has_bytes(bytes, material_id_offset, material_count * 2U)) {
        return make_error("MAT3 material ID table exceeds file bounds.");
    }

    std::vector<MaterialInfo> materials(material_count, MaterialInfo{});
    for (std::size_t material_index = 0U; material_index < material_count; ++material_index) {
        const auto material_id = static_cast<std::size_t>(binary::read_u16_be(bytes, material_id_offset + material_index * 2U));
        const auto init_offset = material_init_offset + material_id * 0x14CU;
        if (!binary::has_bytes(bytes, init_offset + 0x84U, 2U)) {
            return make_error("MAT3 material init data exceeds file bounds.");
        }

        const auto mat_color_index = binary::read_u16_be(bytes, init_offset + 0x08U);
        if (mat_color_index != 0xFFFFU) {
            const auto mat_color_entry = mat_color_offset + static_cast<std::size_t>(mat_color_index) * 4U;
            if (!binary::has_bytes(bytes, mat_color_entry, 4U)) {
                return make_error("MAT3 material color table exceeds file bounds.");
            }
            materials[material_index].color = read_rgba8(bytes, mat_color_entry);
        }

        const auto tex_no_index = binary::read_u16_be(bytes, init_offset + 0x84U);
        if (tex_no_index == 0xFFFFU) {
            continue;
        }
        const auto tex_no_entry = tex_no_offset + static_cast<std::size_t>(tex_no_index) * 2U;
        if (!binary::has_bytes(bytes, tex_no_entry, 2U)) {
            return make_error("MAT3 texture number table exceeds file bounds.");
        }
        materials[material_index].texture_index = static_cast<std::size_t>(binary::read_u16_be(bytes, tex_no_entry));
    }

    return materials;
}

[[nodiscard]] AssetResult<std::unordered_map<std::size_t, std::size_t>> parse_shape_material_map(std::span<const std::byte> bytes, const Section &section) {
    if (!binary::has_bytes(bytes, section.offset, 0x18U)) {
        return make_error("INF1 section is too small.");
    }

    const auto hierarchy_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x14U));
    if (!binary::has_bytes(bytes, hierarchy_offset, 4U)) {
        return make_error("INF1 hierarchy exceeds file bounds.");
    }

    std::unordered_map<std::size_t, std::size_t> material_by_shape{};
    std::size_t current_material = 0U;
    for (std::size_t cursor = hierarchy_offset; binary::has_bytes(bytes, cursor, 4U); cursor += 4U) {
        const auto kind = binary::read_u16_be(bytes, cursor + 0U);
        const auto index = static_cast<std::size_t>(binary::read_u16_be(bytes, cursor + 2U));
        if (kind == 0U) {
            break;
        }
        if (kind == 0x11U) {
            current_material = index;
        } else if (kind == 0x12U) {
            material_by_shape[index] = current_material;
        }
    }

    return material_by_shape;
}

[[nodiscard]] std::vector<VtxDesc> read_vtx_desc_list(std::span<const std::byte> bytes, std::size_t offset) {
    std::vector<VtxDesc> descriptors{};
    for (std::size_t index = 0U; binary::has_bytes(bytes, offset + index * 8U, 8U); ++index) {
        const auto entry_offset = offset + index * 8U;
        const auto attr = binary::read_u32_be(bytes, entry_offset + 0U);
        const auto type = binary::read_u32_be(bytes, entry_offset + 4U);
        if (attr == GX_VA_NULL) {
            break;
        }
        descriptors.push_back(VtxDesc{
            .attr = attr,
            .type = type,
        });
    }
    return descriptors;
}

[[nodiscard]] Vertex read_display_vertex(std::span<const std::byte> bytes, std::size_t *cursor, const std::vector<VtxDesc> &descriptors,
                                         const VertexArrays &arrays, const MaterialInfo &material) {
    Vertex vertex{};
    vertex.texture_index = material.texture_index;
    vertex.material_color = material.color;

    for (const auto &descriptor : descriptors) {
        if (descriptor.type != GX_INDEX16 || !binary::has_bytes(bytes, *cursor, 2U)) {
            *cursor = bytes.size();
            return vertex;
        }

        const auto index = static_cast<std::size_t>(binary::read_u16_be(bytes, *cursor));
        *cursor += 2U;

        if (descriptor.attr == GX_VA_POS) {
            vertex.position = read_position(bytes, arrays.position_offset + index * arrays.format.position_stride, arrays.format);
        } else if (descriptor.attr == GX_VA_NRM) {
            vertex.normal = read_vec3_s16(bytes, arrays.normal_offset + index * 6U, arrays.format.normal_frac);
        } else if (descriptor.attr == GX_VA_CLR0) {
            vertex.color = read_vertex_color(bytes, arrays.color_offset + index * arrays.format.color_stride, arrays.format);
        } else if (descriptor.attr == GX_VA_TEX0) {
            vertex.texcoord = read_vec2_s16(bytes, arrays.texcoord_offset + index * 4U, arrays.format.texcoord_frac);
        }
    }

    return vertex;
}

void push_strip_triangles(std::vector<Triangle> *triangles, const std::vector<Vertex> &strip) {
    if (triangles == nullptr || strip.size() < 3U) {
        return;
    }

    for (std::size_t index = 2U; index < strip.size(); ++index) {
        if (index % 2U == 0U) {
            triangles->push_back(Triangle{strip[index - 2U], strip[index - 1U], strip[index]});
        } else {
            triangles->push_back(Triangle{strip[index - 1U], strip[index - 2U], strip[index]});
        }
    }
}

[[nodiscard]] AssetResult<std::vector<Triangle>> parse_triangles(std::span<const std::byte> bytes, const Section &section, const VertexArrays &arrays,
                                                                 const std::unordered_map<std::size_t, std::size_t> &material_by_shape,
                                                                 const std::vector<MaterialInfo> &materials) {
    if (!binary::has_bytes(bytes, section.offset, 0x2CU)) {
        return make_error("SHP1 section is too small.");
    }

    const auto shape_count = static_cast<std::size_t>(binary::read_u16_be(bytes, section.offset + 0x08U));
    const auto shape_init_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x0CU));
    const auto index_table_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x10U));
    const auto vtx_desc_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x18U));
    const auto display_list_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x20U));
    const auto mtx_init_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x24U));
    const auto draw_init_offset = section.offset + static_cast<std::size_t>(binary::read_u32_be(bytes, section.offset + 0x28U));

    if (!binary::has_bytes(bytes, index_table_offset, shape_count * 2U)) {
        return make_error("SHP1 index table exceeds file bounds.");
    }

    std::vector<Triangle> triangles{};
    triangles.reserve(shape_count * 256U);
    for (std::size_t shape_index = 0U; shape_index < shape_count; ++shape_index) {
        const auto shape_init_index = static_cast<std::size_t>(binary::read_u16_be(bytes, index_table_offset + shape_index * 2U));
        const auto init_offset = shape_init_offset + shape_init_index * 0x28U;
        if (!binary::has_bytes(bytes, init_offset, 0x28U)) {
            return make_error("SHP1 shape init data exceeds file bounds.");
        }

        const auto matrix_group_count = static_cast<std::size_t>(binary::read_u16_be(bytes, init_offset + 0x02U));
        const auto vtx_desc_index = static_cast<std::size_t>(binary::read_u16_be(bytes, init_offset + 0x04U));
        const auto mtx_init_index = static_cast<std::size_t>(binary::read_u16_be(bytes, init_offset + 0x06U));
        const auto draw_init_index = static_cast<std::size_t>(binary::read_u16_be(bytes, init_offset + 0x08U));
        const auto descriptors = read_vtx_desc_list(bytes, vtx_desc_offset + vtx_desc_index);
        if (descriptors.empty()) {
            return make_error("SHP1 shape has no vertex descriptors.");
        }

        const auto material_found = material_by_shape.find(shape_index);
        const auto material_index = material_found == material_by_shape.end() ? shape_index : material_found->second;
        const auto material = material_index < materials.size() ? materials[material_index] : MaterialInfo{};

        for (std::size_t group_index = 0U; group_index < matrix_group_count; ++group_index) {
            const auto mtx_offset = mtx_init_offset + (mtx_init_index + group_index) * 8U;
            const auto draw_offset = draw_init_offset + (draw_init_index + group_index) * 8U;
            if (!binary::has_bytes(bytes, mtx_offset, 8U) || !binary::has_bytes(bytes, draw_offset, 8U)) {
                return make_error("SHP1 matrix or draw init data exceeds file bounds.");
            }

            const auto display_list_size = static_cast<std::size_t>(binary::read_u32_be(bytes, draw_offset + 0U));
            const auto display_list_index = static_cast<std::size_t>(binary::read_u32_be(bytes, draw_offset + 4U));
            std::size_t cursor = display_list_offset + display_list_index;
            const auto end = cursor + display_list_size;
            if (!binary::has_bytes(bytes, cursor, display_list_size)) {
                return make_error("SHP1 display list exceeds file bounds.");
            }

            while (cursor < end && cursor < bytes.size()) {
                const auto primitive = binary::read_u8(bytes, cursor++);
                if (primitive == 0U) {
                    continue;
                }
                if (!binary::has_bytes(bytes, cursor, 2U)) {
                    return make_error("SHP1 primitive count exceeds file bounds.");
                }
                const auto vertex_count = static_cast<std::size_t>(binary::read_u16_be(bytes, cursor));
                cursor += 2U;

                std::vector<Vertex> vertices{};
                vertices.reserve(vertex_count);
                for (std::size_t vertex_index = 0U; vertex_index < vertex_count; ++vertex_index) {
                    vertices.push_back(read_display_vertex(bytes, &cursor, descriptors, arrays, material));
                }

                if (primitive == GX_TRIANGLESTRIP) {
                    push_strip_triangles(&triangles, vertices);
                } else if (primitive == GX_TRIANGLES) {
                    for (std::size_t index = 2U; index < vertices.size(); index += 3U) {
                        triangles.push_back(Triangle{vertices[index - 2U], vertices[index - 1U], vertices[index]});
                    }
                } else if (primitive == GX_TRIANGLEFAN) {
                    for (std::size_t index = 2U; index < vertices.size(); ++index) {
                        triangles.push_back(Triangle{vertices[0U], vertices[index - 1U], vertices[index]});
                    }
                }
            }
        }
    }

    return triangles;
}

[[nodiscard]] Vec3f rotate_point(Vec3f point, float yaw_degrees, float pitch_degrees) {
    constexpr float DEG_TO_RAD = 0.017453292519943295F;
    const float yaw = yaw_degrees * DEG_TO_RAD;
    const float pitch = pitch_degrees * DEG_TO_RAD;
    const float cy = std::cos(yaw);
    const float sy = std::sin(yaw);
    const float cp = std::cos(pitch);
    const float sp = std::sin(pitch);

    const Vec3f yawed{
        .x = point.x * cy + point.z * sy,
        .y = point.y,
        .z = -point.x * sy + point.z * cy,
    };
    return Vec3f{
        .x = yawed.x,
        .y = yawed.y * cp - yawed.z * sp,
        .z = yawed.y * sp + yawed.z * cp,
    };
}

[[nodiscard]] float edge_function(float ax, float ay, float bx, float by, float cx, float cy) {
    return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
}

[[nodiscard]] std::uint8_t clamp_u8(float value) {
    if (value <= 0.0F) {
        return 0U;
    }
    if (value >= 255.0F) {
        return 255U;
    }
    return static_cast<std::uint8_t>(std::lround(value));
}

[[nodiscard]] Rgba sample_texture(const std::vector<J3dTexture> &textures, std::size_t texture_index, float u, float v) {
    if (texture_index >= textures.size() || textures[texture_index].image.empty()) {
        return {};
    }

    const auto &image = textures[texture_index].image;
    u = u - std::floor(u);
    v = v - std::floor(v);
    const auto x = std::min<std::uint16_t>(image.width - 1U, static_cast<std::uint16_t>(u * static_cast<float>(image.width)));
    const auto y = std::min<std::uint16_t>(image.height - 1U, static_cast<std::uint16_t>(v * static_cast<float>(image.height)));
    const auto offset = (static_cast<std::size_t>(y) * image.width + x) * 4U;

    return Rgba{
        .r = static_cast<float>(image.rgba8[offset + 0U]) / 255.0F,
        .g = static_cast<float>(image.rgba8[offset + 1U]) / 255.0F,
        .b = static_cast<float>(image.rgba8[offset + 2U]) / 255.0F,
        .a = static_cast<float>(image.rgba8[offset + 3U]) / 255.0F,
    };
}

}  // namespace

AssetResult<tpl::DecodedImage> render_j3d_thumbnail(std::span<const std::byte> bdl_bytes, const J3dThumbnailOptions &options) {
    const auto inf1 = find_section(bdl_bytes, "INF1");
    const auto vtx1 = find_section(bdl_bytes, "VTX1");
    const auto shp1 = find_section(bdl_bytes, "SHP1");
    const auto mat3 = find_section(bdl_bytes, "MAT3");
    if (!inf1 || !vtx1 || !shp1 || !mat3) {
        return make_error("J3D thumbnail renderer requires INF1, VTX1, SHP1, and MAT3 sections.");
    }

    auto textures = parse_j3d_tex1_textures(bdl_bytes);
    if (!textures) {
        return textures.failure();
    }
    if (textures->empty()) {
        return make_error("J3D thumbnail renderer requires at least one TEX1 texture.");
    }

    auto arrays = parse_vertex_arrays(bdl_bytes, *vtx1);
    if (!arrays) {
        return arrays.failure();
    }

    auto materials = parse_material_infos(bdl_bytes, *mat3);
    if (!materials) {
        return materials.failure();
    }

    auto shape_materials = parse_shape_material_map(bdl_bytes, *inf1);
    if (!shape_materials) {
        return shape_materials.failure();
    }

    auto triangles = parse_triangles(bdl_bytes, *shp1, *arrays, *shape_materials, *materials);
    if (!triangles) {
        return triangles.failure();
    }
    if (triangles->empty()) {
        return make_error("J3D thumbnail renderer found no triangles.");
    }

    Vec3f minimum{
        .x = std::numeric_limits<float>::max(),
        .y = std::numeric_limits<float>::max(),
        .z = std::numeric_limits<float>::max(),
    };
    Vec3f maximum{
        .x = std::numeric_limits<float>::lowest(),
        .y = std::numeric_limits<float>::lowest(),
        .z = std::numeric_limits<float>::lowest(),
    };
    for (const auto &triangle : *triangles) {
        for (const auto &vertex : {triangle.v0, triangle.v1, triangle.v2}) {
            const auto rotated = rotate_point(vertex.position, options.yaw_degrees, options.pitch_degrees);
            minimum.x = std::min(minimum.x, rotated.x);
            minimum.y = std::min(minimum.y, rotated.y);
            minimum.z = std::min(minimum.z, rotated.z);
            maximum.x = std::max(maximum.x, rotated.x);
            maximum.y = std::max(maximum.y, rotated.y);
            maximum.z = std::max(maximum.z, rotated.z);
        }
    }

    const float span_x = std::max(0.001F, maximum.x - minimum.x);
    const float span_y = std::max(0.001F, maximum.y - minimum.y);
    const float margin = std::clamp(options.margin, 0.1F, 1.0F);
    const float scale = std::min(static_cast<float>(options.width) / span_x, static_cast<float>(options.height) / span_y) * margin;
    const float center_x = (minimum.x + maximum.x) * 0.5F;
    const float center_y = (minimum.y + maximum.y) * 0.5F;

    tpl::DecodedImage output{};
    output.width = std::max<std::uint16_t>(1U, options.width);
    output.height = std::max<std::uint16_t>(1U, options.height);
    output.rgba8.assign(static_cast<std::size_t>(output.width) * output.height * 4U, 0U);
    std::vector<float> depth_buffer(static_cast<std::size_t>(output.width) * output.height, std::numeric_limits<float>::infinity());

    const Vec3f light = rotate_point(Vec3f{.x = -0.35F, .y = 0.45F, .z = 0.82F}, options.yaw_degrees, options.pitch_degrees);
    for (const auto &triangle : *triangles) {
        struct ProjectedVertex {
            float x{};
            float y{};
            float z{};
            Vec3f normal{};
            Vec2f texcoord{};
            Rgba color{};
            Rgba material_color{};
            std::size_t texture_index{};
        };

        const auto project = [&](const Vertex &vertex) {
            const auto rotated_position = rotate_point(vertex.position, options.yaw_degrees, options.pitch_degrees);
            const auto rotated_normal = rotate_point(vertex.normal, options.yaw_degrees, options.pitch_degrees);
            return ProjectedVertex{
                .x = (rotated_position.x - center_x) * scale + static_cast<float>(output.width) * 0.5F,
                .y = static_cast<float>(output.height) * 0.5F - (rotated_position.y - center_y) * scale,
                .z = -rotated_position.z,
                .normal = rotated_normal,
                .texcoord = vertex.texcoord,
                .color = vertex.color,
                .material_color = vertex.material_color,
                .texture_index = vertex.texture_index,
            };
        };

        const auto p0 = project(triangle.v0);
        const auto p1 = project(triangle.v1);
        const auto p2 = project(triangle.v2);
        const float area = edge_function(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y);
        if (std::fabs(area) < 0.00001F) {
            continue;
        }

        const int min_x = std::max(0, static_cast<int>(std::floor(std::min({p0.x, p1.x, p2.x}))));
        const int max_x = std::min<int>(output.width - 1, static_cast<int>(std::ceil(std::max({p0.x, p1.x, p2.x}))));
        const int min_y = std::max(0, static_cast<int>(std::floor(std::min({p0.y, p1.y, p2.y}))));
        const int max_y = std::min<int>(output.height - 1, static_cast<int>(std::ceil(std::max({p0.y, p1.y, p2.y}))));

        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                const float px = static_cast<float>(x) + 0.5F;
                const float py = static_cast<float>(y) + 0.5F;
                const float w0 = edge_function(p1.x, p1.y, p2.x, p2.y, px, py) / area;
                const float w1 = edge_function(p2.x, p2.y, p0.x, p0.y, px, py) / area;
                const float w2 = edge_function(p0.x, p0.y, p1.x, p1.y, px, py) / area;
                if (w0 < -0.0001F || w1 < -0.0001F || w2 < -0.0001F) {
                    continue;
                }

                const float depth = p0.z * w0 + p1.z * w1 + p2.z * w2;
                const auto pixel_index = static_cast<std::size_t>(y) * output.width + static_cast<std::size_t>(x);
                if (depth >= depth_buffer[pixel_index]) {
                    continue;
                }

                const auto texcoord = Vec2f{
                    .x = p0.texcoord.x * w0 + p1.texcoord.x * w1 + p2.texcoord.x * w2,
                    .y = p0.texcoord.y * w0 + p1.texcoord.y * w1 + p2.texcoord.y * w2,
                };
                const auto vertex_color = Rgba{
                    .r = (p0.color.r * p0.material_color.r) * w0 + (p1.color.r * p1.material_color.r) * w1 +
                         (p2.color.r * p2.material_color.r) * w2,
                    .g = (p0.color.g * p0.material_color.g) * w0 + (p1.color.g * p1.material_color.g) * w1 +
                         (p2.color.g * p2.material_color.g) * w2,
                    .b = (p0.color.b * p0.material_color.b) * w0 + (p1.color.b * p1.material_color.b) * w1 +
                         (p2.color.b * p2.material_color.b) * w2,
                    .a = (p0.color.a * p0.material_color.a) * w0 + (p1.color.a * p1.material_color.a) * w1 +
                         (p2.color.a * p2.material_color.a) * w2,
                };
                const auto normal = Vec3f{
                    .x = p0.normal.x * w0 + p1.normal.x * w1 + p2.normal.x * w2,
                    .y = p0.normal.y * w0 + p1.normal.y * w1 + p2.normal.y * w2,
                    .z = p0.normal.z * w0 + p1.normal.z * w1 + p2.normal.z * w2,
                };
                const float normal_length = std::max(0.001F, std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z));
                const float diffuse =
                    std::clamp((normal.x * light.x + normal.y * light.y + normal.z * light.z) / normal_length, 0.0F, 1.0F);
                const float shade = options.ambient_light + options.diffuse_light * diffuse;
                const auto texture_color = sample_texture(*textures, p0.texture_index, texcoord.x, texcoord.y);
                if (texture_color.a <= 0.01F) {
                    continue;
                }

                depth_buffer[pixel_index] = depth;
                const auto rgba_index = pixel_index * 4U;
                output.rgba8[rgba_index + 0U] = clamp_u8(texture_color.r * vertex_color.r * shade * options.color_scale_r * 255.0F);
                output.rgba8[rgba_index + 1U] = clamp_u8(texture_color.g * vertex_color.g * shade * options.color_scale_g * 255.0F);
                output.rgba8[rgba_index + 2U] = clamp_u8(texture_color.b * vertex_color.b * shade * options.color_scale_b * 255.0F);
                output.rgba8[rgba_index + 3U] = clamp_u8(texture_color.a * vertex_color.a * 255.0F);
            }
        }
    }

    return output;
}

}  // namespace smgpc::assets::layout
