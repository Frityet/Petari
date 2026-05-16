#include "J3dModel.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace smgpc::game {
    namespace {

        constexpr auto J3D1_MAGIC = std::uint32_t{0x4a334431U};
        constexpr auto J3D2_MAGIC = std::uint32_t{0x4a334432U};
        constexpr auto GX_VA_NULL = std::uint32_t{0xffU};
        constexpr auto GX_NONE = std::uint32_t{0U};
        constexpr auto GX_DIRECT = std::uint32_t{1U};
        constexpr auto GX_INDEX8 = std::uint32_t{2U};
        constexpr auto GX_INDEX16 = std::uint32_t{3U};

        [[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 2U > data.size()) {
                throw std::runtime_error("J3D read past end of buffer");
            }

            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8U) | static_cast<std::uint16_t>(data[offset + 1U]));
        }

        [[nodiscard]] std::int16_t read_be_s16(std::span<const std::uint8_t> data, std::size_t offset) {
            return std::bit_cast<std::int16_t>(read_be16(data, offset));
        }

        [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 4U > data.size()) {
                throw std::runtime_error("J3D read past end of buffer");
            }

            return (static_cast<std::uint32_t>(data[offset]) << 24U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
                   (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) | static_cast<std::uint32_t>(data[offset + 3U]);
        }

        [[nodiscard]] float read_be_float(std::span<const std::uint8_t> data, std::size_t offset) {
            return std::bit_cast<float>(read_be32(data, offset));
        }

        [[nodiscard]] std::string read_tag(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 4U > data.size()) {
                throw std::runtime_error("J3D tag read past end of buffer");
            }

            return std::string(reinterpret_cast<const char *>(data.data() + offset), 4U);
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

        [[nodiscard]] std::vector<std::string> read_name_table(std::span<const std::uint8_t> data, std::size_t section_offset,
                                                               std::uint32_t table_relative_offset) {
            if (table_relative_offset == 0U) {
                return {};
            }

            const auto table_offset = section_offset + table_relative_offset;
            if (table_offset + 4U > data.size()) {
                throw std::runtime_error("J3D name table outside buffer");
            }

            const auto count = read_be16(data, table_offset);
            auto names = std::vector<std::string>{};
            names.reserve(count);
            for (auto i = 0U; i < count; ++i) {
                const auto entry_offset = table_offset + 4U + i * 4U;
                if (entry_offset + 4U > data.size()) {
                    throw std::runtime_error("J3D name table entry outside buffer");
                }

                names.push_back(read_string(data, table_offset + read_be16(data, entry_offset + 2U)));
            }

            return names;
        }

        [[nodiscard]] std::optional<std::size_t> section_offset_for(const std::vector<J3dSectionInfo> &sections, std::string_view tag) {
            const auto it = std::ranges::find_if(sections, [tag](const auto &section) { return section.tag == tag; });

            if (it == sections.end()) {
                return std::nullopt;
            }

            return it->offset;
        }

        [[nodiscard]] std::optional<J3dSectionInfo> section_for(const std::vector<J3dSectionInfo> &sections, std::string_view tag) {
            const auto it = std::ranges::find_if(sections, [tag](const auto &section) { return section.tag == tag; });

            if (it == sections.end()) {
                return std::nullopt;
            }

            return *it;
        }

        [[nodiscard]] std::size_t relative_offset(std::size_t section_offset, std::uint32_t value) {
            return section_offset + value;
        }

        [[nodiscard]] bool has_relative_offset(std::uint32_t value) {
            return value != 0U;
        }

        [[nodiscard]] std::uint32_t scalar_component_size(std::uint32_t component_type) {
            switch (component_type) {
            case 0U:
            case 1U:
                return 1U;
            case 2U:
            case 3U:
                return 2U;
            case 4U:
                return 4U;
            default:
                return 0U;
            }
        }

        [[nodiscard]] std::uint32_t direct_attribute_size(const std::vector<J3dVertexAttributeFormat> &formats, std::uint32_t attr) {
            if (attr <= 8U) {
                return 1U;
            }
            if (attr == 11U || attr == 12U) {
                return 4U;
            }

            const auto it = std::ranges::find_if(formats, [attr](const auto &format) { return format.attr == attr; });
            if (it == formats.end()) {
                return 0U;
            }

            auto component_count = std::uint32_t{0U};
            if (attr == 9U) {
                component_count = it->component_count == 0U ? 2U : 3U;
            } else if (attr == 10U) {
                component_count = it->component_count == 2U ? 9U : 3U;
            } else if (attr >= 13U && attr <= 20U) {
                component_count = it->component_count == 0U ? 1U : 2U;
            }

            return component_count * scalar_component_size(it->component_type);
        }

        [[nodiscard]] const J3dVertexAttributeFormat *format_for(const std::vector<J3dVertexAttributeFormat> &formats, std::uint32_t attr) {
            const auto it = std::ranges::find_if(formats, [attr](const auto &format) { return format.attr == attr; });

            return it == formats.end() ? nullptr : &*it;
        }

        [[nodiscard]] std::uint32_t display_list_vertex_size(const std::vector<J3dVertexDesc> &desc,
                                                             const std::vector<J3dVertexAttributeFormat> &formats) {
            auto size = std::uint32_t{};
            for (const auto &entry : desc) {
                switch (entry.type) {
                case GX_NONE:
                    break;
                case GX_DIRECT:
                    size += direct_attribute_size(formats, entry.attr);
                    break;
                case GX_INDEX8:
                    size += 1U;
                    break;
                case GX_INDEX16:
                    size += 2U;
                    break;
                default:
                    return 0U;
                }
            }

            return size;
        }

        [[nodiscard]] std::uint32_t primitive_triangle_count(std::uint8_t primitive, std::uint16_t vertex_count) {
            switch (primitive) {
            case 0x80U:
                return static_cast<std::uint32_t>(vertex_count / 4U) * 2U;
            case 0x90U:
                return vertex_count / 3U;
            case 0x98U:
            case 0xa0U:
                return vertex_count >= 3U ? vertex_count - 2U : 0U;
            default:
                return 0U;
            }
        }

        [[nodiscard]] bool skip_gx_display_list_command(std::span<const std::uint8_t> data, std::size_t &cursor, std::size_t end,
                                                        std::uint8_t command) {
            const auto require = [&](std::size_t bytes) {
                if (cursor + bytes > end || cursor + bytes > data.size()) {
                    return false;
                }
                cursor += bytes;
                return true;
            };

            switch (command) {
            case 0x08U:
                return require(5U);
            case 0x10U: {
                if (cursor + 4U > end || cursor + 4U > data.size()) {
                    return false;
                }

                const auto transfer_count = read_be16(data, cursor);
                cursor += 4U;
                return require((static_cast<std::size_t>(transfer_count) + 1U) * 4U);
            }
            case 0x20U:
            case 0x28U:
            case 0x30U:
            case 0x38U:
                return require(4U);
            case 0x40U:
                return require(8U);
            case 0x48U:
                return true;
            case 0x61U:
                return require(4U);
            default:
                return false;
            }
        }

        [[nodiscard]] std::vector<J3dPrimitiveSummary> parse_display_list(std::span<const std::uint8_t> data, std::size_t offset,
                                                                          std::uint32_t size, const std::vector<J3dVertexDesc> &desc,
                                                                          const std::vector<J3dVertexAttributeFormat> &formats,
                                                                          std::uint32_t &parsed_bytes) {
            if (offset + size > data.size()) {
                throw std::runtime_error("J3D shape display list outside buffer");
            }

            auto primitives = std::vector<J3dPrimitiveSummary>{};
            const auto vertex_size = display_list_vertex_size(desc, formats);
            if (vertex_size == 0U) {
                parsed_bytes = 0U;
                return primitives;
            }

            auto cursor = offset;
            const auto end = offset + size;
            while (cursor < end) {
                const auto command = data[cursor++];
                if (command == 0U) {
                    continue;
                }

                const auto primitive = static_cast<std::uint8_t>(command & 0xf8U);
                const auto vertex_format = static_cast<std::uint8_t>(command & 0x07U);
                if (primitive < 0x80U || primitive > 0xb8U || cursor + 2U > end) {
                    if (skip_gx_display_list_command(data, cursor, end, command)) {
                        continue;
                    }

                    break;
                }

                const auto vertex_count = read_be16(data, cursor);
                cursor += 2U;

                const auto payload_size = static_cast<std::size_t>(vertex_count) * vertex_size;
                if (cursor + payload_size > end) {
                    break;
                }

                primitives.push_back(J3dPrimitiveSummary{
                    .command = command,
                    .primitive = primitive,
                    .vertex_format = vertex_format,
                    .vertex_count = vertex_count,
                    .triangle_count = primitive_triangle_count(primitive, vertex_count),
                });
                cursor += payload_size;
            }

            parsed_bytes = static_cast<std::uint32_t>(cursor - offset);
            return primitives;
        }

        [[nodiscard]] std::uint32_t array_stride_for_attr(const std::vector<J3dVertexAttributeFormat> &formats, std::uint32_t attr) {
            const auto it = std::ranges::find_if(formats, [attr](const auto &format) { return format.attr == attr; });
            if (it == formats.end()) {
                return 0U;
            }

            return direct_attribute_size(formats, attr);
        }

        [[nodiscard]] std::uint32_t infer_array_count(const std::vector<std::uint32_t> &array_offsets, std::uint32_t current_offset,
                                                      std::uint32_t section_size, std::uint32_t stride) {
            if (current_offset == 0U || stride == 0U) {
                return 0U;
            }

            auto next_offset = section_size;
            for (const auto offset : array_offsets) {
                if (offset > current_offset && offset < next_offset) {
                    next_offset = offset;
                }
            }

            return next_offset > current_offset ? (next_offset - current_offset) / stride : 0U;
        }

        [[nodiscard]] J3dInfoSummary parse_inf1(std::span<const std::uint8_t> data, const J3dSectionInfo &section) {
            const auto section_offset = static_cast<std::size_t>(section.offset);
            if (section.size < 0x18U) {
                throw std::runtime_error("J3D INF1 section is too small");
            }

            auto info = J3dInfoSummary{};
            info.flags = read_be16(data, section_offset + 0x08U);
            info.packet_count = read_be32(data, section_offset + 0x0cU);
            info.vertex_count = read_be32(data, section_offset + 0x10U);

            const auto hierarchy_relative = read_be32(data, section_offset + 0x14U);
            if (hierarchy_relative != 0U) {
                auto cursor = relative_offset(section_offset, hierarchy_relative);
                while (cursor + 4U <= data.size()) {
                    const auto entry = J3dHierarchyEntry{
                        .type = read_be16(data, cursor),
                        .value = read_be16(data, cursor + 2U),
                    };
                    info.hierarchy.push_back(entry);
                    cursor += 4U;
                    if (entry.type == 0U) {
                        break;
                    }
                }
            }

            return info;
        }

        [[nodiscard]] J3dJointBlockSummary parse_jnt1(std::span<const std::uint8_t> data, const J3dSectionInfo &section) {
            const auto section_offset = static_cast<std::size_t>(section.offset);
            if (section.size < 0x18U) {
                throw std::runtime_error("J3D JNT1 section is too small");
            }

            auto summary = J3dJointBlockSummary{};
            summary.joint_count = read_be16(data, section_offset + 0x08U);
            const auto joint_init_relative = read_be32(data, section_offset + 0x0cU);
            const auto remap_relative = read_be32(data, section_offset + 0x10U);
            const auto name_table_relative = read_be32(data, section_offset + 0x14U);
            if (!has_relative_offset(joint_init_relative) || !has_relative_offset(remap_relative)) {
                return summary;
            }

            const auto names = read_name_table(data, section_offset, name_table_relative);
            summary.remap_table.reserve(summary.joint_count);
            for (auto i = 0U; i < summary.joint_count; ++i) {
                summary.remap_table.push_back(read_be16(data, relative_offset(section_offset, remap_relative) + i * 2U));
            }

            summary.joints.reserve(summary.joint_count);
            for (auto i = 0U; i < summary.joint_count; ++i) {
                const auto remapped_index = i < summary.remap_table.size() ? summary.remap_table[i] : static_cast<std::uint16_t>(i);
                const auto joint_offset = relative_offset(section_offset, joint_init_relative) + static_cast<std::size_t>(remapped_index) * 0x40U;
                if (joint_offset + 0x40U > data.size()) {
                    throw std::runtime_error("J3D JNT1 joint init data outside buffer");
                }

                summary.joints.push_back(J3dJointSummary{
                    .name = i < names.size() ? names[i] : std::string{},
                    .index = static_cast<std::uint16_t>(i),
                    .kind = read_be16(data, joint_offset),
                    .scale_compensate = data[joint_offset + 0x02U],
                    .scale = {read_be_float(data, joint_offset + 0x04U), read_be_float(data, joint_offset + 0x08U),
                              read_be_float(data, joint_offset + 0x0cU)},
                    .rotation = {read_be_s16(data, joint_offset + 0x10U), read_be_s16(data, joint_offset + 0x12U),
                                 read_be_s16(data, joint_offset + 0x14U)},
                    .translation = {read_be_float(data, joint_offset + 0x18U), read_be_float(data, joint_offset + 0x1cU),
                                    read_be_float(data, joint_offset + 0x20U)},
                    .radius = read_be_float(data, joint_offset + 0x24U),
                    .min = {read_be_float(data, joint_offset + 0x28U), read_be_float(data, joint_offset + 0x2cU),
                            read_be_float(data, joint_offset + 0x30U)},
                    .max = {read_be_float(data, joint_offset + 0x34U), read_be_float(data, joint_offset + 0x38U),
                            read_be_float(data, joint_offset + 0x3cU)},
                });
            }

            return summary;
        }

        [[nodiscard]] J3dEnvelopeBlockSummary parse_evp1(std::span<const std::uint8_t> data, const J3dSectionInfo &section) {
            const auto section_offset = static_cast<std::size_t>(section.offset);
            if (section.size < 0x1cU) {
                throw std::runtime_error("J3D EVP1 section is too small");
            }

            auto summary = J3dEnvelopeBlockSummary{};
            summary.matrix_count = read_be16(data, section_offset + 0x08U);
            if (summary.matrix_count == 0U) {
                return summary;
            }

            const auto mix_count_relative = read_be32(data, section_offset + 0x0cU);
            const auto mix_index_relative = read_be32(data, section_offset + 0x10U);
            const auto mix_weight_relative = read_be32(data, section_offset + 0x14U);
            const auto inv_joint_relative = read_be32(data, section_offset + 0x18U);
            if (!has_relative_offset(mix_count_relative) || !has_relative_offset(mix_index_relative) || !has_relative_offset(mix_weight_relative)) {
                throw std::runtime_error("J3D EVP1 missing required envelope tables");
            }

            const auto mix_count_offset = relative_offset(section_offset, mix_count_relative);
            const auto mix_index_offset = relative_offset(section_offset, mix_index_relative);
            const auto mix_weight_offset = relative_offset(section_offset, mix_weight_relative);

            auto influence_cursor = std::size_t{};
            auto max_joint_index = std::uint16_t{};
            summary.matrices.reserve(summary.matrix_count);
            for (auto matrix_index = 0U; matrix_index < summary.matrix_count; ++matrix_index) {
                if (mix_count_offset + matrix_index >= data.size()) {
                    throw std::runtime_error("J3D EVP1 mix-count table outside buffer");
                }

                const auto influence_count = data[mix_count_offset + matrix_index];
                auto matrix = J3dEnvelopeMatrixSummary{};
                matrix.joint_indices.reserve(influence_count);
                matrix.weights.reserve(influence_count);
                for (auto influence = 0U; influence < influence_count; ++influence) {
                    const auto index = read_be16(data, mix_index_offset + (influence_cursor + influence) * 2U);
                    const auto weight = read_be_float(data, mix_weight_offset + (influence_cursor + influence) * 4U);
                    max_joint_index = std::max(max_joint_index, index);
                    matrix.joint_indices.push_back(index);
                    matrix.weights.push_back(weight);
                }
                influence_cursor += influence_count;
                summary.matrices.push_back(std::move(matrix));
            }

            if (has_relative_offset(inv_joint_relative) && inv_joint_relative < section.size) {
                const auto inv_joint_offset = relative_offset(section_offset, inv_joint_relative);
                const auto available_bytes = static_cast<std::size_t>(section.size - inv_joint_relative);
                const auto matrix_count = std::min<std::size_t>(static_cast<std::size_t>(max_joint_index) + 1U, available_bytes / 48U);
                summary.inverse_bind_matrices.reserve(matrix_count);
                for (auto matrix_index = 0U; matrix_index < matrix_count; ++matrix_index) {
                    auto matrix = std::array<float, 12U>{};
                    for (auto value = 0U; value < matrix.size(); ++value) {
                        matrix[value] = read_be_float(data, inv_joint_offset + matrix_index * 48U + value * 4U);
                    }
                    summary.inverse_bind_matrices.push_back(matrix);
                }
            }

            return summary;
        }

        [[nodiscard]] J3dDrawBlockSummary parse_drw1(std::span<const std::uint8_t> data, const J3dSectionInfo &section) {
            const auto section_offset = static_cast<std::size_t>(section.offset);
            if (section.size < 0x14U) {
                throw std::runtime_error("J3D DRW1 section is too small");
            }

            auto summary = J3dDrawBlockSummary{};
            summary.matrix_count = read_be16(data, section_offset + 0x08U);
            const auto flag_relative = read_be32(data, section_offset + 0x0cU);
            const auto index_relative = read_be32(data, section_offset + 0x10U);
            if (!has_relative_offset(flag_relative) || !has_relative_offset(index_relative)) {
                return summary;
            }

            const auto flag_offset = relative_offset(section_offset, flag_relative);
            const auto index_offset = relative_offset(section_offset, index_relative);
            if (flag_offset + summary.matrix_count > data.size() ||
                index_offset + static_cast<std::size_t>(summary.matrix_count) * 2U > data.size()) {
                throw std::runtime_error("J3D DRW1 matrix tables outside buffer");
            }

            summary.matrices.reserve(summary.matrix_count);
            for (auto i = 0U; i < summary.matrix_count; ++i) {
                summary.matrices.push_back(J3dDrawMatrixSummary{
                    .weighted = data[flag_offset + i] != 0U,
                    .index = read_be16(data, index_offset + i * 2U),
                });
            }

            return summary;
        }

        [[nodiscard]] std::vector<J3dVertexAttributeFormat> parse_vertex_formats(std::span<const std::uint8_t> data, std::size_t offset) {
            auto formats = std::vector<J3dVertexAttributeFormat>{};
            if (offset == 0U) {
                return formats;
            }

            for (auto cursor = offset; cursor + 16U <= data.size(); cursor += 16U) {
                const auto attr = read_be32(data, cursor);
                if (attr == GX_VA_NULL) {
                    break;
                }

                formats.push_back(J3dVertexAttributeFormat{
                    .attr = attr,
                    .component_count = read_be32(data, cursor + 4U),
                    .component_type = read_be32(data, cursor + 8U),
                    .fraction = data[cursor + 12U],
                });
            }

            return formats;
        }

        [[nodiscard]] J3dVertexSummary parse_vtx1(std::span<const std::uint8_t> data, const J3dSectionInfo &section) {
            const auto section_offset = static_cast<std::size_t>(section.offset);
            if (section.size < 0x40U) {
                throw std::runtime_error("J3D VTX1 section is too small");
            }

            auto summary = J3dVertexSummary{};
            const auto format_relative = read_be32(data, section_offset + 0x08U);
            if (has_relative_offset(format_relative)) {
                summary.formats = parse_vertex_formats(data, relative_offset(section_offset, format_relative));
            }

            constexpr std::array<std::uint32_t, 13U> attrs{
                9U,
                10U,
                25U,
                11U,
                12U,
                13U,
                14U,
                15U,
                16U,
                17U,
                18U,
                19U,
                20U,
            };

            std::array<std::uint32_t, attrs.size()> relatives{};
            for (auto i = std::size_t{}; i < relatives.size(); ++i) {
                relatives[i] = read_be32(data, section_offset + 0x0cU + i * 4U);
            }

            auto nonzero_offsets = std::vector<std::uint32_t>{};
            for (const auto offset : relatives) {
                if (offset != 0U && offset < section.size) {
                    nonzero_offsets.push_back(offset);
                }
            }
            std::ranges::sort(nonzero_offsets);

            for (auto i = std::size_t{}; i < attrs.size(); ++i) {
                const auto offset = relatives[i];
                if (offset == 0U || offset >= section.size) {
                    continue;
                }

                const auto stride = array_stride_for_attr(summary.formats, attrs[i]);
                summary.arrays.push_back(J3dVertexArraySummary{
                    .attr = attrs[i],
                    .offset = offset,
                    .stride = stride,
                    .inferred_count = infer_array_count(nonzero_offsets, offset, section.size, stride),
                });
            }

            return summary;
        }

        [[nodiscard]] std::vector<J3dVertexDesc> parse_vertex_desc(std::span<const std::uint8_t> data, std::size_t offset) {
            auto desc = std::vector<J3dVertexDesc>{};
            for (auto cursor = offset; cursor + 8U <= data.size(); cursor += 8U) {
                const auto attr = read_be32(data, cursor);
                if (attr == GX_VA_NULL) {
                    break;
                }

                desc.push_back(J3dVertexDesc{
                    .attr = attr,
                    .type = read_be32(data, cursor + 4U),
                });
            }

            return desc;
        }

        [[nodiscard]] std::vector<std::uint16_t> shape_materials_from_hierarchy(const std::optional<J3dInfoSummary> &info,
                                                                                std::uint16_t shape_count) {
            auto materials = std::vector<std::uint16_t>(shape_count, 0xffffU);
            if (!info.has_value()) {
                return materials;
            }

            auto current_material = std::uint16_t{0xffffU};
            for (const auto &entry : info->hierarchy) {
                if (entry.type == 0x11U) {
                    current_material = entry.value;
                } else if (entry.type == 0x12U && entry.value < materials.size()) {
                    materials[entry.value] = current_material;
                }
            }

            return materials;
        }

        [[nodiscard]] std::vector<std::uint16_t> shape_joints_from_hierarchy(const std::optional<J3dInfoSummary> &info,
                                                                             std::uint16_t shape_count) {
            auto joints = std::vector<std::uint16_t>(shape_count, 0xffffU);
            if (!info.has_value()) {
                return joints;
            }

            auto current_joint = std::uint16_t{0xffffU};
            for (const auto &entry : info->hierarchy) {
                if (entry.type == 0x10U) {
                    current_joint = entry.value;
                } else if (entry.type == 0x12U && entry.value < joints.size()) {
                    joints[entry.value] = current_joint;
                }
            }

            return joints;
        }

        [[nodiscard]] std::vector<std::uint16_t> shape_draw_orders_from_hierarchy(const std::optional<J3dInfoSummary> &info,
                                                                                  std::uint16_t shape_count) {
            auto draw_orders = std::vector<std::uint16_t>(shape_count, 0xffffU);
            if (!info.has_value()) {
                for (auto i = std::uint16_t{}; i < shape_count; ++i) {
                    draw_orders[i] = i;
                }
                return draw_orders;
            }

            auto draw_order = std::uint16_t{};
            for (const auto &entry : info->hierarchy) {
                if (entry.type == 0x12U && entry.value < draw_orders.size()) {
                    draw_orders[entry.value] = draw_order++;
                }
            }

            return draw_orders;
        }

        [[nodiscard]] std::vector<std::uint16_t> joint_parent_indices_from_hierarchy(const std::optional<J3dInfoSummary> &info,
                                                                                     std::uint16_t joint_count) {
            auto parents = std::vector<std::uint16_t>(joint_count, 0xffffU);
            if (!info.has_value()) {
                return parents;
            }

            auto stack = std::vector<std::uint16_t>{};
            auto child_scope_is_joint = std::vector<bool>{};
            auto last_joint = std::uint16_t{0xffffU};
            auto last_entry_was_joint = false;
            for (const auto &entry : info->hierarchy) {
                switch (entry.type) {
                case 0x10U:
                    if (entry.value < parents.size()) {
                        parents[entry.value] = stack.empty() ? static_cast<std::uint16_t>(0xffffU) : stack.back();
                    }
                    last_joint = entry.value;
                    last_entry_was_joint = true;
                    break;
                case 0x01U:
                    child_scope_is_joint.push_back(last_entry_was_joint && last_joint != 0xffffU);
                    if (child_scope_is_joint.back()) {
                        stack.push_back(last_joint);
                    }
                    last_entry_was_joint = false;
                    break;
                case 0x02U:
                    if (!child_scope_is_joint.empty() && child_scope_is_joint.back() && !stack.empty()) {
                        stack.pop_back();
                    }
                    if (!child_scope_is_joint.empty()) {
                        child_scope_is_joint.pop_back();
                    }
                    last_entry_was_joint = false;
                    break;
                default:
                    last_entry_was_joint = false;
                    break;
                }
            }

            return parents;
        }

        [[nodiscard]] J3dShapeBlockSummary parse_shp1(std::span<const std::uint8_t> data, const J3dSectionInfo &section,
                                                      const std::optional<J3dInfoSummary> &info,
                                                      const std::optional<J3dVertexSummary> &vertices) {
            const auto section_offset = static_cast<std::size_t>(section.offset);
            if (section.size < 0x2cU) {
                throw std::runtime_error("J3D SHP1 section is too small");
            }

            const auto shape_count = read_be16(data, section_offset + 0x08U);
            const auto shape_init_relative = read_be32(data, section_offset + 0x0cU);
            const auto index_table_relative = read_be32(data, section_offset + 0x10U);
            const auto name_table_relative = read_be32(data, section_offset + 0x14U);
            const auto vtx_desc_relative = read_be32(data, section_offset + 0x18U);
            const auto matrix_table_relative = read_be32(data, section_offset + 0x1cU);
            const auto display_list_relative = read_be32(data, section_offset + 0x20U);
            const auto matrix_init_relative = read_be32(data, section_offset + 0x24U);
            const auto draw_init_relative = read_be32(data, section_offset + 0x28U);
            if (!has_relative_offset(shape_init_relative) || !has_relative_offset(index_table_relative) || !has_relative_offset(vtx_desc_relative) ||
                !has_relative_offset(display_list_relative) || !has_relative_offset(matrix_init_relative) ||
                !has_relative_offset(draw_init_relative)) {
                throw std::runtime_error("J3D SHP1 missing required tables");
            }

            const auto names = read_name_table(data, section_offset, name_table_relative);
            const auto material_indices = shape_materials_from_hierarchy(info, shape_count);
            const auto joint_indices = shape_joints_from_hierarchy(info, shape_count);
            const auto draw_orders = shape_draw_orders_from_hierarchy(info, shape_count);
            auto summary = J3dShapeBlockSummary{};
            summary.shape_count = shape_count;
            summary.shapes.reserve(shape_count);

            const auto formats = vertices.has_value() ? vertices->formats : std::vector<J3dVertexAttributeFormat>{};
            for (auto i = std::uint16_t{}; i < shape_count; ++i) {
                const auto shape_init_index = read_be16(data, relative_offset(section_offset, index_table_relative) + i * 2U);
                const auto shape_init_offset = relative_offset(section_offset, shape_init_relative) + shape_init_index * 0x28U;
                const auto matrix_group_count = read_be16(data, shape_init_offset + 0x02U);
                const auto vertex_desc_list_index = read_be16(data, shape_init_offset + 0x04U);
                const auto matrix_init_data_index = read_be16(data, shape_init_offset + 0x06U);
                const auto draw_init_data_index = read_be16(data, shape_init_offset + 0x08U);

                auto shape = J3dShapeSummary{};
                shape.name = i < names.size() ? names[i] : std::string{};
                shape.index = i;
                shape.draw_order = i < draw_orders.size() ? draw_orders[i] : static_cast<std::uint16_t>(0xffffU);
                shape.material_index = i < material_indices.size() ? material_indices[i] : static_cast<std::uint16_t>(0xffffU);
                shape.joint_index = i < joint_indices.size() ? joint_indices[i] : static_cast<std::uint16_t>(0xffffU);
                shape.matrix_type = data[shape_init_offset];
                shape.matrix_group_count = matrix_group_count;
                shape.vertex_desc_list_index = vertex_desc_list_index;
                shape.matrix_init_data_index = matrix_init_data_index;
                shape.draw_init_data_index = draw_init_data_index;
                shape.radius = read_be_float(data, shape_init_offset + 0x0cU);
                shape.min = {read_be_float(data, shape_init_offset + 0x10U), read_be_float(data, shape_init_offset + 0x14U),
                             read_be_float(data, shape_init_offset + 0x18U)};
                shape.max = {read_be_float(data, shape_init_offset + 0x1cU), read_be_float(data, shape_init_offset + 0x20U),
                             read_be_float(data, shape_init_offset + 0x24U)};

                shape.vertex_desc = parse_vertex_desc(data, relative_offset(section_offset, vtx_desc_relative) + vertex_desc_list_index);

                for (auto group = 0U; group < matrix_group_count; ++group) {
                    const auto matrix_init_offset =
                        relative_offset(section_offset, matrix_init_relative) + static_cast<std::size_t>(matrix_init_data_index + group) * 8U;
                    const auto draw_init_offset = relative_offset(section_offset, draw_init_relative) + (draw_init_data_index + group) * 8U;
                    const auto use_matrix_index = read_be16(data, matrix_init_offset);
                    const auto use_matrix_count = read_be16(data, matrix_init_offset + 2U);
                    const auto first_matrix_table_index = read_be32(data, matrix_init_offset + 4U);
                    const auto display_list_size = read_be32(data, draw_init_offset);
                    const auto display_list_index = read_be32(data, draw_init_offset + 4U);

                    auto matrix_group = J3dShapeMatrixGroupSummary{
                        .group_index = static_cast<std::uint16_t>(group),
                        .use_matrix_index = use_matrix_index,
                        .use_matrix_count = use_matrix_count,
                        .first_matrix_table_index = first_matrix_table_index,
                        .display_list_offset = display_list_index,
                        .display_list_size = display_list_size,
                        .matrix_table = {},
                        .primitives = {},
                        .parsed_display_list_bytes = 0U,
                        .triangle_count = 0U,
                    };
                    if (shape.matrix_type == 3U && has_relative_offset(matrix_table_relative) && use_matrix_count > 0U) {
                        const auto matrix_table_offset =
                            relative_offset(section_offset, matrix_table_relative) + static_cast<std::size_t>(first_matrix_table_index) * 2U;
                        matrix_group.matrix_table.reserve(use_matrix_count);
                        for (auto matrix = 0U; matrix < use_matrix_count; ++matrix) {
                            matrix_group.matrix_table.push_back(read_be16(data, matrix_table_offset + matrix * 2U));
                        }
                    } else if (use_matrix_index != 0xffffU) {
                        matrix_group.matrix_table.push_back(use_matrix_index);
                    }

                    shape.display_list_bytes += display_list_size;

                    auto parsed_bytes = std::uint32_t{};
                    auto primitives = parse_display_list(data, relative_offset(section_offset, display_list_relative) + display_list_index,
                                                         display_list_size, shape.vertex_desc, formats, parsed_bytes);
                    shape.parsed_display_list_bytes += parsed_bytes;
                    for (const auto &primitive : primitives) {
                        shape.triangle_count += primitive.triangle_count;
                    }
                    matrix_group.parsed_display_list_bytes = parsed_bytes;
                    for (const auto &primitive : primitives) {
                        matrix_group.triangle_count += primitive.triangle_count;
                    }
                    matrix_group.primitives = primitives;
                    shape.primitives.insert(shape.primitives.end(), primitives.begin(), primitives.end());
                    shape.matrix_groups.push_back(std::move(matrix_group));
                }

                summary.shapes.push_back(std::move(shape));
            }

            return summary;
        }

        [[nodiscard]] J3dMaterialBlockSummary parse_mat3(std::span<const std::uint8_t> data, const J3dSectionInfo &section) {
            const auto section_offset = static_cast<std::size_t>(section.offset);
            if (section.size < 0x84U) {
                throw std::runtime_error("J3D MAT3 section is too small");
            }

            const auto material_count = read_be16(data, section_offset + 0x08U);
            const auto init_relative = read_be32(data, section_offset + 0x0cU);
            const auto material_id_relative = read_be32(data, section_offset + 0x10U);
            const auto name_table_relative = read_be32(data, section_offset + 0x14U);
            const auto cull_mode_relative = read_be32(data, section_offset + 0x1cU);
            const auto mat_color_relative = read_be32(data, section_offset + 0x20U);
            const auto color_chan_num_relative = read_be32(data, section_offset + 0x24U);
            const auto color_chan_relative = read_be32(data, section_offset + 0x28U);
            const auto amb_color_relative = read_be32(data, section_offset + 0x2cU);
            const auto texgen_count_relative = read_be32(data, section_offset + 0x34U);
            const auto texcoord_relative = read_be32(data, section_offset + 0x38U);
            const auto texmtx_relative = read_be32(data, section_offset + 0x40U);
            const auto tex_no_relative = read_be32(data, section_offset + 0x48U);
            const auto tev_order_relative = read_be32(data, section_offset + 0x4cU);
            const auto tev_color_relative = read_be32(data, section_offset + 0x50U);
            const auto tev_k_color_relative = read_be32(data, section_offset + 0x54U);
            const auto tev_stage_count_relative = read_be32(data, section_offset + 0x58U);
            const auto tev_stage_relative = read_be32(data, section_offset + 0x5cU);
            const auto alpha_comp_relative = read_be32(data, section_offset + 0x6cU);
            const auto blend_relative = read_be32(data, section_offset + 0x70U);
            const auto z_mode_relative = read_be32(data, section_offset + 0x74U);
            const auto z_comp_loc_relative = read_be32(data, section_offset + 0x78U);
            if (!has_relative_offset(init_relative) || !has_relative_offset(material_id_relative)) {
                throw std::runtime_error("J3D MAT3 missing required tables");
            }

            const auto names = read_name_table(data, section_offset, name_table_relative);
            auto summary = J3dMaterialBlockSummary{};
            summary.material_count = material_count;
            summary.materials.reserve(material_count);

            for (auto i = std::uint16_t{}; i < material_count; ++i) {
                const auto material_id = read_be16(data, relative_offset(section_offset, material_id_relative) + i * 2U);
                const auto init_offset = relative_offset(section_offset, init_relative) + material_id * 0x14cU;
                auto material = J3dMaterialSummary{};
                material.name = i < names.size() ? names[i] : std::string{};
                material.index = i;
                material.material_id = material_id;
                material.material_mode = data[init_offset];
                material.cull_mode_index = data[init_offset + 0x01U];
                material.color_channel_count_index = data[init_offset + 0x02U];
                material.z_comp_loc_index = data[init_offset + 0x05U];
                material.z_mode_index = data[init_offset + 0x06U];
                material.alpha_comp_index = read_be16(data, init_offset + 0x146U);
                material.blend_index = read_be16(data, init_offset + 0x148U);
                if (material.cull_mode_index != 0xffU && has_relative_offset(cull_mode_relative)) {
                    material.cull_mode = data[relative_offset(section_offset, cull_mode_relative) + material.cull_mode_index];
                }
                if (material.z_comp_loc_index != 0xffU && has_relative_offset(z_comp_loc_relative)) {
                    material.z_comp_loc = data[relative_offset(section_offset, z_comp_loc_relative) + material.z_comp_loc_index];
                }
                if (material.z_mode_index != 0xffU && has_relative_offset(z_mode_relative)) {
                    const auto z_mode_offset = relative_offset(section_offset, z_mode_relative) + material.z_mode_index * 4U;
                    material.z_mode = J3dZModeSummary{
                        .compare_enable = data[z_mode_offset],
                        .function = data[z_mode_offset + 1U],
                        .update_enable = data[z_mode_offset + 2U],
                        .enabled = true,
                    };
                }
                if (has_relative_offset(mat_color_relative)) {
                    for (auto color_slot = 0U; color_slot < 2U; ++color_slot) {
                        const auto color_index = read_be16(data, init_offset + 0x08U + color_slot * 2U);
                        if (color_index == 0xffffU) {
                            continue;
                        }

                        const auto color_offset = relative_offset(section_offset, mat_color_relative) + color_index * 4U;
                        material.material_colors[color_slot] = {data[color_offset], data[color_offset + 1U], data[color_offset + 2U],
                                                                data[color_offset + 3U]};
                    }
                }
                if (material.color_channel_count_index != 0xffU && has_relative_offset(color_chan_num_relative)) {
                    material.color_channel_count = data[relative_offset(section_offset, color_chan_num_relative) + material.color_channel_count_index];
                }
                if (has_relative_offset(amb_color_relative)) {
                    for (auto color_slot = 0U; color_slot < 2U; ++color_slot) {
                        const auto color_index = read_be16(data, init_offset + 0x14U + color_slot * 2U);
                        if (color_index == 0xffffU) {
                            continue;
                        }

                        const auto color_offset = relative_offset(section_offset, amb_color_relative) + color_index * 4U;
                        material.ambient_colors[color_slot] = {data[color_offset], data[color_offset + 1U], data[color_offset + 2U],
                                                               data[color_offset + 3U]};
                    }
                }
                if (has_relative_offset(color_chan_relative)) {
                    for (auto control_slot = 0U; control_slot < 4U; ++control_slot) {
                        const auto control_index = read_be16(data, init_offset + 0x0cU + control_slot * 2U);
                        if (control_index == 0xffffU) {
                            continue;
                        }

                        const auto control_offset = relative_offset(section_offset, color_chan_relative) + control_index * 8U;
                        auto control = gx_color_channel_control_from_j3d(data[control_offset], data[control_offset + 1U],
                                                                         data[control_offset + 2U], data[control_offset + 3U],
                                                                         data[control_offset + 4U], data[control_offset + 5U]);
                        if (control_slot == 0U) {
                            material.color_channel_controls[0U] = control;
                        } else if (control_slot == 1U) {
                            material.alpha_channel_controls[0U] = control;
                        } else if (control_slot == 2U) {
                            material.color_channel_controls[1U] = control;
                        } else {
                            material.alpha_channel_controls[1U] = control;
                        }
                    }
                }
                if (has_relative_offset(tev_k_color_relative)) {
                    for (auto color_slot = 0U; color_slot < 4U; ++color_slot) {
                        const auto color_index = read_be16(data, init_offset + 0x94U + color_slot * 2U);
                        if (color_index == 0xffffU) {
                            continue;
                        }

                        const auto color_offset = relative_offset(section_offset, tev_k_color_relative) + color_index * 4U;
                        material.tev_k_colors[color_slot] = {data[color_offset], data[color_offset + 1U], data[color_offset + 2U],
                                                             data[color_offset + 3U]};
                    }
                }
                if (has_relative_offset(tev_color_relative)) {
                    for (auto color_slot = 0U; color_slot < material.tev_colors.size(); ++color_slot) {
                        const auto color_index = read_be16(data, init_offset + 0x8cU + color_slot * 2U);
                        if (color_index == 0xffffU) {
                            continue;
                        }

                        const auto color_offset = relative_offset(section_offset, tev_color_relative) + color_index * 8U;
                        material.tev_colors[color_slot] = {
                            read_be_s16(data, color_offset),
                            read_be_s16(data, color_offset + 2U),
                            read_be_s16(data, color_offset + 4U),
                            read_be_s16(data, color_offset + 6U),
                        };
                    }
                }

                const auto texgen_count_index = data[init_offset + 0x03U];
                if (texgen_count_index != 0xffU && has_relative_offset(texgen_count_relative)) {
                    material.texgen_count = data[relative_offset(section_offset, texgen_count_relative) + texgen_count_index];
                }

                const auto tev_stage_count_index = data[init_offset + 0x04U];
                if (tev_stage_count_index != 0xffU && has_relative_offset(tev_stage_count_relative)) {
                    material.tev_stage_count = data[relative_offset(section_offset, tev_stage_count_relative) + tev_stage_count_index];
                }

                if (has_relative_offset(tex_no_relative)) {
                    for (auto slot = 0U; slot < 8U; ++slot) {
                        const auto tex_no_index = read_be16(data, init_offset + 0x84U + slot * 2U);
                        if (tex_no_index == 0xffffU) {
                            continue;
                        }

                        material.textures.push_back(J3dMaterialTextureBinding{
                            .slot = static_cast<std::uint8_t>(slot),
                            .texture_index = read_be16(data, relative_offset(section_offset, tex_no_relative) + tex_no_index * 2U),
                        });
                    }
                }

                if (has_relative_offset(texcoord_relative)) {
                    for (auto slot = 0U; slot < material.texgen_count && slot < 8U; ++slot) {
                        const auto texcoord_index = read_be16(data, init_offset + 0x28U + slot * 2U);
                        if (texcoord_index == 0xffffU) {
                            continue;
                        }

                        const auto texcoord_offset = relative_offset(section_offset, texcoord_relative) + texcoord_index * 4U;
                        material.tex_coord_gens.push_back(J3dTexCoordGenSummary{
                            .slot = static_cast<std::uint8_t>(slot),
                            .type = data[texcoord_offset],
                            .source = data[texcoord_offset + 1U],
                            .matrix = data[texcoord_offset + 2U],
                        });
                    }
                }

                if (has_relative_offset(texmtx_relative)) {
                    for (auto slot = 0U; slot < 8U; ++slot) {
                        const auto texmtx_index = read_be16(data, init_offset + 0x48U + slot * 2U);
                        if (texmtx_index == 0xffffU) {
                            continue;
                        }

                        const auto texmtx_offset = relative_offset(section_offset, texmtx_relative) + texmtx_index * 0x64U;
                        auto tex_matrix = J3dTexMatrixSummary{
                            .slot = static_cast<std::uint8_t>(slot),
                            .projection = data[texmtx_offset],
                            .info = data[texmtx_offset + 1U],
                            .center =
                                {
                                    read_be_float(data, texmtx_offset + 0x04U),
                                    read_be_float(data, texmtx_offset + 0x08U),
                                    read_be_float(data, texmtx_offset + 0x0cU),
                                },
                            .scale_s = read_be_float(data, texmtx_offset + 0x10U),
                            .scale_t = read_be_float(data, texmtx_offset + 0x14U),
                            .rotation = read_be_s16(data, texmtx_offset + 0x18U),
                            .translate_s = read_be_float(data, texmtx_offset + 0x1cU),
                            .translate_t = read_be_float(data, texmtx_offset + 0x20U),
                        };
                        for (auto value = 0U; value < tex_matrix.effect_matrix.size(); ++value) {
                            tex_matrix.effect_matrix[value] = read_be_float(data, texmtx_offset + 0x24U + value * 4U);
                        }
                        material.tex_matrices.push_back(tex_matrix);
                    }
                }

                if (has_relative_offset(tev_order_relative)) {
                    for (auto stage = 0U; stage < material.tev_stage_count && stage < 16U; ++stage) {
                        const auto tev_order_index = read_be16(data, init_offset + 0xbcU + stage * 2U);
                        if (tev_order_index == 0xffffU) {
                            continue;
                        }

                        const auto tev_order_offset = relative_offset(section_offset, tev_order_relative) + tev_order_index * 4U;
                        material.tev_orders.push_back(J3dTevOrderSummary{
                            .stage = static_cast<std::uint8_t>(stage),
                            .tex_coord = data[tev_order_offset],
                            .tex_map = data[tev_order_offset + 1U],
                            .color_channel = data[tev_order_offset + 2U],
                        });
                    }
                }

                if (has_relative_offset(tev_stage_relative)) {
                    for (auto stage = 0U; stage < material.tev_stage_count && stage < 16U; ++stage) {
                        const auto tev_stage_index = read_be16(data, init_offset + 0xe4U + stage * 2U);
                        if (tev_stage_index == 0xffffU) {
                            continue;
                        }

                        const auto tev_stage_offset = relative_offset(section_offset, tev_stage_relative) + tev_stage_index * 20U;
                        auto tev_stage = J3dTevStageSummary{
                            .stage = static_cast<std::uint8_t>(stage),
                        };
                        std::ranges::copy(data.subspan(tev_stage_offset, tev_stage.raw.size()), tev_stage.raw.begin());
                        tev_stage.color_in = {tev_stage.raw[1U], tev_stage.raw[2U], tev_stage.raw[3U], tev_stage.raw[4U]};
                        tev_stage.color_op = tev_stage.raw[5U];
                        tev_stage.color_bias = tev_stage.raw[6U];
                        tev_stage.color_scale = tev_stage.raw[7U];
                        tev_stage.color_clamp = tev_stage.raw[8U];
                        tev_stage.color_out = tev_stage.raw[9U];
                        tev_stage.k_color_sel = data[init_offset + 0x9cU + stage];
                        tev_stage.alpha_in = {tev_stage.raw[10U], tev_stage.raw[11U], tev_stage.raw[12U], tev_stage.raw[13U]};
                        tev_stage.alpha_op = tev_stage.raw[14U];
                        tev_stage.alpha_bias = tev_stage.raw[15U];
                        tev_stage.alpha_scale = tev_stage.raw[16U];
                        tev_stage.alpha_clamp = tev_stage.raw[17U];
                        tev_stage.alpha_out = tev_stage.raw[18U];
                        tev_stage.k_alpha_sel = data[init_offset + 0xacU + stage];
                        material.tev_stages.push_back(tev_stage);
                    }
                }

                if (material.alpha_comp_index != 0xffffU && has_relative_offset(alpha_comp_relative)) {
                    const auto alpha_offset = relative_offset(section_offset, alpha_comp_relative) + material.alpha_comp_index * 8U;
                    material.alpha_compare = J3dAlphaCompareSummary{
                        .comp0 = data[alpha_offset],
                        .ref0 = data[alpha_offset + 1U],
                        .op = data[alpha_offset + 2U],
                        .comp1 = data[alpha_offset + 3U],
                        .ref1 = data[alpha_offset + 4U],
                        .enabled = true,
                    };
                }

                if (material.blend_index != 0xffffU && has_relative_offset(blend_relative)) {
                    const auto blend_offset = relative_offset(section_offset, blend_relative) + material.blend_index * 4U;
                    material.blend = J3dBlendSummary{
                        .type = data[blend_offset],
                        .src_factor = data[blend_offset + 1U],
                        .dst_factor = data[blend_offset + 2U],
                        .op = data[blend_offset + 3U],
                        .enabled = true,
                    };
                }

                material.gx_state = gx_state_from_j3d_material(material);
                summary.materials.push_back(std::move(material));
            }

            return summary;
        }

        [[nodiscard]] J3dMdl3BlockSummary parse_mdl3(std::span<const std::uint8_t> data, const J3dSectionInfo &section) {
            const auto section_offset = static_cast<std::size_t>(section.offset);
            if (section.size < 0x20U) {
                throw std::runtime_error("J3D MDL3 section is too small");
            }

            auto summary = J3dMdl3BlockSummary{};
            summary.material_count = read_be16(data, section_offset + 0x08U);
            const auto display_init_relative = read_be32(data, section_offset + 0x0cU);
            if (!has_relative_offset(display_init_relative)) {
                return summary;
            }

            const auto display_init_offset = relative_offset(section_offset, display_init_relative);
            if (display_init_offset + static_cast<std::size_t>(summary.material_count) * 8U > data.size()) {
                throw std::runtime_error("J3D MDL3 display-list init table outside buffer");
            }

            summary.packets.reserve(summary.material_count);
            for (auto i = 0U; i < summary.material_count; ++i) {
                const auto entry_offset = display_init_offset + i * 8U;
                const auto packet_offset = entry_offset + read_be32(data, entry_offset);
                const auto packet_size = read_be32(data, entry_offset + 4U);
                if (packet_offset + packet_size > data.size()) {
                    throw std::runtime_error("J3D MDL3 display list outside buffer");
                }

                auto packet = J3dMdl3PacketSummary{
                    .offset = static_cast<std::uint32_t>(packet_offset - section_offset),
                    .size = packet_size,
                    .bytes = {},
                };
                packet.bytes.assign(data.begin() + static_cast<std::ptrdiff_t>(packet_offset),
                                    data.begin() + static_cast<std::ptrdiff_t>(packet_offset + packet_size));
                summary.packets.push_back(std::move(packet));
            }

            return summary;
        }

        struct RawVertexSource {
            std::vector<J3dVertexAttributeFormat> formats;
            std::array<std::uint32_t, 26U> array_offsets{};
            std::size_t section_offset = 0U;
        };

        struct RawDisplayVertex {
            std::uint8_t position_matrix_slot = 0xffU;
            std::uint32_t pos_index = 0U;
            std::uint32_t normal_index = std::numeric_limits<std::uint32_t>::max();
            std::uint32_t color0_index = std::numeric_limits<std::uint32_t>::max();
            std::array<std::uint32_t, 8U> tex_coord_indices{
                std::numeric_limits<std::uint32_t>::max(),
                std::numeric_limits<std::uint32_t>::max(),
                std::numeric_limits<std::uint32_t>::max(),
                std::numeric_limits<std::uint32_t>::max(),
                std::numeric_limits<std::uint32_t>::max(),
                std::numeric_limits<std::uint32_t>::max(),
                std::numeric_limits<std::uint32_t>::max(),
                std::numeric_limits<std::uint32_t>::max(),
            };
        };

        [[nodiscard]] RawVertexSource parse_raw_vertex_source(std::span<const std::uint8_t> data, const J3dSectionInfo &section) {
            const auto section_offset = static_cast<std::size_t>(section.offset);
            auto source = RawVertexSource{};
            source.section_offset = section_offset;
            const auto format_relative = read_be32(data, section_offset + 0x08U);
            if (has_relative_offset(format_relative)) {
                source.formats = parse_vertex_formats(data, relative_offset(section_offset, format_relative));
            }

            constexpr std::array<std::uint32_t, 12U> attrs{
                9U,
                10U,
                25U,
                11U,
                12U,
                13U,
                14U,
                15U,
                16U,
                17U,
                18U,
                19U,
            };
            for (auto i = std::size_t{}; i < attrs.size(); ++i) {
                source.array_offsets[attrs[i]] = read_be32(data, section_offset + 0x0cU + i * 4U);
            }

            return source;
        }

        [[nodiscard]] float fixed_scale(std::uint8_t fraction) {
            return static_cast<float>(1U << fraction);
        }

        [[nodiscard]] float read_component(std::span<const std::uint8_t> data, std::size_t offset, std::uint32_t component_type,
                                           std::uint8_t fraction) {
            switch (component_type) {
            case 0U:
                return static_cast<float>(data[offset]) / fixed_scale(fraction);
            case 1U:
                return static_cast<float>(static_cast<std::int8_t>(data[offset])) / fixed_scale(fraction);
            case 2U:
                return static_cast<float>(read_be16(data, offset)) / fixed_scale(fraction);
            case 3U:
                return static_cast<float>(static_cast<std::int16_t>(read_be16(data, offset))) / fixed_scale(fraction);
            case 4U:
                return read_be_float(data, offset);
            default:
                return 0.0F;
            }
        }

        [[nodiscard]] std::array<float, 3U> read_position(std::span<const std::uint8_t> data, const RawVertexSource &source,
                                                          std::uint32_t index) {
            const auto *format = format_for(source.formats, 9U);
            if (format == nullptr || source.array_offsets[9U] == 0U) {
                return {};
            }

            const auto stride = direct_attribute_size(source.formats, 9U);
            const auto offset = source.section_offset + source.array_offsets[9U] + static_cast<std::size_t>(index) * stride;
            return {
                read_component(data, offset, format->component_type, format->fraction),
                read_component(data, offset + scalar_component_size(format->component_type), format->component_type, format->fraction),
                read_component(data, offset + scalar_component_size(format->component_type) * 2U, format->component_type, format->fraction),
            };
        }

        [[nodiscard]] std::array<float, 3U> read_normal(std::span<const std::uint8_t> data, const RawVertexSource &source, std::uint32_t index) {
            const auto *format = format_for(source.formats, 10U);
            if (format == nullptr || source.array_offsets[10U] == 0U) {
                return {0.0F, 0.0F, 1.0F};
            }

            const auto stride = direct_attribute_size(source.formats, 10U);
            const auto component_size = scalar_component_size(format->component_type);
            const auto offset = source.section_offset + source.array_offsets[10U] + static_cast<std::size_t>(index) * stride;
            return {
                read_component(data, offset, format->component_type, format->fraction),
                read_component(data, offset + component_size, format->component_type, format->fraction),
                read_component(data, offset + component_size * 2U, format->component_type, format->fraction),
            };
        }

        [[nodiscard]] std::array<float, 2U> read_texcoord(std::span<const std::uint8_t> data, const RawVertexSource &source, std::uint8_t slot,
                                                         std::uint32_t index) {
            const auto attr = static_cast<std::uint32_t>(13U + slot);
            const auto *format = format_for(source.formats, attr);
            if (format == nullptr || source.array_offsets[attr] == 0U) {
                return {};
            }

            const auto stride = direct_attribute_size(source.formats, attr);
            const auto component_size = scalar_component_size(format->component_type);
            const auto offset = source.section_offset + source.array_offsets[attr] + static_cast<std::size_t>(index) * stride;
            return {
                read_component(data, offset, format->component_type, format->fraction),
                read_component(data, offset + component_size, format->component_type, format->fraction),
            };
        }

        [[nodiscard]] std::array<std::uint8_t, 4U> read_color0(std::span<const std::uint8_t> data, const RawVertexSource &source,
                                                               std::uint32_t index) {
            const auto *format = format_for(source.formats, 11U);
            if (format == nullptr || source.array_offsets[11U] == 0U) {
                return {255U, 255U, 255U, 255U};
            }

            const auto offset =
                source.section_offset + source.array_offsets[11U] + static_cast<std::size_t>(index) * direct_attribute_size(source.formats, 11U);
            switch (format->component_type) {
            case 1U:
                return {data[offset], data[offset + 1U], data[offset + 2U], 255U};
            case 5U:
            default:
                return {data[offset], data[offset + 1U], data[offset + 2U], data[offset + 3U]};
            }
        }

        [[nodiscard]] J3dMeshVertex make_mesh_vertex(std::span<const std::uint8_t> data, const RawVertexSource &source,
                                                     const RawDisplayVertex &display_vertex, std::uint16_t draw_matrix_index) {
            const auto position = read_position(data, source, display_vertex.pos_index);
            const auto normal = display_vertex.normal_index == std::numeric_limits<std::uint32_t>::max() ?
                                    std::array<float, 3U>{0.0F, 0.0F, 1.0F} :
                                    read_normal(data, source, display_vertex.normal_index);
            auto tex_coords = std::array<std::array<float, 2U>, 8U>{};
            auto tex_coord_count = std::uint8_t{};
            for (auto slot = std::uint8_t{}; slot < tex_coords.size(); ++slot) {
                if (display_vertex.tex_coord_indices[slot] == std::numeric_limits<std::uint32_t>::max()) {
                    continue;
                }

                tex_coords[slot] = read_texcoord(data, source, slot, display_vertex.tex_coord_indices[slot]);
                tex_coord_count = static_cast<std::uint8_t>(slot + 1U);
            }
            const auto color = display_vertex.color0_index == std::numeric_limits<std::uint32_t>::max() ?
                                   std::array<std::uint8_t, 4U>{255U, 255U, 255U, 255U} :
                                   read_color0(data, source, display_vertex.color0_index);

            return J3dMeshVertex{
                .x = position[0U],
                .y = position[1U],
                .z = position[2U],
                .normal = normal,
                .u = tex_coords[0U][0U],
                .v = tex_coords[0U][1U],
                .tex_coords = tex_coords,
                .tex_coord_count = tex_coord_count,
                .color = color,
                .position_matrix_slot = display_vertex.position_matrix_slot,
                .draw_matrix_index = draw_matrix_index,
            };
        }

        [[nodiscard]] std::uint32_t read_attribute_value(std::span<const std::uint8_t> data, std::size_t &cursor, std::uint32_t type,
                                                         std::uint32_t direct_size) {
            switch (type) {
            case GX_DIRECT:
                cursor += direct_size;
                return std::numeric_limits<std::uint32_t>::max();
            case GX_INDEX8:
                return data[cursor++];
            case GX_INDEX16: {
                const auto value = read_be16(data, cursor);
                cursor += 2U;
                return value;
            }
            default:
                return std::numeric_limits<std::uint32_t>::max();
            }
        }

        [[nodiscard]] RawDisplayVertex read_display_vertex(std::span<const std::uint8_t> data, std::size_t &cursor,
                                                           const std::vector<J3dVertexDesc> &desc,
                                                           const std::vector<J3dVertexAttributeFormat> &formats) {
            auto vertex = RawDisplayVertex{};
            for (const auto &entry : desc) {
                if (entry.type == GX_NONE) {
                    continue;
                }

                auto value = std::uint32_t{};
                if (entry.type == GX_DIRECT && entry.attr <= 8U) {
                    value = data[cursor++] & 0x3fU;
                } else {
                    value = read_attribute_value(data, cursor, entry.type, direct_attribute_size(formats, entry.attr));
                }
                if (value == std::numeric_limits<std::uint32_t>::max()) {
                    continue;
                }

                if (entry.attr == 0U) {
                    vertex.position_matrix_slot = static_cast<std::uint8_t>(value & 0x3fU);
                } else if (entry.attr == 9U) {
                    vertex.pos_index = value;
                } else if (entry.attr == 10U) {
                    vertex.normal_index = value;
                } else if (entry.attr == 11U) {
                    vertex.color0_index = value;
                } else if (entry.attr >= 13U && entry.attr <= 20U) {
                    vertex.tex_coord_indices[entry.attr - 13U] = value;
                }
            }

            return vertex;
        }

        void push_triangle(std::vector<std::uint16_t> &indices, std::uint16_t a, std::uint16_t b, std::uint16_t c) {
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(c);
        }

        void append_primitive_indices(std::vector<std::uint16_t> &indices, std::uint8_t primitive, std::span<const std::uint16_t> vertices) {
            switch (primitive) {
            case 0x80U:
                for (auto i = std::size_t{}; i + 3U < vertices.size(); i += 4U) {
                    push_triangle(indices, vertices[i], vertices[i + 1U], vertices[i + 2U]);
                    push_triangle(indices, vertices[i], vertices[i + 2U], vertices[i + 3U]);
                }
                break;
            case 0x90U:
                for (auto i = std::size_t{}; i + 2U < vertices.size(); i += 3U) {
                    push_triangle(indices, vertices[i], vertices[i + 1U], vertices[i + 2U]);
                }
                break;
            case 0x98U:
                for (auto i = std::size_t{}; i + 2U < vertices.size(); ++i) {
                    if ((i & 1U) == 0U) {
                        push_triangle(indices, vertices[i], vertices[i + 1U], vertices[i + 2U]);
                    } else {
                        push_triangle(indices, vertices[i + 1U], vertices[i], vertices[i + 2U]);
                    }
                }
                break;
            case 0xa0U:
                for (auto i = std::size_t{1U}; i + 1U < vertices.size(); ++i) {
                    push_triangle(indices, vertices[0U], vertices[i], vertices[i + 1U]);
                }
                break;
            default:
                break;
            }
        }

        [[nodiscard]] std::uint16_t draw_matrix_index_for_vertex(const J3dShapeMatrixGroupSummary &group, std::uint8_t matrix_type,
                                                                 const RawDisplayVertex &vertex) {
            if (matrix_type == 3U && !group.matrix_table.empty()) {
                const auto slot = vertex.position_matrix_slot == 0xffU ? 0U : static_cast<std::uint32_t>(vertex.position_matrix_slot / 3U);
                if (slot < group.matrix_table.size()) {
                    return group.matrix_table[slot];
                }
            }

            if (!group.matrix_table.empty()) {
                return group.matrix_table.front();
            }

            return group.use_matrix_index;
        }

        void append_display_list_packet_mesh(std::span<const std::uint8_t> data, J3dShapeDrawPacketMesh &packet, const RawVertexSource &source,
                                             std::size_t offset, std::uint32_t size, const std::vector<J3dVertexDesc> &desc,
                                             std::uint8_t matrix_type) {
            if (offset + size > data.size()) {
                throw std::runtime_error("J3D geometry display list outside buffer");
            }

            const auto vertex_size = display_list_vertex_size(desc, source.formats);
            if (vertex_size == 0U) {
                return;
            }

            auto cursor = offset;
            const auto end = offset + size;
            while (cursor < end) {
                const auto command = data[cursor++];
                if (command == 0U) {
                    continue;
                }

                const auto primitive = static_cast<std::uint8_t>(command & 0xf8U);
                if (primitive < 0x80U || primitive > 0xb8U || cursor + 2U > end) {
                    if (skip_gx_display_list_command(data, cursor, end, command)) {
                        continue;
                    }

                    break;
                }

                const auto vertex_count = read_be16(data, cursor);
                cursor += 2U;

                const auto payload_size = static_cast<std::size_t>(vertex_count) * vertex_size;
                if (cursor + payload_size > end || packet.vertices.size() + vertex_count > std::numeric_limits<std::uint16_t>::max()) {
                    break;
                }

                auto primitive_vertices = std::vector<std::uint16_t>{};
                primitive_vertices.reserve(vertex_count);
                for (auto i = 0U; i < vertex_count; ++i) {
                    const auto display_vertex = read_display_vertex(data, cursor, desc, source.formats);
                    packet.vertices.push_back(make_mesh_vertex(data, source, display_vertex,
                                                               draw_matrix_index_for_vertex(packet.matrix_group, matrix_type, display_vertex)));
                    primitive_vertices.push_back(static_cast<std::uint16_t>(packet.vertices.size() - 1U));
                }

                append_primitive_indices(packet.indices, primitive, primitive_vertices);
            }
        }

        [[nodiscard]] std::vector<J3dShapeMesh> extract_shape_meshes(std::span<const std::uint8_t> data, const J3dSectionInfo &shape_section,
                                                                     const RawVertexSource &vertex_source,
                                                                     const std::optional<J3dInfoSummary> &info) {
            const auto section_offset = static_cast<std::size_t>(shape_section.offset);
            const auto shape_count = read_be16(data, section_offset + 0x08U);
            const auto shape_init_relative = read_be32(data, section_offset + 0x0cU);
            const auto index_table_relative = read_be32(data, section_offset + 0x10U);
            const auto vtx_desc_relative = read_be32(data, section_offset + 0x18U);
            const auto matrix_table_relative = read_be32(data, section_offset + 0x1cU);
            const auto display_list_relative = read_be32(data, section_offset + 0x20U);
            const auto matrix_init_relative = read_be32(data, section_offset + 0x24U);
            const auto draw_init_relative = read_be32(data, section_offset + 0x28U);
            const auto material_indices = shape_materials_from_hierarchy(info, shape_count);
            const auto joint_indices = shape_joints_from_hierarchy(info, shape_count);
            const auto draw_orders = shape_draw_orders_from_hierarchy(info, shape_count);

            auto meshes = std::vector<J3dShapeMesh>{};
            meshes.reserve(shape_count);
            for (auto i = std::uint16_t{}; i < shape_count; ++i) {
                const auto shape_init_index = read_be16(data, relative_offset(section_offset, index_table_relative) + i * 2U);
                const auto shape_init_offset = relative_offset(section_offset, shape_init_relative) + shape_init_index * 0x28U;
                const auto matrix_group_count = read_be16(data, shape_init_offset + 0x02U);
                const auto vertex_desc_list_index = read_be16(data, shape_init_offset + 0x04U);
                const auto matrix_init_data_index = read_be16(data, shape_init_offset + 0x06U);
                const auto draw_init_data_index = read_be16(data, shape_init_offset + 0x08U);
                const auto desc = parse_vertex_desc(data, relative_offset(section_offset, vtx_desc_relative) + vertex_desc_list_index);

                auto mesh = J3dShapeMesh{};
                mesh.shape_index = i;
                mesh.draw_order = i < draw_orders.size() ? draw_orders[i] : static_cast<std::uint16_t>(0xffffU);
                mesh.material_index = i < material_indices.size() ? material_indices[i] : static_cast<std::uint16_t>(0xffffU);
                mesh.joint_index = i < joint_indices.size() ? joint_indices[i] : static_cast<std::uint16_t>(0xffffU);
                mesh.matrix_type = data[shape_init_offset];
                for (auto group = 0U; group < matrix_group_count; ++group) {
                    const auto matrix_init_offset =
                        relative_offset(section_offset, matrix_init_relative) + static_cast<std::size_t>(matrix_init_data_index + group) * 8U;
                    const auto draw_init_offset = relative_offset(section_offset, draw_init_relative) + (draw_init_data_index + group) * 8U;
                    const auto use_matrix_index = read_be16(data, matrix_init_offset);
                    const auto use_matrix_count = read_be16(data, matrix_init_offset + 2U);
                    const auto first_matrix_table_index = read_be32(data, matrix_init_offset + 4U);
                    const auto display_list_size = read_be32(data, draw_init_offset);
                    const auto display_list_index = read_be32(data, draw_init_offset + 4U);

                    auto matrix_group = J3dShapeMatrixGroupSummary{
                        .group_index = static_cast<std::uint16_t>(group),
                        .use_matrix_index = use_matrix_index,
                        .use_matrix_count = use_matrix_count,
                        .first_matrix_table_index = first_matrix_table_index,
                        .display_list_offset = display_list_index,
                        .display_list_size = display_list_size,
                        .matrix_table = {},
                        .primitives = {},
                        .parsed_display_list_bytes = 0U,
                        .triangle_count = 0U,
                    };
                    if (mesh.matrix_type == 3U && has_relative_offset(matrix_table_relative) && use_matrix_count > 0U) {
                        const auto matrix_table_offset =
                            relative_offset(section_offset, matrix_table_relative) + static_cast<std::size_t>(first_matrix_table_index) * 2U;
                        matrix_group.matrix_table.reserve(use_matrix_count);
                        for (auto matrix = 0U; matrix < use_matrix_count; ++matrix) {
                            matrix_group.matrix_table.push_back(read_be16(data, matrix_table_offset + matrix * 2U));
                        }
                    } else if (use_matrix_index != 0xffffU) {
                        matrix_group.matrix_table.push_back(use_matrix_index);
                    }

                    auto parsed_bytes = std::uint32_t{};
                    auto primitives = parse_display_list(data, relative_offset(section_offset, display_list_relative) + display_list_index,
                                                         display_list_size, desc, vertex_source.formats, parsed_bytes);
                    matrix_group.parsed_display_list_bytes = parsed_bytes;
                    for (const auto &primitive : primitives) {
                        matrix_group.triangle_count += primitive.triangle_count;
                    }
                    matrix_group.primitives = std::move(primitives);

                    auto packet = J3dShapeDrawPacketMesh{
                        .matrix_group = std::move(matrix_group),
                        .vertices = {},
                        .indices = {},
                    };
                    append_display_list_packet_mesh(data, packet, vertex_source,
                                                    relative_offset(section_offset, display_list_relative) + display_list_index, display_list_size,
                                                    desc, mesh.matrix_type);
                    mesh.draw_packets.push_back(std::move(packet));
                }

                meshes.push_back(std::move(mesh));
            }

            return meshes;
        }

    }  // namespace

    J3dModelSummary inspect_j3d_model(std::span<const std::uint8_t> model_data) {
        if (model_data.size() < 0x20U) {
            throw std::runtime_error("J3D model is too small");
        }

        auto summary = J3dModelSummary{};
        summary.magic = read_be32(model_data, 0U);
        summary.model_type = read_be32(model_data, 4U);
        summary.section_count = read_be32(model_data, 0x0cU);
        if (summary.magic != J3D1_MAGIC && summary.magic != J3D2_MAGIC) {
            throw std::runtime_error("J3D model has unexpected magic");
        }

        auto offset = std::size_t{0x20U};
        for (auto i = 0U; i < summary.section_count; ++i) {
            if (offset + 8U > model_data.size()) {
                throw std::runtime_error("J3D section header outside buffer");
            }

            const auto section_size = read_be32(model_data, offset + 4U);
            if (section_size < 8U || offset + section_size > model_data.size() || section_size > std::numeric_limits<std::uint32_t>::max()) {
                throw std::runtime_error("J3D section size outside buffer");
            }

            summary.sections.push_back(J3dSectionInfo{
                .tag = read_tag(model_data, offset),
                .offset = static_cast<std::uint32_t>(offset),
                .size = section_size,
            });
            offset += section_size;
        }

        if (const auto section = section_for(summary.sections, "INF1"); section.has_value()) {
            summary.info = parse_inf1(model_data, *section);
        }
        if (const auto section = section_for(summary.sections, "VTX1"); section.has_value()) {
            summary.vertices = parse_vtx1(model_data, *section);
        }
        if (const auto section = section_for(summary.sections, "EVP1"); section.has_value()) {
            summary.envelopes = parse_evp1(model_data, *section);
        }
        if (const auto section = section_for(summary.sections, "DRW1"); section.has_value()) {
            summary.draw_matrices = parse_drw1(model_data, *section);
        }
        if (const auto section = section_for(summary.sections, "JNT1"); section.has_value()) {
            summary.joints = parse_jnt1(model_data, *section);
            summary.joints->parent_indices = joint_parent_indices_from_hierarchy(summary.info, summary.joints->joint_count);
        }
        if (const auto section = section_for(summary.sections, "MAT3"); section.has_value()) {
            summary.materials = parse_mat3(model_data, *section);
        }
        if (const auto section = section_for(summary.sections, "MDL3"); section.has_value()) {
            summary.mdl3 = parse_mdl3(model_data, *section);
        }
        if (summary.materials.has_value() && summary.mdl3.has_value()) {
            const auto packet_count = std::min(summary.materials->materials.size(), summary.mdl3->packets.size());
            for (auto i = std::size_t{}; i < packet_count; ++i) {
                gx_apply_mdl3_display_list(summary.materials->materials[i].gx_state, summary.mdl3->packets[i].bytes);
            }
        }
        if (const auto section = section_for(summary.sections, "SHP1"); section.has_value()) {
            summary.shapes = parse_shp1(model_data, *section, summary.info, summary.vertices);
        }
        if (section_offset_for(summary.sections, "TEX1").has_value()) {
            summary.textures = extract_j3d_textures(model_data);
        }

        return summary;
    }

    J3dModelGeometry extract_j3d_model_geometry(std::span<const std::uint8_t> model_data) {
        const auto summary = inspect_j3d_model(model_data);
        auto geometry = J3dModelGeometry{};
        geometry.materials = summary.materials;
        geometry.envelopes = summary.envelopes;
        geometry.draw_matrices = summary.draw_matrices;
        geometry.joints = summary.joints;
        geometry.textures = summary.textures;

        const auto vertex_section = section_for(summary.sections, "VTX1");
        const auto shape_section = section_for(summary.sections, "SHP1");
        if (!vertex_section.has_value() || !shape_section.has_value()) {
            return geometry;
        }

        const auto vertex_source = parse_raw_vertex_source(model_data, *vertex_section);
        geometry.shapes = extract_shape_meshes(model_data, *shape_section, vertex_source, summary.info);
        return geometry;
    }

    std::string j3d_hierarchy_type_name(std::uint16_t type) {
        switch (type) {
        case 0x00U:
            return "end";
        case 0x01U:
            return "begin-child";
        case 0x02U:
            return "end-child";
        case 0x10U:
            return "joint";
        case 0x11U:
            return "material";
        case 0x12U:
            return "shape";
        default:
            return "unknown";
        }
    }

    std::string j3d_vertex_attr_name(std::uint32_t attr) {
        switch (attr) {
        case 0U:
            return "PNMTXIDX";
        case 1U:
            return "TEX0MTXIDX";
        case 2U:
            return "TEX1MTXIDX";
        case 3U:
            return "TEX2MTXIDX";
        case 4U:
            return "TEX3MTXIDX";
        case 5U:
            return "TEX4MTXIDX";
        case 6U:
            return "TEX5MTXIDX";
        case 7U:
            return "TEX6MTXIDX";
        case 8U:
            return "TEX7MTXIDX";
        case 9U:
            return "POS";
        case 10U:
            return "NRM";
        case 11U:
            return "CLR0";
        case 12U:
            return "CLR1";
        case 13U:
            return "TEX0";
        case 14U:
            return "TEX1";
        case 15U:
            return "TEX2";
        case 16U:
            return "TEX3";
        case 17U:
            return "TEX4";
        case 18U:
            return "TEX5";
        case 19U:
            return "TEX6";
        case 20U:
            return "TEX7";
        case 25U:
            return "NBT";
        case 0xffU:
            return "NULL";
        default:
            return "attr-" + std::to_string(attr);
        }
    }

    std::string j3d_vertex_attr_type_name(std::uint32_t type) {
        switch (type) {
        case GX_NONE:
            return "none";
        case GX_DIRECT:
            return "direct";
        case GX_INDEX8:
            return "index8";
        case GX_INDEX16:
            return "index16";
        default:
            return "type-" + std::to_string(type);
        }
    }

    std::string j3d_primitive_name(std::uint8_t primitive) {
        switch (primitive) {
        case 0x80U:
            return "quads";
        case 0x90U:
            return "triangles";
        case 0x98U:
            return "triangle-strip";
        case 0xa0U:
            return "triangle-fan";
        case 0xa8U:
            return "lines";
        case 0xb0U:
            return "line-strip";
        case 0xb8U:
            return "points";
        default:
            return "primitive-" + std::to_string(primitive);
        }
    }

}  // namespace smgpc::game
