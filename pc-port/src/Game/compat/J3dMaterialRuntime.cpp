#include "J3dMaterialRuntime.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace smgpc::game {
    namespace {

        constexpr auto GX_TG_MTX3X4 = 0U;
        constexpr auto GX_TG_POS = 0U;

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

        [[nodiscard]] J3dTextureCoordinate transform_with_matrix(const TexGenInput& coord, const J3dTexCoordGenSummary* tex_coord_gen,
                                                                 const J3dMatrix3x4& matrix) {
            const auto u = matrix.m[0U] * coord.x + matrix.m[1U] * coord.y + matrix.m[2U] * coord.z + matrix.m[3U];
            const auto v = matrix.m[4U] * coord.x + matrix.m[5U] * coord.y + matrix.m[6U] * coord.z + matrix.m[7U];
            const auto q = matrix.m[8U] * coord.x + matrix.m[9U] * coord.y + matrix.m[10U] * coord.z + matrix.m[11U];
            if (tex_coord_gen != nullptr && tex_coord_gen->type == GX_TG_MTX3X4 && q != 0.0F) {
                return J3dTextureCoordinate{
                    .u = u / q,
                    .v = v / q,
                };
            }

            return J3dTextureCoordinate{
                .u = u,
                .v = v,
            };
        }

        [[nodiscard]] bool uses_projected_texture_matrix(const J3dTexMatrixSummary& tex_matrix) {
            const auto mode = static_cast< std::uint8_t >(tex_matrix.info & 0x3fU);
            return mode == 8U || mode == 9U || mode == 11U;
        }

        [[nodiscard]] J3dTextureCoordinate transform_projected_tex_coord(const TexGenInput& coord, const J3dTexCoordGenSummary* tex_coord_gen,
                                                                         const J3dTexMatrixSummary& tex_matrix, const J3dMatrix3x4* model_matrix) {
            return transform_with_matrix(coord, tex_coord_gen, texture_projection_matrix(tex_matrix, model_matrix));
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

        [[nodiscard]] TevColor evaluate_tev_stages(const J3dMaterialSummary& material, const TevColor& texture, const TevColor& raster) {
            auto registers = TevRegisters{};
            auto output = TevColor{0, 0, 0, 0};
            if (material.tev_stages.empty()) {
                return texture;
            }

            std::uint8_t last_color_register = 0U;
            std::uint8_t last_alpha_register = 0U;
            for (const auto& stage : material.tev_stages) {
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

    J3dTextureCoordinate j3d_transform_tex_coord(const J3dMeshVertex& source, const J3dTexCoordGenSummary* tex_coord_gen,
                                                 const J3dTexMatrixSummary* tex_matrix, const J3dMatrix3x4* model_matrix) {
        const auto coord = tex_gen_input_for_source(source, tex_coord_gen);
        if (tex_matrix == nullptr) {
            return J3dTextureCoordinate{
                .u = coord.x,
                .v = coord.y,
            };
        }

        if (uses_projected_texture_matrix(*tex_matrix)) {
            return transform_projected_tex_coord(coord, tex_coord_gen, *tex_matrix, model_matrix);
        }

        constexpr auto pi = 3.14159265358979323846F;
        const auto angle = static_cast< float >(tex_matrix->rotation) * pi / 32768.0F;
        const auto cos_angle = std::cos(angle);
        const auto sin_angle = std::sin(angle);

        const auto centered_u = coord.x - tex_matrix->center[0U];
        const auto centered_v = coord.y - tex_matrix->center[1U];
        const auto u = tex_matrix->scale_s * (centered_u * cos_angle - centered_v * sin_angle) + tex_matrix->center[0U] + tex_matrix->translate_s;
        const auto v = tex_matrix->scale_t * (centered_u * sin_angle + centered_v * cos_angle) + tex_matrix->center[1U] + tex_matrix->translate_t;

        if (tex_coord_gen != nullptr && tex_coord_gen->type == GX_TG_MTX3X4 && coord.z != 0.0F) {
            return J3dTextureCoordinate{
                .u = u / coord.z,
                .v = v / coord.z,
            };
        }

        return J3dTextureCoordinate{
            .u = u,
            .v = v,
        };
    }

}  // namespace smgpc::game
