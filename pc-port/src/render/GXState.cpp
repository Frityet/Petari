#include "GXState.hpp"

#include "J3dModel.hpp"
#include "layout/BrlytLayout.hpp"

#include <algorithm>
#include <bit>
#include <cmath>

namespace smgpc::render {
    namespace {

        [[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) {
            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8U) | static_cast<std::uint16_t>(data[offset + 1U]));
        }

        [[nodiscard]] std::uint32_t read_be24(std::span<const std::uint8_t> data, std::size_t offset) {
            return (static_cast<std::uint32_t>(data[offset]) << 16U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 8U) |
                   static_cast<std::uint32_t>(data[offset + 2U]);
        }

        [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
            return (static_cast<std::uint32_t>(data[offset]) << 24U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
                   (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) | static_cast<std::uint32_t>(data[offset + 3U]);
        }

        [[nodiscard]] std::uint8_t bits(std::uint32_t value, std::uint8_t shift, std::uint8_t count) {
            return static_cast<std::uint8_t>((value >> shift) & ((1U << count) - 1U));
        }

        [[nodiscard]] std::uint32_t encode_gen_mode_register(const GXMaterialState &state) {
            auto value = std::uint32_t{};
            value |= static_cast<std::uint32_t>(state.texgen_count & 0x0fU);
            value |= static_cast<std::uint32_t>(state.color_channel_count & 0x07U) << 4U;
            if (state.tev_stage_count != 0U) {
                value |= static_cast<std::uint32_t>((state.tev_stage_count - 1U) & 0x0fU) << 10U;
            }
            if (state.cull_mode != 0xffU) {
                value |= static_cast<std::uint32_t>(state.cull_mode & 0x03U) << 14U;
            }
            value |= static_cast<std::uint32_t>(state.indirect.stage_count & 0x07U) << 16U;
            return value;
        }

        [[nodiscard]] std::uint32_t encode_z_mode_register(const GXZModeState &state) {
            return static_cast<std::uint32_t>(state.compare_enable & 0x01U) |
                   (static_cast<std::uint32_t>(state.function & 0x07U) << 1U) |
                   (static_cast<std::uint32_t>(state.update_enable & 0x01U) << 4U);
        }

        [[nodiscard]] std::uint32_t encode_blend_register(const GXBlendState &state) {
            const auto blend_enabled = state.type == 1U || state.type == 3U;
            const auto logic_enabled = state.type == 2U;
            const auto subtract_enabled = state.type == 3U;
            return (blend_enabled ? 1U : 0U) | (logic_enabled ? 2U : 0U) |
                   (state.color_update ? 0x08U : 0U) | (state.alpha_update ? 0x10U : 0U) |
                   (static_cast<std::uint32_t>(state.dst_factor & 0x07U) << 5U) |
                   (static_cast<std::uint32_t>(state.src_factor & 0x07U) << 8U) |
                   (subtract_enabled ? 0x800U : 0U) | (static_cast<std::uint32_t>(state.op & 0x0fU) << 12U);
        }

        [[nodiscard]] std::uint32_t encode_alpha_compare_register(const GXAlphaCompareState &state) {
            return static_cast<std::uint32_t>(state.ref0) | (static_cast<std::uint32_t>(state.ref1) << 8U) |
                   (static_cast<std::uint32_t>(state.comp0 & 0x07U) << 16U) |
                   (static_cast<std::uint32_t>(state.comp1 & 0x07U) << 19U) |
                   (static_cast<std::uint32_t>(state.op & 0x03U) << 22U);
        }

        [[nodiscard]] GXBPRegisterState initial_bp_registers_from_state(const GXMaterialState &state) {
            auto registers = GXBPRegisterState{};
            registers[0x00U] = encode_gen_mode_register(state);
            registers[0x40U] = encode_z_mode_register(state.z_mode);
            registers[0x41U] = encode_blend_register(state.blend);
            registers[0xf3U] = encode_alpha_compare_register(state.alpha_compare);
            return registers;
        }

        [[nodiscard]] std::uint8_t gx_attenuation_function_from_xf(std::uint32_t value) {
            constexpr auto lookup = std::array<std::uint8_t, 4U>{2U, 0U, 2U, 1U};
            return lookup[value & 0x3U];
        }

        [[nodiscard]] std::uint32_t gx_xf_attenuation_bits_from_function(std::uint8_t attenuation_function) {
            constexpr auto gx_af_spec = std::uint8_t{0U};
            constexpr auto gx_af_none = std::uint8_t{2U};
            return (attenuation_function != gx_af_none ? 1U : 0U) | ((attenuation_function != gx_af_spec ? 1U : 0U) << 1U);
        }

        [[nodiscard]] std::int16_t signed_bits_11(std::uint32_t value, std::uint8_t shift) {
            auto raw = static_cast<std::int16_t>((value >> shift) & 0x7ffU);
            if ((raw & 0x400) != 0) {
                raw = static_cast<std::int16_t>(raw | static_cast<std::int16_t>(~0x7ff));
            }
            return raw;
        }

        [[nodiscard]] float gx_fog_float_value(std::uint32_t mantissa, std::uint32_t exponent, std::uint32_t sign) {
            return std::bit_cast<float>((sign << 31U) | (exponent << 23U) | (mantissa << 12U));
        }

        [[nodiscard]] std::array<float, 3U> subtract3(std::array<float, 3U> left, std::array<float, 3U> right) {
            return {left[0U] - right[0U], left[1U] - right[1U], left[2U] - right[2U]};
        }

        [[nodiscard]] float dot3(std::array<float, 3U> left, std::array<float, 3U> right) {
            return left[0U] * right[0U] + left[1U] * right[1U] + left[2U] * right[2U];
        }

        [[nodiscard]] std::array<float, 3U> normalized_or(std::array<float, 3U> value, std::array<float, 3U> fallback) {
            const auto length = std::sqrt(dot3(value, value));
            if (length <= 0.000001F) {
                return fallback;
            }

            return {value[0U] / length, value[1U] / length, value[2U] / length};
        }

        [[nodiscard]] float safe_ratio(float numerator, float denominator) {
            if (std::abs(denominator) <= 0.000001F) {
                return 0.0F;
            }

            return numerator / denominator;
        }

        [[nodiscard]] std::uint8_t gx_color_channel_index(std::uint8_t color_channel) {
            if (color_channel == 1U || color_channel == 3U || color_channel == 5U) {
                return 1U;
            }

            return 0U;
        }

        [[nodiscard]] bool gx_color_channel_is_null(std::uint8_t color_channel) {
            return color_channel == 0xffU || color_channel == 6U || color_channel == 7U;
        }

        [[nodiscard]] float light_attenuation(const GXLightState &light, const GXColorChannelControlState &control,
                                              std::array<float, 3U> position, std::array<float, 3U> normal,
                                              std::array<float, 3U> &light_direction) {
            auto delta = subtract3(light.position, position);
            auto distance_squared = dot3(delta, delta);
            auto distance = std::sqrt(distance_squared);
            light_direction = normalized_or(delta, normal);

            constexpr auto attenuation_none = std::uint8_t{0U};
            constexpr auto attenuation_spec = std::uint8_t{1U};
            constexpr auto attenuation_dir = std::uint8_t{2U};
            constexpr auto attenuation_spot = std::uint8_t{3U};
            switch (control.attenuation_mode) {
            case attenuation_spec: {
                const auto light_normal = normalized_or(light.direction, {0.0F, 0.0F, -1.0F});
                const auto specular = dot3(normal, light_direction) >= 0.0F ? std::max(0.0F, dot3(normal, light_normal)) : 0.0F;
                const auto cos_terms = std::array<float, 3U>{1.0F, specular, specular * specular};
                auto dist_terms = light.distance_attenuation;
                if (control.diffuse_function != 0U) {
                    dist_terms = normalized_or(dist_terms, {1.0F, 0.0F, 0.0F});
                }
                const auto numerator = std::max(0.0F, dot3(light.cosine_attenuation, cos_terms));
                return safe_ratio(numerator, dot3(dist_terms, cos_terms));
            }
            case attenuation_spot: {
                if (distance <= 0.000001F) {
                    distance = 0.0F;
                    distance_squared = 0.0F;
                    light_direction = normal;
                }
                const auto light_normal = normalized_or(light.direction, {0.0F, 0.0F, -1.0F});
                const auto spot = std::max(0.0F, dot3(light_direction, light_normal));
                const auto cos_terms = std::array<float, 3U>{1.0F, spot, spot * spot};
                const auto dist_terms = std::array<float, 3U>{1.0F, distance, distance_squared};
                const auto numerator = std::max(0.0F, dot3(light.cosine_attenuation, cos_terms));
                return safe_ratio(numerator, dot3(light.distance_attenuation, dist_terms));
            }
            case attenuation_none:
            case attenuation_dir:
            default:
                return 1.0F;
            }
        }

        [[nodiscard]] std::array<int, 4U> lighting_accumulator(const GXMaterialState &state, const GXColorChannelState &channel,
                                                               const GXColorChannelControlState &control, GXColorValue vertex_color,
                                                               std::array<float, 3U> position, std::array<float, 3U> normal) {
            if (!control.lighting_enabled) {
                return {255, 255, 255, 255};
            }

            const auto ambient = control.ambient_source != 0U ? vertex_color : channel.ambient_color;
            auto accumulator = std::array<int, 4U>{
                static_cast<int>(ambient[0U]),
                static_cast<int>(ambient[1U]),
                static_cast<int>(ambient[2U]),
                static_cast<int>(ambient[3U]),
            };
            normal = normalized_or(normal, {0.0F, 0.0F, 1.0F});

            for (auto light_index = 0U; light_index < state.lights.size(); ++light_index) {
                if ((control.light_mask & (1U << light_index)) == 0U) {
                    continue;
                }

                const auto &light = state.lights[light_index];
                if (!light.loaded) {
                    continue;
                }

                auto light_direction = std::array<float, 3U>{};
                const auto attenuation = light_attenuation(light, control, position, normal, light_direction);
                auto contribution = attenuation;
                switch (control.diffuse_function) {
                case 1U:
                    contribution *= dot3(light_direction, normal);
                    break;
                case 2U:
                    contribution *= std::max(0.0F, dot3(light_direction, normal));
                    break;
                default:
                    break;
                }

                for (auto component = 0U; component < accumulator.size(); ++component) {
                    accumulator[component] += static_cast<int>(std::lround(contribution * static_cast<float>(light.color[component])));
                }
            }

            for (auto &component : accumulator) {
                component = std::clamp(component, 0, 255);
            }
            return accumulator;
        }

        [[nodiscard]] std::uint8_t apply_lighting_accumulator(std::uint8_t material, int accumulator) {
            return static_cast<std::uint8_t>((static_cast<int>(material) * (accumulator + (accumulator >> 7))) >> 8);
        }

        GXTevOrderState &tev_order_for_stage(GXMaterialState &state, std::uint8_t stage) {
            if (auto it = std::ranges::find_if(state.tev_orders, [stage](const auto &order) { return order.stage == stage; });
                it != state.tev_orders.end()) {
                return *it;
            }

            state.tev_orders.push_back(GXTevOrderState{.stage = stage});
            return state.tev_orders.back();
        }

        GXTevStageState &tev_stage_for_stage(GXMaterialState &state, std::uint8_t stage) {
            if (auto it = std::ranges::find_if(state.tev_stages, [stage](const auto &tev_stage) { return tev_stage.stage == stage; });
                it != state.tev_stages.end()) {
                return *it;
            }

            state.tev_stages.push_back(GXTevStageState{.stage = stage});
            return state.tev_stages.back();
        }

        void decode_two_tev_orders(GXMaterialState &state, std::uint8_t address, std::uint32_t value) {
            const auto stage_even = static_cast<std::uint8_t>((address - 0x28U) * 2U);
            auto &even = tev_order_for_stage(state, stage_even);
            even.tex_map = bits(value, 6U, 1U) != 0U ? bits(value, 0U, 3U) : 0xffU;
            even.tex_coord = bits(value, 6U, 1U) != 0U ? bits(value, 3U, 3U) : 0xffU;
            even.color_channel = bits(value, 7U, 3U);

            auto &odd = tev_order_for_stage(state, static_cast<std::uint8_t>(stage_even + 1U));
            odd.tex_map = bits(value, 18U, 1U) != 0U ? bits(value, 12U, 3U) : 0xffU;
            odd.tex_coord = bits(value, 18U, 1U) != 0U ? bits(value, 15U, 3U) : 0xffU;
            odd.color_channel = bits(value, 19U, 3U);
        }

        void decode_color_tev_stage(GXMaterialState &state, std::uint8_t stage_index, std::uint32_t value) {
            auto &stage = tev_stage_for_stage(state, stage_index);
            stage.color_in = {bits(value, 12U, 4U), bits(value, 8U, 4U), bits(value, 4U, 4U), bits(value, 0U, 4U)};
            stage.color_bias = bits(value, 16U, 2U);
            stage.color_op = bits(value, 18U, 1U);
            stage.color_clamp = bits(value, 19U, 1U);
            stage.color_scale = bits(value, 20U, 2U);
            stage.color_out = bits(value, 22U, 2U);
        }

        void decode_alpha_tev_stage(GXMaterialState &state, std::uint8_t stage_index, std::uint32_t value) {
            auto &stage = tev_stage_for_stage(state, stage_index);
            stage.alpha_in = {bits(value, 13U, 3U), bits(value, 10U, 3U), bits(value, 7U, 3U), bits(value, 4U, 3U)};
            stage.alpha_bias = bits(value, 16U, 2U);
            stage.alpha_op = bits(value, 18U, 1U);
            stage.alpha_clamp = bits(value, 19U, 1U);
            stage.alpha_scale = bits(value, 20U, 2U);
            stage.alpha_out = bits(value, 22U, 2U);
        }

        void decode_tev_ksel(GXMaterialState &state, std::uint8_t address, std::uint32_t value) {
            const auto even_stage = static_cast<std::uint8_t>((address - 0xf6U) * 2U);
            auto &even = tev_stage_for_stage(state, even_stage);
            even.k_color_sel = bits(value, 4U, 5U);
            even.k_alpha_sel = bits(value, 9U, 5U);

            auto &odd = tev_stage_for_stage(state, static_cast<std::uint8_t>(even_stage + 1U));
            odd.k_color_sel = bits(value, 14U, 5U);
            odd.k_alpha_sel = bits(value, 19U, 5U);
        }

        void decode_tev_register(GXMaterialState &state, std::uint8_t address, std::uint32_t value) {
            const auto register_index = static_cast<std::size_t>((address - 0xe0U) / 2U);
            if (register_index >= state.tev_registers.size()) {
                return;
            }

            const auto is_konst = bits(value, 23U, 1U) != 0U;
            if ((address & 1U) == 0U) {
                const auto red = signed_bits_11(value, 0U);
                const auto alpha = signed_bits_11(value, 12U);
                if (is_konst && register_index < state.tev_k_colors.size()) {
                    state.tev_k_colors[register_index][0U] = static_cast<std::uint8_t>(std::clamp<std::int16_t>(red, 0, 255));
                    state.tev_k_colors[register_index][3U] = static_cast<std::uint8_t>(std::clamp<std::int16_t>(alpha, 0, 255));
                } else {
                    state.tev_registers[register_index][0U] = red;
                    state.tev_registers[register_index][3U] = alpha;
                }
                return;
            }

            const auto blue = signed_bits_11(value, 0U);
            const auto green = signed_bits_11(value, 12U);
            if (is_konst && register_index < state.tev_k_colors.size()) {
                state.tev_k_colors[register_index][2U] = static_cast<std::uint8_t>(std::clamp<std::int16_t>(blue, 0, 255));
                state.tev_k_colors[register_index][1U] = static_cast<std::uint8_t>(std::clamp<std::int16_t>(green, 0, 255));
            } else {
                state.tev_registers[register_index][2U] = blue;
                state.tev_registers[register_index][1U] = green;
            }
        }

        GXIndirectTextureMatrixState &indirect_matrix_for_index(GXMaterialState &state, std::uint8_t matrix_index) {
            if (auto it = std::ranges::find_if(state.indirect.texture_matrices,
                                               [matrix_index](const auto &matrix) { return matrix.matrix == matrix_index; });
                it != state.indirect.texture_matrices.end()) {
                return *it;
            }

            state.indirect.texture_matrices.push_back(GXIndirectTextureMatrixState{.matrix = matrix_index});
            return state.indirect.texture_matrices.back();
        }

        GXIndirectTextureCoordScaleState &indirect_scale_for_stage(GXMaterialState &state, std::uint8_t stage) {
            if (auto it = std::ranges::find_if(state.indirect.texture_coord_scales,
                                               [stage](const auto &scale) { return scale.stage == stage; });
                it != state.indirect.texture_coord_scales.end()) {
                return *it;
            }

            state.indirect.texture_coord_scales.push_back(GXIndirectTextureCoordScaleState{.stage = stage});
            return state.indirect.texture_coord_scales.back();
        }

        GXIndirectTevStageState &indirect_tev_stage_for_stage(GXMaterialState &state, std::uint8_t tev_stage) {
            if (auto it = std::ranges::find_if(state.indirect.tev_stages,
                                               [tev_stage](const auto &stage) { return stage.tev_stage == tev_stage; });
                it != state.indirect.tev_stages.end()) {
                return *it;
            }

            state.indirect.tev_stages.push_back(GXIndirectTevStageState{.tev_stage = tev_stage});
            return state.indirect.tev_stages.back();
        }

        void update_indirect_matrix_scale(GXIndirectTextureMatrixState &matrix) {
            matrix.scale = static_cast<std::uint8_t>(bits(matrix.raw[0U], 22U, 2U) | (bits(matrix.raw[1U], 22U, 2U) << 2U) |
                                                     (bits(matrix.raw[2U], 22U, 1U) << 4U));
        }

        void decode_indirect_matrix(GXMaterialState &state, std::uint8_t address, std::uint32_t value) {
            const auto relative = static_cast<std::uint8_t>(address - 0x06U);
            auto &matrix = indirect_matrix_for_index(state, static_cast<std::uint8_t>(relative / 3U));
            const auto column = static_cast<std::uint8_t>(relative % 3U);
            matrix.raw[column] = value;
            switch (column) {
            case 0U:
                matrix.ma = signed_bits_11(value, 0U);
                matrix.mb = signed_bits_11(value, 11U);
                break;
            case 1U:
                matrix.mc = signed_bits_11(value, 0U);
                matrix.md = signed_bits_11(value, 11U);
                break;
            case 2U:
                matrix.me = signed_bits_11(value, 0U);
                matrix.mf = signed_bits_11(value, 11U);
                break;
            default:
                break;
            }
            update_indirect_matrix_scale(matrix);
        }

        void decode_indirect_tev_stage(GXMaterialState &state, std::uint8_t address, std::uint32_t value) {
            auto &stage = indirect_tev_stage_for_stage(state, static_cast<std::uint8_t>(address - 0x10U));
            stage.raw = value & 0x1fffffU;
            stage.ind_stage = bits(value, 0U, 2U);
            stage.format = bits(value, 2U, 2U);
            stage.bias = bits(value, 4U, 3U);
            stage.bump_alpha = bits(value, 7U, 2U);
            stage.matrix_index = bits(value, 9U, 2U);
            stage.matrix_id = bits(value, 11U, 2U);
            stage.wrap_s = bits(value, 13U, 3U);
            stage.wrap_t = bits(value, 16U, 3U);
            stage.use_original_lod = bits(value, 19U, 1U) != 0U;
            stage.add_previous = bits(value, 20U, 1U) != 0U;
            stage.active = stage.bump_alpha != 0U || stage.matrix_index != 0U;
        }

        void decode_indirect_tex_coord_scale(GXMaterialState &state, std::uint8_t address, std::uint32_t value) {
            const auto even_stage = static_cast<std::uint8_t>((address - 0x25U) * 2U);
            auto &even = indirect_scale_for_stage(state, even_stage);
            even.scale_s = bits(value, 0U, 4U);
            even.scale_t = bits(value, 4U, 4U);

            auto &odd = indirect_scale_for_stage(state, static_cast<std::uint8_t>(even_stage + 1U));
            odd.scale_s = bits(value, 8U, 4U);
            odd.scale_t = bits(value, 12U, 4U);
        }

        void decode_su_line_point_state(GXMaterialState &state, std::uint32_t value) {
            state.su_line_point = GXSULinePointState{
                .line_size = bits(value, 0U, 8U),
                .point_size = bits(value, 8U, 8U),
                .line_tex_offset = bits(value, 16U, 3U),
                .point_tex_offset = bits(value, 19U, 3U),
                .field_mode = bits(value, 22U, 1U) != 0U,
                .loaded = true,
                .raw = value,
            };
        }

        void decode_tex_coord_scale(GXMaterialState &state, std::uint8_t address, std::uint32_t value) {
            const auto slot = static_cast<std::uint8_t>((address - 0x30U) / 2U);
            if (slot >= state.tex_coord_scales.size()) {
                return;
            }

            auto &scale = state.tex_coord_scales[slot];
            if ((address & 1U) == 0U) {
                scale.s_scale_minus_1 = static_cast<std::uint16_t>(value & 0xffffU);
                scale.s_bias = bits(value, 16U, 1U) != 0U;
                scale.s_wrap = bits(value, 17U, 1U) != 0U;
                scale.line_offset = bits(value, 18U, 1U) != 0U;
                scale.point_offset = bits(value, 19U, 1U) != 0U;
                scale.s_loaded = true;
                scale.raw_s = value;
                return;
            }

            scale.t_scale_minus_1 = static_cast<std::uint16_t>(value & 0xffffU);
            scale.t_bias = bits(value, 16U, 1U) != 0U;
            scale.t_wrap = bits(value, 17U, 1U) != 0U;
            scale.t_loaded = true;
            scale.raw_t = value;
        }

        void decode_fog_register(GXMaterialState &state, std::uint8_t address, std::uint32_t value) {
            const auto raw_index = static_cast<std::size_t>(address - 0xe8U) * 4U;
            if (raw_index + 3U < state.fog.raw.size()) {
                state.fog.raw[raw_index] = static_cast<std::uint8_t>((value >> 24U) & 0xffU);
                state.fog.raw[raw_index + 1U] = static_cast<std::uint8_t>((value >> 16U) & 0xffU);
                state.fog.raw[raw_index + 2U] = static_cast<std::uint8_t>((value >> 8U) & 0xffU);
                state.fog.raw[raw_index + 3U] = static_cast<std::uint8_t>(value & 0xffU);
            }

            if (address == 0xe8U) {
                state.fog.range_center = static_cast<std::uint16_t>(value & 0x3ffU);
                state.fog.range_adjust_enabled = ((value >> 10U) & 1U) != 0U;
                return;
            }

            if (address >= 0xe9U && address <= 0xedU) {
                const auto index = static_cast<std::size_t>(address - 0xe9U) * 2U;
                if (index + 1U < state.fog.range_k.size()) {
                    state.fog.range_k[index] = static_cast<float>(value & 0xfffU) / 256.0F;
                    state.fog.range_k[index + 1U] = static_cast<float>((value >> 12U) & 0xfffU) / 256.0F;
                }
                return;
            }

            if (address == 0xeeU) {
                state.fog.a = gx_fog_float_value(value & 0x7ffU, (value >> 11U) & 0xffU, (value >> 19U) & 1U);
                return;
            }

            if (address == 0xefU) {
                state.fog.b_magnitude = value;
                return;
            }

            if (address == 0xf0U) {
                state.fog.b_shift = value;
                return;
            }

            if (address == 0xf1U) {
                state.fog.c = gx_fog_float_value(value & 0x7ffU, (value >> 11U) & 0xffU, (value >> 19U) & 1U);
                state.fog.projection = bits(value, 20U, 1U);
                state.fog.type = bits(value, 21U, 3U);
                state.fog.enabled = state.fog.type != 0U;
                return;
            }

            if (address == 0xf2U) {
                state.fog.color = {bits(value, 16U, 8U), bits(value, 8U, 8U), bits(value, 0U, 8U), 255U};
                return;
            }
        }

        void apply_xf_light_register(GXMaterialState &state, std::uint16_t address, std::uint32_t value) {
            const auto light_index = static_cast<std::size_t>((address - 0x0600U) / 16U);
            const auto word_index = static_cast<std::size_t>((address - 0x0600U) % 16U);
            if (light_index >= state.lights.size()) {
                return;
            }

            auto &light = state.lights[light_index];
            light.loaded = true;
            light.raw_words[word_index] = value;
            light.word_loaded[word_index] = true;

            if (word_index == 3U) {
                light.color = gx_color_from_xf_value(value);
                return;
            }

            if (word_index >= 4U && word_index <= 6U) {
                light.cosine_attenuation[word_index - 4U] = std::bit_cast<float>(value);
                return;
            }

            if (word_index >= 7U && word_index <= 9U) {
                light.distance_attenuation[word_index - 7U] = std::bit_cast<float>(value);
                return;
            }

            if (word_index >= 10U && word_index <= 12U) {
                light.position[word_index - 10U] = std::bit_cast<float>(value);
                return;
            }

            if (word_index >= 13U && word_index <= 15U) {
                light.direction[word_index - 13U] = std::bit_cast<float>(value);
            }
        }

        void apply_xf_register(GXMaterialState &state, std::uint16_t address, std::uint32_t value) {
            if (address >= 0x0600U && address < 0x0680U) {
                apply_xf_light_register(state, address, value);
                return;
            }

            if (address == 0x1009U) {
                state.color_channel_count = static_cast<std::uint8_t>(value & 0x3U);
                return;
            }

            if (address >= 0x100aU && address <= 0x100bU) {
                state.color_channels[address - 0x100aU].ambient_color = gx_color_from_xf_value(value);
                return;
            }

            if (address >= 0x100cU && address <= 0x100dU) {
                state.color_channels[address - 0x100cU].material_color = gx_color_from_xf_value(value);
                return;
            }

            if (address >= 0x100eU && address <= 0x100fU) {
                state.color_channels[address - 0x100eU].color_control = gx_color_channel_control_from_xf(value);
                return;
            }

            if (address >= 0x1010U && address <= 0x1011U) {
                state.color_channels[address - 0x1010U].alpha_control = gx_color_channel_control_from_xf(value);
            }
        }

        void apply_bp_register(GXMaterialState &state, std::uint8_t address, std::uint32_t value) {
            if (address == 0x00U) {
                state.texgen_count = bits(value, 0U, 4U);
                state.color_channel_count = bits(value, 4U, 3U);
                state.tev_stage_count = static_cast<std::uint8_t>(bits(value, 10U, 4U) + 1U);
                state.cull_mode = bits(value, 14U, 2U);
                state.indirect.stage_count = bits(value, 16U, 3U);
                return;
            }

            if (address >= 0x06U && address <= 0x0eU) {
                decode_indirect_matrix(state, address, value);
                return;
            }

            if (address >= 0x10U && address <= 0x1fU) {
                decode_indirect_tev_stage(state, address, value);
                return;
            }

            if (address == 0x22U) {
                decode_su_line_point_state(state, value);
                return;
            }

            if (address >= 0x25U && address <= 0x26U) {
                decode_indirect_tex_coord_scale(state, address, value);
                return;
            }

            if (address == 0x27U) {
                state.indirect.texture_orders.clear();
                state.indirect.texture_orders.reserve(4U);
                for (auto stage = 0U; stage < 4U; ++stage) {
                    state.indirect.texture_orders.push_back(GXIndirectTextureOrderState{
                        .stage = static_cast<std::uint8_t>(stage),
                        .tex_map = bits(value, static_cast<std::uint8_t>(stage * 6U), 3U),
                        .tex_coord = bits(value, static_cast<std::uint8_t>(stage * 6U + 3U), 3U),
                    });
                }
                return;
            }

            if (address >= 0x28U && address <= 0x2fU) {
                decode_two_tev_orders(state, address, value);
                return;
            }

            if (address >= 0x30U && address <= 0x3fU) {
                decode_tex_coord_scale(state, address, value);
                return;
            }

            if (address == 0x40U) {
                state.z_mode = GXZModeState{
                    .compare_enable = bits(value, 0U, 1U),
                    .function = bits(value, 1U, 3U),
                    .update_enable = bits(value, 4U, 1U),
                    .enabled = true,
                };
                return;
            }

            if (address == 0x41U) {
                const auto blend_enabled = bits(value, 0U, 1U) != 0U;
                const auto logic_enabled = bits(value, 1U, 1U) != 0U;
                const auto subtract_enabled = bits(value, 11U, 1U) != 0U;
                state.blend = GXBlendState{
                    .type = static_cast<std::uint8_t>(subtract_enabled ? 3U : (blend_enabled ? 1U : (logic_enabled ? 2U : 0U))),
                    .src_factor = bits(value, 8U, 3U),
                    .dst_factor = bits(value, 5U, 3U),
                    .op = bits(value, 12U, 4U),
                    .color_update = bits(value, 3U, 1U) != 0U,
                    .alpha_update = bits(value, 4U, 1U) != 0U,
                    .enabled = true,
                };
                return;
            }

            if (address == 0x43U) {
                state.z_comp_loc = bits(value, 6U, 1U);
                return;
            }

            if (address >= 0xc0U && address <= 0xdfU) {
                const auto stage = static_cast<std::uint8_t>((address - 0xc0U) / 2U);
                if ((address & 1U) == 0U) {
                    decode_color_tev_stage(state, stage, value);
                } else {
                    decode_alpha_tev_stage(state, stage, value);
                }
                return;
            }

            if (address >= 0xe0U && address <= 0xe7U) {
                decode_tev_register(state, address, value);
                return;
            }

            if (address >= 0xe8U && address <= 0xf2U) {
                decode_fog_register(state, address, value);
                return;
            }

            if (address == 0xf3U) {
                state.alpha_compare = GXAlphaCompareState{
                    .comp0 = bits(value, 16U, 3U),
                    .ref0 = bits(value, 0U, 8U),
                    .op = bits(value, 22U, 2U),
                    .comp1 = bits(value, 19U, 3U),
                    .ref1 = bits(value, 8U, 8U),
                    .enabled = true,
                };
                return;
            }

            if (address >= 0xf6U && address <= 0xfdU) {
                decode_tev_ksel(state, address, value);
            }
        }

        void append_register_load(GXMaterialState &state, GXRegisterSpace space, std::uint32_t byte_offset, std::uint16_t address, std::uint8_t count,
                                  std::uint32_t value) {
            state.mdl3_register_loads.push_back(GXRegisterLoadState{
                .space = space,
                .byte_offset = byte_offset,
                .address = address,
                .count = count,
                .value = value,
            });
        }

        [[nodiscard]] GXAlphaCompareState gx_alpha_compare_from_j3d(const J3dAlphaCompareSummary &value) {
            return GXAlphaCompareState{
                .comp0 = value.comp0,
                .ref0 = value.ref0,
                .op = value.op,
                .comp1 = value.comp1,
                .ref1 = value.ref1,
                .enabled = value.enabled,
            };
        }

        [[nodiscard]] GXBlendState gx_blend_from_j3d(const J3dBlendSummary &value) {
            return GXBlendState{
                .type = value.type,
                .src_factor = value.src_factor,
                .dst_factor = value.dst_factor,
                .op = value.op,
                .color_update = true,
                .alpha_update = true,
                .enabled = value.enabled,
            };
        }

        [[nodiscard]] GXZModeState gx_z_mode_from_j3d(const J3dZModeSummary &value) {
            return GXZModeState{
                .compare_enable = value.compare_enable,
                .function = value.function,
                .update_enable = value.update_enable,
                .enabled = value.enabled,
            };
        }

        [[nodiscard]] GXTevStageState gx_tev_stage_from_brlyt(const smgpc::layout::BrlytTevStage &stage, std::uint8_t stage_index) {
            auto raw = std::array<std::uint8_t, 20U>{};
            raw[0U] = stage.tex_coord_gen;
            raw[1U] = stage.color_chan;
            raw[2U] = static_cast<std::uint8_t>(stage.tex_map & 0xffU);
            raw[3U] = static_cast<std::uint8_t>(((stage.tex_map >> 8U) & 0x1U) | (stage.ras_swap << 1U) | (stage.tex_swap << 3U));
            raw[4U] = static_cast<std::uint8_t>((stage.color.a & 0xfU) | ((stage.color.b & 0xfU) << 4U));
            raw[5U] = static_cast<std::uint8_t>((stage.color.c & 0xfU) | ((stage.color.d & 0xfU) << 4U));
            raw[6U] = static_cast<std::uint8_t>((stage.color.op & 0xfU) | ((stage.color.bias & 0x3U) << 4U) | ((stage.color.scale & 0x3U) << 6U));
            raw[7U] = static_cast<std::uint8_t>((stage.color.clamp ? 1U : 0U) | ((stage.color.out_reg & 0x3U) << 1U) |
                                                ((stage.color.k_sel & 0x1fU) << 3U));
            raw[8U] = static_cast<std::uint8_t>((stage.alpha.a & 0xfU) | ((stage.alpha.b & 0xfU) << 4U));
            raw[9U] = static_cast<std::uint8_t>((stage.alpha.c & 0xfU) | ((stage.alpha.d & 0xfU) << 4U));
            raw[10U] = static_cast<std::uint8_t>((stage.alpha.op & 0xfU) | ((stage.alpha.bias & 0x3U) << 4U) | ((stage.alpha.scale & 0x3U) << 6U));
            raw[11U] = static_cast<std::uint8_t>((stage.alpha.clamp ? 1U : 0U) | ((stage.alpha.out_reg & 0x3U) << 1U) |
                                                 ((stage.alpha.k_sel & 0x1fU) << 3U));

            return GXTevStageState{
                .stage = stage_index,
                .raw = raw,
                .color_in = {stage.color.a, stage.color.b, stage.color.c, stage.color.d},
                .color_op = stage.color.op,
                .color_bias = stage.color.bias,
                .color_scale = stage.color.scale,
                .color_clamp = stage.color.clamp ? static_cast<std::uint8_t>(1U) : static_cast<std::uint8_t>(0U),
                .color_out = stage.color.out_reg,
                .k_color_sel = stage.color.k_sel,
                .alpha_in = {stage.alpha.a, stage.alpha.b, stage.alpha.c, stage.alpha.d},
                .alpha_op = stage.alpha.op,
                .alpha_bias = stage.alpha.bias,
                .alpha_scale = stage.alpha.scale,
                .alpha_clamp = stage.alpha.clamp ? static_cast<std::uint8_t>(1U) : static_cast<std::uint8_t>(0U),
                .alpha_out = stage.alpha.out_reg,
                .k_alpha_sel = stage.alpha.k_sel,
            };
        }

    }  // namespace

    GXColorValue gx_color_from_xf_value(std::uint32_t value) {
        return {
            static_cast<std::uint8_t>((value >> 24U) & 0xffU),
            static_cast<std::uint8_t>((value >> 16U) & 0xffU),
            static_cast<std::uint8_t>((value >> 8U) & 0xffU),
            static_cast<std::uint8_t>(value & 0xffU),
        };
    }

    GXColorChannelControlState gx_color_channel_control_from_xf(std::uint32_t value) {
        const auto attenuation_mode = bits(value, 9U, 2U);
        return GXColorChannelControlState{
            .raw = value & 0x7fffU,
            .material_source = bits(value, 0U, 1U),
            .lighting_enabled = bits(value, 1U, 1U) != 0U,
            .light_mask = static_cast<std::uint8_t>(bits(value, 2U, 4U) | (bits(value, 11U, 4U) << 4U)),
            .ambient_source = bits(value, 6U, 1U),
            .diffuse_function = bits(value, 7U, 2U),
            .attenuation_function = gx_attenuation_function_from_xf(attenuation_mode),
            .attenuation_mode = attenuation_mode,
        };
    }

    GXColorChannelControlState gx_color_channel_control_from_j3d(std::uint8_t enable, std::uint8_t material_source, std::uint8_t light_mask,
                                                                 std::uint8_t diffuse_function, std::uint8_t attenuation_function,
                                                                 std::uint8_t ambient_source) {
        auto raw = std::uint32_t{};
        raw |= static_cast<std::uint32_t>(material_source & 1U);
        raw |= static_cast<std::uint32_t>(enable & 1U) << 1U;
        raw |= static_cast<std::uint32_t>(light_mask & 0x0fU) << 2U;
        raw |= static_cast<std::uint32_t>(ambient_source & 1U) << 6U;
        raw |= static_cast<std::uint32_t>((attenuation_function == 0U ? 0U : diffuse_function) & 0x3U) << 7U;
        raw |= gx_xf_attenuation_bits_from_function(attenuation_function) << 9U;
        raw |= static_cast<std::uint32_t>((light_mask >> 4U) & 0x0fU) << 11U;
        auto state = gx_color_channel_control_from_xf(raw);
        state.attenuation_function = attenuation_function;
        return state;
    }

    GXColorValue gx_evaluate_lit_raster_color(const GXMaterialState &state, std::uint8_t color_channel, GXColorValue vertex_color,
                                              std::array<float, 3U> position, std::array<float, 3U> normal) {
        if (gx_color_channel_is_null(color_channel)) {
            return {0U, 0U, 0U, 0U};
        }

        const auto channel_index = gx_color_channel_index(color_channel);
        if (channel_index >= state.color_channel_count || channel_index >= state.color_channels.size()) {
            return {0U, 0U, 0U, 0U};
        }

        const auto &channel = state.color_channels[channel_index];
        const auto color_source = channel.color_control.material_source != 0U ? vertex_color : channel.material_color;
        const auto alpha_source = channel.alpha_control.material_source != 0U ? vertex_color : channel.material_color;
        const auto color_accumulator = lighting_accumulator(state, channel, channel.color_control, vertex_color, position, normal);
        const auto alpha_accumulator = lighting_accumulator(state, channel, channel.alpha_control, vertex_color, position, normal);

        return {
            apply_lighting_accumulator(color_source[0U], color_accumulator[0U]),
            apply_lighting_accumulator(color_source[1U], color_accumulator[1U]),
            apply_lighting_accumulator(color_source[2U], color_accumulator[2U]),
            apply_lighting_accumulator(alpha_source[3U], alpha_accumulator[3U]),
        };
    }

    GXMaterialState gx_state_from_j3d_material(const J3dMaterialSummary &material) {
        auto state = GXMaterialState{};
        state.source = "J3D";
        state.name = material.name;
        state.material_mode = material.material_mode;
        state.cull_mode = material.cull_mode;
        state.color_channel_count = material.color_channel_count;
        state.texgen_count = material.texgen_count;
        state.tev_stage_count = material.tev_stage_count;
        state.z_comp_loc = material.z_comp_loc;
        for (auto channel = 0U; channel < state.color_channels.size(); ++channel) {
            state.color_channels[channel].material_color = material.material_colors[channel];
            state.color_channels[channel].ambient_color = material.ambient_colors[channel];
            state.color_channels[channel].color_control = material.color_channel_controls[channel];
            state.color_channels[channel].alpha_control = material.alpha_channel_controls[channel];
        }
        state.tev_k_colors = material.tev_k_colors;
        for (auto register_index = 0U; register_index < 3U; ++register_index) {
            state.tev_registers[register_index + 1U] = material.tev_colors[register_index];
        }
        state.textures.reserve(material.textures.size());
        for (const auto &texture : material.textures) {
            state.textures.push_back(GXTextureBindingState{
                .slot = texture.slot,
                .texture_index = texture.texture_index,
            });
        }
        state.tex_coord_gens.reserve(material.tex_coord_gens.size());
        for (const auto &texgen : material.tex_coord_gens) {
            state.tex_coord_gens.push_back(GXTexCoordGenState{
                .slot = texgen.slot,
                .type = texgen.type,
                .source = texgen.source,
                .matrix = texgen.matrix,
            });
        }
        state.tex_matrices.reserve(material.tex_matrices.size());
        for (const auto &matrix : material.tex_matrices) {
            state.tex_matrices.push_back(GXTexMatrixState{
                .slot = matrix.slot,
                .projection = matrix.projection,
                .info = matrix.info,
                .center = matrix.center,
                .scale_s = matrix.scale_s,
                .scale_t = matrix.scale_t,
                .rotation = matrix.rotation,
                .translate_s = matrix.translate_s,
                .translate_t = matrix.translate_t,
                .effect_matrix = matrix.effect_matrix,
            });
        }
        state.tev_orders.reserve(material.tev_orders.size());
        for (const auto &order : material.tev_orders) {
            state.tev_orders.push_back(GXTevOrderState{
                .stage = order.stage,
                .tex_coord = order.tex_coord,
                .tex_map = order.tex_map,
                .color_channel = order.color_channel,
            });
        }
        state.tev_stages.reserve(material.tev_stages.size());
        for (const auto &stage : material.tev_stages) {
            state.tev_stages.push_back(GXTevStageState{
                .stage = stage.stage,
                .raw = stage.raw,
                .color_in = stage.color_in,
                .color_op = stage.color_op,
                .color_bias = stage.color_bias,
                .color_scale = stage.color_scale,
                .color_clamp = stage.color_clamp,
                .color_out = stage.color_out,
                .k_color_sel = stage.k_color_sel,
                .alpha_in = stage.alpha_in,
                .alpha_op = stage.alpha_op,
                .alpha_bias = stage.alpha_bias,
                .alpha_scale = stage.alpha_scale,
                .alpha_clamp = stage.alpha_clamp,
                .alpha_out = stage.alpha_out,
                .k_alpha_sel = stage.k_alpha_sel,
            });
        }
        state.alpha_compare = gx_alpha_compare_from_j3d(material.alpha_compare);
        state.blend = gx_blend_from_j3d(material.blend);
        state.z_mode = gx_z_mode_from_j3d(material.z_mode);
        return state;
    }

    GXMaterialState gx_state_from_brlyt_material(const smgpc::layout::BrlytMaterial &material) {
        auto state = GXMaterialState{};
        state.source = "BRLYT";
        state.name = material.name;
        state.texgen_count = static_cast<std::uint8_t>(material.tex_coord_gens.size());
        state.tev_stage_count = static_cast<std::uint8_t>(material.tev_stages.size());
        state.color_channel_count = material.has_chan_ctrl ? 1U : 0U;
        state.color_channels[0U].material_color = material.mat_color;
        state.color_channels[0U].color_control = gx_color_channel_control_from_j3d(0U, material.chan_color_src, 0U, 0U, 2U, 0U);
        state.color_channels[0U].alpha_control = gx_color_channel_control_from_j3d(0U, material.chan_alpha_src, 0U, 0U, 2U, 0U);
        state.tev_k_colors = material.tev_k_colors;

        state.textures.reserve(material.textures.size());
        for (auto slot = 0U; slot < material.textures.size(); ++slot) {
            const auto &texture = material.textures[slot];
            state.textures.push_back(GXTextureBindingState{
                .slot = static_cast<std::uint8_t>(slot),
                .texture_index = texture.texture_index,
                .wrap_s = texture.wrap_s,
                .wrap_t = texture.wrap_t,
                .min_filter = texture.min_filter,
                .mag_filter = texture.mag_filter,
            });
        }

        state.tex_coord_gens.reserve(material.tex_coord_gens.size());
        for (auto slot = 0U; slot < material.tex_coord_gens.size(); ++slot) {
            const auto &texgen = material.tex_coord_gens[slot];
            state.tex_coord_gens.push_back(GXTexCoordGenState{
                .slot = static_cast<std::uint8_t>(slot),
                .type = texgen.tex_gen_type,
                .source = texgen.tex_gen_src,
                .matrix = texgen.tex_mtx,
            });
        }

        state.tex_matrices.reserve(material.tex_srts.size());
        for (auto slot = 0U; slot < material.tex_srts.size(); ++slot) {
            const auto &srt = material.tex_srts[slot];
            state.tex_matrices.push_back(GXTexMatrixState{
                .slot = static_cast<std::uint8_t>(slot),
                .projection = 1U,
                .center = {0.5F, 0.5F, 0.0F},
                .scale_s = srt.scale_s,
                .scale_t = srt.scale_t,
                .rotation = static_cast<std::int16_t>(srt.rotate),
                .translate_s = srt.translate_s,
                .translate_t = srt.translate_t,
            });
        }

        state.tev_orders.reserve(material.tev_stages.size());
        state.tev_stages.reserve(material.tev_stages.size());
        for (auto stage_index = 0U; stage_index < material.tev_stages.size(); ++stage_index) {
            const auto &stage = material.tev_stages[stage_index];
            state.tev_orders.push_back(GXTevOrderState{
                .stage = static_cast<std::uint8_t>(stage_index),
                .tex_coord = stage.tex_coord_gen,
                .tex_map = static_cast<std::uint8_t>(stage.tex_map & 0xffU),
                .color_channel = stage.color_chan,
            });
            state.tev_stages.push_back(gx_tev_stage_from_brlyt(stage, static_cast<std::uint8_t>(stage_index)));
        }

        for (auto register_index = 0U; register_index < material.tev_colors.size(); ++register_index) {
            state.tev_registers[register_index + 1U] = material.tev_colors[register_index];
        }
        state.alpha_compare = GXAlphaCompareState{
            .comp0 = material.alpha_compare.comp0,
            .ref0 = material.alpha_compare.ref0,
            .op = material.alpha_compare.op,
            .comp1 = material.alpha_compare.comp1,
            .ref1 = material.alpha_compare.ref1,
            .enabled = material.alpha_compare.enabled,
        };
        state.blend = GXBlendState{
            .type = material.blend_mode.type,
            .src_factor = material.blend_mode.src_factor,
            .dst_factor = material.blend_mode.dst_factor,
            .op = material.blend_mode.op,
            .color_update = true,
            .alpha_update = true,
            .enabled = true,
        };
        return state;
    }

    GXBPRegisterState gx_bp_registers_from_state(const GXMaterialState &state) {
        return initial_bp_registers_from_state(state);
    }

    void gx_apply_mdl3_display_list(GXMaterialState &state, std::span<const std::uint8_t> display_list) {
        auto bp_registers = gx_bp_registers_from_state(state);
        gx_apply_mdl3_display_list(state, display_list, &bp_registers);
    }

    void gx_apply_mdl3_display_list(GXMaterialState &state, std::span<const std::uint8_t> display_list, GXBPRegisterState *bp_registers) {
        state.mdl3_display_list.assign(display_list.begin(), display_list.end());
        state.mdl3_stats = GXDisplayListStats{};
        state.mdl3_register_loads.clear();

        auto bp_mask = std::uint32_t{0xffffffU};

        auto cursor = std::size_t{};
        while (cursor < display_list.size()) {
            const auto command_offset = cursor;
            const auto command = display_list[cursor++];
            if (command == 0x00U) {
                ++state.mdl3_stats.command_count;
                state.mdl3_stats.parsed_bytes = static_cast<std::uint32_t>(cursor);
                continue;
            }

            switch (command) {
            case 0x08U:
                if (cursor + 5U > display_list.size()) {
                    ++state.mdl3_stats.unknown_opcode_count;
                    cursor = display_list.size();
                    break;
                }
                append_register_load(state, GXRegisterSpace::CP, static_cast<std::uint32_t>(command_offset), display_list[cursor], 1U,
                                     read_be32(display_list, cursor + 1U));
                ++state.mdl3_stats.command_count;
                ++state.mdl3_stats.cp_load_count;
                cursor += 5U;
                break;
            case 0x10U: {
                if (cursor + 4U > display_list.size()) {
                    ++state.mdl3_stats.unknown_opcode_count;
                    cursor = display_list.size();
                    break;
                }

                const auto header = read_be32(display_list, cursor);
                cursor += 4U;
                const auto address = static_cast<std::uint16_t>(header & 0xffffU);
                const auto count = static_cast<std::uint8_t>(((header >> 16U) & 0x0fU) + 1U);
                if (cursor + static_cast<std::size_t>(count) * 4U > display_list.size()) {
                    ++state.mdl3_stats.unknown_opcode_count;
                    cursor = display_list.size();
                    break;
                }

                for (auto i = 0U; i < count; ++i) {
                    const auto value = read_be32(display_list, cursor + i * 4U);
                    append_register_load(state, GXRegisterSpace::XF, static_cast<std::uint32_t>(command_offset),
                                         static_cast<std::uint16_t>(address + i), count, value);
                    apply_xf_register(state, static_cast<std::uint16_t>(address + i), value);
                }
                ++state.mdl3_stats.command_count;
                ++state.mdl3_stats.xf_load_count;
                cursor += static_cast<std::size_t>(count) * 4U;
                break;
            }
            case 0x20U:
            case 0x28U:
            case 0x30U:
            case 0x38U:
                if (cursor + 4U > display_list.size()) {
                    ++state.mdl3_stats.unknown_opcode_count;
                    cursor = display_list.size();
                    break;
                }
                append_register_load(state,
                                     command == 0x20U ? GXRegisterSpace::IndexedA :
                                     command == 0x28U ? GXRegisterSpace::IndexedB :
                                     command == 0x30U ? GXRegisterSpace::IndexedC :
                                                        GXRegisterSpace::IndexedD,
                                     static_cast<std::uint32_t>(command_offset), static_cast<std::uint16_t>(read_be32(display_list, cursor) & 0xfffU),
                                     static_cast<std::uint8_t>(((read_be32(display_list, cursor) >> 12U) & 0x0fU) + 1U),
                                     read_be32(display_list, cursor));
                ++state.mdl3_stats.command_count;
                ++state.mdl3_stats.indexed_load_count;
                cursor += 4U;
                break;
            case 0x40U:
                if (cursor + 8U > display_list.size()) {
                    ++state.mdl3_stats.unknown_opcode_count;
                    cursor = display_list.size();
                    break;
                }
                ++state.mdl3_stats.command_count;
                cursor += 8U;
                break;
            case 0x48U:
                ++state.mdl3_stats.command_count;
                break;
            case 0x61U: {
                if (cursor + 4U > display_list.size()) {
                    ++state.mdl3_stats.unknown_opcode_count;
                    cursor = display_list.size();
                    break;
                }

                const auto address = display_list[cursor];
                const auto value = read_be24(display_list, cursor + 1U);
                append_register_load(state, GXRegisterSpace::BP, static_cast<std::uint32_t>(command_offset), address, 1U, value);
                if (address == 0xfeU) {
                    bp_mask = value & 0xffffffU;
                } else {
                    const auto effective_value = ((*bp_registers)[address] & ~bp_mask) | (value & bp_mask);
                    (*bp_registers)[address] = effective_value;
                    apply_bp_register(state, address, effective_value);
                    bp_mask = 0xffffffU;
                }
                ++state.mdl3_stats.command_count;
                ++state.mdl3_stats.bp_load_count;
                cursor += 4U;
                break;
            }
            default:
                if (command >= 0x80U && command <= 0xbfU) {
                    if (cursor + 2U > display_list.size()) {
                        ++state.mdl3_stats.unknown_opcode_count;
                        cursor = display_list.size();
                        break;
                    }

                    const auto vertex_count = read_be16(display_list, cursor);
                    cursor += 2U;
                    ++state.mdl3_stats.command_count;
                    ++state.mdl3_stats.primitive_count;
                    cursor = std::min(display_list.size(), cursor + static_cast<std::size_t>(vertex_count));
                    break;
                }

                ++state.mdl3_stats.command_count;
                ++state.mdl3_stats.unknown_opcode_count;
                cursor = display_list.size();
                break;
            }

            state.mdl3_stats.parsed_bytes = static_cast<std::uint32_t>(cursor);
        }
    }

}  // namespace smgpc::render
