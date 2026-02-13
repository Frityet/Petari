#include "Brlyt.hpp"

#include <stack>
#include <string>

#include "Binary.hpp"

namespace smgpc::assets::layout {
namespace {

[[nodiscard]] AssetError make_error(std::string message) {
    return AssetError {
        .code = AssetErrorCode::InvalidFormat,
        .message = std::move(message)
    };
}

[[nodiscard]] Color read_color_rgba(std::span<const std::byte> bytes, std::size_t offset) {
    const auto value = binary::read_u32_be(bytes, offset);
    return Color {
        .r = static_cast<std::uint8_t>((value >> 24U) & 0xFFU),
        .g = static_cast<std::uint8_t>((value >> 16U) & 0xFFU),
        .b = static_cast<std::uint8_t>((value >> 8U) & 0xFFU),
        .a = static_cast<std::uint8_t>(value & 0xFFU),
    };
}

[[nodiscard]] AssetResult<void> parse_texture_list(std::span<const std::byte> block, std::vector<std::string> *output) {
    using namespace binary;

    if (output == nullptr) {
        return make_error("Texture list output pointer is null.");
    }
    if (not has_bytes(block, 8U, 4U)) {
        return make_error("txl1 block is too small.");
    }

    const auto count = static_cast<std::size_t>(read_u16_be(block, 8U));
    const std::size_t list_base = 12U;
    if (not has_bytes(block, list_base, count * 8U)) {
        return make_error("txl1 entries exceed block bounds.");
    }

    output->clear();
    output->reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto entry_offset = list_base + i * 8U;
        const auto name_offset = static_cast<std::size_t>(read_u32_be(block, entry_offset));
        output->push_back(read_c_string(block, list_base + name_offset));
    }

    return {};
}

[[nodiscard]] AssetResult<void> parse_font_list(std::span<const std::byte> block, std::vector<std::string> *output) {
    using namespace binary;

    if (output == nullptr) {
        return make_error("Font list output pointer is null.");
    }
    if (not has_bytes(block, 8U, 4U)) {
        return make_error("fnl1 block is too small.");
    }

    const auto count = static_cast<std::size_t>(read_u16_be(block, 8U));
    const std::size_t list_base = 12U;
    if (not has_bytes(block, list_base, count * 8U)) {
        return make_error("fnl1 entries exceed block bounds.");
    }

    output->clear();
    output->reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto entry_offset = list_base + i * 8U;
        const auto name_offset = static_cast<std::size_t>(read_u32_be(block, entry_offset));
        output->push_back(read_c_string(block, list_base + name_offset));
    }

    return {};
}

[[nodiscard]] AssetResult<void> parse_materials(std::span<const std::byte> block, std::vector<MaterialDefinition> *materials) {
    using namespace binary;

    if (materials == nullptr) {
        return make_error("Material output pointer is null.");
    }
    if (not has_bytes(block, 8U, 4U)) {
        return make_error("mat1 block is too small.");
    }

    const auto count = static_cast<std::size_t>(read_u16_be(block, 8U));
    const std::size_t offset_table = 12U;
    if (not has_bytes(block, offset_table, count * 4U)) {
        return make_error("mat1 offset table exceeds block bounds.");
    }

    materials->clear();
    materials->reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto mat_offset = static_cast<std::size_t>(read_u32_be(block, offset_table + i * 4U));
        if (not has_bytes(block, mat_offset, 64U)) {
            return make_error("mat1 material body exceeds block bounds.");
        }

        MaterialDefinition material {};
        material.name = read_fixed_string(block, mat_offset, 20U);

        const auto res_num_bits = read_u32_be(block, mat_offset + 60U);
        const auto tex_map_num = static_cast<std::size_t>(res_num_bits & 0xFU);
        const auto tex_srt_num = static_cast<std::size_t>((res_num_bits >> 4U) & 0xFU);
        const auto tex_coord_num = static_cast<std::size_t>((res_num_bits >> 8U) & 0xFU);
        const auto has_tev_swap = ((res_num_bits >> 12U) & 0x1U) != 0U;
        const auto ind_tex_srt_num = static_cast<std::size_t>((res_num_bits >> 13U) & 0x3U);
        const auto ind_stage_num = static_cast<std::size_t>((res_num_bits >> 15U) & 0x7U);
        const auto tev_stage_num = static_cast<std::size_t>((res_num_bits >> 18U) & 0x1FU);
        const auto has_alpha_compare = ((res_num_bits >> 23U) & 0x1U) != 0U;
        const auto has_blend_mode = ((res_num_bits >> 24U) & 0x1U) != 0U;
        const auto chan_ctrl_num = static_cast<std::size_t>((res_num_bits >> 25U) & 0x1U);
        const auto mat_col_num = static_cast<std::size_t>((res_num_bits >> 27U) & 0x1U);
        material.tev_stage_count = static_cast<std::int32_t>(tev_stage_num);

        std::size_t resource_offset = 64U;
        if (not has_bytes(block, mat_offset, resource_offset)) {
            return make_error("mat1 material resource area exceeds block bounds.");
        }

        material.texture_indices.reserve(tex_map_num);
        if (tex_map_num > 0U) {
            if (not has_bytes(block, mat_offset + resource_offset, tex_map_num * 4U)) {
                return make_error("mat1 texture map array exceeds block bounds.");
            }

            for (std::size_t tex_map_index = 0; tex_map_index < tex_map_num; ++tex_map_index) {
                const auto tex_index = read_u16_be(block, mat_offset + resource_offset + tex_map_index * 4U);
                if (tex_index != 0xFFFFU) {
                    material.texture_indices.push_back(static_cast<std::int32_t>(tex_index));
                }
            }
        }

        resource_offset += tex_map_num * 4U;
        resource_offset += tex_srt_num * 20U;
        resource_offset += tex_coord_num * 4U;
        resource_offset += chan_ctrl_num * 4U;

        if (not material.texture_indices.empty()) {
            material.texture_index = material.texture_indices.front();
        }

        if (mat_col_num > 0U) {
            if (not has_bytes(block, mat_offset + resource_offset, 4U)) {
                return make_error("mat1 material color table exceeds block bounds.");
            }

            const auto mat_color = read_u32_be(block, mat_offset + resource_offset);
            material.mat_color = {
                static_cast<std::uint8_t>((mat_color >> 24U) & 0xFFU),
                static_cast<std::uint8_t>((mat_color >> 16U) & 0xFFU),
                static_cast<std::uint8_t>((mat_color >> 8U) & 0xFFU),
                static_cast<std::uint8_t>(mat_color & 0xFFU),
            };
        }

        resource_offset += mat_col_num * 4U;
        if (has_tev_swap) {
            resource_offset += 4U;
        }
        resource_offset += ind_tex_srt_num * 20U;
        resource_offset += ind_stage_num * 4U;
        resource_offset += tev_stage_num * 16U;
        if (has_alpha_compare) {
            resource_offset += 4U;
        }

        if (has_blend_mode) {
            if (not has_bytes(block, mat_offset + resource_offset, 4U)) {
                return make_error("mat1 blend mode table exceeds block bounds.");
            }

            constexpr std::uint8_t GX_BM_BLEND = 1U;
            constexpr std::uint8_t GX_BL_ONE = 1U;

            const auto blend_type = read_u8(block, mat_offset + resource_offset + 0U);
            const auto blend_src = read_u8(block, mat_offset + resource_offset + 1U);
            const auto blend_dst = read_u8(block, mat_offset + resource_offset + 2U);
            if (blend_type == GX_BM_BLEND && (blend_src == GX_BL_ONE || blend_dst == GX_BL_ONE)) {
                material.blend_mode = MaterialBlendMode::Additive;
            }
        }

        materials->push_back(std::move(material));
    }

    return {};
}

[[nodiscard]] PaneDefinition parse_common_pane(std::span<const std::byte> block) {
    using namespace binary;

    PaneDefinition pane {};
    const auto flags = read_u8(block, 8U);
    pane.visible = (flags & (1U << 0U)) != 0U;
    pane.influenced_alpha = (flags & (1U << 1U)) != 0U;
    pane.location_adjust = (flags & (1U << 2U)) != 0U;
    pane.base_position = read_u8(block, 9U);
    pane.alpha = read_u8(block, 10U);
    pane.name = read_fixed_string(block, 12U, 16U);
    pane.user_data = read_fixed_string(block, 28U, 8U);
    pane.translate = Vec3 {
        .x = read_f32_be(block, 36U),
        .y = read_f32_be(block, 40U),
        .z = read_f32_be(block, 44U),
    };
    pane.rotate = Vec3 {
        .x = read_f32_be(block, 48U),
        .y = read_f32_be(block, 52U),
        .z = read_f32_be(block, 56U),
    };
    pane.scale = Vec2 {
        .x = read_f32_be(block, 60U),
        .y = read_f32_be(block, 64U),
    };
    pane.size = Vec2 {
        .x = read_f32_be(block, 68U),
        .y = read_f32_be(block, 72U),
    };
    return pane;
}

[[nodiscard]] std::u16string decode_utf16be(std::span<const std::byte> bytes) {
    std::u16string text {};
    text.reserve(bytes.size() / 2U);
    for (std::size_t i = 0; i + 1U < bytes.size(); i += 2U) {
        const auto code_unit = static_cast<char16_t>(
            (static_cast<std::uint16_t>(binary::read_u8(bytes, i)) << 8U) |
            static_cast<std::uint16_t>(binary::read_u8(bytes, i + 1U)));
        if (code_unit == 0U) {
            break;
        }
        text.push_back(code_unit);
    }
    return text;
}

}  // namespace

AssetResult<LayoutDefinition> parse_brlyt(std::span<const std::byte> bytes) {
    using namespace binary;

    if (bytes.size() < 0x10U) {
        return make_error("BRLYT file is too small.");
    }
    if (not fourcc_equals(bytes, 0U, "RLYT")) {
        return make_error("BRLYT signature mismatch.");
    }

    const auto header_size = static_cast<std::size_t>(read_u16_be(bytes, 0x0CU));
    const auto block_count = static_cast<std::size_t>(read_u16_be(bytes, 0x0EU));
    if (header_size < 0x10U or header_size > bytes.size()) {
        return make_error("BRLYT header size is invalid.");
    }

    LayoutDefinition layout {};
    std::stack<std::int32_t> parent_stack {};
    std::int32_t last_pane {-1};

    std::size_t block_offset = header_size;
    for (std::size_t block_index = 0; block_index < block_count; ++block_index) {
        if (not has_bytes(bytes, block_offset, 8U)) {
            return make_error("BRLYT block header exceeds file bounds.");
        }

        const auto block_size = static_cast<std::size_t>(read_u32_be(bytes, block_offset + 4U));
        if (block_size < 8U or not has_bytes(bytes, block_offset, block_size)) {
            return make_error("BRLYT block size is invalid.");
        }

        const auto block = subspan(bytes, block_offset, block_size);
        const auto kind = read_fourcc(block, 0U);
        const auto kind_string = std::string(kind.begin(), kind.end());

        if (kind_string == "lyt1") {
            if (not has_bytes(block, 12U, 8U)) {
                return make_error("lyt1 block too small.");
            }
            layout.center_origin = read_u8(block, 8U) != 0U;
            layout.size = Vec2 {
                .x = read_f32_be(block, 12U),
                .y = read_f32_be(block, 16U),
            };
        } else if (kind_string == "txl1") {
            const auto parse_result = parse_texture_list(block, &layout.texture_names);
            if (not parse_result) {
                return parse_result.failure();
            }
        } else if (kind_string == "fnl1") {
            const auto parse_result = parse_font_list(block, &layout.font_names);
            if (not parse_result) {
                return parse_result.failure();
            }
        } else if (kind_string == "mat1") {
            const auto parse_result = parse_materials(block, &layout.materials);
            if (not parse_result) {
                return parse_result.failure();
            }
        } else if (kind_string == "pan1" or kind_string == "pic1" or kind_string == "txt1") {
            if (not has_bytes(block, 0U, 76U)) {
                return make_error("Pane block too small.");
            }

            PaneDefinition pane = parse_common_pane(block);
            if (kind_string == "pic1") {
                pane.type = PaneType::Picture;
                if (not has_bytes(block, 76U, 20U)) {
                    return make_error("pic1 block too small.");
                }
                pane.vertex_colors[0] = read_color_rgba(block, 76U);
                pane.vertex_colors[1] = read_color_rgba(block, 80U);
                pane.vertex_colors[2] = read_color_rgba(block, 84U);
                pane.vertex_colors[3] = read_color_rgba(block, 88U);
                pane.material_index = static_cast<std::int32_t>(read_u16_be(block, 92U));

                const auto tex_coord_count = static_cast<std::size_t>(read_u8(block, 94U));
                if (tex_coord_count > 0U) {
                    constexpr std::size_t UV_BASE = 96U;
                    constexpr std::size_t UV_SIZE = 32U;
                    if (not has_bytes(block, UV_BASE, UV_SIZE)) {
                        return make_error("pic1 texture coordinates exceed block bounds.");
                    }

                    pane.tex_coords = {
                        read_f32_be(block, UV_BASE + 0U),
                        read_f32_be(block, UV_BASE + 4U),
                        read_f32_be(block, UV_BASE + 8U),
                        read_f32_be(block, UV_BASE + 12U),
                        read_f32_be(block, UV_BASE + 16U),
                        read_f32_be(block, UV_BASE + 20U),
                        read_f32_be(block, UV_BASE + 24U),
                        read_f32_be(block, UV_BASE + 28U),
                    };
                }
            } else if (kind_string == "txt1") {
                pane.type = PaneType::Text;
                if (not has_bytes(block, 76U, 40U)) {
                    return make_error("txt1 block too small.");
                }
                const auto text_str_bytes = static_cast<std::size_t>(read_u16_be(block, 78U));
                pane.material_index = static_cast<std::int32_t>(read_u16_be(block, 80U));
                pane.font_index = static_cast<std::int32_t>(read_u16_be(block, 82U));
                pane.text_position = read_u8(block, 84U);
                pane.text_alignment = read_u8(block, 85U);
                const auto text_offset = static_cast<std::size_t>(read_u32_be(block, 88U));
                pane.text_colors[0] = read_color_rgba(block, 92U);
                pane.text_colors[1] = read_color_rgba(block, 96U);
                pane.text_font_size = Vec2 {
                    .x = read_f32_be(block, 100U),
                    .y = read_f32_be(block, 104U),
                };
                pane.text_char_space = read_f32_be(block, 108U);
                pane.text_line_space = read_f32_be(block, 112U);

                if (text_str_bytes >= 2U and text_offset > 0U and has_bytes(block, text_offset, text_str_bytes)) {
                    pane.text = decode_utf16be(subspan(block, text_offset, text_str_bytes));
                }
            }

            const auto pane_index = static_cast<std::int32_t>(layout.panes.size());
            pane.parent = parent_stack.empty() ? -1 : parent_stack.top();
            layout.panes.push_back(std::move(pane));

            if (layout.root_pane < 0) {
                layout.root_pane = pane_index;
            }
            if (not parent_stack.empty()) {
                layout.panes[parent_stack.top()].children.push_back(pane_index);
            }

            last_pane = pane_index;
        } else if (kind_string == "pas1") {
            if (last_pane >= 0) {
                parent_stack.push(last_pane);
            }
        } else if (kind_string == "pae1") {
            if (not parent_stack.empty()) {
                parent_stack.pop();
            }
        }

        block_offset += block_size;
    }

    return layout;
}

}  // namespace smgpc::assets::layout
