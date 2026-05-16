#include "J3dModel.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Binary.hpp"

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
        constexpr std::uint16_t J3D_INVALID_TABLE_INDEX = 0xFFFFU;

        struct Section {
            std::size_t offset{};
            std::size_t size{};

            [[nodiscard]] std::size_t end() const {
                return offset + size;
            }
        };

        struct ArrayRange {
            std::size_t begin{};
            std::size_t end{};
            bool present{};
        };

        struct VertexFormat {
            std::uint32_t position_type{GX_S16};
            std::uint8_t position_frac{};
            std::uint8_t position_stride{6U};
            std::uint8_t normal_frac{};
            std::uint8_t texcoord_frac{};
            std::uint32_t color_type{GX_RGB565};
            std::uint8_t color_stride{2U};
            bool has_position{};
        };

        struct VertexArrays {
            std::span< const std::byte > bytes{};
            Section section{};
            ArrayRange position{};
            ArrayRange normal{};
            ArrayRange color{};
            ArrayRange texcoord{};
            VertexFormat format{};
        };

        struct VtxDesc {
            std::uint32_t attr{};
            std::uint32_t type{};
        };

        struct HierarchyInfo {
            std::unordered_map< std::size_t, std::size_t > material_by_shape{};
            std::vector< std::uint16_t > joint_parents{};
        };

        [[nodiscard]] AssetError make_error(std::string message) {
            return AssetError{
                .code = AssetErrorCode::InvalidFormat,
                .message = std::move(message),
            };
        }

        [[nodiscard]] bool has_section_bytes(std::span< const std::byte > bytes, const Section& section, std::size_t offset, std::size_t size) {
            return offset >= section.offset && offset <= section.end() && size <= section.end() - offset && binary::has_bytes(bytes, offset, size);
        }

        [[nodiscard]] AssetResult< std::size_t > required_section_offset(const Section& section, std::uint32_t relative_offset,
                                                                         std::string_view label) {
            if (relative_offset == 0U || relative_offset >= section.size) {
                return make_error(std::string(label) + " offset exceeds section bounds.");
            }

            return section.offset + static_cast< std::size_t >(relative_offset);
        }

        [[nodiscard]] AssetResult< std::optional< std::size_t > > optional_section_offset(const Section& section, std::uint32_t relative_offset,
                                                                                          std::string_view label) {
            if (relative_offset == 0U) {
                return std::optional< std::size_t >{};
            }
            if (relative_offset >= section.size) {
                return make_error(std::string(label) + " offset exceeds section bounds.");
            }

            return std::optional< std::size_t >{section.offset + static_cast< std::size_t >(relative_offset)};
        }

        [[nodiscard]] AssetResult< std::vector< Section > > read_sections(std::span< const std::byte > bytes) {
            if (bytes.size() < 0x20U || !binary::fourcc_equals(bytes, 0U, "J3D2")) {
                return make_error("J3D file header is invalid.");
            }
            if (!binary::fourcc_equals(bytes, 4U, "bdl4")) {
                return make_error("J3D model parser only supports J3D2 bdl4 files.");
            }

            const auto file_size = static_cast< std::size_t >(binary::read_u32_be(bytes, 0x08U));
            if (file_size > bytes.size()) {
                return make_error("J3D file size exceeds buffer bounds.");
            }

            const auto section_count = static_cast< std::size_t >(binary::read_u32_be(bytes, 0x0CU));
            std::vector< Section > sections{};
            sections.reserve(section_count);

            std::size_t offset = 0x20U;
            for (std::size_t section_index = 0U; section_index < section_count; ++section_index) {
                if (!binary::has_bytes(bytes, offset, 8U)) {
                    return make_error("J3D section header exceeds file bounds.");
                }

                const auto section_size = static_cast< std::size_t >(binary::read_u32_be(bytes, offset + 4U));
                if (section_size < 8U || section_size > bytes.size() - offset || offset + section_size > file_size) {
                    return make_error("J3D section exceeds file bounds.");
                }

                sections.push_back(Section{
                    .offset = offset,
                    .size = section_size,
                });
                offset += section_size;
            }

            return sections;
        }

        [[nodiscard]] std::optional< Section > find_section(std::span< const std::byte > bytes, const std::vector< Section >& sections,
                                                            std::string_view name) {
            for (const auto& section : sections) {
                if (binary::fourcc_equals(bytes, section.offset, name)) {
                    return section;
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] AssetResult< Section > require_section(std::span< const std::byte > bytes, const std::vector< Section >& sections,
                                                             std::string_view name) {
            const auto section = find_section(bytes, sections, name);
            if (!section) {
                return make_error("J3D model is missing required " + std::string(name) + " section.");
            }

            return *section;
        }

        [[nodiscard]] std::string read_c_string_in_section(std::span< const std::byte > bytes, const Section& section, std::size_t offset) {
            if (!has_section_bytes(bytes, section, offset, 1U)) {
                return {};
            }

            std::string text{};
            for (std::size_t cursor = offset; cursor < section.end(); ++cursor) {
                const auto character = static_cast< char >(binary::read_u8(bytes, cursor));
                if (character == '\0') {
                    break;
                }
                text.push_back(character);
            }
            return text;
        }

        [[nodiscard]] std::string read_string_table_name(std::span< const std::byte > bytes, const Section& section, std::uint32_t relative_offset,
                                                         std::size_t index) {
            if (relative_offset == 0U) {
                return {};
            }

            const auto table_offset = section.offset + static_cast< std::size_t >(relative_offset);
            if (!has_section_bytes(bytes, section, table_offset, 4U)) {
                return {};
            }

            const auto count = static_cast< std::size_t >(binary::read_u16_be(bytes, table_offset));
            if (index >= count) {
                return {};
            }

            const auto entry_offset = table_offset + 4U + index * 4U;
            if (!has_section_bytes(bytes, section, entry_offset, 4U)) {
                return {};
            }

            const auto name_offset = static_cast< std::size_t >(binary::read_u16_be(bytes, entry_offset + 2U));
            return read_c_string_in_section(bytes, section, table_offset + name_offset);
        }

        [[nodiscard]] float decode_s16(std::span< const std::byte > bytes, std::size_t offset, std::uint8_t frac) {
            const auto raw = static_cast< std::int16_t >(binary::read_u16_be(bytes, offset));
            return static_cast< float >(raw) / static_cast< float >(1U << frac);
        }

        [[nodiscard]] bool is_valid_s16_frac(std::uint8_t frac) {
            return frac < 31U;
        }

        [[nodiscard]] J3dVec3 read_vec3_s16(std::span< const std::byte > bytes, std::size_t offset, std::uint8_t frac) {
            return J3dVec3{
                .x = decode_s16(bytes, offset + 0U, frac),
                .y = decode_s16(bytes, offset + 2U, frac),
                .z = decode_s16(bytes, offset + 4U, frac),
            };
        }

        [[nodiscard]] J3dVec3 read_vec3_f32(std::span< const std::byte > bytes, std::size_t offset) {
            return J3dVec3{
                .x = binary::read_f32_be(bytes, offset + 0U),
                .y = binary::read_f32_be(bytes, offset + 4U),
                .z = binary::read_f32_be(bytes, offset + 8U),
            };
        }

        [[nodiscard]] J3dVec2 read_vec2_s16(std::span< const std::byte > bytes, std::size_t offset, std::uint8_t frac) {
            return J3dVec2{
                .x = decode_s16(bytes, offset + 0U, frac),
                .y = decode_s16(bytes, offset + 2U, frac),
            };
        }

        [[nodiscard]] J3dColor read_rgb565(std::span< const std::byte > bytes, std::size_t offset) {
            const auto packed = binary::read_u16_be(bytes, offset);
            const auto expand5 = [](std::uint16_t value) { return static_cast< std::uint8_t >((value << 3U) | (value >> 2U)); };
            const auto expand6 = [](std::uint16_t value) { return static_cast< std::uint8_t >((value << 2U) | (value >> 4U)); };

            return J3dColor{
                .r = expand5(static_cast< std::uint16_t >((packed >> 11U) & 0x1FU)),
                .g = expand6(static_cast< std::uint16_t >((packed >> 5U) & 0x3FU)),
                .b = expand5(static_cast< std::uint16_t >(packed & 0x1FU)),
                .a = 255U,
            };
        }

        [[nodiscard]] J3dColor read_rgba8(std::span< const std::byte > bytes, std::size_t offset) {
            return J3dColor{
                .r = binary::read_u8(bytes, offset + 0U),
                .g = binary::read_u8(bytes, offset + 1U),
                .b = binary::read_u8(bytes, offset + 2U),
                .a = binary::read_u8(bytes, offset + 3U),
            };
        }

        [[nodiscard]] J3dTevStageInfoRaw read_tev_stage_info(std::span< const std::byte > bytes, std::size_t offset) {
            J3dTevStageInfoRaw stage{
                .valid = true,
            };

            for (std::size_t byte_index = 0U; byte_index < stage.bytes.size(); ++byte_index) {
                stage.bytes[byte_index] = binary::read_u8(bytes, offset + byte_index);
            }

            stage.color_args = J3dTevStageArgs{
                .a = stage.bytes[1U],
                .b = stage.bytes[2U],
                .c = stage.bytes[3U],
                .d = stage.bytes[4U],
            };
            stage.color_op = J3dTevStageOp{
                .op = stage.bytes[5U],
                .bias = stage.bytes[6U],
                .scale = stage.bytes[7U],
                .clamp = stage.bytes[8U],
                .output_register = stage.bytes[9U],
            };
            stage.alpha_args = J3dTevStageArgs{
                .a = stage.bytes[10U],
                .b = stage.bytes[11U],
                .c = stage.bytes[12U],
                .d = stage.bytes[13U],
            };
            stage.alpha_op = J3dTevStageOp{
                .op = stage.bytes[14U],
                .bias = stage.bytes[15U],
                .scale = stage.bytes[16U],
                .clamp = stage.bytes[17U],
                .output_register = stage.bytes[18U],
            };

            return stage;
        }

        [[nodiscard]] AssetResult< J3dVec3 > read_position(const VertexArrays& arrays, std::size_t index) {
            if (!arrays.position.present) {
                return make_error("VTX1 position array is missing.");
            }

            const auto offset = arrays.position.begin + index * arrays.format.position_stride;
            if (offset > arrays.position.end || arrays.format.position_stride > arrays.position.end - offset) {
                return make_error("SHP1 position index exceeds VTX1 position array bounds.");
            }

            if (arrays.format.position_type == GX_F32) {
                return read_vec3_f32(arrays.bytes, offset);
            }

            return read_vec3_s16(arrays.bytes, offset, arrays.format.position_frac);
        }

        [[nodiscard]] AssetResult< J3dVec3 > read_normal(const VertexArrays& arrays, std::size_t index) {
            if (!arrays.normal.present) {
                return make_error("SHP1 normal index references a missing VTX1 normal array.");
            }

            const auto offset = arrays.normal.begin + index * 6U;
            if (offset > arrays.normal.end || 6U > arrays.normal.end - offset) {
                return make_error("SHP1 normal index exceeds VTX1 normal array bounds.");
            }

            return read_vec3_s16(arrays.bytes, offset, arrays.format.normal_frac);
        }

        [[nodiscard]] AssetResult< J3dColor > read_color(const VertexArrays& arrays, std::size_t index) {
            if (!arrays.color.present) {
                return make_error("SHP1 color index references a missing VTX1 color array.");
            }

            const auto offset = arrays.color.begin + index * arrays.format.color_stride;
            if (offset > arrays.color.end || arrays.format.color_stride > arrays.color.end - offset) {
                return make_error("SHP1 color index exceeds VTX1 color array bounds.");
            }

            if (arrays.format.color_type == GX_RGBA8) {
                return read_rgba8(arrays.bytes, offset);
            }

            return read_rgb565(arrays.bytes, offset);
        }

        [[nodiscard]] AssetResult< J3dVec2 > read_texcoord(const VertexArrays& arrays, std::size_t index) {
            if (!arrays.texcoord.present) {
                return make_error("SHP1 texture coordinate index references a missing VTX1 texture coordinate array.");
            }

            const auto offset = arrays.texcoord.begin + index * 4U;
            if (offset > arrays.texcoord.end || 4U > arrays.texcoord.end - offset) {
                return make_error("SHP1 texture coordinate index exceeds VTX1 texture coordinate array bounds.");
            }

            return read_vec2_s16(arrays.bytes, offset, arrays.format.texcoord_frac);
        }

        [[nodiscard]] AssetResult< VertexArrays > parse_vertex_arrays(std::span< const std::byte > bytes, const Section& section) {
            if (!has_section_bytes(bytes, section, section.offset, 0x40U)) {
                return make_error("VTX1 section is too small.");
            }

            const auto format_offset = required_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x08U), "VTX1 format table");
            if (!format_offset) {
                return format_offset.failure();
            }

            std::vector< std::size_t > array_offsets{};
            array_offsets.reserve(12U);
            const auto read_array_offset = [&](std::size_t header_offset) -> AssetResult< std::optional< std::size_t > > {
                const auto relative_offset = binary::read_u32_be(bytes, section.offset + header_offset);
                auto absolute = optional_section_offset(section, relative_offset, "VTX1 array");
                if (!absolute) {
                    return absolute.failure();
                }
                if (*absolute) {
                    array_offsets.push_back(**absolute);
                }
                return *absolute;
            };

            const auto position_offset = read_array_offset(0x0CU);
            if (!position_offset) {
                return position_offset.failure();
            }
            const auto normal_offset = read_array_offset(0x10U);
            if (!normal_offset) {
                return normal_offset.failure();
            }
            const auto color_offset = read_array_offset(0x18U);
            if (!color_offset) {
                return color_offset.failure();
            }
            const auto texcoord_offset = read_array_offset(0x20U);
            if (!texcoord_offset) {
                return texcoord_offset.failure();
            }
            for (std::size_t header_offset : {0x14U, 0x1CU, 0x24U, 0x28U, 0x2CU, 0x30U, 0x34U, 0x38U, 0x3CU}) {
                auto ignored = read_array_offset(header_offset);
                if (!ignored) {
                    return ignored.failure();
                }
            }

            std::sort(array_offsets.begin(), array_offsets.end());
            array_offsets.erase(std::unique(array_offsets.begin(), array_offsets.end()), array_offsets.end());

            const auto make_range = [&](std::optional< std::size_t > offset) {
                if (!offset) {
                    return ArrayRange{};
                }

                auto next = std::find_if(array_offsets.begin(), array_offsets.end(), [&](std::size_t candidate) { return candidate > *offset; });
                return ArrayRange{
                    .begin = *offset,
                    .end = next == array_offsets.end() ? section.end() : *next,
                    .present = true,
                };
            };

            VertexArrays arrays{
                .bytes = bytes,
                .section = section,
                .position = make_range(*position_offset),
                .normal = make_range(*normal_offset),
                .color = make_range(*color_offset),
                .texcoord = make_range(*texcoord_offset),
            };

            bool found_terminator = false;
            for (std::size_t index = 0U; has_section_bytes(bytes, section, *format_offset + index * 16U, 16U); ++index) {
                const auto entry_offset = *format_offset + index * 16U;
                const auto attr = binary::read_u32_be(bytes, entry_offset + 0U);
                if (attr == GX_VA_NULL) {
                    found_terminator = true;
                    break;
                }

                const auto count = binary::read_u32_be(bytes, entry_offset + 4U);
                const auto type = binary::read_u32_be(bytes, entry_offset + 8U);
                const auto frac = binary::read_u8(bytes, entry_offset + 12U);

                if (attr == GX_VA_POS) {
                    if ((type != GX_S16 && type != GX_F32) || count != 1U || (type == GX_S16 && !is_valid_s16_frac(frac))) {
                        return make_error("VTX1 only supports S16/F32 XYZ positions.");
                    }
                    arrays.format.position_type = type;
                    arrays.format.position_frac = frac;
                    arrays.format.position_stride = type == GX_F32 ? 12U : 6U;
                    arrays.format.has_position = true;
                } else if (attr == GX_VA_NRM) {
                    if (type != GX_S16 || count != 0U || !is_valid_s16_frac(frac)) {
                        return make_error("VTX1 only supports S16 XYZ normals.");
                    }
                    arrays.format.normal_frac = frac;
                } else if (attr == GX_VA_CLR0) {
                    if (type != GX_RGB565 && type != GX_RGBA8) {
                        return make_error("VTX1 only supports RGB565/RGBA8 vertex colors.");
                    }
                    arrays.format.color_type = type;
                    arrays.format.color_stride = type == GX_RGBA8 ? 4U : 2U;
                } else if (attr == GX_VA_TEX0) {
                    if (type != GX_S16 || count != 1U || !is_valid_s16_frac(frac)) {
                        return make_error("VTX1 only supports S16 ST texture coordinates.");
                    }
                    arrays.format.texcoord_frac = frac;
                }
            }

            if (!found_terminator) {
                return make_error("VTX1 format table is unterminated.");
            }
            if (!arrays.format.has_position || !arrays.position.present) {
                return make_error("VTX1 position format or array is missing.");
            }

            return arrays;
        }

        [[nodiscard]] AssetResult< std::vector< J3dMaterial > > parse_materials(std::span< const std::byte > bytes, const Section& section,
                                                                                std::size_t texture_count) {
            if (!has_section_bytes(bytes, section, section.offset, 0x78U)) {
                return make_error("MAT3 section is too small.");
            }

            const auto material_count = static_cast< std::size_t >(binary::read_u16_be(bytes, section.offset + 0x08U));
            const auto material_init_offset =
                required_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x0CU), "MAT3 material init table");
            if (!material_init_offset) {
                return material_init_offset.failure();
            }
            const auto material_id_offset =
                required_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x10U), "MAT3 material ID table");
            if (!material_id_offset) {
                return material_id_offset.failure();
            }
            const auto name_table_offset = binary::read_u32_be(bytes, section.offset + 0x14U);
            const auto ind_init_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x18U), "MAT3 indirect init table");
            if (!ind_init_offset) {
                return ind_init_offset.failure();
            }
            const auto cull_mode_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x1CU), "MAT3 cull mode table");
            if (!cull_mode_offset) {
                return cull_mode_offset.failure();
            }
            const auto mat_color_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x20U), "MAT3 material color table");
            if (!mat_color_offset) {
                return mat_color_offset.failure();
            }
            const auto color_chan_num_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x24U), "MAT3 color channel count table");
            if (!color_chan_num_offset) {
                return color_chan_num_offset.failure();
            }
            const auto color_chan_info_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x28U), "MAT3 color channel info table");
            if (!color_chan_info_offset) {
                return color_chan_info_offset.failure();
            }
            const auto tex_gen_num_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x34U), "MAT3 texture generator count table");
            if (!tex_gen_num_offset) {
                return tex_gen_num_offset.failure();
            }
            const auto tex_coord_info_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x38U), "MAT3 texture coordinate info table");
            if (!tex_coord_info_offset) {
                return tex_coord_info_offset.failure();
            }
            const auto tex_mtx_info_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x40U), "MAT3 texture matrix info table");
            if (!tex_mtx_info_offset) {
                return tex_mtx_info_offset.failure();
            }
            const auto tex_no_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x48U), "MAT3 texture number table");
            if (!tex_no_offset) {
                return tex_no_offset.failure();
            }
            const auto tev_order_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x4CU), "MAT3 TEV order table");
            if (!tev_order_offset) {
                return tev_order_offset.failure();
            }
            const auto tev_color_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x50U), "MAT3 TEV color table");
            if (!tev_color_offset) {
                return tev_color_offset.failure();
            }
            const auto tev_k_color_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x54U), "MAT3 TEV K color table");
            if (!tev_k_color_offset) {
                return tev_k_color_offset.failure();
            }
            const auto tev_stage_num_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x58U), "MAT3 TEV stage count table");
            if (!tev_stage_num_offset) {
                return tev_stage_num_offset.failure();
            }
            const auto tev_stage_info_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x5CU), "MAT3 TEV stage info table");
            if (!tev_stage_info_offset) {
                return tev_stage_info_offset.failure();
            }
            const auto tev_swap_mode_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x60U), "MAT3 TEV swap mode table");
            if (!tev_swap_mode_offset) {
                return tev_swap_mode_offset.failure();
            }
            const auto tev_swap_mode_table_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x64U), "MAT3 TEV swap mode table info");
            if (!tev_swap_mode_table_offset) {
                return tev_swap_mode_table_offset.failure();
            }
            const auto alpha_compare_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x6CU), "MAT3 alpha compare table");
            if (!alpha_compare_offset) {
                return alpha_compare_offset.failure();
            }
            const auto blend_mode_offset =
                optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x70U), "MAT3 blend mode table");
            if (!blend_mode_offset) {
                return blend_mode_offset.failure();
            }
            const auto z_mode_offset = optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x74U), "MAT3 Z mode table");
            if (!z_mode_offset) {
                return z_mode_offset.failure();
            }

            if (!has_section_bytes(bytes, section, *material_id_offset, material_count * 2U)) {
                return make_error("MAT3 material ID table exceeds section bounds.");
            }

            std::vector< J3dMaterial > materials{};
            materials.reserve(material_count);
            for (std::size_t material_index = 0U; material_index < material_count; ++material_index) {
                const auto material_id = static_cast< std::size_t >(binary::read_u16_be(bytes, *material_id_offset + material_index * 2U));
                const auto init_offset = *material_init_offset + material_id * 0x14CU;
                if (!has_section_bytes(bytes, section, init_offset, 0x14CU)) {
                    return make_error("MAT3 material init data exceeds section bounds.");
                }

                J3dMaterial material{
                    .name = read_string_table_name(bytes, section, name_table_offset, material_index),
                };
                material.material_mode = binary::read_u8(bytes, init_offset + 0x00U);

                const auto color_chan_num_index = binary::read_u8(bytes, init_offset + 0x02U);
                if (color_chan_num_index != 0xFFU) {
                    if (!*color_chan_num_offset) {
                        return make_error("MAT3 material references a missing color channel count table.");
                    }
                    const auto color_chan_num_entry = **color_chan_num_offset + static_cast< std::size_t >(color_chan_num_index);
                    if (!has_section_bytes(bytes, section, color_chan_num_entry, 1U)) {
                        return make_error("MAT3 color channel count table exceeds section bounds.");
                    }
                    material.color_channel_count = binary::read_u8(bytes, color_chan_num_entry);
                }

                const auto cull_mode_index = binary::read_u8(bytes, init_offset + 0x01U);
                if (cull_mode_index != 0xFFU) {
                    if (!*cull_mode_offset) {
                        return make_error("MAT3 material references a missing cull mode table.");
                    }
                    const auto cull_mode_entry = **cull_mode_offset + static_cast< std::size_t >(cull_mode_index);
                    if (!has_section_bytes(bytes, section, cull_mode_entry, 1U)) {
                        return make_error("MAT3 cull mode table exceeds section bounds.");
                    }
                    material.cull_mode = binary::read_u8(bytes, cull_mode_entry);
                }

                const auto tex_gen_num_index = binary::read_u8(bytes, init_offset + 0x03U);
                if (tex_gen_num_index != 0xFFU) {
                    if (!*tex_gen_num_offset) {
                        return make_error("MAT3 material references a missing texture generator count table.");
                    }
                    const auto tex_gen_num_entry = **tex_gen_num_offset + static_cast< std::size_t >(tex_gen_num_index);
                    if (!has_section_bytes(bytes, section, tex_gen_num_entry, 1U)) {
                        return make_error("MAT3 texture generator count table exceeds section bounds.");
                    }
                    material.texture_generator_count = binary::read_u8(bytes, tex_gen_num_entry);
                }

                const auto tev_stage_num_index = binary::read_u8(bytes, init_offset + 0x04U);
                if (tev_stage_num_index != 0xFFU) {
                    if (!*tev_stage_num_offset) {
                        return make_error("MAT3 material references a missing TEV stage count table.");
                    }
                    const auto tev_stage_num_entry = **tev_stage_num_offset + static_cast< std::size_t >(tev_stage_num_index);
                    if (!has_section_bytes(bytes, section, tev_stage_num_entry, 1U)) {
                        return make_error("MAT3 TEV stage count table exceeds section bounds.");
                    }
                    material.tev_stage_count = binary::read_u8(bytes, tev_stage_num_entry);
                }

                const auto mat_color_index = binary::read_u16_be(bytes, init_offset + 0x08U);
                if (mat_color_index != J3D_INVALID_TABLE_INDEX) {
                    if (!*mat_color_offset) {
                        return make_error("MAT3 material references a missing material color table.");
                    }
                    const auto mat_color_entry = **mat_color_offset + static_cast< std::size_t >(mat_color_index) * 4U;
                    if (!has_section_bytes(bytes, section, mat_color_entry, 4U)) {
                        return make_error("MAT3 material color table exceeds section bounds.");
                    }
                    material.material_color = read_rgba8(bytes, mat_color_entry);
                }

                if (*ind_init_offset) {
                    constexpr std::size_t J3D_IND_INIT_SIZE = 0x138U;
                    const auto indirect_init_entry = **ind_init_offset + material_index * J3D_IND_INIT_SIZE;
                    if (!has_section_bytes(bytes, section, indirect_init_entry, J3D_IND_INIT_SIZE)) {
                        return make_error("MAT3 indirect init table exceeds section bounds.");
                    }

                    const bool indirect_enabled = binary::read_u8(bytes, indirect_init_entry + 0x00U) != 0U;
                    if (indirect_enabled) {
                        material.indirect_texture_stage_count =
                            std::min< std::uint8_t >(binary::read_u8(bytes, indirect_init_entry + 0x01U), 3U);

                        for (std::size_t slot = 0U; slot < material.indirect_texture_orders.size(); ++slot) {
                            const auto entry = indirect_init_entry + 0x04U + slot * 4U;
                            material.indirect_texture_orders[slot] = J3dIndirectTextureOrder{
                                .valid = slot < material.indirect_texture_stage_count,
                                .texture_coordinate = binary::read_u8(bytes, entry + 0U),
                                .texture_map = binary::read_u8(bytes, entry + 1U),
                                .raw =
                                    {
                                        binary::read_u8(bytes, entry + 0U),
                                        binary::read_u8(bytes, entry + 1U),
                                        binary::read_u8(bytes, entry + 2U),
                                        binary::read_u8(bytes, entry + 3U),
                                    },
                            };
                        }

                        for (std::size_t slot = 0U; slot < material.indirect_texture_matrices.size(); ++slot) {
                            const auto entry = indirect_init_entry + 0x14U + slot * 0x1CU;
                            J3dIndirectTextureMatrix matrix{
                                .valid = slot < material.indirect_texture_stage_count,
                                .values =
                                    {
                                        binary::read_f32_be(bytes, entry + 0x00U),
                                        binary::read_f32_be(bytes, entry + 0x04U),
                                        binary::read_f32_be(bytes, entry + 0x08U),
                                        binary::read_f32_be(bytes, entry + 0x0CU),
                                        binary::read_f32_be(bytes, entry + 0x10U),
                                        binary::read_f32_be(bytes, entry + 0x14U),
                                    },
                                .scale_exponent = static_cast< std::int8_t >(binary::read_u8(bytes, entry + 0x18U)),
                            };
                            for (std::size_t byte_index = 0U; byte_index < matrix.raw.size(); ++byte_index) {
                                matrix.raw[byte_index] = binary::read_u8(bytes, entry + byte_index);
                            }
                            material.indirect_texture_matrices[slot] = matrix;
                        }

                        for (std::size_t slot = 0U; slot < material.indirect_texture_coord_scales.size(); ++slot) {
                            const auto entry = indirect_init_entry + 0x68U + slot * 4U;
                            material.indirect_texture_coord_scales[slot] = J3dIndirectTextureCoordScale{
                                .valid = slot < material.indirect_texture_stage_count,
                                .scale_s = binary::read_u8(bytes, entry + 0U),
                                .scale_t = binary::read_u8(bytes, entry + 1U),
                                .raw =
                                    {
                                        binary::read_u8(bytes, entry + 0U),
                                        binary::read_u8(bytes, entry + 1U),
                                        binary::read_u8(bytes, entry + 2U),
                                        binary::read_u8(bytes, entry + 3U),
                                    },
                            };
                        }

                        for (std::size_t slot = 0U; slot < material.indirect_tev_stages.size(); ++slot) {
                            const auto entry = indirect_init_entry + 0x78U + slot * 0x0CU;
                            J3dIndirectTevStage stage{
                                .valid = true,
                                .ind_stage = binary::read_u8(bytes, entry + 0U),
                                .format = binary::read_u8(bytes, entry + 1U),
                                .bias = binary::read_u8(bytes, entry + 2U),
                                .matrix = binary::read_u8(bytes, entry + 3U),
                                .wrap_s = binary::read_u8(bytes, entry + 4U),
                                .wrap_t = binary::read_u8(bytes, entry + 5U),
                                .add_prev = binary::read_u8(bytes, entry + 6U),
                                .use_original_lod = binary::read_u8(bytes, entry + 7U),
                                .alpha = binary::read_u8(bytes, entry + 8U),
                            };
                            for (std::size_t byte_index = 0U; byte_index < stage.raw.size(); ++byte_index) {
                                stage.raw[byte_index] = binary::read_u8(bytes, entry + byte_index);
                            }
                            material.indirect_tev_stages[slot] = stage;
                        }
                    }
                }

                for (std::size_t slot = 0U; slot < material.color_channels.size(); ++slot) {
                    const auto color_chan_index = binary::read_u16_be(bytes, init_offset + 0x0CU + slot * 2U);
                    if (color_chan_index == J3D_INVALID_TABLE_INDEX) {
                        continue;
                    }
                    if (!*color_chan_info_offset) {
                        return make_error("MAT3 material references a missing color channel info table.");
                    }

                    const auto color_chan_entry = **color_chan_info_offset + static_cast< std::size_t >(color_chan_index) * 8U;
                    if (!has_section_bytes(bytes, section, color_chan_entry, 8U)) {
                        return make_error("MAT3 color channel info table exceeds section bounds.");
                    }

                    material.color_channels[slot] = J3dColorChannelInfo{
                        .valid = true,
                        .enable = binary::read_u8(bytes, color_chan_entry + 0U),
                        .material_source = binary::read_u8(bytes, color_chan_entry + 1U),
                        .light_mask = binary::read_u8(bytes, color_chan_entry + 2U),
                        .diffuse_function = binary::read_u8(bytes, color_chan_entry + 3U),
                        .attenuation_function = binary::read_u8(bytes, color_chan_entry + 4U),
                        .ambient_source = binary::read_u8(bytes, color_chan_entry + 5U),
                    };
                }

                for (std::size_t slot = 0U; slot < material.tev_k_colors.size(); ++slot) {
                    const auto tev_color_index = binary::read_u16_be(bytes, init_offset + 0xDCU + slot * 2U);
                    if (tev_color_index != J3D_INVALID_TABLE_INDEX) {
                        if (!*tev_color_offset) {
                            return make_error("MAT3 material references a missing TEV color table.");
                        }

                        const auto tev_color_entry = **tev_color_offset + static_cast< std::size_t >(tev_color_index) * 8U;
                        if (!has_section_bytes(bytes, section, tev_color_entry, 8U)) {
                            return make_error("MAT3 TEV color table exceeds section bounds.");
                        }

                        material.tev_colors[slot] = J3dTevColorS10{
                            .valid = true,
                            .r = static_cast< std::int16_t >(binary::read_u16_be(bytes, tev_color_entry + 0U)),
                            .g = static_cast< std::int16_t >(binary::read_u16_be(bytes, tev_color_entry + 2U)),
                            .b = static_cast< std::int16_t >(binary::read_u16_be(bytes, tev_color_entry + 4U)),
                            .a = static_cast< std::int16_t >(binary::read_u16_be(bytes, tev_color_entry + 6U)),
                        };
                    }

                    const auto tev_k_color_index = binary::read_u16_be(bytes, init_offset + 0x94U + slot * 2U);
                    if (tev_k_color_index != J3D_INVALID_TABLE_INDEX) {
                        if (!*tev_k_color_offset) {
                            return make_error("MAT3 material references a missing TEV K color table.");
                        }

                        const auto tev_k_color_entry = **tev_k_color_offset + static_cast< std::size_t >(tev_k_color_index) * 4U;
                        if (!has_section_bytes(bytes, section, tev_k_color_entry, 4U)) {
                            return make_error("MAT3 TEV K color table exceeds section bounds.");
                        }

                        material.tev_k_colors[slot] = J3dTevKColor{
                            .valid = true,
                            .r = binary::read_u8(bytes, tev_k_color_entry + 0U),
                            .g = binary::read_u8(bytes, tev_k_color_entry + 1U),
                            .b = binary::read_u8(bytes, tev_k_color_entry + 2U),
                            .a = binary::read_u8(bytes, tev_k_color_entry + 3U),
                        };
                    }
                }

                for (std::size_t slot = 0U; slot < material.tev_k_color_selectors.size(); ++slot) {
                    material.tev_k_color_selectors[slot] = binary::read_u8(bytes, init_offset + 0x9CU + slot);
                    material.tev_k_alpha_selectors[slot] = binary::read_u8(bytes, init_offset + 0xACU + slot);
                }

                for (std::size_t slot = 0U; slot < material.texture_indices.size(); ++slot) {
                    const auto tex_coord_index = binary::read_u16_be(bytes, init_offset + 0x28U + slot * 2U);
                    if (tex_coord_index != J3D_INVALID_TABLE_INDEX) {
                        if (!*tex_coord_info_offset) {
                            return make_error("MAT3 material references a missing texture coordinate info table.");
                        }

                        const auto tex_coord_entry = **tex_coord_info_offset + static_cast< std::size_t >(tex_coord_index) * 4U;
                        if (!has_section_bytes(bytes, section, tex_coord_entry, 4U)) {
                            return make_error("MAT3 texture coordinate info table exceeds section bounds.");
                        }
                        material.texture_coord_generators[slot] = J3dTextureCoordGen{
                            .valid = true,
                            .type = binary::read_u8(bytes, tex_coord_entry + 0U),
                            .source = binary::read_u8(bytes, tex_coord_entry + 1U),
                            .matrix = binary::read_u8(bytes, tex_coord_entry + 2U),
                        };
                    }

                    const auto tex_mtx_index = binary::read_u16_be(bytes, init_offset + 0x48U + slot * 2U);
                    if (tex_mtx_index != J3D_INVALID_TABLE_INDEX) {
                        if (!*tex_mtx_info_offset) {
                            return make_error("MAT3 material references a missing texture matrix info table.");
                        }

                        const auto tex_mtx_entry = **tex_mtx_info_offset + static_cast< std::size_t >(tex_mtx_index) * 0x64U;
                        if (!has_section_bytes(bytes, section, tex_mtx_entry, 0x64U)) {
                            return make_error("MAT3 texture matrix info table exceeds section bounds.");
                        }
                        J3dTextureMatrix texture_matrix{
                            .valid = true,
                            .projection = binary::read_u8(bytes, tex_mtx_entry + 0x00U),
                            .info = binary::read_u8(bytes, tex_mtx_entry + 0x01U),
                            .center = read_vec3_f32(bytes, tex_mtx_entry + 0x04U),
                            .srt =
                                J3dTextureSrt{
                                    .scale_x = binary::read_f32_be(bytes, tex_mtx_entry + 0x10U),
                                    .scale_y = binary::read_f32_be(bytes, tex_mtx_entry + 0x14U),
                                    .rotation = static_cast< std::int16_t >(binary::read_u16_be(bytes, tex_mtx_entry + 0x18U)),
                                    .translation_x = binary::read_f32_be(bytes, tex_mtx_entry + 0x1CU),
                                    .translation_y = binary::read_f32_be(bytes, tex_mtx_entry + 0x20U),
                                },
                        };
                        for (std::size_t row = 0U; row < 4U; ++row) {
                            for (std::size_t column = 0U; column < 4U; ++column) {
                                texture_matrix.effect_matrix[row * 4U + column] =
                                    binary::read_f32_be(bytes, tex_mtx_entry + 0x24U + (row * 4U + column) * 4U);
                            }
                        }
                        material.texture_matrices[slot] = texture_matrix;
                    }

                    const auto tex_no_index = binary::read_u16_be(bytes, init_offset + 0x84U + slot * 2U);
                    if (tex_no_index == J3D_INVALID_TABLE_INDEX) {
                        continue;
                    }
                    if (!*tex_no_offset) {
                        return make_error("MAT3 material references a missing texture number table.");
                    }

                    const auto tex_no_entry = **tex_no_offset + static_cast< std::size_t >(tex_no_index) * 2U;
                    if (!has_section_bytes(bytes, section, tex_no_entry, 2U)) {
                        return make_error("MAT3 texture number table exceeds section bounds.");
                    }

                    const auto texture_index = binary::read_u16_be(bytes, tex_no_entry);
                    if (static_cast< std::size_t >(texture_index) >= texture_count) {
                        return make_error("MAT3 texture slot references a missing TEX1 texture.");
                    }
                    material.texture_indices[slot] = texture_index;
                }

                for (std::size_t slot = 0U; slot < material.tev_orders.size(); ++slot) {
                    const auto tev_order_index = binary::read_u16_be(bytes, init_offset + 0xBCU + slot * 2U);
                    if (tev_order_index == J3D_INVALID_TABLE_INDEX) {
                        continue;
                    }
                    if (!*tev_order_offset) {
                        return make_error("MAT3 material references a missing TEV order table.");
                    }

                    const auto tev_order_entry = **tev_order_offset + static_cast< std::size_t >(tev_order_index) * 4U;
                    if (!has_section_bytes(bytes, section, tev_order_entry, 4U)) {
                        return make_error("MAT3 TEV order table exceeds section bounds.");
                    }

                    material.tev_orders[slot] = J3dTevOrder{
                        .valid = true,
                        .texture_coordinate = binary::read_u8(bytes, tev_order_entry + 0U),
                        .texture_map = binary::read_u8(bytes, tev_order_entry + 1U),
                        .color_channel = binary::read_u8(bytes, tev_order_entry + 2U),
                    };
                }

                for (std::size_t slot = 0U; slot < material.tev_stages.size(); ++slot) {
                    const auto tev_stage_index = binary::read_u16_be(bytes, init_offset + 0xE4U + slot * 2U);
                    if (tev_stage_index != J3D_INVALID_TABLE_INDEX) {
                        if (!*tev_stage_info_offset) {
                            return make_error("MAT3 material references a missing TEV stage info table.");
                        }

                        const auto tev_stage_entry = **tev_stage_info_offset + static_cast< std::size_t >(tev_stage_index) * 0x14U;
                        if (!has_section_bytes(bytes, section, tev_stage_entry, 0x14U)) {
                            return make_error("MAT3 TEV stage info table exceeds section bounds.");
                        }

                        material.tev_stages[slot] = read_tev_stage_info(bytes, tev_stage_entry);
                    }

                    const auto tev_swap_mode_index = binary::read_u16_be(bytes, init_offset + 0x104U + slot * 2U);
                    if (tev_swap_mode_index != J3D_INVALID_TABLE_INDEX) {
                        if (!*tev_swap_mode_offset) {
                            return make_error("MAT3 material references a missing TEV swap mode table.");
                        }

                        const auto tev_swap_mode_entry = **tev_swap_mode_offset + static_cast< std::size_t >(tev_swap_mode_index) * 4U;
                        if (!has_section_bytes(bytes, section, tev_swap_mode_entry, 4U)) {
                            return make_error("MAT3 TEV swap mode table exceeds section bounds.");
                        }

                        material.tev_swap_modes[slot] = J3dTevSwapMode{
                            .valid = true,
                            .ras_sel = binary::read_u8(bytes, tev_swap_mode_entry + 0U),
                            .tex_sel = binary::read_u8(bytes, tev_swap_mode_entry + 1U),
                            .raw =
                                {
                                    binary::read_u8(bytes, tev_swap_mode_entry + 0U),
                                    binary::read_u8(bytes, tev_swap_mode_entry + 1U),
                                    binary::read_u8(bytes, tev_swap_mode_entry + 2U),
                                    binary::read_u8(bytes, tev_swap_mode_entry + 3U),
                                },
                        };
                    }
                }

                for (std::size_t slot = 0U; slot < material.tev_swap_mode_tables.size(); ++slot) {
                    const auto tev_swap_mode_table_index = binary::read_u16_be(bytes, init_offset + 0x124U + slot * 2U);
                    if (tev_swap_mode_table_index == J3D_INVALID_TABLE_INDEX) {
                        continue;
                    }
                    if (!*tev_swap_mode_table_offset) {
                        return make_error("MAT3 material references a missing TEV swap mode table info table.");
                    }

                    const auto tev_swap_mode_table_entry = **tev_swap_mode_table_offset + static_cast< std::size_t >(tev_swap_mode_table_index) * 4U;
                    if (!has_section_bytes(bytes, section, tev_swap_mode_table_entry, 4U)) {
                        return make_error("MAT3 TEV swap mode table info exceeds section bounds.");
                    }

                    material.tev_swap_mode_tables[slot] = J3dTevSwapModeTable{
                        .valid = true,
                        .channels =
                            {
                                binary::read_u8(bytes, tev_swap_mode_table_entry + 0U),
                                binary::read_u8(bytes, tev_swap_mode_table_entry + 1U),
                                binary::read_u8(bytes, tev_swap_mode_table_entry + 2U),
                                binary::read_u8(bytes, tev_swap_mode_table_entry + 3U),
                            },
                    };
                }

                const auto alpha_compare_index = binary::read_u16_be(bytes, init_offset + 0x146U);
                if (alpha_compare_index != J3D_INVALID_TABLE_INDEX) {
                    if (!*alpha_compare_offset) {
                        return make_error("MAT3 material references a missing alpha compare table.");
                    }

                    const auto alpha_compare_entry = **alpha_compare_offset + static_cast< std::size_t >(alpha_compare_index) * 8U;
                    if (!has_section_bytes(bytes, section, alpha_compare_entry, 8U)) {
                        return make_error("MAT3 alpha compare table exceeds section bounds.");
                    }

                    material.alpha_compare = J3dAlphaCompare{
                        .valid = true,
                        .comp0 = binary::read_u8(bytes, alpha_compare_entry + 0U),
                        .ref0 = binary::read_u8(bytes, alpha_compare_entry + 1U),
                        .op = binary::read_u8(bytes, alpha_compare_entry + 2U),
                        .comp1 = binary::read_u8(bytes, alpha_compare_entry + 3U),
                        .ref1 = binary::read_u8(bytes, alpha_compare_entry + 4U),
                        .raw =
                            {
                                binary::read_u8(bytes, alpha_compare_entry + 0U),
                                binary::read_u8(bytes, alpha_compare_entry + 1U),
                                binary::read_u8(bytes, alpha_compare_entry + 2U),
                                binary::read_u8(bytes, alpha_compare_entry + 3U),
                                binary::read_u8(bytes, alpha_compare_entry + 4U),
                                binary::read_u8(bytes, alpha_compare_entry + 5U),
                                binary::read_u8(bytes, alpha_compare_entry + 6U),
                                binary::read_u8(bytes, alpha_compare_entry + 7U),
                            },
                    };
                }

                const auto blend_mode_index = binary::read_u16_be(bytes, init_offset + 0x148U);
                if (blend_mode_index != J3D_INVALID_TABLE_INDEX) {
                    if (!*blend_mode_offset) {
                        return make_error("MAT3 material references a missing blend mode table.");
                    }

                    const auto blend_mode_entry = **blend_mode_offset + static_cast< std::size_t >(blend_mode_index) * 4U;
                    if (!has_section_bytes(bytes, section, blend_mode_entry, 4U)) {
                        return make_error("MAT3 blend mode table exceeds section bounds.");
                    }

                    material.blend = J3dBlendMode{
                        .valid = true,
                        .type = binary::read_u8(bytes, blend_mode_entry + 0U),
                        .source_factor = binary::read_u8(bytes, blend_mode_entry + 1U),
                        .destination_factor = binary::read_u8(bytes, blend_mode_entry + 2U),
                        .operation = binary::read_u8(bytes, blend_mode_entry + 3U),
                    };
                }

                const auto z_mode_index = binary::read_u8(bytes, init_offset + 0x06U);
                if (z_mode_index != 0xFFU) {
                    if (!*z_mode_offset) {
                        return make_error("MAT3 material references a missing Z mode table.");
                    }

                    const auto z_mode_entry = **z_mode_offset + static_cast< std::size_t >(z_mode_index) * 4U;
                    if (!has_section_bytes(bytes, section, z_mode_entry, 4U)) {
                        return make_error("MAT3 Z mode table exceeds section bounds.");
                    }

                    material.z_mode = J3dZMode{
                        .valid = true,
                        .compare_enable = binary::read_u8(bytes, z_mode_entry + 0U),
                        .function = binary::read_u8(bytes, z_mode_entry + 1U),
                        .update_enable = binary::read_u8(bytes, z_mode_entry + 2U),
                    };
                }

                materials.push_back(std::move(material));
            }

            return materials;
        }

        [[nodiscard]] AssetResult< HierarchyInfo > parse_hierarchy_info(std::span< const std::byte > bytes, const Section& section,
                                                                        std::size_t joint_count) {
            if (!has_section_bytes(bytes, section, section.offset, 0x18U)) {
                return make_error("INF1 section is too small.");
            }

            const auto hierarchy_offset = required_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x14U), "INF1 hierarchy");
            if (!hierarchy_offset) {
                return hierarchy_offset.failure();
            }

            HierarchyInfo info{};
            info.joint_parents.assign(joint_count, J3D_NO_JOINT_INDEX);
            std::size_t current_material = 0U;
            std::uint16_t last_context = J3D_NO_JOINT_INDEX;
            std::vector< std::uint16_t > joint_stack{};

            for (std::size_t cursor = *hierarchy_offset; has_section_bytes(bytes, section, cursor, 4U); cursor += 4U) {
                const auto kind = binary::read_u16_be(bytes, cursor + 0U);
                const auto index = static_cast< std::size_t >(binary::read_u16_be(bytes, cursor + 2U));
                if (kind == 0U) {
                    return info;
                }
                if (kind == 0x01U) {
                    joint_stack.push_back(last_context);
                    continue;
                }
                if (kind == 0x02U) {
                    if (!joint_stack.empty()) {
                        joint_stack.pop_back();
                    }
                    continue;
                }
                if (kind == 0x11U) {
                    current_material = index;
                    last_context = joint_stack.empty() ? J3D_NO_JOINT_INDEX : joint_stack.back();
                } else if (kind == 0x12U) {
                    info.material_by_shape[index] = current_material;
                    last_context = joint_stack.empty() ? J3D_NO_JOINT_INDEX : joint_stack.back();
                } else if (kind == 0x10U) {
                    if (index >= joint_count) {
                        return make_error("INF1 hierarchy references a missing JNT1 joint.");
                    }
                    info.joint_parents[index] = joint_stack.empty() ? J3D_NO_JOINT_INDEX : joint_stack.back();
                    last_context = static_cast< std::uint16_t >(index);
                }
            }

            return make_error("INF1 hierarchy is unterminated.");
        }

        [[nodiscard]] AssetResult< std::vector< J3dDrawMatrix > > parse_draw_matrices(std::span< const std::byte > bytes, const Section& section) {
            if (!has_section_bytes(bytes, section, section.offset, 0x14U)) {
                return make_error("DRW1 section is too small.");
            }

            const auto matrix_count = static_cast< std::size_t >(binary::read_u16_be(bytes, section.offset + 0x08U));
            const auto flag_offset = required_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x0CU), "DRW1 matrix flag table");
            if (!flag_offset) {
                return flag_offset.failure();
            }
            const auto index_offset = required_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x10U), "DRW1 matrix index table");
            if (!index_offset) {
                return index_offset.failure();
            }

            if (!has_section_bytes(bytes, section, *flag_offset, matrix_count) ||
                !has_section_bytes(bytes, section, *index_offset, matrix_count * 2U)) {
                return make_error("DRW1 matrix tables exceed section bounds.");
            }

            std::vector< J3dDrawMatrix > matrices{};
            matrices.reserve(matrix_count);
            for (std::size_t matrix_index = 0U; matrix_index < matrix_count; ++matrix_index) {
                matrices.push_back(J3dDrawMatrix{
                    .weighted = binary::read_u8(bytes, *flag_offset + matrix_index) != 0U,
                    .index = binary::read_u16_be(bytes, *index_offset + matrix_index * 2U),
                });
            }

            return matrices;
        }

        [[nodiscard]] AssetResult< std::vector< J3dJoint > > parse_joints(std::span< const std::byte > bytes, const Section& section,
                                                                          const std::vector< std::uint16_t >& joint_parents) {
            if (!has_section_bytes(bytes, section, section.offset, 0x18U)) {
                return make_error("JNT1 section is too small.");
            }

            const auto joint_count = static_cast< std::size_t >(binary::read_u16_be(bytes, section.offset + 0x08U));
            const auto joint_init_offset =
                required_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x0CU), "JNT1 joint init table");
            if (!joint_init_offset) {
                return joint_init_offset.failure();
            }
            const auto index_table_offset = required_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x10U), "JNT1 index table");
            if (!index_table_offset) {
                return index_table_offset.failure();
            }
            const auto name_table_offset = binary::read_u32_be(bytes, section.offset + 0x14U);

            if (!has_section_bytes(bytes, section, *index_table_offset, joint_count * 2U)) {
                return make_error("JNT1 index table exceeds section bounds.");
            }

            std::vector< J3dJoint > joints{};
            joints.reserve(joint_count);
            for (std::size_t joint_index = 0U; joint_index < joint_count; ++joint_index) {
                const auto init_index = static_cast< std::size_t >(binary::read_u16_be(bytes, *index_table_offset + joint_index * 2U));
                const auto init_offset = *joint_init_offset + init_index * 0x40U;
                if (!has_section_bytes(bytes, section, init_offset, 0x40U)) {
                    return make_error("JNT1 joint init data exceeds section bounds.");
                }

                const auto scale_compensate = binary::read_u8(bytes, init_offset + 0x02U);
                joints.push_back(J3dJoint{
                    .name = read_string_table_name(bytes, section, name_table_offset, joint_index),
                    .parent_index = joint_index < joint_parents.size() ? joint_parents[joint_index] : J3D_NO_JOINT_INDEX,
                    .kind = binary::read_u16_be(bytes, init_offset + 0x00U),
                    .scale_compensate = scale_compensate != 0U && scale_compensate != 0xFFU,
                    .scale = read_vec3_f32(bytes, init_offset + 0x04U),
                    .rotation =
                        {
                            static_cast< std::int16_t >(binary::read_u16_be(bytes, init_offset + 0x10U)),
                            static_cast< std::int16_t >(binary::read_u16_be(bytes, init_offset + 0x12U)),
                            static_cast< std::int16_t >(binary::read_u16_be(bytes, init_offset + 0x14U)),
                        },
                    .translation = read_vec3_f32(bytes, init_offset + 0x18U),
                    .bounding_radius = binary::read_f32_be(bytes, init_offset + 0x24U),
                    .bounds_min = read_vec3_f32(bytes, init_offset + 0x28U),
                    .bounds_max = read_vec3_f32(bytes, init_offset + 0x34U),
                });
            }

            return joints;
        }

        [[nodiscard]] AssetResult< std::vector< VtxDesc > > read_vtx_desc_list(std::span< const std::byte > bytes, const Section& section,
                                                                               std::size_t offset) {
            if (!has_section_bytes(bytes, section, offset, 8U)) {
                return make_error("SHP1 vertex descriptor list exceeds section bounds.");
            }

            std::vector< VtxDesc > descriptors{};
            for (std::size_t index = 0U; has_section_bytes(bytes, section, offset + index * 8U, 8U); ++index) {
                const auto entry_offset = offset + index * 8U;
                const auto attr = binary::read_u32_be(bytes, entry_offset + 0U);
                const auto type = binary::read_u32_be(bytes, entry_offset + 4U);
                if (attr == GX_VA_NULL) {
                    return descriptors;
                }
                descriptors.push_back(VtxDesc{
                    .attr = attr,
                    .type = type,
                });
            }

            return make_error("SHP1 vertex descriptor list is unterminated.");
        }

        [[nodiscard]] AssetResult< J3dVertex > read_display_vertex(std::span< const std::byte > bytes, std::size_t* cursor, std::size_t end,
                                                                   const std::vector< VtxDesc >& descriptors, const VertexArrays& arrays,
                                                                   std::uint16_t draw_matrix_index) {
            J3dVertex vertex{
                .draw_matrix_index = draw_matrix_index,
            };
            bool has_position = false;

            for (const auto& descriptor : descriptors) {
                if (descriptor.type != GX_INDEX16) {
                    return make_error("SHP1 display lists only support INDEX16 vertex descriptors.");
                }
                if (*cursor > end || 2U > end - *cursor) {
                    return make_error("SHP1 display list vertex exceeds display list bounds.");
                }

                const auto index = binary::read_u16_be(bytes, *cursor);
                *cursor += 2U;

                if (descriptor.attr == GX_VA_POS) {
                    auto position = read_position(arrays, index);
                    if (!position) {
                        return position.failure();
                    }
                    vertex.position = *position;
                    vertex.position_index = index;
                    has_position = true;
                } else if (descriptor.attr == GX_VA_NRM) {
                    auto normal = read_normal(arrays, index);
                    if (!normal) {
                        return normal.failure();
                    }
                    vertex.normal = *normal;
                    vertex.normal_index = index;
                } else if (descriptor.attr == GX_VA_CLR0) {
                    auto color = read_color(arrays, index);
                    if (!color) {
                        return color.failure();
                    }
                    vertex.color = *color;
                    vertex.color_index = index;
                } else if (descriptor.attr == GX_VA_TEX0) {
                    auto texcoord = read_texcoord(arrays, index);
                    if (!texcoord) {
                        return texcoord.failure();
                    }
                    vertex.texcoord = *texcoord;
                    vertex.texcoord_index = index;
                }
            }

            if (!has_position) {
                return make_error("SHP1 display vertex is missing a position attribute.");
            }

            return vertex;
        }

        void push_strip_triangles(std::vector< J3dTriangle >* triangles, const std::vector< J3dVertex >& strip) {
            if (triangles == nullptr || strip.size() < 3U) {
                return;
            }

            for (std::size_t index = 2U; index < strip.size(); ++index) {
                if (index % 2U == 0U) {
                    triangles->push_back(J3dTriangle{strip[index - 2U], strip[index - 1U], strip[index]});
                } else {
                    triangles->push_back(J3dTriangle{strip[index - 1U], strip[index - 2U], strip[index]});
                }
            }
        }

        [[nodiscard]] AssetResult< void > append_display_list_triangles(std::span< const std::byte > bytes, const Section& section,
                                                                        std::size_t display_offset, std::size_t display_size,
                                                                        const std::vector< VtxDesc >& descriptors, const VertexArrays& arrays,
                                                                        std::uint16_t draw_matrix_index, std::vector< J3dTriangle >* triangles) {
            if (triangles == nullptr) {
                return make_error("SHP1 triangle output pointer is null.");
            }
            if (!has_section_bytes(bytes, section, display_offset, display_size)) {
                return make_error("SHP1 display list exceeds section bounds.");
            }

            std::size_t cursor = display_offset;
            const auto end = display_offset + display_size;
            while (cursor < end) {
                const auto primitive = binary::read_u8(bytes, cursor++);
                if (primitive == 0U) {
                    continue;
                }
                if (cursor > end || 2U > end - cursor) {
                    return make_error("SHP1 primitive count exceeds display list bounds.");
                }

                const auto vertex_count = static_cast< std::size_t >(binary::read_u16_be(bytes, cursor));
                cursor += 2U;

                std::vector< J3dVertex > vertices{};
                vertices.reserve(vertex_count);
                for (std::size_t vertex_index = 0U; vertex_index < vertex_count; ++vertex_index) {
                    auto vertex = read_display_vertex(bytes, &cursor, end, descriptors, arrays, draw_matrix_index);
                    if (!vertex) {
                        return vertex.failure();
                    }
                    vertices.push_back(*vertex);
                }

                switch (primitive & 0xF8U) {
                case GX_TRIANGLESTRIP:
                    push_strip_triangles(triangles, vertices);
                    break;
                case GX_TRIANGLES:
                    for (std::size_t index = 2U; index < vertices.size(); index += 3U) {
                        triangles->push_back(J3dTriangle{vertices[index - 2U], vertices[index - 1U], vertices[index]});
                    }
                    break;
                case GX_TRIANGLEFAN:
                    for (std::size_t index = 2U; index < vertices.size(); ++index) {
                        triangles->push_back(J3dTriangle{vertices[0U], vertices[index - 1U], vertices[index]});
                    }
                    break;
                default:
                    return make_error("SHP1 display list uses an unsupported primitive.");
                }
            }

            return {};
        }

        [[nodiscard]] AssetResult< std::vector< J3dShape > > parse_shapes(std::span< const std::byte > bytes, const Section& section,
                                                                          const VertexArrays& arrays,
                                                                          const std::unordered_map< std::size_t, std::size_t >& material_by_shape,
                                                                          std::size_t material_count) {
            if (!has_section_bytes(bytes, section, section.offset, 0x2CU)) {
                return make_error("SHP1 section is too small.");
            }

            const auto shape_count = static_cast< std::size_t >(binary::read_u16_be(bytes, section.offset + 0x08U));
            const auto shape_init_offset =
                required_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x0CU), "SHP1 shape init table");
            if (!shape_init_offset) {
                return shape_init_offset.failure();
            }
            const auto index_table_offset = required_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x10U), "SHP1 index table");
            if (!index_table_offset) {
                return index_table_offset.failure();
            }
            const auto name_table_offset = binary::read_u32_be(bytes, section.offset + 0x14U);
            const auto vtx_desc_offset =
                required_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x18U), "SHP1 vertex descriptor table");
            if (!vtx_desc_offset) {
                return vtx_desc_offset.failure();
            }
            const auto mtx_table_offset = optional_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x1CU), "SHP1 matrix table");
            if (!mtx_table_offset) {
                return mtx_table_offset.failure();
            }
            const auto display_list_offset =
                required_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x20U), "SHP1 display list data");
            if (!display_list_offset) {
                return display_list_offset.failure();
            }
            const auto mtx_init_offset =
                required_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x24U), "SHP1 matrix init table");
            if (!mtx_init_offset) {
                return mtx_init_offset.failure();
            }
            const auto draw_init_offset =
                required_section_offset(section, binary::read_u32_be(bytes, section.offset + 0x28U), "SHP1 draw init table");
            if (!draw_init_offset) {
                return draw_init_offset.failure();
            }

            if (!has_section_bytes(bytes, section, *index_table_offset, shape_count * 2U)) {
                return make_error("SHP1 index table exceeds section bounds.");
            }

            std::vector< J3dShape > shapes{};
            shapes.reserve(shape_count);
            for (std::size_t shape_index = 0U; shape_index < shape_count; ++shape_index) {
                const auto shape_init_index = binary::read_u16_be(bytes, *index_table_offset + shape_index * 2U);
                const auto init_offset = *shape_init_offset + static_cast< std::size_t >(shape_init_index) * 0x28U;
                if (!has_section_bytes(bytes, section, init_offset, 0x28U)) {
                    return make_error("SHP1 shape init data exceeds section bounds.");
                }

                const auto matrix_group_count = static_cast< std::size_t >(binary::read_u16_be(bytes, init_offset + 0x02U));
                const auto vtx_desc_index = static_cast< std::size_t >(binary::read_u16_be(bytes, init_offset + 0x04U));
                const auto mtx_init_index = static_cast< std::size_t >(binary::read_u16_be(bytes, init_offset + 0x06U));
                const auto draw_init_index = static_cast< std::size_t >(binary::read_u16_be(bytes, init_offset + 0x08U));

                auto descriptors = read_vtx_desc_list(bytes, section, *vtx_desc_offset + vtx_desc_index);
                if (!descriptors) {
                    return descriptors.failure();
                }
                if (descriptors->empty()) {
                    return make_error("SHP1 shape has no vertex descriptors.");
                }

                const auto material_found = material_by_shape.find(shape_index);
                const auto material_index = material_found == material_by_shape.end() ? shape_index : material_found->second;
                if (material_count > 0U && material_index >= material_count) {
                    return make_error("INF1 shape-material map references a missing MAT3 material.");
                }

                J3dShape shape{
                    .name = read_string_table_name(bytes, section, name_table_offset, shape_index),
                    .material_index = static_cast< std::uint16_t >(material_index),
                    .shape_init_index = shape_init_index,
                };
                shape.matrix_groups.reserve(matrix_group_count);
                shape.triangles.reserve(matrix_group_count * 256U);

                for (std::size_t group_index = 0U; group_index < matrix_group_count; ++group_index) {
                    const auto mtx_offset = *mtx_init_offset + (mtx_init_index + group_index) * 8U;
                    const auto draw_offset = *draw_init_offset + (draw_init_index + group_index) * 8U;
                    if (!has_section_bytes(bytes, section, mtx_offset, 8U) || !has_section_bytes(bytes, section, draw_offset, 8U)) {
                        return make_error("SHP1 matrix or draw init data exceeds section bounds.");
                    }

                    const auto matrix_count = binary::read_u16_be(bytes, mtx_offset + 0x02U);
                    const auto first_matrix_table_index = binary::read_u32_be(bytes, mtx_offset + 0x04U);
                    if (matrix_count > 1U) {
                        if (!*mtx_table_offset) {
                            return make_error("SHP1 multi-matrix shape references a missing matrix table.");
                        }
                        const auto matrix_table_entry = **mtx_table_offset + static_cast< std::size_t >(first_matrix_table_index) * 2U;
                        if (!has_section_bytes(bytes, section, matrix_table_entry, static_cast< std::size_t >(matrix_count) * 2U)) {
                            return make_error("SHP1 matrix table exceeds section bounds.");
                        }
                    }

                    const auto display_size = binary::read_u32_be(bytes, draw_offset + 0U);
                    const auto display_index = binary::read_u32_be(bytes, draw_offset + 4U);
                    const auto draw_matrix_index = binary::read_u16_be(bytes, mtx_offset + 0x00U);
                    shape.matrix_groups.push_back(J3dShapeMatrixGroup{
                        .draw_matrix_index = draw_matrix_index,
                        .matrix_count = matrix_count,
                        .first_matrix_table_index = first_matrix_table_index,
                        .display_list_offset = display_index,
                        .display_list_size = display_size,
                    });

                    auto display_result = append_display_list_triangles(
                        bytes, section, *display_list_offset + static_cast< std::size_t >(display_index), static_cast< std::size_t >(display_size),
                        *descriptors, arrays, draw_matrix_index, &shape.triangles);
                    if (!display_result) {
                        return display_result.failure();
                    }
                }

                shapes.push_back(std::move(shape));
            }

            return shapes;
        }

    }  // namespace

    AssetResult< J3dModel > parse_j3d_model(std::span< const std::byte > bdl_bytes) {
        auto sections = read_sections(bdl_bytes);
        if (!sections) {
            return sections.failure();
        }

        const auto inf1 = require_section(bdl_bytes, *sections, "INF1");
        if (!inf1) {
            return inf1.failure();
        }
        const auto vtx1 = require_section(bdl_bytes, *sections, "VTX1");
        if (!vtx1) {
            return vtx1.failure();
        }
        const auto drw1 = require_section(bdl_bytes, *sections, "DRW1");
        if (!drw1) {
            return drw1.failure();
        }
        const auto jnt1 = require_section(bdl_bytes, *sections, "JNT1");
        if (!jnt1) {
            return jnt1.failure();
        }
        const auto shp1 = require_section(bdl_bytes, *sections, "SHP1");
        if (!shp1) {
            return shp1.failure();
        }
        const auto mat3 = require_section(bdl_bytes, *sections, "MAT3");
        if (!mat3) {
            return mat3.failure();
        }

        auto textures = parse_j3d_tex1_textures(bdl_bytes);
        if (!textures) {
            return textures.failure();
        }

        auto arrays = parse_vertex_arrays(bdl_bytes, *vtx1);
        if (!arrays) {
            return arrays.failure();
        }

        auto materials = parse_materials(bdl_bytes, *mat3, textures->size());
        if (!materials) {
            return materials.failure();
        }

        auto draw_matrices = parse_draw_matrices(bdl_bytes, *drw1);
        if (!draw_matrices) {
            return draw_matrices.failure();
        }

        const auto joint_count = static_cast< std::size_t >(binary::read_u16_be(bdl_bytes, jnt1->offset + 0x08U));
        auto hierarchy = parse_hierarchy_info(bdl_bytes, *inf1, joint_count);
        if (!hierarchy) {
            return hierarchy.failure();
        }

        auto joints = parse_joints(bdl_bytes, *jnt1, hierarchy->joint_parents);
        if (!joints) {
            return joints.failure();
        }

        auto shapes = parse_shapes(bdl_bytes, *shp1, *arrays, hierarchy->material_by_shape, materials->size());
        if (!shapes) {
            return shapes.failure();
        }

        return J3dModel{
            .textures = std::move(*textures),
            .materials = std::move(*materials),
            .joints = std::move(*joints),
            .draw_matrices = std::move(*draw_matrices),
            .shapes = std::move(*shapes),
        };
    }

}  // namespace smgpc::assets::layout
