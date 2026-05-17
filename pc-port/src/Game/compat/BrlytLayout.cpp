#include "BrlytLayout.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace smgpc::game {
    namespace {

        struct PaneState {
            float translate_x = 0.0F;
            float translate_y = 0.0F;
            float rotate_z = 0.0F;
            float scale_x = 1.0F;
            float scale_y = 1.0F;
        };

        [[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 2U > data.size()) {
                throw std::runtime_error("BRLYT read_be16 out of range");
            }

            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8U) | data[offset + 1U]);
        }

        [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 4U > data.size()) {
                throw std::runtime_error("BRLYT read_be32 out of range");
            }

            return (static_cast<std::uint32_t>(data[offset]) << 24U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) | (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) | data[offset + 3U];
        }

        [[nodiscard]] std::int16_t read_be_s16(std::span<const std::uint8_t> data, std::size_t offset) {
            return std::bit_cast<std::int16_t>(read_be16(data, offset));
        }

        [[nodiscard]] float read_be_float(std::span<const std::uint8_t> data, std::size_t offset) {
            const auto bits = read_be32(data, offset);
            return std::bit_cast<float>(bits);
        }

        [[nodiscard]] bool has_magic(std::span<const std::uint8_t> data, std::size_t offset, std::string_view magic) {
            if (offset + magic.size() > data.size()) {
                return false;
            }

            for (std::size_t i = 0U; i < magic.size(); ++i) {
                if (data[offset + i] != static_cast<std::uint8_t>(magic[i])) {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] std::string read_fixed_string(std::span<const std::uint8_t> data, std::size_t offset, std::size_t capacity) {
            if (offset + capacity > data.size()) {
                throw std::runtime_error("BRLYT fixed string out of range");
            }

            auto length = 0U;
            while (length < capacity && data[offset + length] != 0U) {
                ++length;
            }

            return std::string(reinterpret_cast<const char *>(data.data() + offset), length);
        }

        [[nodiscard]] std::string read_c_string(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset >= data.size()) {
                throw std::runtime_error("BRLYT string out of range");
            }

            auto end = offset;
            while (end < data.size() && data[end] != 0U) {
                ++end;
            }
            if (end == data.size()) {
                throw std::runtime_error("BRLYT string is not null terminated");
            }

            return std::string(reinterpret_cast<const char *>(data.data() + offset), end - offset);
        }

        void parse_name_list(std::vector<std::string> &names, std::span<const std::uint8_t> block, std::string_view kind) {
            const auto name_count = read_be16(block, 8U);
            const auto name_array_offset = 12U;
            const auto name_array_size = static_cast<std::size_t>(name_count) * 8U;
            if (name_array_offset + name_array_size > block.size()) {
                throw std::runtime_error("BRLYT " + std::string(kind) + " list is truncated");
            }

            auto name_array = block.subspan(name_array_offset);
            names.clear();
            names.reserve(name_count);
            for (auto i = 0U; i < name_count; ++i) {
                const auto entry_offset = static_cast<std::size_t>(i) * 8U;
                const auto name_offset = read_be32(name_array, entry_offset);
                names.push_back(read_c_string(name_array, name_offset));
            }
        }

        void parse_texture_list(BrlytLayout &layout, std::span<const std::uint8_t> block) {
            parse_name_list(layout.texture_names, block, "texture");
        }

        void parse_font_list(BrlytLayout &layout, std::span<const std::uint8_t> block) {
            parse_name_list(layout.font_names, block, "font");
        }

        [[nodiscard]] std::uint8_t material_texmap_count(std::uint32_t bits) {
            return static_cast<std::uint8_t>(bits & 0xFU);
        }

        [[nodiscard]] std::uint8_t material_texsrt_count(std::uint32_t bits) {
            return static_cast<std::uint8_t>((bits >> 4U) & 0xFU);
        }

        [[nodiscard]] std::uint8_t material_texcoordgen_count(std::uint32_t bits) {
            return static_cast<std::uint8_t>((bits >> 8U) & 0xFU);
        }

        [[nodiscard]] bool material_has_tev_swap_table(std::uint32_t bits) {
            return ((bits >> 12U) & 0x1U) != 0U;
        }

        [[nodiscard]] std::uint8_t material_indtexsrt_count(std::uint32_t bits) {
            return static_cast<std::uint8_t>((bits >> 13U) & 0x3U);
        }

        [[nodiscard]] std::uint8_t material_indtexstage_count(std::uint32_t bits) {
            return static_cast<std::uint8_t>((bits >> 15U) & 0x7U);
        }

        [[nodiscard]] std::uint8_t material_tev_stage_count(std::uint32_t bits) {
            return static_cast<std::uint8_t>((bits >> 18U) & 0x1FU);
        }

        [[nodiscard]] bool material_has_alpha_compare(std::uint32_t bits) {
            return ((bits >> 23U) & 0x1U) != 0U;
        }

        [[nodiscard]] bool material_has_blend_mode(std::uint32_t bits) {
            return ((bits >> 24U) & 0x1U) != 0U;
        }

        [[nodiscard]] bool material_has_chan_ctrl(std::uint32_t bits) {
            return ((bits >> 25U) & 0x1U) != 0U;
        }

        [[nodiscard]] bool material_has_mat_color(std::uint32_t bits) {
            return ((bits >> 27U) & 0x1U) != 0U;
        }

        [[nodiscard]] std::uint8_t clamp_material_color(std::int16_t value) {
            return static_cast<std::uint8_t>(std::clamp<std::int16_t>(value, 0, 255));
        }

        [[nodiscard]] std::array<std::uint8_t, 4U> material_color_to_rgba8(const GXTevRegisterColor &color) {
            return std::array<std::uint8_t, 4U>{
                clamp_material_color(color[0U]),
                clamp_material_color(color[1U]),
                clamp_material_color(color[2U]),
                clamp_material_color(color[3U]),
            };
        }

        [[nodiscard]] BrlytTevStageInOp parse_tev_stage_in_op(std::span<const std::uint8_t> block, std::size_t offset) {
            const auto ab = block[offset];
            const auto cd = block[offset + 1U];
            const auto op = block[offset + 2U];
            const auto cl = block[offset + 3U];
            return BrlytTevStageInOp{
                .a = static_cast<std::uint8_t>(ab & 0xFU),
                .b = static_cast<std::uint8_t>((ab >> 4U) & 0xFU),
                .c = static_cast<std::uint8_t>(cd & 0xFU),
                .d = static_cast<std::uint8_t>((cd >> 4U) & 0xFU),
                .op = static_cast<std::uint8_t>(op & 0xFU),
                .bias = static_cast<std::uint8_t>((op >> 4U) & 0x3U),
                .scale = static_cast<std::uint8_t>((op >> 6U) & 0x3U),
                .out_reg = static_cast<std::uint8_t>((cl >> 1U) & 0x3U),
                .k_sel = static_cast<std::uint8_t>((cl >> 3U) & 0x1FU),
                .clamp = (cl & 0x1U) != 0U,
            };
        }

        std::vector<BrlytMaterial> parse_material_list(std::span<const std::uint8_t> block, const std::vector<std::string> &texture_names) {
            const auto material_count = read_be16(block, 8U);
            const auto offset_table_offset = 12U;
            if (offset_table_offset + static_cast<std::size_t>(material_count) * 4U > block.size()) {
                throw std::runtime_error("BRLYT material offset table is truncated");
            }

            auto materials = std::vector<BrlytMaterial>{};
            materials.reserve(material_count);
            for (auto i = 0U; i < material_count; ++i) {
                const auto material_offset = read_be32(block, offset_table_offset + static_cast<std::size_t>(i) * 4U);
                if (material_offset + 64U > block.size()) {
                    throw std::runtime_error("BRLYT material is truncated");
                }

                const auto res_num_bits = read_be32(block, material_offset + 60U);
                const auto texmap_count = material_texmap_count(res_num_bits);
                const auto texsrt_count = material_texsrt_count(res_num_bits);
                const auto texcoordgen_count = material_texcoordgen_count(res_num_bits);
                const auto tev_stage_count = material_tev_stage_count(res_num_bits);
                auto material = BrlytMaterial{};
                material.name = read_fixed_string(block, material_offset, 20U);
                for (auto color_index = 0U; color_index < material.tev_colors.size(); ++color_index) {
                    const auto color_offset = material_offset + 20U + static_cast<std::size_t>(color_index) * 8U;
                    material.tev_colors[color_index] = GXTevRegisterColor{
                        read_be_s16(block, color_offset),
                        read_be_s16(block, color_offset + 2U),
                        read_be_s16(block, color_offset + 4U),
                        read_be_s16(block, color_offset + 6U),
                    };
                }
                for (auto color_index = 0U; color_index < material.tev_k_colors.size(); ++color_index) {
                    const auto color_offset = material_offset + 44U + static_cast<std::size_t>(color_index) * 4U;
                    material.tev_k_colors[color_index] = std::array<std::uint8_t, 4U>{
                        block[color_offset],
                        block[color_offset + 1U],
                        block[color_offset + 2U],
                        block[color_offset + 3U],
                    };
                }

                auto cursor = material_offset + 64U;
                if (texmap_count > 0U) {
                    if (cursor + static_cast<std::size_t>(texmap_count) * 4U > block.size()) {
                        throw std::runtime_error("BRLYT material texture map is truncated");
                    }
                    material.textures.reserve(texmap_count);
                    for (auto texture_map = 0U; texture_map < texmap_count; ++texture_map) {
                        const auto wrap_s_filter = block[cursor + 2U];
                        const auto wrap_t_filter = block[cursor + 3U];
                        const auto texture_index = read_be16(block, cursor);
                        material.textures.push_back(BrlytMaterialTexture{
                            .texture_index = texture_index,
                            .texture_name = texture_index < texture_names.size() ? texture_names[texture_index] : std::string{},
                            .wrap_s = static_cast<std::uint8_t>(wrap_s_filter & 0x3U),
                            .wrap_t = static_cast<std::uint8_t>(wrap_t_filter & 0x3U),
                            .min_filter = static_cast<std::uint8_t>((wrap_s_filter >> 2U) & 0x7U),
                            .mag_filter = static_cast<std::uint8_t>((wrap_t_filter >> 2U) & 0x1U),
                        });
                        cursor += 4U;
                    }
                }
                if (cursor + static_cast<std::size_t>(texsrt_count) * 20U > block.size()) {
                    throw std::runtime_error("BRLYT material texture SRT is truncated");
                }
                material.tex_srts.reserve(texsrt_count);
                for (auto tex_srt = 0U; tex_srt < texsrt_count; ++tex_srt) {
                    material.tex_srts.push_back(BrlytTexSrt{
                        .translate_s = read_be_float(block, cursor),
                        .translate_t = read_be_float(block, cursor + 4U),
                        .rotate = read_be_float(block, cursor + 8U),
                        .scale_s = read_be_float(block, cursor + 12U),
                        .scale_t = read_be_float(block, cursor + 16U),
                    });
                    cursor += 20U;
                }

                if (cursor + static_cast<std::size_t>(texcoordgen_count) * 4U > block.size()) {
                    throw std::runtime_error("BRLYT material texture coord gen is truncated");
                }
                material.tex_coord_gens.reserve(texcoordgen_count);
                for (auto tex_coord_gen = 0U; tex_coord_gen < texcoordgen_count; ++tex_coord_gen) {
                    material.tex_coord_gens.push_back(BrlytTexCoordGen{
                        .tex_gen_type = block[cursor],
                        .tex_gen_src = block[cursor + 1U],
                        .tex_mtx = block[cursor + 2U],
                    });
                    cursor += 4U;
                }

                if (material_has_chan_ctrl(res_num_bits)) {
                    if (cursor + 4U > block.size()) {
                        throw std::runtime_error("BRLYT material channel control is truncated");
                    }
                    material.chan_color_src = block[cursor];
                    material.chan_alpha_src = block[cursor + 1U];
                    material.has_chan_ctrl = true;
                    cursor += 4U;
                }

                if (material_has_mat_color(res_num_bits)) {
                    if (cursor + 4U > block.size()) {
                        throw std::runtime_error("BRLYT material color is truncated");
                    }
                    material.mat_color = std::array<std::uint8_t, 4U>{block[cursor], block[cursor + 1U], block[cursor + 2U], block[cursor + 3U]};
                    material.has_mat_color = true;
                    cursor += 4U;
                }

                if (material_has_tev_swap_table(res_num_bits)) {
                    if (cursor + 4U > block.size()) {
                        throw std::runtime_error("BRLYT material TEV swap table is truncated");
                    }
                    cursor += 4U;
                }

                const auto ind_texsrt_count = material_indtexsrt_count(res_num_bits);
                if (cursor + static_cast<std::size_t>(ind_texsrt_count) * 20U > block.size()) {
                    throw std::runtime_error("BRLYT material indirect texture SRT is truncated");
                }
                cursor += static_cast<std::size_t>(ind_texsrt_count) * 20U;

                const auto ind_texstage_count = material_indtexstage_count(res_num_bits);
                if (cursor + static_cast<std::size_t>(ind_texstage_count) * 4U > block.size()) {
                    throw std::runtime_error("BRLYT material indirect texture stage is truncated");
                }
                cursor += static_cast<std::size_t>(ind_texstage_count) * 4U;

                if (cursor + static_cast<std::size_t>(tev_stage_count) * 16U > block.size()) {
                    throw std::runtime_error("BRLYT material TEV stage is truncated");
                }
                material.tev_stages.reserve(tev_stage_count);
                for (auto tev_stage = 0U; tev_stage < tev_stage_count; ++tev_stage) {
                    const auto tex_map_low = block[cursor + 2U];
                    const auto swap = block[cursor + 3U];
                    material.tev_stages.push_back(BrlytTevStage{
                        .tex_coord_gen = block[cursor],
                        .color_chan = block[cursor + 1U],
                        .tex_map = static_cast<std::uint16_t>((static_cast<std::uint16_t>(swap & 0x1U) << 8U) | tex_map_low),
                        .ras_swap = static_cast<std::uint8_t>((swap >> 1U) & 0x3U),
                        .tex_swap = static_cast<std::uint8_t>((swap >> 3U) & 0x3U),
                        .color = parse_tev_stage_in_op(block, cursor + 4U),
                        .alpha = parse_tev_stage_in_op(block, cursor + 8U),
                    });
                    cursor += 16U;
                }

                if (material_has_alpha_compare(res_num_bits)) {
                    if (cursor + 4U > block.size()) {
                        throw std::runtime_error("BRLYT material alpha compare is truncated");
                    }
                    const auto comp = block[cursor];
                    material.alpha_compare = BrlytAlphaCompare{
                        .comp0 = static_cast<std::uint8_t>(comp & 0xFU),
                        .ref0 = block[cursor + 2U],
                        .op = block[cursor + 1U],
                        .comp1 = static_cast<std::uint8_t>((comp >> 4U) & 0xFU),
                        .ref1 = block[cursor + 3U],
                        .enabled = true,
                    };
                    cursor += 4U;
                }

                if (material_has_blend_mode(res_num_bits)) {
                    if (cursor + 4U > block.size()) {
                        throw std::runtime_error("BRLYT material blend mode is truncated");
                    }
                    material.blend_mode = BrlytBlendMode{
                        .type = block[cursor],
                        .src_factor = block[cursor + 1U],
                        .dst_factor = block[cursor + 2U],
                        .op = block[cursor + 3U],
                        .enabled = true,
                    };
                    cursor += 4U;
                }
                material.gx_state = gx_state_from_brlyt_material(material);
                materials.push_back(material);
            }

            return materials;
        }

        [[nodiscard]] PaneState parse_pane_state(std::span<const std::uint8_t> block) {
            return PaneState{
                .translate_x = read_be_float(block, 36U),
                .translate_y = read_be_float(block, 40U),
                .rotate_z = read_be_float(block, 56U),
                .scale_x = read_be_float(block, 60U),
                .scale_y = read_be_float(block, 64U),
            };
        }

        [[nodiscard]] PaneState combine_state(const PaneState &parent, const PaneState &local) {
            const auto parent_scale_x = parent.scale_x == 0.0F ? 1.0F : parent.scale_x;
            const auto parent_scale_y = parent.scale_y == 0.0F ? 1.0F : parent.scale_y;
            return PaneState{
                .translate_x = parent.translate_x + local.translate_x * parent_scale_x,
                .translate_y = parent.translate_y + local.translate_y * parent_scale_y,
                .rotate_z = parent.rotate_z + local.rotate_z,
                .scale_x = parent_scale_x * local.scale_x,
                .scale_y = parent_scale_y * local.scale_y,
            };
        }

        [[nodiscard]] BrlytPane parse_pane(std::span<const std::uint8_t> block, std::int32_t parent_index) {
            if (block.size() < 76U) {
                throw std::runtime_error("BRLYT pane is truncated");
            }

            return BrlytPane{
                .name = read_fixed_string(block, 12U, 16U),
                .parent_index = parent_index,
                .translate_x = read_be_float(block, 36U),
                .translate_y = read_be_float(block, 40U),
                .rotate_z = read_be_float(block, 56U),
                .scale_x = read_be_float(block, 60U),
                .scale_y = read_be_float(block, 64U),
                .width = read_be_float(block, 68U),
                .height = read_be_float(block, 72U),
                .base_position = block[9U],
                .alpha = block[10U],
                .visible = (block[8U] & 0x1U) != 0U,
            };
        }

        [[nodiscard]] float base_position_x(std::uint8_t base_position, float width) {
            switch (base_position % 3U) {
            case 1U:
                return -width * 0.5F;
            case 2U:
                return -width;
            default:
                return 0.0F;
            }
        }

        [[nodiscard]] float base_position_y(std::uint8_t base_position, float height) {
            switch (base_position / 3U) {
            case 1U:
                return -height * 0.5F;
            case 2U:
                return -height;
            default:
                return 0.0F;
            }
        }

        [[nodiscard]] BrlytPicturePane parse_picture(std::span<const std::uint8_t> block, const PaneState &global_state, const std::vector<BrlytMaterial> &materials) {
            if (block.size() < 96U) {
                throw std::runtime_error("BRLYT picture pane is truncated");
            }

            const auto base_position = block[9U];
            const auto alpha = block[10U];
            const auto name = read_fixed_string(block, 12U, 16U);
            const auto width = read_be_float(block, 68U) * global_state.scale_x;
            const auto height = read_be_float(block, 72U) * global_state.scale_y;
            const auto material_index = read_be16(block, 92U);
            auto texture_name = std::string{};
            auto wrap_s = std::uint8_t{};
            auto wrap_t = std::uint8_t{};
            auto min_filter = std::uint8_t{};
            auto mag_filter = std::uint8_t{};
            if (material_index < materials.size() && !materials[material_index].textures.empty()) {
                const auto &texture_info = materials[material_index].textures.front();
                if (!texture_info.texture_name.empty()) {
                    texture_name = texture_info.texture_name;
                    wrap_s = texture_info.wrap_s;
                    wrap_t = texture_info.wrap_t;
                    min_filter = texture_info.min_filter;
                    mag_filter = texture_info.mag_filter;
                }
            }

            auto vertex_colors = std::array<std::array<std::uint8_t, 4U>, 4U>{};
            for (auto vertex = 0U; vertex < vertex_colors.size(); ++vertex) {
                const auto color_offset = 76U + static_cast<std::size_t>(vertex) * 4U;
                vertex_colors[vertex] = std::array<std::uint8_t, 4U>{
                    block[color_offset],
                    block[color_offset + 1U],
                    block[color_offset + 2U],
                    block[color_offset + 3U],
                };
            }

            auto tex_coords = std::array<BrlytTexCoord, 4U>{
                BrlytTexCoord{0.0F, 0.0F},
                BrlytTexCoord{1.0F, 0.0F},
                BrlytTexCoord{1.0F, 1.0F},
                BrlytTexCoord{0.0F, 1.0F},
            };
            const auto tex_coord_count = block[94U];
            if (tex_coord_count > 0U && block.size() >= 128U) {
                const auto raw_tex_coords = std::array<BrlytTexCoord, 4U>{
                    BrlytTexCoord{read_be_float(block, 96U), read_be_float(block, 100U)},
                    BrlytTexCoord{read_be_float(block, 104U), read_be_float(block, 108U)},
                    BrlytTexCoord{read_be_float(block, 112U), read_be_float(block, 116U)},
                    BrlytTexCoord{read_be_float(block, 120U), read_be_float(block, 124U)},
                };
                tex_coords = {
                    raw_tex_coords[0U],
                    raw_tex_coords[1U],
                    raw_tex_coords[3U],
                    raw_tex_coords[2U],
                };
            }

            return BrlytPicturePane{
                .name = name,
                .texture_name = texture_name,
                .pane_index = 0U,
                .material_index = material_index,
                .wrap_s = wrap_s,
                .wrap_t = wrap_t,
                .min_filter = min_filter,
                .mag_filter = mag_filter,
                .x = global_state.translate_x + base_position_x(base_position, width),
                .y = global_state.translate_y + base_position_y(base_position, height),
                .width = width,
                .height = height,
                .color = {255U, 255U, 255U, alpha},
                .vertex_colors = vertex_colors,
                .tex_coords = tex_coords,
                .visible = (block[8U] & 0x1U) != 0U,
            };
        }

        [[nodiscard]] std::vector<std::uint16_t> parse_utf16be_string(std::span<const std::uint8_t> block, std::uint32_t offset, std::uint16_t byte_count) {
            if (offset + byte_count > block.size()) {
                throw std::runtime_error("BRLYT text string is truncated");
            }

            auto text = std::vector<std::uint16_t>{};
            text.reserve(byte_count / 2U);
            for (auto cursor = static_cast<std::size_t>(offset); cursor + 1U < static_cast<std::size_t>(offset) + byte_count; cursor += 2U) {
                const auto code = read_be16(block, cursor);
                if (code == 0U) {
                    break;
                }
                text.push_back(code);
            }

            return text;
        }

        [[nodiscard]] BrlytTextBox parse_text_box(std::span<const std::uint8_t> block, const PaneState &global_state, const std::vector<std::string> &font_names, const std::vector<BrlytMaterial> &materials) {
            if (block.size() < 116U) {
                throw std::runtime_error("BRLYT text box pane is truncated");
            }

            const auto base_position = block[9U];
            const auto alpha = block[10U];
            const auto name = read_fixed_string(block, 12U, 16U);
            const auto width = read_be_float(block, 68U) * global_state.scale_x;
            const auto height = read_be_float(block, 72U) * global_state.scale_y;
            const auto text_byte_count = read_be16(block, 78U);
            const auto material_index = read_be16(block, 80U);
            const auto font_index = read_be16(block, 82U);
            const auto text_position = block[84U];
            const auto text_alignment = block[85U];
            const auto text_offset = read_be32(block, 88U);
            auto color = std::array<std::uint8_t, 4U>{block[92U], block[93U], block[94U], block[95U]};
            color[3U] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(color[3U]) * alpha) / 255U);
            const auto color_mapping_min = material_index < materials.size() ? material_color_to_rgba8(materials[material_index].tev_colors[0U]) : std::array<std::uint8_t, 4U>{0U, 0U, 0U, 0U};
            const auto color_mapping_max = material_index < materials.size() ? material_color_to_rgba8(materials[material_index].tev_colors[1U]) : std::array<std::uint8_t, 4U>{255U, 255U, 255U, 255U};

            return BrlytTextBox{
                .name = name,
                .font_name = font_index < font_names.size() ? font_names[font_index] : std::string{},
                .text = parse_utf16be_string(block, text_offset, text_byte_count),
                .pane_index = 0U,
                .material_index = material_index,
                .x = global_state.translate_x + base_position_x(base_position, width),
                .y = global_state.translate_y + base_position_y(base_position, height),
                .width = width,
                .height = height,
                .font_width = read_be_float(block, 100U) * global_state.scale_x,
                .font_height = read_be_float(block, 104U) * global_state.scale_y,
                .char_space = read_be_float(block, 108U) * global_state.scale_x,
                .line_space = read_be_float(block, 112U) * global_state.scale_y,
                .text_position = text_position,
                .text_alignment = text_alignment,
                .color = color,
                .color_mapping_min = color_mapping_min,
                .color_mapping_max = color_mapping_max,
                .visible = (block[8U] & 0x1U) != 0U,
            };
        }

    }  // namespace

    BrlytLayout parse_brlyt_layout(std::span<const std::uint8_t> data) {
        if (!has_magic(data, 0U, "RLYT")) {
            throw std::runtime_error("BRLYT file is missing RLYT magic");
        }

        const auto header_size = read_be16(data, 12U);
        const auto block_count = read_be16(data, 14U);
        auto cursor = static_cast<std::size_t>(header_size);
        auto layout = BrlytLayout{};
        auto parent_stack = std::vector<PaneState>{};
        auto parent_index_stack = std::vector<std::int32_t>{};
        auto last_state = PaneState{};
        auto last_pane_index = std::int32_t{-1};

        for (auto i = 0U; i < block_count; ++i) {
            if (cursor + 8U > data.size()) {
                throw std::runtime_error("BRLYT data block header is truncated");
            }

            const auto block_size = read_be32(data, cursor + 4U);
            if (block_size < 8U || cursor + block_size > data.size()) {
                throw std::runtime_error("BRLYT data block size is invalid");
            }

            const auto block = data.subspan(cursor, block_size);
            if (has_magic(block, 0U, "lyt1")) {
                layout.width = read_be_float(block, 12U);
                layout.height = read_be_float(block, 16U);
            } else if (has_magic(block, 0U, "txl1")) {
                parse_texture_list(layout, block);
            } else if (has_magic(block, 0U, "fnl1")) {
                parse_font_list(layout, block);
            } else if (has_magic(block, 0U, "mat1")) {
                layout.materials = parse_material_list(block, layout.texture_names);
            } else if (has_magic(block, 0U, "pan1") || has_magic(block, 0U, "bnd1") || has_magic(block, 0U, "wnd1")) {
                const auto local = parse_pane_state(block);
                const auto parent = parent_stack.empty() ? PaneState{} : parent_stack.back();
                last_state = combine_state(parent, local);
                layout.panes.push_back(parse_pane(block, parent_index_stack.empty() ? -1 : parent_index_stack.back()));
                last_pane_index = static_cast<std::int32_t>(layout.panes.size() - 1U);
            } else if (has_magic(block, 0U, "txt1")) {
                const auto local = parse_pane_state(block);
                const auto parent = parent_stack.empty() ? PaneState{} : parent_stack.back();
                last_state = combine_state(parent, local);
                layout.panes.push_back(parse_pane(block, parent_index_stack.empty() ? -1 : parent_index_stack.back()));
                last_pane_index = static_cast<std::int32_t>(layout.panes.size() - 1U);
                auto text_box = parse_text_box(block, last_state, layout.font_names, layout.materials);
                text_box.pane_index = static_cast<std::size_t>(last_pane_index);
                if (!text_box.font_name.empty()) {
                    layout.text_boxes.push_back(std::move(text_box));
                }
            } else if (has_magic(block, 0U, "pic1")) {
                const auto local = parse_pane_state(block);
                const auto parent = parent_stack.empty() ? PaneState{} : parent_stack.back();
                last_state = combine_state(parent, local);
                layout.panes.push_back(parse_pane(block, parent_index_stack.empty() ? -1 : parent_index_stack.back()));
                last_pane_index = static_cast<std::int32_t>(layout.panes.size() - 1U);
                auto picture = parse_picture(block, last_state, layout.materials);
                picture.pane_index = static_cast<std::size_t>(last_pane_index);
                if (!picture.texture_name.empty()) {
                    layout.pictures.push_back(std::move(picture));
                }
            } else if (has_magic(block, 0U, "pas1")) {
                parent_stack.push_back(last_state);
                parent_index_stack.push_back(last_pane_index);
            } else if (has_magic(block, 0U, "pae1")) {
                if (!parent_stack.empty()) {
                    parent_stack.pop_back();
                }
                if (!parent_index_stack.empty()) {
                    parent_index_stack.pop_back();
                }
            }

            cursor += block_size;
        }

        return layout;
    }

}  // namespace smgpc::game
