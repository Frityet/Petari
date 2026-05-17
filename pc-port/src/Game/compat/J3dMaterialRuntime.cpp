#include "J3dMaterialRuntime.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace smgpc::game {
    namespace {

        constexpr auto GX_TG_MTX3X4 = 0U;
        constexpr auto GX_TG_POS = 0U;
        constexpr auto GX_TG_NRM = 1U;
        constexpr auto GX_TG_TEX0 = 4U;

        struct TexGenInput {
            float x = 0.0F;
            float y = 0.0F;
            float z = 1.0F;
        };

        struct TevOperation {
            std::uint8_t op = 0U;
            std::uint8_t bias = 0U;
            std::uint8_t scale = 0U;
            bool clamp = true;
        };

        using Matrix4 = std::array< float, 16U >;
        using TevColor = std::array< int, 4U >;
        using TevRegisters = std::array< TevColor, 4U >;

        [[nodiscard]] const J3dMaterialTextureBinding* find_texture_binding(const J3dMaterialSummary& material, std::uint8_t slot) {
            const auto it = std::ranges::find_if(material.textures, [slot](const auto& binding) { return binding.slot == slot; });
            return it == material.textures.end() ? nullptr : &*it;
        }

        [[nodiscard]] const J3dTexCoordGenSummary* find_tex_coord_gen(const J3dMaterialSummary& material, std::uint8_t slot) {
            const auto it = std::ranges::find_if(material.tex_coord_gens, [slot](const auto& gen) { return gen.slot == slot; });
            return it == material.tex_coord_gens.end() ? nullptr : &*it;
        }

        [[nodiscard]] const J3dTexMatrixSummary* find_tex_matrix(const J3dMaterialSummary& material, const J3dTexCoordGenSummary* gen) {
            if (gen == nullptr) {
                return nullptr;
            }

            const auto matrix_slot = j3d_tex_matrix_slot_from_gx_matrix(gen->matrix);
            if (!matrix_slot.has_value()) {
                return nullptr;
            }

            const auto it = std::ranges::find_if(material.tex_matrices, [slot = *matrix_slot](const auto& matrix) { return matrix.slot == slot; });
            return it == material.tex_matrices.end() ? nullptr : &*it;
        }

        void append_pass_for_slots(std::vector< J3dMaterialTexturePass >& passes, const J3dMaterialSummary& material, std::uint8_t stage,
                                   std::uint8_t tex_coord_slot, std::uint8_t tex_map_slot) {
            if (tex_map_slot == 0xffU) {
                return;
            }

            const auto* binding = find_texture_binding(material, tex_map_slot);
            if (binding == nullptr || binding->texture_index == 0xffffU) {
                return;
            }

            const auto* gen = tex_coord_slot == 0xffU ? nullptr : find_tex_coord_gen(material, tex_coord_slot);
            const auto* matrix = find_tex_matrix(material, gen);
            passes.push_back(J3dMaterialTexturePass{
                .stage = stage,
                .tex_coord_slot = tex_coord_slot,
                .tex_map_slot = tex_map_slot,
                .texture_index = binding->texture_index,
                .tex_coord_gen = gen == nullptr ? std::optional< J3dTexCoordGenSummary >{} : *gen,
                .tex_matrix = matrix == nullptr ? std::optional< J3dTexMatrixSummary >{} : *matrix,
            });
        }

        [[nodiscard]] TexGenInput tex_gen_input_for_source(const J3dMeshVertex& source, const J3dTexCoordGenSummary* tex_coord_gen) {
            if (tex_coord_gen != nullptr && tex_coord_gen->source == GX_TG_POS) {
                return TexGenInput{
                    .x = source.x,
                    .y = source.y,
                    .z = source.z,
                };
            }
            if (tex_coord_gen != nullptr && tex_coord_gen->source == GX_TG_NRM) {
                return TexGenInput{
                    .x = source.normal[0U],
                    .y = source.normal[1U],
                    .z = source.normal[2U],
                };
            }
            if (tex_coord_gen != nullptr && tex_coord_gen->source >= GX_TG_TEX0) {
                const auto slot = static_cast<std::size_t>(tex_coord_gen->source - GX_TG_TEX0);
                if (slot < source.tex_coords.size() && slot < source.tex_coord_count) {
                    return TexGenInput{
                        .x = source.tex_coords[slot][0U],
                        .y = source.tex_coords[slot][1U],
                        .z = 1.0F,
                    };
                }
                if (slot == 0U) {
                    return TexGenInput{
                        .x = source.u,
                        .y = source.v,
                        .z = 1.0F,
                    };
                }
            }

            return TexGenInput{
                .x = source.u,
                .y = source.v,
                .z = 1.0F,
            };
        }

        [[nodiscard]] J3dMatrix3x4 texture_srt_matrix(const J3dTexMatrixSummary& tex_matrix) {
            constexpr auto pi = 3.14159265358979323846F;
            const auto angle = static_cast< float >(tex_matrix.rotation) * pi / 32768.0F;
            const auto cos_angle = std::cos(angle);
            const auto sin_angle = std::sin(angle);
            const auto cx = tex_matrix.scale_s * cos_angle;
            const auto sx = tex_matrix.scale_s * sin_angle;
            const auto sy = tex_matrix.scale_t * sin_angle;
            const auto cy = tex_matrix.scale_t * cos_angle;

            return J3dMatrix3x4{
                .m =
                    {
                        cx,
                        -sx,
                        (-cx * tex_matrix.center[0U] + sx * tex_matrix.center[1U]) + tex_matrix.center[0U] + tex_matrix.translate_s,
                        0.0F,
                        sy,
                        cy,
                        (-sy * tex_matrix.center[0U] - cy * tex_matrix.center[1U]) + tex_matrix.center[1U] + tex_matrix.translate_t,
                        0.0F,
                        0.0F,
                        0.0F,
                        1.0F,
                        0.0F,
                    },
            };
        }

        [[nodiscard]] J3dMatrix3x4 texture_projection_bias_matrix() {
            return J3dMatrix3x4{
                .m =
                    {
                        0.5F,
                        0.0F,
                        0.5F,
                        0.0F,
                        0.0F,
                        -0.5F,
                        0.5F,
                        0.0F,
                        0.0F,
                        0.0F,
                        1.0F,
                        0.0F,
                    },
            };
        }

        [[nodiscard]] J3dMatrix3x4 concat_affine_3x4(const J3dMatrix3x4& left, const J3dMatrix3x4& right) {
            auto result = J3dMatrix3x4{};
            result.m.fill(0.0F);
            for (auto row = 0U; row < 3U; ++row) {
                for (auto column = 0U; column < 3U; ++column) {
                    auto value = 0.0F;
                    for (auto k = 0U; k < 3U; ++k) {
                        value += left.m[row * 4U + k] * right.m[k * 4U + column];
                    }
                    result.m[row * 4U + column] = value;
                }

                result.m[row * 4U + 3U] = left.m[row * 4U + 0U] * right.m[3U] + left.m[row * 4U + 1U] * right.m[7U] +
                                          left.m[row * 4U + 2U] * right.m[11U] + left.m[row * 4U + 3U];
            }
            return result;
        }

        [[nodiscard]] J3dMatrix3x4 concat_projected_3x4_4x4(const J3dMatrix3x4& left, const Matrix4& right) {
            auto result = J3dMatrix3x4{};
            result.m.fill(0.0F);
            for (auto row = 0U; row < 3U; ++row) {
                for (auto column = 0U; column < 4U; ++column) {
                    auto value = 0.0F;
                    for (auto k = 0U; k < 4U; ++k) {
                        value += left.m[row * 4U + k] * right[k * 4U + column];
                    }
                    result.m[row * 4U + column] = value;
                }
            }
            return result;
        }

        [[nodiscard]] J3dMatrix3x4 texture_projection_matrix(const J3dTexMatrixSummary& tex_matrix, const J3dMatrix3x4* model_matrix) {
            auto matrix = concat_affine_3x4(texture_srt_matrix(tex_matrix), texture_projection_bias_matrix());
            matrix = concat_projected_3x4_4x4(matrix, tex_matrix.effect_matrix);
            if (model_matrix != nullptr) {
                matrix = concat_affine_3x4(matrix, *model_matrix);
            }
            return matrix;
        }

        [[nodiscard]] J3dTextureProjectionCoordinate project_with_matrix(const TexGenInput& coord, const J3dTexCoordGenSummary* tex_coord_gen,
                                                                         const J3dMatrix3x4& matrix) {
            const auto u = matrix.m[0U] * coord.x + matrix.m[1U] * coord.y + matrix.m[2U] * coord.z + matrix.m[3U];
            const auto v = matrix.m[4U] * coord.x + matrix.m[5U] * coord.y + matrix.m[6U] * coord.z + matrix.m[7U];
            const auto q = matrix.m[8U] * coord.x + matrix.m[9U] * coord.y + matrix.m[10U] * coord.z + matrix.m[11U];
            return J3dTextureProjectionCoordinate{
                .u = u,
                .v = v,
                .q = tex_coord_gen != nullptr && tex_coord_gen->type == GX_TG_MTX3X4 ? q : 1.0F,
            };
        }

        [[nodiscard]] bool uses_projected_texture_matrix(const J3dTexMatrixSummary& tex_matrix) {
            const auto mode = static_cast< std::uint8_t >(tex_matrix.info & 0x3fU);
            return mode == 8U || mode == 9U || mode == 11U;
        }

        [[nodiscard]] const J3dTevOrderSummary* find_tev_order(const J3dMaterialSummary& material, std::uint8_t stage_index) {
            const auto it = std::ranges::find_if(material.tev_orders, [stage_index](const auto& order) { return order.stage == stage_index; });
            return it == material.tev_orders.end() ? nullptr : &*it;
        }

        [[nodiscard]] TevColor color_to_tev(std::array< std::uint8_t, 4U > color) {
            return {
                color[0U],
                color[1U],
                color[2U],
                color[3U],
            };
        }

        [[nodiscard]] TevColor konst_color(const J3dMaterialSummary& material, std::uint8_t selector) {
            constexpr std::array< int, 8U > constants{255, 223, 191, 159, 128, 96, 64, 32};
            if (selector < constants.size()) {
                return {constants[selector], constants[selector], constants[selector], constants[selector]};
            }
            if (selector >= 12U && selector <= 15U) {
                const auto& color = material.tev_k_colors[selector - 12U];
                return color_to_tev(color);
            }
            if (selector >= 16U && selector <= 31U) {
                const auto color_index = (selector - 16U) % 4U;
                const auto component = (selector - 16U) / 4U;
                const auto value = material.tev_k_colors[color_index][component];
                return {value, value, value, value};
            }

            return {0, 0, 0, 0};
        }

        [[nodiscard]] int color_arg_value(std::uint8_t arg, std::size_t component, const TevRegisters& registers, const TevColor& texture,
                                          const TevColor& raster, const TevColor& konst) {
            switch (arg) {
            case 0U:
                return registers[0U][component];
            case 1U:
                return registers[0U][3U];
            case 2U:
                return registers[1U][component];
            case 3U:
                return registers[1U][3U];
            case 4U:
                return registers[2U][component];
            case 5U:
                return registers[2U][3U];
            case 6U:
                return registers[3U][component];
            case 7U:
                return registers[3U][3U];
            case 8U:
                return texture[component];
            case 9U:
                return texture[3U];
            case 10U:
                return raster[component];
            case 11U:
                return raster[3U];
            case 12U:
                return 255;
            case 13U:
                return 128;
            case 14U:
                return konst[component];
            default:
                return 0;
            }
        }

        [[nodiscard]] int alpha_arg_value(std::uint8_t arg, const TevRegisters& registers, const TevColor& texture, const TevColor& raster,
                                          const TevColor& konst) {
            switch (arg) {
            case 0U:
                return registers[0U][3U];
            case 1U:
                return registers[1U][3U];
            case 2U:
                return registers[2U][3U];
            case 3U:
                return registers[3U][3U];
            case 4U:
                return texture[3U];
            case 5U:
                return raster[3U];
            case 6U:
                return konst[3U];
            default:
                return 0;
            }
        }

        [[nodiscard]] int tev_regular(const TevOperation& operation, int a, int b, int c, int d) {
            constexpr std::array< int, 4U > bias{0, 128, -128, 0};
            constexpr std::array< int, 4U > scale_lshift{0, 1, 2, 0};
            constexpr std::array< int, 4U > scale_rshift{0, 0, 0, 1};
            const auto scale = std::min< std::uint8_t >(operation.scale, 3U);
            const auto c256 = c + (c >> 7);
            auto temp = a * (256 - c256) + b * c256;
            temp <<= scale_lshift[scale];
            if (scale != 3U) {
                temp += operation.op == 1U ? 127 : 128;
            }
            temp >>= 8;
            if (operation.op == 1U) {
                temp = -temp;
            }

            auto result = ((d + bias[std::min< std::uint8_t >(operation.bias, 3U)]) << scale_lshift[scale]) + temp;
            result >>= scale_rshift[scale];
            if (operation.clamp) {
                return std::clamp(result, 0, 255);
            }

            return std::clamp(result, -1024, 1023);
        }

        [[nodiscard]] bool compare_alpha(int alpha, std::uint8_t compare, std::uint8_t reference) {
            switch (compare) {
            case 0U:
                return false;
            case 1U:
                return alpha < reference;
            case 2U:
                return alpha == reference;
            case 3U:
                return alpha <= reference;
            case 4U:
                return alpha > reference;
            case 5U:
                return alpha != reference;
            case 6U:
                return alpha >= reference;
            default:
                return true;
            }
        }

        [[nodiscard]] bool passes_alpha_compare(const J3dAlphaCompareSummary& alpha_compare, int alpha) {
            if (!alpha_compare.enabled) {
                return true;
            }

            const auto lhs = compare_alpha(alpha, alpha_compare.comp0, alpha_compare.ref0);
            const auto rhs = compare_alpha(alpha, alpha_compare.comp1, alpha_compare.ref1);
            switch (alpha_compare.op) {
            case 1U:
                return lhs || rhs;
            case 2U:
                return lhs != rhs;
            case 3U:
                return lhs == rhs;
            default:
                return lhs && rhs;
            }
        }

        [[nodiscard]] TevColor sample_texture_rgba8(const DecodedTexture& texture, bool wrap_s, bool wrap_t, float u, float v) {
            if (texture.width == 0U || texture.height == 0U ||
                texture.rgba.size() < static_cast< std::size_t >(texture.width) * texture.height * 4U) {
                return {0, 0, 0, 0};
            }

            const auto wrap_coord = [](float value) {
                value = value - std::floor(value);
                return value < 0.0F ? value + 1.0F : value;
            };
            const auto clamp_coord = [](float value) { return std::clamp(value, 0.0F, 1.0F); };

            const auto sample_u = wrap_s ? wrap_coord(u) : clamp_coord(u);
            const auto sample_v = wrap_t ? wrap_coord(v) : clamp_coord(v);
            const auto x = sample_u * static_cast< float >(texture.width) - 0.5F;
            const auto y = sample_v * static_cast< float >(texture.height) - 0.5F;
            const auto x0 = static_cast< int >(std::floor(x));
            const auto y0 = static_cast< int >(std::floor(y));
            const auto fx = x - static_cast< float >(x0);
            const auto fy = y - static_cast< float >(y0);

            const auto resolve = [](int coordinate, std::uint16_t size, bool wrap) {
                if (wrap) {
                    auto value = coordinate % static_cast< int >(size);
                    return value < 0 ? value + static_cast< int >(size) : value;
                }
                return std::clamp(coordinate, 0, static_cast< int >(size) - 1);
            };

            const auto pixel = [&](int px, int py, std::size_t component) {
                const auto sx = resolve(px, texture.width, wrap_s);
                const auto sy = resolve(py, texture.height, wrap_t);
                const auto offset = (static_cast< std::size_t >(sy) * texture.width + static_cast< std::size_t >(sx)) * 4U + component;
                return static_cast< float >(texture.rgba[offset]);
            };

            auto color = TevColor{};
            for (auto component = 0U; component < 4U; ++component) {
                const auto top = pixel(x0, y0, component) * (1.0F - fx) + pixel(x0 + 1, y0, component) * fx;
                const auto bottom = pixel(x0, y0 + 1, component) * (1.0F - fx) + pixel(x0 + 1, y0 + 1, component) * fx;
                color[component] = static_cast< int >(std::clamp(std::round(top * (1.0F - fy) + bottom * fy), 0.0F, 255.0F));
            }

            return color;
        }

        [[nodiscard]] const GXIndirectTevStageState* find_active_indirect_tev_stage(const J3dMaterialSummary& material, std::uint8_t tev_stage) {
            const auto& indirect = material.gx_state.indirect;
            const auto it = std::ranges::find_if(indirect.tev_stages, [tev_stage, &indirect](const auto& stage) {
                return stage.active && stage.tev_stage == tev_stage && stage.ind_stage < indirect.stage_count;
            });
            return it == indirect.tev_stages.end() ? nullptr : &*it;
        }

        [[nodiscard]] const GXIndirectTextureOrderState* find_indirect_texture_order(const GXIndirectState& indirect, std::uint8_t stage) {
            const auto it = std::ranges::find_if(indirect.texture_orders, [stage](const auto& order) { return order.stage == stage; });
            return it == indirect.texture_orders.end() ? nullptr : &*it;
        }

        [[nodiscard]] const GXIndirectTextureMatrixState* find_indirect_texture_matrix(const GXIndirectState& indirect, std::uint8_t matrix_index) {
            if (matrix_index == 0U) {
                return nullptr;
            }

            const auto decoded_index = static_cast< std::uint8_t >(matrix_index - 1U);
            const auto it =
                std::ranges::find_if(indirect.texture_matrices, [decoded_index](const auto& matrix) { return matrix.matrix == decoded_index; });
            return it == indirect.texture_matrices.end() ? nullptr : &*it;
        }

        [[nodiscard]] const GXIndirectTextureCoordScaleState* find_indirect_texture_scale(const GXIndirectState& indirect, std::uint8_t stage) {
            const auto it = std::ranges::find_if(indirect.texture_coord_scales, [stage](const auto& scale) { return scale.stage == stage; });
            return it == indirect.texture_coord_scales.end() ? nullptr : &*it;
        }

        [[nodiscard]] std::array< std::int32_t, 3U > indirect_coordinate_from_sample(const GXIndirectTevStageState& stage,
                                                                                     const TevColor& sample) {
            constexpr std::array< std::uint8_t, 4U > format_shifts{0U, 3U, 4U, 5U};
            const auto shift = format_shifts[std::min< std::size_t >(stage.format, format_shifts.size() - 1U)];
            const auto bias = stage.format == 0U ? -128 : 1;
            auto coord = std::array< std::int32_t, 3U >{
                sample[3U] >> shift,
                sample[2U] >> shift,
                sample[1U] >> shift,
            };

            if ((stage.bias & 0x1U) != 0U) {
                coord[0U] += bias;
            }
            if ((stage.bias & 0x2U) != 0U) {
                coord[1U] += bias;
            }
            if ((stage.bias & 0x4U) != 0U) {
                coord[2U] += bias;
            }
            return coord;
        }

        [[nodiscard]] std::int64_t shift_indirect_value(std::int64_t value, int shift) {
            if (shift >= 0) {
                return value / (std::int64_t{1} << std::min(shift, 30));
            }

            return value * (std::int64_t{1} << std::min(-shift, 30));
        }

        [[nodiscard]] std::int64_t wrap_indirect_coordinate(std::int64_t coord, std::uint8_t wrap) {
            switch (wrap) {
            case 0U:
                return coord;
            case 1U:
                return coord & ((std::int64_t{256} << 7U) - 1);
            case 2U:
                return coord & ((std::int64_t{128} << 7U) - 1);
            case 3U:
                return coord & ((std::int64_t{64} << 7U) - 1);
            case 4U:
                return coord & ((std::int64_t{32} << 7U) - 1);
            case 5U:
                return coord & ((std::int64_t{16} << 7U) - 1);
            default:
                return 0;
            }
        }

        [[nodiscard]] std::array< std::int64_t, 2U > indirect_translation(const GXIndirectTevStageState& stage,
                                                                          const GXIndirectTextureMatrixState* matrix, std::int64_t base_s,
                                                                          std::int64_t base_t,
                                                                          std::array< std::int32_t, 3U > indcoord) {
            if (matrix == nullptr) {
                return {0, 0};
            }

            auto translation = std::array< std::int64_t, 2U >{0, 0};
            switch (stage.matrix_id) {
            case 0U:
                translation[0U] =
                    (static_cast< std::int64_t >(matrix->ma) * indcoord[0U] + static_cast< std::int64_t >(matrix->mc) * indcoord[1U] +
                     static_cast< std::int64_t >(matrix->me) * indcoord[2U]) /
                    8;
                translation[1U] =
                    (static_cast< std::int64_t >(matrix->mb) * indcoord[0U] + static_cast< std::int64_t >(matrix->md) * indcoord[1U] +
                     static_cast< std::int64_t >(matrix->mf) * indcoord[2U]) /
                    8;
                break;
            case 1U:
                translation[0U] = (base_s * indcoord[0U]) / 256;
                translation[1U] = (base_t * indcoord[0U]) / 256;
                break;
            case 2U:
                translation[0U] = (base_s * indcoord[1U]) / 256;
                translation[1U] = (base_t * indcoord[1U]) / 256;
                break;
            default:
                return translation;
            }

            const auto shift = 17 - static_cast< int >(matrix->scale);
            translation[0U] = shift_indirect_value(translation[0U], shift);
            translation[1U] = shift_indirect_value(translation[1U], shift);
            return translation;
        }

        [[nodiscard]] std::optional< J3dIndirectTextureTrace >
        trace_indirect_texture_transform(const J3dMaterialSummary& material, std::span< const J3dTexture > textures,
                                         const J3dMeshVertex& source, const J3dMaterialTexturePass& pass,
                                         const J3dTextureCoordinate& coord, const DecodedTexture& base_texture,
                                         const J3dMatrix3x4* model_matrix) {
            const auto* stage = find_active_indirect_tev_stage(material, pass.stage);
            if (stage == nullptr || base_texture.width == 0U || base_texture.height == 0U) {
                return std::nullopt;
            }

            const auto* order = find_indirect_texture_order(material.gx_state.indirect, stage->ind_stage);
            if (order == nullptr) {
                return std::nullopt;
            }

            const auto* binding = find_texture_binding(material, order->tex_map);
            if (binding == nullptr || binding->texture_index >= textures.size()) {
                return std::nullopt;
            }

            const auto& indirect_texture = textures[binding->texture_index].image;
            if (indirect_texture.width == 0U || indirect_texture.height == 0U) {
                return std::nullopt;
            }

            auto trace = J3dIndirectTextureTrace{
                .tev_stage = pass.stage,
                .indirect_stage = stage->ind_stage,
                .indirect_tex_map = order->tex_map,
                .indirect_tex_coord = order->tex_coord,
                .format = stage->format,
                .bias = stage->bias,
                .matrix_index = stage->matrix_index,
                .matrix_id = stage->matrix_id,
                .wrap_s = stage->wrap_s,
                .wrap_t = stage->wrap_t,
                .add_previous = stage->add_previous,
                .base_coord = coord,
            };
            auto indirect_coord = coord;
            if (const auto* gen = find_tex_coord_gen(material, order->tex_coord); gen != nullptr) {
                const auto* matrix = find_tex_matrix(material, gen);
                indirect_coord = j3d_transform_tex_coord(source, gen, matrix, model_matrix);
            }
            trace.indirect_coord = indirect_coord;

            const auto* scale = find_indirect_texture_scale(material.gx_state.indirect, stage->ind_stage);
            trace.base_indirect_s =
                static_cast< std::int64_t >(std::llround(indirect_coord.u * static_cast< float >(indirect_texture.width) * 128.0F));
            trace.base_indirect_t =
                static_cast< std::int64_t >(std::llround(indirect_coord.v * static_cast< float >(indirect_texture.height) * 128.0F));
            trace.scaled_indirect_s = shift_indirect_value(trace.base_indirect_s, scale == nullptr ? 0 : scale->scale_s);
            trace.scaled_indirect_t = shift_indirect_value(trace.base_indirect_t, scale == nullptr ? 0 : scale->scale_t);
            const auto sampled = sample_texture_rgba8(indirect_texture, textures[binding->texture_index].wrap_s != 0U,
                                                      textures[binding->texture_index].wrap_t != 0U,
                                                      static_cast< float >(trace.scaled_indirect_s) /
                                                          (static_cast< float >(indirect_texture.width) * 128.0F),
                                                      static_cast< float >(trace.scaled_indirect_t) /
                                                          (static_cast< float >(indirect_texture.height) * 128.0F));
            trace.sampled_indirect_color = sampled;

            trace.base_s = static_cast< std::int64_t >(std::llround(coord.u * static_cast< float >(base_texture.width) * 128.0F));
            trace.base_t = static_cast< std::int64_t >(std::llround(coord.v * static_cast< float >(base_texture.height) * 128.0F));
            const auto* matrix = find_indirect_texture_matrix(material.gx_state.indirect, stage->matrix_index);
            trace.biased_indirect_coord = indirect_coordinate_from_sample(*stage, sampled);
            trace.translation = indirect_translation(*stage, matrix, trace.base_s, trace.base_t, trace.biased_indirect_coord);
            trace.transformed_s = wrap_indirect_coordinate(trace.base_s, stage->wrap_s) + trace.translation[0U];
            trace.transformed_t = wrap_indirect_coordinate(trace.base_t, stage->wrap_t) + trace.translation[1U];
            if (stage->add_previous) {
                trace.transformed_s += trace.base_s;
                trace.transformed_t += trace.base_t;
            }

            trace.transformed_coord = J3dTextureCoordinate{
                .u = static_cast< float >(trace.transformed_s) / (static_cast< float >(base_texture.width) * 128.0F),
                .v = static_cast< float >(trace.transformed_t) / (static_cast< float >(base_texture.height) * 128.0F),
            };
            return trace;
        }

        [[nodiscard]] J3dTextureCoordinate apply_indirect_texture_transform(const J3dMaterialSummary& material,
                                                                            std::span< const J3dTexture > textures,
                                                                            const J3dMeshVertex& source,
                                                                            const J3dMaterialTexturePass& pass,
                                                                            const J3dTextureCoordinate& coord,
                                                                            const DecodedTexture& base_texture,
                                                                            const J3dMatrix3x4* model_matrix) {
            const auto trace = trace_indirect_texture_transform(material, textures, source, pass, coord, base_texture, model_matrix);
            return trace.has_value() ? trace->transformed_coord : coord;
        }

        [[nodiscard]] TevColor texture_for_stage(std::span< const TevColor > textures_by_stage, std::uint8_t stage, const TevColor& fallback) {
            if (stage < textures_by_stage.size()) {
                return textures_by_stage[stage];
            }

            return fallback;
        }

        [[nodiscard]] TevRegisters initial_tev_registers_for_material(const J3dMaterialSummary& material) {
            auto registers = TevRegisters{};
            for (auto register_index = 0U; register_index < registers.size(); ++register_index) {
                for (auto component = 0U; component < registers[register_index].size(); ++component) {
                    registers[register_index][component] = material.gx_state.tev_registers[register_index][component];
                }
            }
            return registers;
        }

        [[nodiscard]] TevColor evaluate_tev_stages(const J3dMaterialSummary& material, std::span< const TevColor > textures_by_stage,
                                                   const TevColor& fallback_texture, const TevColor& raster) {
            auto registers = initial_tev_registers_for_material(material);
            auto output = TevColor{0, 0, 0, 0};
            if (material.tev_stages.empty()) {
                return fallback_texture;
            }

            std::uint8_t last_color_register = 0U;
            std::uint8_t last_alpha_register = 0U;
            for (const auto& stage : material.tev_stages) {
                const auto texture = texture_for_stage(textures_by_stage, stage.stage, fallback_texture);
                const auto color_konst = konst_color(material, stage.k_color_sel);
                const auto alpha_konst = konst_color(material, stage.k_alpha_sel);
                const auto stage_konst = TevColor{color_konst[0U], color_konst[1U], color_konst[2U], alpha_konst[3U]};

                const auto color_operation = TevOperation{
                    .op = stage.color_op,
                    .bias = stage.color_bias,
                    .scale = stage.color_scale,
                    .clamp = stage.color_clamp != 0U,
                };
                for (auto component = 0U; component < 3U; ++component) {
                    const auto a = color_arg_value(stage.color_in[0U], component, registers, texture, raster, stage_konst);
                    const auto b = color_arg_value(stage.color_in[1U], component, registers, texture, raster, stage_konst);
                    const auto c = color_arg_value(stage.color_in[2U], component, registers, texture, raster, stage_konst);
                    const auto d = color_arg_value(stage.color_in[3U], component, registers, texture, raster, stage_konst);
                    registers[stage.color_out][component] = tev_regular(color_operation, a, b, c, d);
                }

                const auto alpha_operation = TevOperation{
                    .op = stage.alpha_op,
                    .bias = stage.alpha_bias,
                    .scale = stage.alpha_scale,
                    .clamp = stage.alpha_clamp != 0U,
                };
                const auto alpha_a = alpha_arg_value(stage.alpha_in[0U], registers, texture, raster, stage_konst);
                const auto alpha_b = alpha_arg_value(stage.alpha_in[1U], registers, texture, raster, stage_konst);
                const auto alpha_c = alpha_arg_value(stage.alpha_in[2U], registers, texture, raster, stage_konst);
                const auto alpha_d = alpha_arg_value(stage.alpha_in[3U], registers, texture, raster, stage_konst);
                registers[stage.alpha_out][3U] = tev_regular(alpha_operation, alpha_a, alpha_b, alpha_c, alpha_d);
                last_color_register = stage.color_out;
                last_alpha_register = stage.alpha_out;
            }

            output = registers[last_color_register];
            output[3U] = registers[last_alpha_register][3U];
            if (!passes_alpha_compare(material.alpha_compare, output[3U])) {
                return {0, 0, 0, 0};
            }

            return output;
        }

        [[nodiscard]] TevColor evaluate_tev_stages(const J3dMaterialSummary& material, const TevColor& texture, const TevColor& raster) {
            const std::array< TevColor, 1U > textures_by_stage{texture};
            return evaluate_tev_stages(material, textures_by_stage, texture, raster);
        }

        [[nodiscard]] bool material_tev_uses_only_texture_slot(const J3dMaterialSummary& material, std::uint8_t texture_map_slot) {
            if (material.tev_stages.empty()) {
                return true;
            }

            for (const auto& stage : material.tev_stages) {
                const auto* order = find_tev_order(material, stage.stage);
                if (order == nullptr || order->tex_map == 0xffU) {
                    continue;
                }
                if (order->tex_map != texture_map_slot) {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] bool pass_uses_source_uv(const J3dMaterialTexturePass& pass) {
            return !pass.tex_coord_gen.has_value() || pass.tex_coord_gen->source >= GX_TG_TEX0;
        }

    }  // namespace

    std::optional< std::uint8_t > j3d_tex_matrix_slot_from_gx_matrix(std::uint8_t matrix) {
        if (matrix < 30U) {
            return std::nullopt;
        }

        const auto offset = static_cast< std::uint8_t >(matrix - 30U);
        if (offset % 3U != 0U) {
            return std::nullopt;
        }

        return static_cast< std::uint8_t >(offset / 3U);
    }

    std::vector< J3dMaterialTexturePass > j3d_material_texture_passes(const J3dMaterialSummary& material) {
        auto passes = std::vector< J3dMaterialTexturePass >{};
        passes.reserve(material.tev_orders.empty() ? material.textures.size() : material.tev_orders.size());

        if (!material.tev_orders.empty()) {
            for (const auto& order : material.tev_orders) {
                append_pass_for_slots(passes, material, order.stage, order.tex_coord, order.tex_map);
            }
            return passes;
        }

        for (const auto& binding : material.textures) {
            append_pass_for_slots(passes, material, binding.slot, binding.slot, binding.slot);
        }
        return passes;
    }

    std::optional< J3dMaterialTexturePass > j3d_representative_texture_pass(const J3dMaterialSummary& material) {
        auto passes = j3d_material_texture_passes(material);
        if (passes.empty()) {
            return std::nullopt;
        }

        const auto base_pass = std::ranges::find_if(passes, [](const auto& pass) { return pass.tex_map_slot == 0U; });
        if (base_pass != passes.end()) {
            return *base_pass;
        }

        return passes.front();
    }

    std::optional< J3dComposedMaterialTexture > j3d_try_compose_material_texture(const J3dMaterialSummary& material, const DecodedTexture& texture,
                                                                                 std::array< std::uint8_t, 4U > raster_color,
                                                                                 std::uint8_t texture_map_slot) {
        if (texture.width == 0U || texture.height == 0U || texture.rgba.empty()) {
            return std::nullopt;
        }
        if (texture.rgba.size() < static_cast< std::size_t >(texture.width) * texture.height * 4U) {
            return std::nullopt;
        }
        if (!material_tev_uses_only_texture_slot(material, texture_map_slot)) {
            return std::nullopt;
        }

        auto composed = J3dComposedMaterialTexture{
            .image = texture,
            .raster_color_baked = !material.tev_stages.empty(),
        };
        const auto raster = color_to_tev(raster_color);
        for (auto y = 0U; y < texture.height; ++y) {
            for (auto x = 0U; x < texture.width; ++x) {
                const auto offset = (static_cast< std::size_t >(y) * texture.width + x) * 4U;
                const auto sampled = TevColor{
                    texture.rgba[offset],
                    texture.rgba[offset + 1U],
                    texture.rgba[offset + 2U],
                    texture.rgba[offset + 3U],
                };
                const auto output = evaluate_tev_stages(material, sampled, raster);
                composed.image.rgba[offset] = static_cast< std::uint8_t >(std::clamp(output[0U], 0, 255));
                composed.image.rgba[offset + 1U] = static_cast< std::uint8_t >(std::clamp(output[1U], 0, 255));
                composed.image.rgba[offset + 2U] = static_cast< std::uint8_t >(std::clamp(output[2U], 0, 255));
                composed.image.rgba[offset + 3U] = static_cast< std::uint8_t >(std::clamp(output[3U], 0, 255));
            }
        }

        return composed;
    }

    std::optional< J3dComposedMaterialTexture > j3d_try_compose_material_constant(const J3dMaterialSummary& material,
                                                                                  std::array< std::uint8_t, 4U > raster_color) {
        for (const auto& order : material.tev_orders) {
            if (order.tex_map != 0xffU) {
                return std::nullopt;
            }
        }

        const auto raster = color_to_tev(raster_color);
        const auto fallback_texture = TevColor{255, 255, 255, 255};
        const auto output =
            material.tev_stages.empty() ? raster : evaluate_tev_stages(material, std::span< const TevColor >{}, fallback_texture, raster);

        auto composed = J3dComposedMaterialTexture{
            .image =
                DecodedTexture{
                    .width = 1U,
                    .height = 1U,
                    .format = TplTextureFormat::RGBA8,
                    .rgba = std::vector< std::uint8_t >(4U),
                },
            .raster_color_baked = true,
        };
        composed.image.rgba[0U] = static_cast< std::uint8_t >(std::clamp(output[0U], 0, 255));
        composed.image.rgba[1U] = static_cast< std::uint8_t >(std::clamp(output[1U], 0, 255));
        composed.image.rgba[2U] = static_cast< std::uint8_t >(std::clamp(output[2U], 0, 255));
        composed.image.rgba[3U] = static_cast< std::uint8_t >(std::clamp(output[3U], 0, 255));
        return composed;
    }

    std::optional< J3dComposedMaterialTexture > j3d_try_compose_material_texture(const J3dMaterialSummary& material,
                                                                                 std::span< const J3dTexture > textures,
                                                                                 std::span< const J3dMaterialTexturePass > passes,
                                                                                 std::array< std::uint8_t, 4U > raster_color) {
        if (passes.empty() || material.tev_stages.empty()) {
            return std::nullopt;
        }

        auto width = std::uint16_t{1U};
        auto height = std::uint16_t{1U};
        for (const auto& pass : passes) {
            if (!pass_uses_source_uv(pass) || pass.texture_index >= textures.size()) {
                return std::nullopt;
            }
            const auto& texture = textures[pass.texture_index].image;
            if (texture.width == 0U || texture.height == 0U ||
                texture.rgba.size() < static_cast< std::size_t >(texture.width) * texture.height * 4U) {
                return std::nullopt;
            }
            width = std::max(width, texture.width);
            height = std::max(height, texture.height);
        }

        auto composed = J3dComposedMaterialTexture{
            .image =
                DecodedTexture{
                    .width = width,
                    .height = height,
                    .format = TplTextureFormat::RGBA8,
                    .rgba = std::vector< std::uint8_t >(static_cast< std::size_t >(width) * height * 4U),
                },
            .raster_color_baked = true,
        };

        const auto raster = color_to_tev(raster_color);
        for (auto y = 0U; y < height; ++y) {
            for (auto x = 0U; x < width; ++x) {
                const auto source = J3dMeshVertex{
                    .u = (static_cast< float >(x) + 0.5F) / static_cast< float >(width),
                    .v = (static_cast< float >(y) + 0.5F) / static_cast< float >(height),
                    .color = raster_color,
                };
                auto textures_by_stage = std::array< TevColor, 16U >{};
                textures_by_stage.fill({255, 255, 255, 255});
                for (const auto& pass : passes) {
                    const auto& texture = textures[pass.texture_index];
                    const auto* tex_coord_gen = pass.tex_coord_gen.has_value() ? &*pass.tex_coord_gen : nullptr;
                    const auto* tex_matrix = pass.tex_matrix.has_value() ? &*pass.tex_matrix : nullptr;
                    const auto coord = apply_indirect_texture_transform(material, textures, source, pass,
                                                                        j3d_transform_tex_coord(source, tex_coord_gen, tex_matrix, nullptr),
                                                                        texture.image, nullptr);
                    textures_by_stage[std::min< std::size_t >(pass.stage, textures_by_stage.size() - 1U)] =
                        sample_texture_rgba8(texture.image, texture.wrap_s != 0U, texture.wrap_t != 0U, coord.u, coord.v);
                }

                const auto output = evaluate_tev_stages(material, textures_by_stage, textures_by_stage.front(), raster);
                const auto offset = (static_cast< std::size_t >(y) * width + x) * 4U;
                composed.image.rgba[offset] = static_cast< std::uint8_t >(std::clamp(output[0U], 0, 255));
                composed.image.rgba[offset + 1U] = static_cast< std::uint8_t >(std::clamp(output[1U], 0, 255));
                composed.image.rgba[offset + 2U] = static_cast< std::uint8_t >(std::clamp(output[2U], 0, 255));
                composed.image.rgba[offset + 3U] = static_cast< std::uint8_t >(std::clamp(output[3U], 0, 255));
            }
        }

        return composed;
    }

    std::optional< std::array< std::uint8_t, 4U > >
    j3d_evaluate_material_color(const J3dMaterialSummary& material, std::span< const J3dTexture > textures,
                                std::span< const J3dMaterialTexturePass > passes, const J3dMeshVertex& source,
                                std::array< std::uint8_t, 4U > raster_color, const J3dMatrix3x4* model_matrix) {
        if (passes.empty()) {
            const auto raster = color_to_tev(raster_color);
            const auto fallback_texture = TevColor{255, 255, 255, 255};
            const auto output =
                material.tev_stages.empty() ? raster : evaluate_tev_stages(material, std::span< const TevColor >{}, fallback_texture, raster);
            return std::array< std::uint8_t, 4U >{
                static_cast< std::uint8_t >(std::clamp(output[0U], 0, 255)),
                static_cast< std::uint8_t >(std::clamp(output[1U], 0, 255)),
                static_cast< std::uint8_t >(std::clamp(output[2U], 0, 255)),
                static_cast< std::uint8_t >(std::clamp(output[3U], 0, 255)),
            };
        }

        auto textures_by_stage = std::array< TevColor, 16U >{};
        textures_by_stage.fill({255, 255, 255, 255});
        for (const auto& pass : passes) {
            if (pass.texture_index >= textures.size()) {
                return std::nullopt;
            }

            const auto& texture = textures[pass.texture_index];
            const auto* tex_coord_gen = pass.tex_coord_gen.has_value() ? &*pass.tex_coord_gen : nullptr;
            const auto* tex_matrix = pass.tex_matrix.has_value() ? &*pass.tex_matrix : nullptr;
            const auto coord = apply_indirect_texture_transform(material, textures, source, pass,
                                                                j3d_transform_tex_coord(source, tex_coord_gen, tex_matrix, model_matrix),
                                                                texture.image, model_matrix);
            textures_by_stage[std::min< std::size_t >(pass.stage, textures_by_stage.size() - 1U)] =
                sample_texture_rgba8(texture.image, texture.wrap_s != 0U, texture.wrap_t != 0U, coord.u, coord.v);
        }

        const auto raster = color_to_tev(raster_color);
        const auto output = evaluate_tev_stages(material, textures_by_stage, textures_by_stage.front(), raster);
        return std::array< std::uint8_t, 4U >{
            static_cast< std::uint8_t >(std::clamp(output[0U], 0, 255)),
            static_cast< std::uint8_t >(std::clamp(output[1U], 0, 255)),
            static_cast< std::uint8_t >(std::clamp(output[2U], 0, 255)),
            static_cast< std::uint8_t >(std::clamp(output[3U], 0, 255)),
        };
    }

    std::optional< J3dIndirectTextureTrace >
    j3d_trace_indirect_texture_transform(const J3dMaterialSummary& material, std::span< const J3dTexture > textures,
                                         const J3dMeshVertex& source, const J3dMaterialTexturePass& pass,
                                         const J3dMatrix3x4* model_matrix) {
        if (pass.texture_index >= textures.size()) {
            return std::nullopt;
        }

        const auto& texture = textures[pass.texture_index];
        const auto* tex_coord_gen = pass.tex_coord_gen.has_value() ? &*pass.tex_coord_gen : nullptr;
        const auto* tex_matrix = pass.tex_matrix.has_value() ? &*pass.tex_matrix : nullptr;
        return trace_indirect_texture_transform(material, textures, source, pass,
                                                j3d_transform_tex_coord(source, tex_coord_gen, tex_matrix, model_matrix), texture.image,
                                                model_matrix);
    }

    J3dTextureCoordinate j3d_transform_tex_coord(const J3dMeshVertex& source, const J3dTexCoordGenSummary* tex_coord_gen,
                                                 const J3dTexMatrixSummary* tex_matrix, const J3dMatrix3x4* model_matrix) {
        const auto projected = j3d_project_tex_coord(source, tex_coord_gen, tex_matrix, model_matrix);
        if (projected.q != 0.0F) {
            return J3dTextureCoordinate{
                .u = projected.u / projected.q,
                .v = projected.v / projected.q,
            };
        }

        return J3dTextureCoordinate{
            .u = projected.u,
            .v = projected.v,
        };
    }

    J3dTextureProjectionCoordinate j3d_project_tex_coord(const J3dMeshVertex& source, const J3dTexCoordGenSummary* tex_coord_gen,
                                                         const J3dTexMatrixSummary* tex_matrix, const J3dMatrix3x4* model_matrix) {
        const auto coord = tex_gen_input_for_source(source, tex_coord_gen);
        if (tex_matrix == nullptr) {
            return J3dTextureProjectionCoordinate{
                .u = coord.x,
                .v = coord.y,
                .q = 1.0F,
            };
        }

        if (uses_projected_texture_matrix(*tex_matrix)) {
            return project_with_matrix(coord, tex_coord_gen, texture_projection_matrix(*tex_matrix, model_matrix));
        }

        constexpr auto pi = 3.14159265358979323846F;
        const auto angle = static_cast< float >(tex_matrix->rotation) * pi / 32768.0F;
        const auto cos_angle = std::cos(angle);
        const auto sin_angle = std::sin(angle);

        const auto centered_u = coord.x - tex_matrix->center[0U];
        const auto centered_v = coord.y - tex_matrix->center[1U];
        const auto u = tex_matrix->scale_s * (centered_u * cos_angle - centered_v * sin_angle) + tex_matrix->center[0U] + tex_matrix->translate_s;
        const auto v = tex_matrix->scale_t * (centered_u * sin_angle + centered_v * cos_angle) + tex_matrix->center[1U] + tex_matrix->translate_t;
        return J3dTextureProjectionCoordinate{
            .u = u,
            .v = v,
            .q = tex_coord_gen != nullptr && tex_coord_gen->type == GX_TG_MTX3X4 ? coord.z : 1.0F,
        };
    }

}  // namespace smgpc::game
