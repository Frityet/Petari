#include "J3dModelRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>

#include "Game/compat/J3dModel.hpp"

namespace smgpc::game {
    namespace {

        constexpr auto GX_TG_POS = std::uint8_t{0U};
        constexpr auto GX_TG_TEX0 = std::uint8_t{4U};

        [[nodiscard]] render::core::TexturedVertex2D vertex(float x, float y, float u, float v, std::array<std::uint8_t, 4U> color) {
            return {
                .x = x,
                .y = y,
                .z = 0.0F,
                .u = u,
                .v = v,
                .color = color,
            };
        }

        [[nodiscard]] std::array<std::uint8_t, 4U> modulate_color(std::array<std::uint8_t, 4U> color, std::array<std::uint8_t, 4U> tint) {
            return {
                static_cast<std::uint8_t>((static_cast<std::uint16_t>(color[0U]) * tint[0U]) / 255U),
                static_cast<std::uint8_t>((static_cast<std::uint16_t>(color[1U]) * tint[1U]) / 255U),
                static_cast<std::uint8_t>((static_cast<std::uint16_t>(color[2U]) * tint[2U]) / 255U),
                static_cast<std::uint8_t>((static_cast<std::uint16_t>(color[3U]) * tint[3U]) / 255U),
            };
        }

        [[nodiscard]] float dot3(std::array<float, 3U> left, std::array<float, 3U> right) {
            return left[0U] * right[0U] + left[1U] * right[1U] + left[2U] * right[2U];
        }

        [[nodiscard]] std::array<float, 3U> cross3(std::array<float, 3U> left, std::array<float, 3U> right) {
            return {
                left[1U] * right[2U] - left[2U] * right[1U],
                left[2U] * right[0U] - left[0U] * right[2U],
                left[0U] * right[1U] - left[1U] * right[0U],
            };
        }

        [[nodiscard]] std::array<float, 3U> normalized_or(std::array<float, 3U> value, std::array<float, 3U> fallback) {
            const auto length = std::sqrt(dot3(value, value));
            if (length <= 0.000001F) {
                return fallback;
            }

            return {value[0U] / length, value[1U] / length, value[2U] / length};
        }

        [[nodiscard]] std::array<float, 3U> camera_space_vector(const CameraPoseCompat &pose, std::array<float, 3U> world_vector) {
            const auto forward = normalized_or({pose.watch.x - pose.eye.x, pose.watch.y - pose.eye.y, pose.watch.z - pose.eye.z},
                                               {0.0F, 0.0F, -1.0F});
            const auto right = normalized_or(cross3(forward, {pose.up.x, pose.up.y, pose.up.z}), {1.0F, 0.0F, 0.0F});
            const auto up = normalized_or(cross3(right, forward), {0.0F, 1.0F, 0.0F});
            return {dot3(world_vector, right), dot3(world_vector, up), dot3(world_vector, forward)};
        }

        [[nodiscard]] std::array<float, 3U> transform_normal_to_camera(const J3dMatrix3x4 &matrix, const CameraPoseCompat &pose,
                                                                       std::array<float, 3U> normal) {
            const auto world_normal = normalized_or(
                {
                    matrix.m[0U] * normal[0U] + matrix.m[1U] * normal[1U] + matrix.m[2U] * normal[2U],
                    matrix.m[4U] * normal[0U] + matrix.m[5U] * normal[1U] + matrix.m[6U] * normal[2U],
                    matrix.m[8U] * normal[0U] + matrix.m[9U] * normal[1U] + matrix.m[10U] * normal[2U],
                },
                {0.0F, 0.0F, 1.0F});
            return normalized_or(camera_space_vector(pose, world_normal), {0.0F, 0.0F, 1.0F});
        }

        [[nodiscard]] std::uint8_t first_raster_channel_selector(const GXMaterialState &state, std::span<const J3dMaterialTexturePass> passes) {
            for (const auto &pass : passes) {
                if (auto it = std::ranges::find_if(state.tev_orders, [&pass](const auto &order) { return order.stage == pass.stage; });
                    it != state.tev_orders.end()) {
                    return it->color_channel;
                }
            }

            return 4U;
        }

        [[nodiscard]] std::array<std::uint8_t, 4U> gx_raster_color(const J3dMeshVertex &source, const GXMaterialState &state,
                                                                   std::uint8_t color_channel, std::array<float, 3U> position,
                                                                   std::array<float, 3U> normal) {
            return gx_evaluate_lit_raster_color(state, color_channel, source.color, position, normal);
        }

        [[nodiscard]] std::array<std::uint8_t, 4U> gx_raster_color(const J3dMeshVertex &source, const GXMaterialState &state,
                                                                   std::span<const J3dMaterialTexturePass> passes,
                                                                   std::array<float, 3U> position, std::array<float, 3U> normal) {
            return gx_raster_color(source, state, first_raster_channel_selector(state, passes), position, normal);
        }

        [[nodiscard]] bool texture_needs_blending(const DecodedTexture &texture) {
            for (auto offset = std::size_t{3U}; offset < texture.rgba.size(); offset += 4U) {
                if (texture.rgba[offset] != 255U) {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] const J3dTevStageSummary *find_tev_stage(const J3dMaterialSummary &material, std::uint8_t stage_index) {
            const auto it = std::ranges::find_if(material.tev_stages, [stage_index](const auto &stage) { return stage.stage == stage_index; });
            return it == material.tev_stages.end() ? nullptr : &*it;
        }

        [[nodiscard]] bool tev_stage_accumulates_color(const J3dTevStageSummary *stage) {
            if (stage == nullptr || stage->color_op != 0U || stage->color_bias != 0U || stage->color_out != 0U) {
                return false;
            }

            constexpr auto gx_cc_cprev = 0U;
            constexpr auto gx_cc_konst = 14U;
            return stage->color_in[3U] == gx_cc_cprev || stage->color_in[3U] == gx_cc_konst;
        }

        [[nodiscard]] render::BlendMode blend_mode_for_material_pass(const J3dMaterialSummary &material, bool texture_has_alpha,
                                                                     const J3dTevStageSummary *stage, bool is_overlay_pass, bool use_gx_blend_mode) {
            if (use_gx_blend_mode && material.blend.enabled) {
                constexpr auto gx_bm_none = std::uint8_t{0U};
                constexpr auto gx_bm_blend = std::uint8_t{1U};
                constexpr auto gx_bl_one = std::uint8_t{1U};
                if (material.blend.type == gx_bm_none) {
                    return render::BlendMode::Opaque;
                }
                if (material.blend.type == gx_bm_blend) {
                    return material.blend.dst_factor == gx_bl_one ? render::BlendMode::Additive : render::BlendMode::Alpha;
                }
            }

            if (material.blend.enabled && material.blend.type == 1U) {
                return render::BlendMode::Additive;
            }

            if (is_overlay_pass || tev_stage_accumulates_color(stage)) {
                return render::BlendMode::Additive;
            }

            if (!texture_has_alpha && !is_overlay_pass) {
                return render::BlendMode::Opaque;
            }

            return render::BlendMode::Alpha;
        }

        [[nodiscard]] render::BlendMode blend_mode_for_composed_material(const J3dMaterialSummary &material, bool texture_has_alpha) {
            if (material.blend.enabled) {
                constexpr auto gx_bm_blend = std::uint8_t{1U};
                constexpr auto gx_bl_one = std::uint8_t{1U};
                if (material.blend.type == gx_bm_blend) {
                    return material.blend.dst_factor == gx_bl_one ? render::BlendMode::Additive : render::BlendMode::Alpha;
                }
            }

            return texture_has_alpha ? render::BlendMode::Alpha : render::BlendMode::Opaque;
        }

        [[nodiscard]] render::DepthCompare depth_compare_from_gx(std::uint8_t function) {
            switch (function) {
            case 0U:
                return render::DepthCompare::Never;
            case 1U:
                return render::DepthCompare::Less;
            case 2U:
                return render::DepthCompare::Equal;
            case 3U:
                return render::DepthCompare::LessEqual;
            case 4U:
                return render::DepthCompare::Greater;
            case 5U:
                return render::DepthCompare::NotEqual;
            case 6U:
                return render::DepthCompare::GreaterEqual;
            case 7U:
                return render::DepthCompare::Always;
            default:
                return render::DepthCompare::LessEqual;
            }
        }

        [[nodiscard]] render::CullMode cull_mode_from_public_gx(std::uint8_t mode) {
            switch (mode) {
            case 0U:
                return render::CullMode::None;
            case 1U:
                return render::CullMode::Front;
            case 2U:
                return render::CullMode::Back;
            case 3U:
                return render::CullMode::FrontAndBack;
            default:
                return render::CullMode::None;
            }
        }

        [[nodiscard]] render::CullMode cull_mode_from_gx_material_state(const J3dMaterialSummary &material) {
            const auto mode = material.gx_state.mdl3_display_list.empty() ? material.cull_mode : material.gx_state.cull_mode;
            if (material.gx_state.mdl3_display_list.empty()) {
                return cull_mode_from_public_gx(mode);
            }

            switch (mode) {
            case 0U:
                return render::CullMode::None;
            case 1U:
                return render::CullMode::Back;
            case 2U:
                return render::CullMode::Front;
            case 3U:
                return render::CullMode::FrontAndBack;
            default:
                return render::CullMode::None;
            }
        }

        [[nodiscard]] bool token_matches_material(std::string_view token, const std::string &material_name, std::uint16_t material_index) {
            while (!token.empty() && token.front() == ' ') {
                token.remove_prefix(1U);
            }
            while (!token.empty() && token.back() == ' ') {
                token.remove_suffix(1U);
            }
            if (token.empty()) {
                return false;
            }

            if (token == material_name) {
                return true;
            }

            const auto index_text = std::to_string(material_index);
            return token == index_text;
        }

        [[nodiscard]] bool material_filter_allows_mesh(std::string_view filter, const std::string &material_name, std::uint16_t material_index) {
            if (filter.empty()) {
                return true;
            }

            while (!filter.empty()) {
                const auto comma = filter.find(',');
                const auto token = comma == std::string_view::npos ? filter : filter.substr(0U, comma);
                if (token_matches_material(token, material_name, material_index)) {
                    return true;
                }
                if (comma == std::string_view::npos) {
                    break;
                }
                filter.remove_prefix(comma + 1U);
            }

            return false;
        }

        [[nodiscard]] float normalized_animation_frame(float frame, std::int16_t frame_max) {
            if (frame_max <= 0) {
                return frame;
            }

            auto wrapped = std::fmod(frame, static_cast<float>(frame_max));
            if (wrapped < 0.0F) {
                wrapped += static_cast<float>(frame_max);
            }
            return wrapped;
        }

        [[nodiscard]] bool material_pass_textures_available(std::span<const J3dMaterialTexturePass> passes,
                                                            std::span<const J3dTexture> textures) {
            return std::ranges::all_of(passes, [textures](const auto &pass) { return pass.texture_index < textures.size(); });
        }

        [[nodiscard]] bool pass_uses_supported_shader_texgen(const J3dMaterialTexturePass &pass) {
            return !pass.tex_coord_gen.has_value() || pass.tex_coord_gen->source == GX_TG_POS || pass.tex_coord_gen->source >= GX_TG_TEX0;
        }

        [[nodiscard]] const GXTextureBindingState *find_texture_binding(const GXMaterialState &state, std::uint8_t slot) {
            const auto it = std::ranges::find_if(state.textures, [slot](const auto &binding) { return binding.slot == slot; });
            return it == state.textures.end() ? nullptr : &*it;
        }

        [[nodiscard]] const GXTexCoordGenState *find_tex_coord_gen(const GXMaterialState &state, std::uint8_t slot) {
            const auto it = std::ranges::find_if(state.tex_coord_gens, [slot](const auto &gen) { return gen.slot == slot; });
            return it == state.tex_coord_gens.end() ? nullptr : &*it;
        }

        [[nodiscard]] const GXTexMatrixState *find_tex_matrix(const GXMaterialState &state, const GXTexCoordGenState *gen) {
            if (gen == nullptr) {
                return nullptr;
            }

            const auto matrix_slot = j3d_tex_matrix_slot_from_gx_matrix(gen->matrix);
            if (!matrix_slot.has_value()) {
                return nullptr;
            }

            const auto it = std::ranges::find_if(state.tex_matrices, [slot = *matrix_slot](const auto &matrix) { return matrix.slot == slot; });
            return it == state.tex_matrices.end() ? nullptr : &*it;
        }

        [[nodiscard]] J3dTexCoordGenSummary j3d_tex_coord_gen_from_gx(const GXTexCoordGenState &gen) {
            return J3dTexCoordGenSummary{
                .slot = gen.slot,
                .type = gen.type,
                .source = gen.source,
                .matrix = gen.matrix,
            };
        }

        [[nodiscard]] J3dTexMatrixSummary j3d_tex_matrix_from_gx(const GXTexMatrixState &matrix) {
            return J3dTexMatrixSummary{
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
            };
        }

        [[nodiscard]] std::vector<J3dMaterialTexturePass> gx_material_texture_passes(const GXMaterialState &state) {
            auto passes = std::vector<J3dMaterialTexturePass>{};
            passes.reserve(state.tev_orders.size());
            for (const auto &order : state.tev_orders) {
                if (state.tev_stage_count != 0U && order.stage >= state.tev_stage_count) {
                    continue;
                }
                if (order.tex_map == 0xffU) {
                    continue;
                }

                const auto *binding = find_texture_binding(state, order.tex_map);
                if (binding == nullptr || binding->texture_index == 0xffffU) {
                    continue;
                }

                const auto *gen = order.tex_coord == 0xffU ? nullptr : find_tex_coord_gen(state, order.tex_coord);
                const auto *matrix = find_tex_matrix(state, gen);
                passes.push_back(J3dMaterialTexturePass{
                    .stage = order.stage,
                    .tex_coord_slot = order.tex_coord,
                    .tex_map_slot = order.tex_map,
                    .texture_index = binding->texture_index,
                    .tex_coord_gen = gen == nullptr ? std::optional<J3dTexCoordGenSummary>{} : j3d_tex_coord_gen_from_gx(*gen),
                    .tex_matrix = matrix == nullptr ? std::optional<J3dTexMatrixSummary>{} : j3d_tex_matrix_from_gx(*matrix),
                });
            }

            return passes;
        }

        [[nodiscard]] bool is_active_tev_stage(const GXMaterialState &state, const GXTevStageState &stage) {
            return state.tev_stage_count == 0U || stage.stage < state.tev_stage_count;
        }

        [[nodiscard]] std::size_t active_tev_stage_count(const GXMaterialState &state) {
            return static_cast<std::size_t>(
                std::ranges::count_if(state.tev_stages, [&state](const auto &stage) { return is_active_tev_stage(state, stage); }));
        }

        [[nodiscard]] std::size_t active_indirect_tev_stage_count(const GXMaterialState &state) {
            return static_cast<std::size_t>(std::ranges::count_if(state.indirect.tev_stages, [](const auto &stage) { return stage.active; }));
        }

        [[nodiscard]] bool has_active_indirect_tev_stage(const GXMaterialState &state) {
            return std::ranges::any_of(state.indirect.tev_stages, [&state](const auto &stage) {
                return stage.active && stage.ind_stage < state.indirect.stage_count &&
                       (state.tev_stage_count == 0U || stage.tev_stage < state.tev_stage_count);
            });
        }

        [[nodiscard]] bool material_uses_gx_lighting(const GXMaterialState &state) {
            return std::ranges::any_of(state.color_channels, [](const auto &channel) {
                return channel.color_control.lighting_enabled || channel.alpha_control.lighting_enabled;
            });
        }

        [[nodiscard]] std::array<std::uint8_t, 4U> konst_color_for_selector(const GXMaterialState &state, std::uint8_t selector) {
            constexpr auto constants = std::array<std::uint8_t, 8U>{255U, 223U, 191U, 159U, 128U, 96U, 64U, 32U};
            if (selector < constants.size()) {
                return {constants[selector], constants[selector], constants[selector], constants[selector]};
            }
            if (selector >= 12U && selector <= 15U) {
                return state.tev_k_colors[selector - 12U];
            }
            if (selector >= 16U && selector <= 31U) {
                const auto color_index = static_cast<std::size_t>((selector - 16U) % 4U);
                const auto component = static_cast<std::size_t>((selector - 16U) / 4U);
                const auto value = state.tev_k_colors[color_index][component];
                return {value, value, value, value};
            }

            return {0U, 0U, 0U, 0U};
        }

        [[nodiscard]] std::array<std::uint8_t, 4U> stage_konst_color(const GXMaterialState &state, const GXTevStageState &stage) {
            const auto color = konst_color_for_selector(state, stage.k_color_sel);
            const auto alpha = konst_color_for_selector(state, stage.k_alpha_sel);
            return {color[0U], color[1U], color[2U], alpha[3U]};
        }

        [[nodiscard]] bool tev_stage_can_use_shader(const GXTevStageState &stage) {
            if (stage.color_out > 3U || stage.alpha_out > 3U || stage.color_op > 1U || stage.alpha_op > 1U || stage.color_bias > 3U ||
                stage.alpha_bias > 3U || stage.color_scale > 3U || stage.alpha_scale > 3U) {
                return false;
            }

            if (!std::ranges::all_of(stage.color_in, [](auto arg) { return arg <= 15U; })) {
                return false;
            }
            if (!std::ranges::all_of(stage.alpha_in, [](auto arg) { return arg <= 7U; })) {
                return false;
            }

            return true;
        }

        [[nodiscard]] bool tev_stage_uses_texture(const GXTevStageState &stage) {
            constexpr auto gx_cc_texc = std::uint8_t{8U};
            constexpr auto gx_cc_texa = std::uint8_t{9U};
            constexpr auto gx_ca_texa = std::uint8_t{4U};
            return std::ranges::any_of(stage.color_in, [=](auto arg) { return arg == gx_cc_texc || arg == gx_cc_texa; }) ||
                   std::ranges::any_of(stage.alpha_in, [=](auto arg) { return arg == gx_ca_texa; });
        }

        [[nodiscard]] std::uint8_t texture_stage_index_for_tev_stage(std::span<const J3dMaterialTexturePass> passes, std::uint8_t stage) {
            for (auto i = std::size_t{}; i < passes.size(); ++i) {
                if (passes[i].stage == stage) {
                    return static_cast<std::uint8_t>(i);
                }
            }

            return 0xffU;
        }

        [[nodiscard]] render::GxTevStage2D gx_tev_stage_from_state(const GXMaterialState &state, const GXTevStageState &stage,
                                                                   std::span<const J3dMaterialTexturePass> passes) {
            return render::GxTevStage2D{
                .texture_stage = texture_stage_index_for_tev_stage(passes, stage.stage),
                .color_in = stage.color_in,
                .color_op = stage.color_op,
                .color_bias = stage.color_bias,
                .color_scale = stage.color_scale,
                .color_clamp = stage.color_clamp != 0U,
                .color_out = stage.color_out,
                .alpha_in = stage.alpha_in,
                .alpha_op = stage.alpha_op,
                .alpha_bias = stage.alpha_bias,
                .alpha_scale = stage.alpha_scale,
                .alpha_clamp = stage.alpha_clamp != 0U,
                .alpha_out = stage.alpha_out,
                .konst_color = stage_konst_color(state, stage),
            };
        }

        [[nodiscard]] render::GxAlphaCompare2D gx_alpha_compare_from_material_state(const GXAlphaCompareState &alpha_compare) {
            return render::GxAlphaCompare2D{
                .comp0 = alpha_compare.comp0,
                .ref0 = alpha_compare.ref0,
                .op = alpha_compare.op,
                .comp1 = alpha_compare.comp1,
                .ref1 = alpha_compare.ref1,
                .enabled = alpha_compare.enabled,
            };
        }

        [[nodiscard]] render::GxBlendMode2D gx_blend_from_material_state(const GXBlendState &blend) {
            return render::GxBlendMode2D{
                .type = blend.type,
                .src_factor = blend.src_factor,
                .dst_factor = blend.dst_factor,
                .op = blend.op,
                .enabled = blend.enabled,
            };
        }

        [[nodiscard]] render::GxFog2D gx_fog_from_material_state(const GXFogState &fog) {
            return render::GxFog2D{
                .enabled = fog.enabled,
                .type = fog.type,
                .projection = fog.projection,
                .range_adjust_enabled = fog.range_adjust_enabled,
                .color = fog.color,
                .a = fog.a,
                .c = fog.c,
                .b_magnitude = fog.b_magnitude,
                .b_shift = fog.b_shift,
            };
        }

        [[nodiscard]] render::GxFog2D gx_fog_for_mesh(const std::optional<J3dMaterialSummary> &material) {
            return material.has_value() ? gx_fog_from_material_state(material->gx_state.fog) : render::GxFog2D{};
        }

        [[nodiscard]] bool material_can_use_shader_gx_tev(const GXMaterialState &state, std::span<const J3dMaterialTexturePass> passes) {
            const auto stage_count = active_tev_stage_count(state);
            if (passes.empty() || passes.size() > render::core::kMaxGxMaterialTextureStages2D || stage_count == 0U ||
                stage_count > render::core::kMaxGxMaterialTevStages2D) {
                return false;
            }
            if (has_active_indirect_tev_stage(state)) {
                return false;
            }
            if (!std::ranges::all_of(passes, pass_uses_supported_shader_texgen)) {
                return false;
            }

            for (const auto &stage : state.tev_stages) {
                if (!is_active_tev_stage(state, stage)) {
                    continue;
                }
                if (!tev_stage_can_use_shader(stage)) {
                    return false;
                }

                const auto pass = std::ranges::find_if(passes, [&stage](const auto &candidate) { return candidate.stage == stage.stage; });
                if (pass == passes.end() && tev_stage_uses_texture(stage)) {
                    return false;
                }
            }

            return true;
        }

        struct ProjectedVertex {
            float x = 0.0F;
            float y = 0.0F;
            float z = 0.0F;
            float u = 0.0F;
            float v = 0.0F;
            float clip_w = 1.0F;
            std::array<std::array<float, 3U>, render::core::kMaxGxMaterialTextureStages2D> tex_coords{};
            std::array<float, 4U> color{255.0F, 255.0F, 255.0F, 255.0F};
        };

        struct MatrixPaletteContext {
            const J3dMatrix3x4 &actor_matrix;
            std::span<const J3dJointTransformValue> transforms;
            std::span<const std::uint16_t> parent_indices;
            std::span<const J3dDrawMatrixSummary> draw_matrices;
            const J3dEnvelopeBlockSummary *envelopes = nullptr;
            const std::optional<J3dBckAnimationSummary> &animation;
            const J3dJointTransformValue *fallback_transform = nullptr;
            std::uint16_t default_joint_index = 0xffffU;
            std::uint64_t frame = 0U;
        };

        [[nodiscard]] J3dJointTransformValue joint_transform_from_summary(const J3dJointSummary &joint) {
            return J3dJointTransformValue{
                .scale = joint.scale,
                .rotation = joint.rotation,
                .translation = joint.translation,
            };
        }

        [[nodiscard]] J3dMatrix3x4 matrix_from_joint_transform(const J3dJointTransformValue *transform) {
            if (transform == nullptr) {
                return J3dMatrix3x4{};
            }

            constexpr auto pi = 3.14159265358979323846F;
            const auto rx = static_cast<float>(transform->rotation[0U]) * pi / 32768.0F;
            const auto ry = static_cast<float>(transform->rotation[1U]) * pi / 32768.0F;
            const auto rz = static_cast<float>(transform->rotation[2U]) * pi / 32768.0F;
            const auto sx = std::sin(rx);
            const auto cx = std::cos(rx);
            const auto sy = std::sin(ry);
            const auto cy = std::cos(ry);
            const auto sz = std::sin(rz);
            const auto cz = std::cos(rz);

            const auto m00 = cz * cy;
            const auto m10 = sz * cy;
            const auto m20 = -sy;
            const auto m21 = cy * sx;
            const auto m22 = cy * cx;
            const auto cxsz = cx * sz;
            const auto sxcz = sx * cz;
            const auto m01 = sxcz * sy - cxsz;
            const auto m12 = cxsz * sy - sxcz;
            const auto sxsz = sx * sz;
            const auto cxcz = cx * cz;
            const auto m02 = cxcz * sy + sxsz;
            const auto m11 = sxsz * sy + cxcz;

            return J3dMatrix3x4{
                .m =
                    {
                        m00 * transform->scale[0U],
                        m01 * transform->scale[1U],
                        m02 * transform->scale[2U],
                        transform->translation[0U],
                        m10 * transform->scale[0U],
                        m11 * transform->scale[1U],
                        m12 * transform->scale[2U],
                        transform->translation[1U],
                        m20 * transform->scale[0U],
                        m21 * transform->scale[1U],
                        m22 * transform->scale[2U],
                        transform->translation[2U],
                    },
            };
        }

        [[nodiscard]] J3dMatrix3x4 concat_matrix(const J3dMatrix3x4 &left, const J3dMatrix3x4 &right) {
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

        [[nodiscard]] J3dMatrix3x4 joint_model_matrix(std::span<const J3dJointTransformValue> transforms,
                                                      std::span<const std::uint16_t> parent_indices,
                                                      const std::optional<J3dBckAnimationSummary> &animation, std::uint16_t joint_index,
                                                      std::uint64_t frame) {
            if (joint_index >= transforms.size()) {
                return J3dMatrix3x4{};
            }

            auto chain = std::vector<std::uint16_t>{};
            auto current = joint_index;
            for (auto guard = 0U; current < transforms.size() && guard < transforms.size(); ++guard) {
                chain.push_back(current);
                if (current >= parent_indices.size() || parent_indices[current] == 0xffffU) {
                    break;
                }
                current = parent_indices[current];
            }

            auto matrix = J3dMatrix3x4{};
            for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
                auto transform = transforms[*it];
                if (animation.has_value()) {
                    if (const auto animated = j3d_evaluate_bck_joint_transform(*animation, *it, static_cast<float>(frame)); animated.has_value()) {
                        transform = *animated;
                    }
                }

                matrix = concat_matrix(matrix, matrix_from_joint_transform(&transform));
            }

            return matrix;
        }

        [[nodiscard]] J3dMatrix3x4 matrix_from_array(std::array<float, 12U> values) {
            return J3dMatrix3x4{.m = values};
        }

        [[nodiscard]] J3dMatrix3x4 weighted_envelope_matrix(std::span<const J3dJointTransformValue> transforms,
                                                            std::span<const std::uint16_t> parent_indices,
                                                            const std::optional<J3dBckAnimationSummary> &animation,
                                                            const J3dEnvelopeBlockSummary *envelopes, std::uint16_t envelope_index,
                                                            std::uint64_t frame) {
            if (envelopes == nullptr || envelope_index >= envelopes->matrices.size()) {
                return J3dMatrix3x4{};
            }

            auto result = J3dMatrix3x4{};
            result.m.fill(0.0F);
            const auto &envelope = envelopes->matrices[envelope_index];
            auto total_weight = 0.0F;
            for (auto influence = std::size_t{}; influence < envelope.joint_indices.size() && influence < envelope.weights.size(); ++influence) {
                const auto joint_index = envelope.joint_indices[influence];
                if (joint_index >= transforms.size()) {
                    continue;
                }

                auto matrix = joint_model_matrix(transforms, parent_indices, animation, joint_index, frame);
                if (joint_index < envelopes->inverse_bind_matrices.size()) {
                    matrix = concat_matrix(matrix, matrix_from_array(envelopes->inverse_bind_matrices[joint_index]));
                }

                const auto weight = envelope.weights[influence];
                total_weight += weight;
                for (auto value = 0U; value < result.m.size(); ++value) {
                    result.m[value] += matrix.m[value] * weight;
                }
            }

            if (total_weight > 0.0F && std::abs(total_weight - 1.0F) > 0.001F) {
                for (auto &value : result.m) {
                    value /= total_weight;
                }
            }

            return result;
        }

        [[nodiscard]] J3dMatrix3x4 local_matrix_for_draw_matrix(const MatrixPaletteContext &context, std::uint16_t draw_matrix_index) {
            if (draw_matrix_index != 0xffffU && draw_matrix_index < context.draw_matrices.size()) {
                const auto &draw_matrix = context.draw_matrices[draw_matrix_index];
                if (draw_matrix.weighted) {
                    return weighted_envelope_matrix(context.transforms, context.parent_indices, context.animation, context.envelopes,
                                                    draw_matrix.index, context.frame);
                }
                if (draw_matrix.index < context.transforms.size()) {
                    return joint_model_matrix(context.transforms, context.parent_indices, context.animation, draw_matrix.index, context.frame);
                }
            }

            if (context.default_joint_index < context.transforms.size()) {
                return joint_model_matrix(context.transforms, context.parent_indices, context.animation, context.default_joint_index, context.frame);
            }

            return matrix_from_joint_transform(context.fallback_transform);
        }

        [[nodiscard]] J3dMatrix3x4 model_matrix_for_source_vertex(const J3dMeshVertex &source, const MatrixPaletteContext &context) {
            return concat_matrix(context.actor_matrix, local_matrix_for_draw_matrix(context, source.draw_matrix_index));
        }

        [[nodiscard]] std::array<float, 3U> transform_point(const J3dMatrix3x4 &matrix, std::array<float, 3U> position) {
            return {
                matrix.m[0U] * position[0U] + matrix.m[1U] * position[1U] + matrix.m[2U] * position[2U] + matrix.m[3U],
                matrix.m[4U] * position[0U] + matrix.m[5U] * position[1U] + matrix.m[6U] * position[2U] + matrix.m[7U],
                matrix.m[8U] * position[0U] + matrix.m[9U] * position[1U] + matrix.m[10U] * position[2U] + matrix.m[11U],
            };
        }

        [[nodiscard]] std::array<float, 4U> color_to_float(std::array<std::uint8_t, 4U> color) {
            return {
                static_cast<float>(color[0U]),
                static_cast<float>(color[1U]),
                static_cast<float>(color[2U]),
                static_cast<float>(color[3U]),
            };
        }

        [[nodiscard]] std::array<std::uint8_t, 4U> color_to_u8(std::array<float, 4U> color) {
            return {
                static_cast<std::uint8_t>(std::clamp(color[0U], 0.0F, 255.0F)),
                static_cast<std::uint8_t>(std::clamp(color[1U], 0.0F, 255.0F)),
                static_cast<std::uint8_t>(std::clamp(color[2U], 0.0F, 255.0F)),
                static_cast<std::uint8_t>(std::clamp(color[3U], 0.0F, 255.0F)),
            };
        }

        [[nodiscard]] ProjectedVertex interpolate_vertex(const ProjectedVertex &a, const ProjectedVertex &b, float t) {
            const auto mix = [t](float left, float right) { return left + (right - left) * t; };

            auto color = std::array<float, 4U>{};
            for (auto i = 0U; i < color.size(); ++i) {
                color[i] = mix(a.color[i], b.color[i]);
            }

            auto tex_coords = std::array<std::array<float, 3U>, render::core::kMaxGxMaterialTextureStages2D>{};
            for (auto i = 0U; i < tex_coords.size(); ++i) {
                tex_coords[i][0U] = mix(a.tex_coords[i][0U], b.tex_coords[i][0U]);
                tex_coords[i][1U] = mix(a.tex_coords[i][1U], b.tex_coords[i][1U]);
                tex_coords[i][2U] = mix(a.tex_coords[i][2U], b.tex_coords[i][2U]);
            }

            return ProjectedVertex{
                .x = mix(a.x, b.x),
                .y = mix(a.y, b.y),
                .z = mix(a.z, b.z),
                .u = mix(a.u, b.u),
                .v = mix(a.v, b.v),
                .clip_w = mix(a.clip_w, b.clip_w),
                .tex_coords = tex_coords,
                .color = color,
            };
        }

        [[nodiscard]] std::vector<ProjectedVertex> clip_to_near_plane(std::span<const ProjectedVertex> polygon, float near_plane) {
            auto clipped = std::vector<ProjectedVertex>{};
            if (polygon.empty()) {
                return clipped;
            }

            auto previous = polygon.back();
            auto previous_inside = previous.z >= near_plane;
            for (const auto &current : polygon) {
                const auto current_inside = current.z >= near_plane;
                if (current_inside != previous_inside) {
                    const auto t = (near_plane - previous.z) / (current.z - previous.z);
                    clipped.push_back(interpolate_vertex(previous, current, t));
                }
                if (current_inside) {
                    clipped.push_back(current);
                }
                previous = current;
                previous_inside = current_inside;
            }

            return clipped;
        }

        [[nodiscard]] ProjectedVertex view_vertex(const J3dMeshVertex &source, std::array<std::uint8_t, 4U> material_color,
                                                  const MatrixPaletteContext &matrix_context, const CameraPoseCompat &camera_pose,
                                                  const J3dTexCoordGenSummary *tex_coord_gen, const J3dTexMatrixSummary *tex_matrix) {
            const auto model_matrix = model_matrix_for_source_vertex(source, matrix_context);
            const auto world = transform_point(model_matrix, {source.x, source.y, source.z});
            const auto camera = transform_world_to_camera(camera_pose, {world[0U], world[1U], world[2U]});
            const auto tex_coord = j3d_transform_tex_coord(source, tex_coord_gen, tex_matrix, &model_matrix);
            auto tex_coords = std::array<std::array<float, 3U>, render::core::kMaxGxMaterialTextureStages2D>{};
            tex_coords[0U] = {tex_coord.u, tex_coord.v, 1.0F};

            return ProjectedVertex{
                .x = camera.x,
                .y = camera.y,
                .z = camera.z,
                .u = tex_coord.u,
                .v = tex_coord.v,
                .clip_w = camera.z,
                .tex_coords = tex_coords,
                .color = color_to_float(modulate_color(source.color, material_color)),
            };
        }

        [[nodiscard]] J3dMeshVertex interpolate_source_vertex(const J3dMeshVertex &a, const J3dMeshVertex &b, const J3dMeshVertex &c, float weight_a,
                                                              float weight_b, float weight_c) {
            const auto mix = [weight_a, weight_b, weight_c](float av, float bv, float cv) { return av * weight_a + bv * weight_b + cv * weight_c; };

            auto color = std::array<std::uint8_t, 4U>{};
            for (auto i = 0U; i < color.size(); ++i) {
                const auto value = static_cast<float>(a.color[i]) * weight_a + static_cast<float>(b.color[i]) * weight_b +
                                   static_cast<float>(c.color[i]) * weight_c;
                color[i] = static_cast<std::uint8_t>(std::clamp(std::round(value), 0.0F, 255.0F));
            }
            auto tex_coords = std::array<std::array<float, 2U>, 8U>{};
            for (auto slot = std::size_t{}; slot < tex_coords.size(); ++slot) {
                tex_coords[slot] = {
                    mix(a.tex_coords[slot][0U], b.tex_coords[slot][0U], c.tex_coords[slot][0U]),
                    mix(a.tex_coords[slot][1U], b.tex_coords[slot][1U], c.tex_coords[slot][1U]),
                };
            }
            const auto tex_coord_count = std::max({a.tex_coord_count, b.tex_coord_count, c.tex_coord_count});

            return J3dMeshVertex{
                .x = mix(a.x, b.x, c.x),
                .y = mix(a.y, b.y, c.y),
                .z = mix(a.z, b.z, c.z),
                .normal =
                    {
                        mix(a.normal[0U], b.normal[0U], c.normal[0U]),
                        mix(a.normal[1U], b.normal[1U], c.normal[1U]),
                        mix(a.normal[2U], b.normal[2U], c.normal[2U]),
                    },
                .u = tex_coords[0U][0U],
                .v = tex_coords[0U][1U],
                .tex_coords = tex_coords,
                .tex_coord_count = tex_coord_count,
                .color = color,
                .position_matrix_slot = a.position_matrix_slot == b.position_matrix_slot && a.position_matrix_slot == c.position_matrix_slot ?
                                            a.position_matrix_slot :
                                            static_cast<std::uint8_t>(0xffU),
                .draw_matrix_index = a.draw_matrix_index == b.draw_matrix_index && a.draw_matrix_index == c.draw_matrix_index ?
                                         a.draw_matrix_index :
                                         static_cast<std::uint16_t>(0xffffU),
            };
        }

        [[nodiscard]] ProjectedVertex material_view_vertex(const J3dMeshVertex &source, const J3dMaterialSummary &material,
                                                           std::span<const J3dTexture> textures, std::span<const J3dMaterialTexturePass> passes,
                                                           const MatrixPaletteContext &matrix_context, const CameraPoseCompat &camera_pose) {
            const auto model_matrix = model_matrix_for_source_vertex(source, matrix_context);
            const auto world = transform_point(model_matrix, {source.x, source.y, source.z});
            const auto camera = transform_world_to_camera(camera_pose, {world[0U], world[1U], world[2U]});
            const auto raster_color = gx_raster_color(source, material.gx_state, passes, {camera.x, camera.y, camera.z},
                                                      transform_normal_to_camera(model_matrix, camera_pose, source.normal));
            const auto evaluated = j3d_evaluate_material_color(material, textures, passes, source, raster_color, &model_matrix)
                                       .value_or(std::array<std::uint8_t, 4U>{0U, 0U, 0U, 0U});

            return ProjectedVertex{
                .x = camera.x,
                .y = camera.y,
                .z = camera.z,
                .u = 0.5F,
                .v = 0.5F,
                .clip_w = camera.z,
                .tex_coords = {{{0.5F, 0.5F, 1.0F}, {0.5F, 0.5F, 1.0F}, {0.5F, 0.5F, 1.0F}}},
                .color = color_to_float(evaluated),
            };
        }

        [[nodiscard]] ProjectedVertex gx_material_view_vertex(const J3dMeshVertex &source, const J3dMaterialSummary &material,
                                                              std::span<const J3dMaterialTexturePass> passes, const MatrixPaletteContext &matrix_context,
                                                              const CameraPoseCompat &camera_pose) {
            const auto model_matrix = model_matrix_for_source_vertex(source, matrix_context);
            const auto world = transform_point(model_matrix, {source.x, source.y, source.z});
            const auto camera = transform_world_to_camera(camera_pose, {world[0U], world[1U], world[2U]});
            auto tex_coords = std::array<std::array<float, 3U>, render::core::kMaxGxMaterialTextureStages2D>{};
            for (auto i = std::size_t{}; i < passes.size() && i < tex_coords.size(); ++i) {
                const auto &pass = passes[i];
                const auto *tex_coord_gen = pass.tex_coord_gen.has_value() ? &*pass.tex_coord_gen : nullptr;
                const auto *tex_matrix = pass.tex_matrix.has_value() ? &*pass.tex_matrix : nullptr;
                const auto tex_coord = j3d_project_tex_coord(source, tex_coord_gen, tex_matrix, &model_matrix);
                tex_coords[i] = {tex_coord.u, tex_coord.v, tex_coord.q};
            }

            return ProjectedVertex{
                .x = camera.x,
                .y = camera.y,
                .z = camera.z,
                .u = tex_coords[0U][2U] != 0.0F ? tex_coords[0U][0U] / tex_coords[0U][2U] : tex_coords[0U][0U],
                .v = tex_coords[0U][2U] != 0.0F ? tex_coords[0U][1U] / tex_coords[0U][2U] : tex_coords[0U][1U],
                .clip_w = camera.z,
                .tex_coords = tex_coords,
                .color = color_to_float(gx_raster_color(source, material.gx_state, passes, {camera.x, camera.y, camera.z},
                                                        transform_normal_to_camera(model_matrix, camera_pose, source.normal))),
            };
        }

        void append_projected_triangle(std::vector<render::TexturedVertex2D> &vertices, std::vector<std::uint16_t> &indices,
                                       const ProjectedVertex &a, const ProjectedVertex &b, const ProjectedVertex &c,
                                       const render::core::FramebufferInfo &framebuffer, const CameraPoseCompat &camera_pose) {
            constexpr auto pi = 3.14159265358979323846F;
            const auto fovy = camera_pose.fovy_degrees * pi / 180.0F;
            const auto focal_y = 1.0F / std::tan(fovy * 0.5F);
            const auto aspect = camera_pose.aspect_ratio;
            const auto half_width = static_cast<float>(framebuffer.width) * 0.5F;
            const auto half_height = static_cast<float>(framebuffer.height) * 0.5F;
            const auto near_plane = camera_pose.near_clip;
            const auto far_plane = camera_pose.far_clip;

            const auto append_vertex = [&](const ProjectedVertex &projected) {
                const auto ndc_x = (projected.x / projected.z) * (focal_y / aspect);
                const auto ndc_y = (projected.y / projected.z) * focal_y;
                const auto ndc_z = std::clamp((projected.z - near_plane) / (far_plane - near_plane), 0.0F, 1.0F);
                vertices.push_back(render::TexturedVertex2D{
                    .x = ndc_x * half_width,
                    .y = ndc_y * half_height,
                    .z = ndc_z,
                    .u = projected.u,
                    .v = projected.v,
                    .color = color_to_u8(projected.color),
                });
            };

            if (vertices.size() + 3U > std::numeric_limits<std::uint16_t>::max()) {
                return;
            }

            const auto first = static_cast<std::uint16_t>(vertices.size());
            append_vertex(a);
            append_vertex(b);
            append_vertex(c);
            indices.push_back(first);
            indices.push_back(static_cast<std::uint16_t>(first + 1U));
            indices.push_back(static_cast<std::uint16_t>(first + 2U));
        }

        void append_projected_gx_triangle(std::vector<render::GxMaterialVertex2D> &vertices, std::vector<std::uint16_t> &indices,
                                          const ProjectedVertex &a, const ProjectedVertex &b, const ProjectedVertex &c,
                                          const render::core::FramebufferInfo &framebuffer, const CameraPoseCompat &camera_pose) {
            constexpr auto pi = 3.14159265358979323846F;
            const auto fovy = camera_pose.fovy_degrees * pi / 180.0F;
            const auto focal_y = 1.0F / std::tan(fovy * 0.5F);
            const auto aspect = camera_pose.aspect_ratio;
            const auto half_width = static_cast<float>(framebuffer.width) * 0.5F;
            const auto half_height = static_cast<float>(framebuffer.height) * 0.5F;
            const auto near_plane = camera_pose.near_clip;
            const auto far_plane = camera_pose.far_clip;

            const auto append_vertex = [&](const ProjectedVertex &projected) {
                const auto ndc_x = (projected.x / projected.z) * (focal_y / aspect);
                const auto ndc_y = (projected.y / projected.z) * focal_y;
                const auto ndc_z = std::clamp((projected.z - near_plane) / (far_plane - near_plane), 0.0F, 1.0F);
                vertices.push_back(render::GxMaterialVertex2D{
                    .x = ndc_x * half_width,
                    .y = ndc_y * half_height,
                    .z = ndc_z,
                    .clip_w = projected.clip_w,
                    .tex_coords = projected.tex_coords,
                    .color = color_to_u8(projected.color),
                });
            };

            if (vertices.size() + 3U > std::numeric_limits<std::uint16_t>::max()) {
                return;
            }

            const auto first = static_cast<std::uint16_t>(vertices.size());
            append_vertex(a);
            append_vertex(b);
            append_vertex(c);
            indices.push_back(first);
            indices.push_back(static_cast<std::uint16_t>(first + 1U));
            indices.push_back(static_cast<std::uint16_t>(first + 2U));
        }

        void project_source_mesh(std::span<const J3dMeshVertex> source_vertices, std::span<const std::uint16_t> source_indices,
                                 std::array<std::uint8_t, 4U> material_color, const render::core::FramebufferInfo &framebuffer,
                                 const MatrixPaletteContext &matrix_context, const CameraPoseCompat &camera_pose,
                                 const J3dTexCoordGenSummary *tex_coord_gen, const J3dTexMatrixSummary *tex_matrix,
                                 std::vector<render::TexturedVertex2D> &vertices, std::vector<std::uint16_t> &indices) {
            const auto triangle_count = source_indices.size() / 3U;
            vertices.clear();
            indices.clear();
            vertices.reserve(triangle_count * 3U);
            indices.reserve(triangle_count * 3U);
            const auto near_plane = camera_pose.near_clip;

            for (auto i = 0U; i + 2U < source_indices.size(); i += 3U) {
                const auto index_a = source_indices[i];
                const auto index_b = source_indices[i + 1U];
                const auto index_c = source_indices[i + 2U];
                if (index_a >= source_vertices.size() || index_b >= source_vertices.size() || index_c >= source_vertices.size()) {
                    continue;
                }

                const std::array<ProjectedVertex, 3U> triangle{
                    view_vertex(source_vertices[index_a], material_color, matrix_context, camera_pose, tex_coord_gen, tex_matrix),
                    view_vertex(source_vertices[index_b], material_color, matrix_context, camera_pose, tex_coord_gen, tex_matrix),
                    view_vertex(source_vertices[index_c], material_color, matrix_context, camera_pose, tex_coord_gen, tex_matrix),
                };
                const auto clipped = clip_to_near_plane(triangle, near_plane);
                if (clipped.size() < 3U) {
                    continue;
                }

                for (auto vertex_index = 1U; vertex_index + 1U < clipped.size(); ++vertex_index) {
                    append_projected_triangle(vertices, indices, clipped[0U], clipped[vertex_index], clipped[vertex_index + 1U], framebuffer,
                                              camera_pose);
                }
            }
        }

        void project_gx_material_source_mesh(std::span<const J3dMeshVertex> source_vertices, std::span<const std::uint16_t> source_indices,
                                             const J3dMaterialSummary &material, std::span<const J3dMaterialTexturePass> passes,
                                             const render::core::FramebufferInfo &framebuffer, const MatrixPaletteContext &matrix_context,
                                             const CameraPoseCompat &camera_pose, std::vector<render::GxMaterialVertex2D> &vertices,
                                             std::vector<std::uint16_t> &indices) {
            const auto triangle_count = source_indices.size() / 3U;
            vertices.clear();
            indices.clear();
            vertices.reserve(triangle_count * 3U);
            indices.reserve(triangle_count * 3U);
            const auto near_plane = camera_pose.near_clip;

            for (auto i = 0U; i + 2U < source_indices.size(); i += 3U) {
                const auto index_a = source_indices[i];
                const auto index_b = source_indices[i + 1U];
                const auto index_c = source_indices[i + 2U];
                if (index_a >= source_vertices.size() || index_b >= source_vertices.size() || index_c >= source_vertices.size()) {
                    continue;
                }

                const std::array<ProjectedVertex, 3U> triangle{
                    gx_material_view_vertex(source_vertices[index_a], material, passes, matrix_context, camera_pose),
                    gx_material_view_vertex(source_vertices[index_b], material, passes, matrix_context, camera_pose),
                    gx_material_view_vertex(source_vertices[index_c], material, passes, matrix_context, camera_pose),
                };
                const auto clipped = clip_to_near_plane(triangle, near_plane);
                if (clipped.size() < 3U) {
                    continue;
                }

                for (auto vertex_index = 1U; vertex_index + 1U < clipped.size(); ++vertex_index) {
                    append_projected_gx_triangle(vertices, indices, clipped[0U], clipped[vertex_index], clipped[vertex_index + 1U], framebuffer,
                                                 camera_pose);
                }
            }
        }

        void append_material_micro_triangle(std::vector<render::TexturedVertex2D> &vertices, std::vector<std::uint16_t> &indices,
                                            const J3dMeshVertex &a, const J3dMeshVertex &b, const J3dMeshVertex &c,
                                            const J3dMaterialSummary &material, std::span<const J3dTexture> textures,
                                            std::span<const J3dMaterialTexturePass> passes, const render::core::FramebufferInfo &framebuffer,
                                            const MatrixPaletteContext &matrix_context,
                                            const CameraPoseCompat &camera_pose) {
            const std::array<ProjectedVertex, 3U> triangle{
                material_view_vertex(a, material, textures, passes, matrix_context, camera_pose),
                material_view_vertex(b, material, textures, passes, matrix_context, camera_pose),
                material_view_vertex(c, material, textures, passes, matrix_context, camera_pose),
            };
            const auto clipped = clip_to_near_plane(triangle, camera_pose.near_clip);
            if (clipped.size() < 3U) {
                return;
            }

            for (auto vertex_index = 1U; vertex_index + 1U < clipped.size(); ++vertex_index) {
                append_projected_triangle(vertices, indices, clipped[0U], clipped[vertex_index], clipped[vertex_index + 1U], framebuffer,
                                          camera_pose);
            }
        }

        void project_material_source_mesh(std::span<const J3dMeshVertex> source_vertices, std::span<const std::uint16_t> source_indices,
                                          const J3dMaterialSummary &material, std::span<const J3dTexture> textures,
                                          std::span<const J3dMaterialTexturePass> passes, const render::core::FramebufferInfo &framebuffer,
                                          const MatrixPaletteContext &matrix_context,
                                          const CameraPoseCompat &camera_pose, std::vector<render::TexturedVertex2D> &vertices,
                                          std::vector<std::uint16_t> &indices) {
            constexpr auto subdivisions = 8U;
            const auto triangle_count = source_indices.size() / 3U;
            vertices.clear();
            indices.clear();
            vertices.reserve(std::min<std::size_t>(triangle_count * subdivisions * subdivisions * 6U, 60000U));
            indices.reserve(std::min<std::size_t>(triangle_count * subdivisions * subdivisions * 6U, 60000U));

            for (auto i = 0U; i + 2U < source_indices.size(); i += 3U) {
                const auto index_a = source_indices[i];
                const auto index_b = source_indices[i + 1U];
                const auto index_c = source_indices[i + 2U];
                if (index_a >= source_vertices.size() || index_b >= source_vertices.size() || index_c >= source_vertices.size()) {
                    continue;
                }

                const auto &a = source_vertices[index_a];
                const auto &b = source_vertices[index_b];
                const auto &c = source_vertices[index_c];
                const auto point = [&](std::uint32_t bi, std::uint32_t ci) {
                    const auto weight_b = static_cast<float>(bi) / static_cast<float>(subdivisions);
                    const auto weight_c = static_cast<float>(ci) / static_cast<float>(subdivisions);
                    const auto weight_a = 1.0F - weight_b - weight_c;
                    return interpolate_source_vertex(a, b, c, weight_a, weight_b, weight_c);
                };

                for (auto bi = 0U; bi < subdivisions; ++bi) {
                    for (auto ci = 0U; ci < subdivisions - bi; ++ci) {
                        if (vertices.size() + 6U > std::numeric_limits<std::uint16_t>::max()) {
                            return;
                        }

                        const auto p0 = point(bi, ci);
                        const auto p1 = point(bi + 1U, ci);
                        const auto p2 = point(bi, ci + 1U);
                        append_material_micro_triangle(vertices, indices, p0, p1, p2, material, textures, passes, framebuffer, matrix_context,
                                                       camera_pose);

                        if (bi + ci + 1U < subdivisions) {
                            const auto p3 = point(bi + 1U, ci + 1U);
                            append_material_micro_triangle(vertices, indices, p1, p3, p2, material, textures, passes, framebuffer, matrix_context,
                                                           camera_pose);
                        }
                    }
                }
            }
        }

    }  // namespace

    void J3dModelRenderer::load(render::IRendererEngine &renderer, std::span<const std::uint8_t> model_data,
                                const J3dModelRendererLoadOptions &options) {
        _loaded = false;
        _meshes.clear();
        _render_packets.clear();
        _textures.clear();
        _joint_transforms.clear();
        _joint_parent_indices.clear();
        _draw_matrices.clear();
        _envelopes.reset();

        const auto geometry = extract_j3d_model_geometry(model_data);
        _textures = geometry.textures;
        if (geometry.draw_matrices.has_value()) {
            _draw_matrices = geometry.draw_matrices->matrices;
        }
        _envelopes = geometry.envelopes;
        if (geometry.joints.has_value()) {
            _joint_transforms.reserve(geometry.joints->joints.size());
            for (const auto &joint : geometry.joints->joints) {
                _joint_transforms.push_back(joint_transform_from_summary(joint));
            }
            _joint_parent_indices = geometry.joints->parent_indices;
        }
        auto texture_handles = std::vector<render::TextureHandle>{};
        texture_handles.reserve(geometry.textures.size());
        for (const auto &texture : geometry.textures) {
            texture_handles.push_back(renderer.create_rgba8_texture(texture.image.width, texture.image.height, texture.image.rgba));
        }

        if (geometry.materials.has_value()) {
            if (options.constant_backdrop_material_index.has_value() &&
                *options.constant_backdrop_material_index < geometry.materials->materials.size()) {
                _meshes.push_back(
                    make_constant_backdrop(renderer, geometry.materials->materials[*options.constant_backdrop_material_index].tev_k_colors[0U]));
            }

            for (const auto &shape : geometry.shapes) {
                if (shape.material_index == 0xffffU || shape.material_index >= geometry.materials->materials.size()) {
                    continue;
                }
                if (shape.material_index < options.min_material_index || shape.material_index > options.max_material_index) {
                    continue;
                }

                const auto &material = geometry.materials->materials[shape.material_index];
                const auto passes = gx_material_texture_passes(material.gx_state);
                for (const auto &shape_packet : shape.draw_packets) {
                    const auto assign_shape_packet = [&](Mesh &mesh) {
                        mesh.material_name = material.name;
                        mesh.shape_index = shape.shape_index;
                        mesh.shape_draw_order = shape.draw_order;
                        mesh.material_index = shape.material_index;
                        mesh.joint_index = shape.joint_index;
                        mesh.matrix_group_count = static_cast<std::uint16_t>(shape.draw_packets.size());
                        mesh.matrix_group = shape_packet.matrix_group;
                        if (geometry.joints.has_value() && shape.joint_index < geometry.joints->joints.size()) {
                            mesh.joint_transform = joint_transform_from_summary(geometry.joints->joints[shape.joint_index]);
                        }
                        mesh.project_source_vertices = true;
                        mesh.source_vertices = shape_packet.vertices;
                        mesh.source_indices = shape_packet.indices;
                    };

                    if (passes.empty()) {
                        if (!options.use_cpu_tev) {
                            continue;
                        }

                        if (material_uses_gx_lighting(material.gx_state)) {
                            const std::array<std::uint8_t, 4U> white_pixel{255U, 255U, 255U, 255U};
                            const auto white_texture =
                                renderer.create_rgba8_texture(1U, 1U, std::span<const std::uint8_t>(white_pixel.data(), white_pixel.size()));
                            if (white_texture.is_valid()) {
                                auto mesh = Mesh{};
                                assign_shape_packet(mesh);
                                mesh.texture = white_texture;
                                mesh.material = material;
                                mesh.material_passes = passes;
                                mesh.packet_mode = J3dRendererPacketMode::CpuTevPerVertex;
                                mesh.material_color = material.material_colors[0U];
                                mesh.pass_order = 0U;
                                mesh.cull_mode = cull_mode_from_gx_material_state(material);
                                mesh.blend_mode = material.blend.enabled ? blend_mode_for_composed_material(material, true) :
                                                                           render::BlendMode::Opaque;
                                mesh.blend = mesh.blend_mode != render::BlendMode::Opaque;
                                if (material.gx_state.z_mode.enabled) {
                                    mesh.depth_test = material.gx_state.z_mode.compare_enable != 0U;
                                    mesh.depth_write = material.gx_state.z_mode.update_enable != 0U;
                                    mesh.depth_compare = depth_compare_from_gx(material.gx_state.z_mode.function);
                                }
                                mesh.evaluate_material_per_vertex = true;
                                if (!mesh.source_vertices.empty() && !mesh.source_indices.empty()) {
                                    _meshes.push_back(std::move(mesh));
                                    continue;
                                }
                            }
                        }

                        const auto composed_texture = j3d_try_compose_material_constant(material, material.material_colors[0U]);
                        if (!composed_texture.has_value()) {
                            continue;
                        }

                        const auto composed_handle = renderer.create_rgba8_texture(composed_texture->image.width, composed_texture->image.height,
                                                                                   composed_texture->image.rgba);
                        if (!composed_handle.is_valid()) {
                            continue;
                        }

                        const auto texture_has_alpha = texture_needs_blending(composed_texture->image);
                        auto mesh = Mesh{};
                        assign_shape_packet(mesh);
                        mesh.texture = composed_handle;
                        mesh.material_color = composed_texture->raster_color_baked ?
                                                  std::array<std::uint8_t, 4U>{255U, 255U, 255U, 255U} :
                                                  material.material_colors[0U];
                        mesh.material = material;
                        mesh.material_passes = passes;
                        mesh.packet_mode = J3dRendererPacketMode::ConstantMaterial;
                        mesh.pass_order = 0U;
                        mesh.cull_mode = cull_mode_from_gx_material_state(material);
                        mesh.blend_mode = blend_mode_for_composed_material(material, texture_has_alpha);
                        mesh.blend = mesh.blend_mode != render::BlendMode::Opaque;
                        if (material.gx_state.z_mode.enabled) {
                            mesh.depth_test = material.gx_state.z_mode.compare_enable != 0U;
                            mesh.depth_write = material.gx_state.z_mode.update_enable != 0U;
                            mesh.depth_compare = depth_compare_from_gx(material.gx_state.z_mode.function);
                        }
                        if (!mesh.source_vertices.empty() && !mesh.source_indices.empty()) {
                            _meshes.push_back(std::move(mesh));
                        }
                        continue;
                    }

                    const auto shader_textures_available = material_pass_textures_available(passes, geometry.textures) &&
                                                           std::ranges::all_of(passes, [&texture_handles](const auto &pass) {
                                                               return pass.texture_index < texture_handles.size() &&
                                                                      texture_handles[pass.texture_index].is_valid();
                                                           });
                    if (shader_textures_available && material_can_use_shader_gx_tev(material.gx_state, passes)) {
                        auto mesh = Mesh{};
                        assign_shape_packet(mesh);
                        mesh.texture = texture_handles[passes.front().texture_index];
                        mesh.material = material;
                        mesh.material_passes = passes;
                        mesh.gx_texture_stage_count = passes.size();
                        for (auto i = std::size_t{}; i < passes.size() && i < mesh.gx_texture_stages.size(); ++i) {
                            const auto texture_index = passes[i].texture_index;
                            mesh.gx_texture_stages[i] = render::GxTextureStage2D{
                                .texture = texture_handles[texture_index],
                                .wrap_u = geometry.textures[texture_index].wrap_s != 0U,
                                .wrap_v = geometry.textures[texture_index].wrap_t != 0U,
                            };
                        }
                        mesh.gx_tev_stage_count = 0U;
                        for (const auto &stage : material.gx_state.tev_stages) {
                            if (!is_active_tev_stage(material.gx_state, stage)) {
                                continue;
                            }
                            if (mesh.gx_tev_stage_count >= mesh.gx_tev_stages.size()) {
                                break;
                            }
                            mesh.gx_tev_stages[mesh.gx_tev_stage_count] = gx_tev_stage_from_state(material.gx_state, stage, passes);
                            ++mesh.gx_tev_stage_count;
                        }
                        mesh.packet_mode = J3dRendererPacketMode::ShaderGxTev;
                        mesh.material_color = material.material_colors[0U];
                        mesh.pass_order = passes.front().stage;
                        mesh.cull_mode = cull_mode_from_gx_material_state(material);
                        mesh.gx_blend = gx_blend_from_material_state(material.gx_state.blend);
                        mesh.gx_alpha_compare = gx_alpha_compare_from_material_state(material.gx_state.alpha_compare);
                        mesh.gx_initial_tev_registers = material.gx_state.tev_registers;
                        mesh.blend = mesh.gx_blend.enabled && mesh.gx_blend.type != 0U;
                        mesh.blend_mode = render::BlendMode::Opaque;
                        if (material.gx_state.z_mode.enabled) {
                            mesh.depth_test = material.gx_state.z_mode.compare_enable != 0U;
                            mesh.depth_write = material.gx_state.z_mode.update_enable != 0U;
                            mesh.depth_compare = depth_compare_from_gx(material.gx_state.z_mode.function);
                        }
                        if (!mesh.source_vertices.empty() && !mesh.source_indices.empty()) {
                            _meshes.push_back(std::move(mesh));
                            continue;
                        }
                    }

                    if (options.use_cpu_tev && (passes.size() > 1U || has_active_indirect_tev_stage(material.gx_state))) {
                        const auto composed_texture =
                            j3d_try_compose_material_texture(material, geometry.textures, passes, material.material_colors[0U]);
                        if (composed_texture.has_value()) {
                            const auto composed_handle = renderer.create_rgba8_texture(composed_texture->image.width, composed_texture->image.height,
                                                                                       composed_texture->image.rgba);
                            if (composed_handle.is_valid()) {
                                const auto texture_has_alpha = texture_needs_blending(composed_texture->image);
                                auto mesh = Mesh{};
                                assign_shape_packet(mesh);
                                mesh.texture = composed_handle;
                                mesh.material_color = composed_texture->raster_color_baked ?
                                                          std::array<std::uint8_t, 4U>{255U, 255U, 255U, 255U} :
                                                          material.material_colors[0U];
                                mesh.material = material;
                                mesh.material_passes = passes;
                                mesh.packet_mode = J3dRendererPacketMode::ComposedMaterial;
                                mesh.pass_order = passes.front().stage;
                                mesh.cull_mode = cull_mode_from_gx_material_state(material);
                                mesh.wrap_u = true;
                                mesh.wrap_v = true;
                                mesh.blend_mode = blend_mode_for_composed_material(material, texture_has_alpha);
                                mesh.blend = mesh.blend_mode != render::BlendMode::Opaque;
                                if (material.gx_state.z_mode.enabled) {
                                    mesh.depth_test = material.gx_state.z_mode.compare_enable != 0U;
                                    mesh.depth_write = material.gx_state.z_mode.update_enable != 0U;
                                    mesh.depth_compare = depth_compare_from_gx(material.gx_state.z_mode.function);
                                }
                                if (!mesh.source_vertices.empty() && !mesh.source_indices.empty()) {
                                    _meshes.push_back(std::move(mesh));
                                    continue;
                                }
                            }
                        }

                        if (material_pass_textures_available(passes, geometry.textures)) {
                            const std::array<std::uint8_t, 4U> white_pixel{255U, 255U, 255U, 255U};
                            const auto white_texture =
                                renderer.create_rgba8_texture(1U, 1U, std::span<const std::uint8_t>(white_pixel.data(), white_pixel.size()));
                            if (white_texture.is_valid()) {
                                auto mesh = Mesh{};
                                assign_shape_packet(mesh);
                                mesh.texture = white_texture;
                                mesh.material = material;
                                mesh.material_passes = passes;
                                mesh.packet_mode = J3dRendererPacketMode::CpuTevPerVertex;
                                mesh.material_color = material.material_colors[0U];
                                mesh.pass_order = passes.front().stage;
                                mesh.cull_mode = cull_mode_from_gx_material_state(material);
                                mesh.blend = true;
                                mesh.blend_mode = render::BlendMode::Alpha;
                                if (material.gx_state.z_mode.enabled) {
                                    mesh.depth_test = material.gx_state.z_mode.compare_enable != 0U;
                                    mesh.depth_write = material.gx_state.z_mode.update_enable != 0U;
                                    mesh.depth_compare = depth_compare_from_gx(material.gx_state.z_mode.function);
                                }
                                mesh.evaluate_material_per_vertex = true;
                                if (!mesh.source_vertices.empty() && !mesh.source_indices.empty()) {
                                    _meshes.push_back(std::move(mesh));
                                    continue;
                                }
                            }
                        }
                    }

                    for (auto pass_index = std::size_t{}; pass_index < passes.size(); ++pass_index) {
                        const auto &pass = passes[pass_index];
                        const auto texture_index = pass.texture_index;
                        if (texture_index >= texture_handles.size() || !texture_handles[texture_index].is_valid()) {
                            continue;
                        }

                        auto texture_handle = texture_handles[texture_index];
                        auto material_color = material.material_colors[0U];
                        auto texture_has_alpha = texture_needs_blending(geometry.textures[texture_index].image);
                        auto tev_baked = false;
                        if (passes.size() == 1U && options.use_cpu_tev) {
                            const auto composed_texture =
                                j3d_try_compose_material_texture(material, geometry.textures[texture_index].image, material_color, pass.tex_map_slot);
                            if (composed_texture.has_value()) {
                                const auto composed_handle =
                                    renderer.create_rgba8_texture(composed_texture->image.width, composed_texture->image.height,
                                                                  composed_texture->image.rgba);
                                if (composed_handle.is_valid()) {
                                    texture_handle = composed_handle;
                                    texture_has_alpha = texture_needs_blending(composed_texture->image);
                                    if (composed_texture->raster_color_baked) {
                                        material_color = {255U, 255U, 255U, 255U};
                                        tev_baked = true;
                                    }
                                }
                            }
                        }

                        const auto *tev_stage = find_tev_stage(material, pass.stage);
                        auto mesh = Mesh{};
                        assign_shape_packet(mesh);
                        mesh.texture = texture_handle;
                        mesh.tex_coord_gen = pass.tex_coord_gen;
                        mesh.tex_matrix = pass.tex_matrix;
                        mesh.material = material;
                        mesh.material_passes = passes;
                        mesh.packet_mode = J3dRendererPacketMode::TexturePass;
                        mesh.material_color = material_color;
                        mesh.pass_order = pass.stage;
                        mesh.cull_mode = cull_mode_from_gx_material_state(material);
                        mesh.wrap_u = geometry.textures[texture_index].wrap_s != 0U;
                        mesh.wrap_v = geometry.textures[texture_index].wrap_t != 0U;
                        mesh.blend_mode = tev_baked ?
                                              blend_mode_for_composed_material(material, texture_has_alpha) :
                                              blend_mode_for_material_pass(material, texture_has_alpha, tev_stage, pass_index != 0U,
                                                                           options.use_gx_blend_mode);
                        mesh.blend = mesh.blend_mode != render::BlendMode::Opaque;
                        if (material.gx_state.z_mode.enabled) {
                            mesh.depth_test = material.gx_state.z_mode.compare_enable != 0U;
                            mesh.depth_write = material.gx_state.z_mode.update_enable != 0U;
                            mesh.depth_compare = depth_compare_from_gx(material.gx_state.z_mode.function);
                        }
                        if (!mesh.source_vertices.empty() && !mesh.source_indices.empty()) {
                            _meshes.push_back(std::move(mesh));
                        }
                    }
                }
            }
        }

        std::ranges::sort(_meshes, [](const auto &a, const auto &b) {
            if (a.material_index == 0xfffeU || b.material_index == 0xfffeU) {
                return a.material_index == 0xfffeU && b.material_index != 0xfffeU;
            }
            if (a.shape_draw_order != b.shape_draw_order) {
                return a.shape_draw_order < b.shape_draw_order;
            }
            if (a.matrix_group.group_index != b.matrix_group.group_index) {
                return a.matrix_group.group_index < b.matrix_group.group_index;
            }

            return a.pass_order < b.pass_order;
        });

        _render_packets.reserve(_meshes.size());
        for (const auto &mesh : _meshes) {
            _render_packets.push_back(packet_state_for_mesh(mesh));
        }

        _loaded = !_meshes.empty();
    }

    void J3dModelRenderer::set_bck_animation(const J3dBckAnimationSummary &animation) {
        _bck_animation = animation;
    }

    void J3dModelRenderer::set_btk_animation(const J3dBtkAnimationSummary &animation) {
        _btk_animation = animation;
    }

    void J3dModelRenderer::clear_animations() {
        _bck_animation.reset();
        _btk_animation.reset();
    }

    void J3dModelRenderer::draw(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose, const J3dMatrix3x4 &actor_matrix,
                                std::uint64_t frame, const J3dModelRendererDrawOptions &options) const {
        if (!_loaded) {
            return;
        }

        for (const auto &mesh : _meshes) {
            if (!material_filter_allows_mesh(options.material_filter, mesh.material_name, mesh.material_index)) {
                continue;
            }
            if (options.translucent_filter.has_value() && mesh.blend != *options.translucent_filter) {
                continue;
            }

            submit_mesh(renderer, mesh, camera_pose, actor_matrix, frame);
        }
    }

    bool J3dModelRenderer::is_loaded() const {
        return _loaded;
    }

    std::size_t J3dModelRenderer::mesh_count() const {
        return _meshes.size();
    }

    std::span<const J3dRendererPacketState> J3dModelRenderer::render_packets() const {
        return _render_packets;
    }

    std::vector<J3dRendererPacketState> J3dModelRenderer::render_packets(std::uint64_t frame) const {
        auto packets = std::vector<J3dRendererPacketState>{};
        packets.reserve(_meshes.size());
        for (const auto &mesh : _meshes) {
            packets.push_back(packet_state_for_mesh(mesh, frame));
        }
        return packets;
    }

    J3dModelRenderer::Mesh J3dModelRenderer::make_constant_backdrop(render::IRendererEngine &renderer, std::array<std::uint8_t, 4U> color) const {
        const std::array<std::uint8_t, 4U> white_pixel{255U, 255U, 255U, 255U};
        const auto texture = renderer.create_rgba8_texture(1U, 1U, std::span<const std::uint8_t>(white_pixel.data(), white_pixel.size()));
        const auto framebuffer = renderer.logical_framebuffer_size();
        const auto half_width = static_cast<float>(framebuffer.width) * 0.5F;
        const auto half_height = static_cast<float>(framebuffer.height) * 0.5F;

        auto mesh = Mesh{};
        mesh.material_name = "constant-backdrop";
        mesh.material_index = 0xfffeU;
        mesh.packet_mode = J3dRendererPacketMode::ConstantBackdrop;
        mesh.texture = texture;
        mesh.vertices = {
            vertex(-half_width, -half_height, 0.0F, 0.0F, color),
            vertex(half_width, -half_height, 1.0F, 0.0F, color),
            vertex(half_width, half_height, 1.0F, 1.0F, color),
            vertex(-half_width, half_height, 0.0F, 1.0F, color),
        };
        mesh.indices = {0U, 1U, 2U, 0U, 2U, 3U};
        mesh.blend = false;
        mesh.blend_mode = render::BlendMode::Opaque;
        return mesh;
    }

    J3dRendererPacketState J3dModelRenderer::packet_state_for_mesh(const Mesh &mesh) {
        auto state = J3dRendererPacketState{
            .material_name = mesh.material_name,
            .shape_index = mesh.shape_index,
            .shape_draw_order = mesh.shape_draw_order,
            .material_index = mesh.material_index,
            .joint_index = mesh.joint_index,
            .matrix_group_index = mesh.matrix_group.group_index,
            .matrix_group_count = mesh.matrix_group_count,
            .use_matrix_index = mesh.matrix_group.use_matrix_index,
            .use_matrix_count = mesh.matrix_group.use_matrix_count,
            .first_matrix_table_index = mesh.matrix_group.first_matrix_table_index,
            .matrix_table_count = mesh.matrix_group.matrix_table.size(),
            .display_list_offset = mesh.matrix_group.display_list_offset,
            .display_list_size = mesh.matrix_group.display_list_size,
            .parsed_display_list_bytes = mesh.matrix_group.parsed_display_list_bytes,
            .draw_packet_triangle_count = mesh.matrix_group.triangle_count,
            .pass_order = mesh.pass_order,
            .packet_mode = mesh.packet_mode,
            .material_pass_count = mesh.material_passes.size(),
            .shader_texture_stage_count = mesh.gx_texture_stage_count,
            .source_vertex_count = mesh.source_vertices.size(),
            .source_triangle_count = mesh.source_indices.size() / 3U,
            .project_source_vertices = mesh.project_source_vertices,
            .evaluate_material_per_vertex = mesh.evaluate_material_per_vertex,
            .blend = mesh.blend,
            .blend_mode = mesh.blend_mode,
            .gx_blend = mesh.gx_blend,
            .gx_alpha_compare = mesh.gx_alpha_compare,
            .gx_initial_tev_registers = mesh.gx_initial_tev_registers,
            .depth_test = mesh.depth_test,
            .depth_write = mesh.depth_write,
            .depth_compare = mesh.depth_compare,
            .cull_mode = mesh.cull_mode,
        };

        if (mesh.material.has_value()) {
            state.color_channel_count = mesh.material->gx_state.color_channel_count;
            for (auto channel = 0U; channel < state.color_channel_material_colors.size(); ++channel) {
                state.color_channel_material_colors[channel] = mesh.material->gx_state.color_channels[channel].material_color;
                state.color_channel_ambient_colors[channel] = mesh.material->gx_state.color_channels[channel].ambient_color;
                state.color_channel_controls[channel] = mesh.material->gx_state.color_channels[channel].color_control;
                state.alpha_channel_controls[channel] = mesh.material->gx_state.color_channels[channel].alpha_control;
            }
            state.lights = mesh.material->gx_state.lights;
            for (auto light = 0U; light < state.lights.size(); ++light) {
                if (state.lights[light].loaded) {
                    state.loaded_light_mask |= static_cast<std::uint8_t>(1U << light);
                }
            }
            state.declared_tev_stage_count = mesh.material->gx_state.tev_stage_count;
            state.active_tev_stage_count = active_tev_stage_count(mesh.material->gx_state);
            state.tev_order_count = mesh.material->gx_state.tev_orders.size();
            state.tev_stage_count = mesh.material->gx_state.tev_stages.size();
            state.texgen_count = mesh.material->gx_state.tex_coord_gens.size();
            state.indirect_stage_count = mesh.material->gx_state.indirect.stage_count;
            state.active_indirect_tev_stage_count = active_indirect_tev_stage_count(mesh.material->gx_state);
            state.indirect_matrix_count = mesh.material->gx_state.indirect.texture_matrices.size();
            state.indirect_texture_order_count = mesh.material->gx_state.indirect.texture_orders.size();
            state.indirect_texture_scale_count = mesh.material->gx_state.indirect.texture_coord_scales.size();
            state.mdl3_packet_bytes = mesh.material->gx_state.mdl3_display_list.size();
            state.mdl3_command_count = mesh.material->gx_state.mdl3_stats.command_count;
            state.mdl3_bp_load_count = mesh.material->gx_state.mdl3_stats.bp_load_count;
            state.mdl3_xf_load_count = mesh.material->gx_state.mdl3_stats.xf_load_count;
            state.fog_enabled = mesh.material->gx_state.fog.enabled;
            state.fog_type = mesh.material->gx_state.fog.type;
            state.fog_projection = mesh.material->gx_state.fog.projection;
            state.fog_range_adjust_enabled = mesh.material->gx_state.fog.range_adjust_enabled;
            state.fog_color = mesh.material->gx_state.fog.color;
        }

        return state;
    }

    J3dRendererPacketState J3dModelRenderer::packet_state_for_mesh(const Mesh &mesh, std::uint64_t frame) const {
        auto state = packet_state_for_mesh(mesh);
        const auto animation_frame = static_cast<float>(frame);

        if (_bck_animation.has_value()) {
            state.bck_active = true;
            state.bck_frame = animation_frame;
            state.bck_normalized_frame = normalized_animation_frame(animation_frame, _bck_animation->frame_max);
            state.bck_frame_max = _bck_animation->frame_max;
            state.bck_joint_count = _bck_animation->joint_count;
        }

        if (_btk_animation.has_value()) {
            state.btk_active = true;
            state.btk_frame = animation_frame;
            state.btk_normalized_frame = normalized_animation_frame(animation_frame, _btk_animation->frame_max);
            state.btk_frame_max = _btk_animation->frame_max;
            state.btk_material_count = static_cast<std::uint16_t>(_btk_animation->materials.size());
        }

        return state;
    }

    void J3dModelRenderer::submit_mesh(render::IRendererEngine &renderer, const Mesh &mesh, const CameraPoseCompat &camera_pose,
                                       const J3dMatrix3x4 &actor_matrix, std::uint64_t frame) const {
        if (mesh.project_source_vertices) {
            auto vertices = std::vector<render::TexturedVertex2D>{};
            auto indices = std::vector<std::uint16_t>{};
            const auto *tex_coord_gen = mesh.tex_coord_gen.has_value() ? &*mesh.tex_coord_gen : nullptr;
            auto effective_tex_matrix = mesh.tex_matrix;
            if (_btk_animation.has_value() && effective_tex_matrix.has_value()) {
                const auto srt =
                    j3d_evaluate_btk_texture_srt(*_btk_animation, mesh.material_name, effective_tex_matrix->slot, static_cast<float>(frame));
                if (srt.has_value()) {
                    effective_tex_matrix->center = srt->center;
                    effective_tex_matrix->scale_s = srt->scale_s;
                    effective_tex_matrix->scale_t = srt->scale_t;
                    effective_tex_matrix->rotation = srt->rotation;
                    effective_tex_matrix->translate_s = srt->translate_s;
                    effective_tex_matrix->translate_t = srt->translate_t;
                }
            }
            const auto *tex_matrix = effective_tex_matrix.has_value() ? &*effective_tex_matrix : nullptr;
            const auto *fallback_transform = mesh.joint_transform.has_value() ? &*mesh.joint_transform : nullptr;
            const auto *envelopes = _envelopes.has_value() ? &*_envelopes : nullptr;
            const auto matrix_context = MatrixPaletteContext{
                .actor_matrix = actor_matrix,
                .transforms = _joint_transforms,
                .parent_indices = _joint_parent_indices,
                .draw_matrices = _draw_matrices,
                .envelopes = envelopes,
                .animation = _bck_animation,
                .fallback_transform = fallback_transform,
                .default_joint_index = mesh.joint_index,
                .frame = frame,
            };
            if (mesh.packet_mode == J3dRendererPacketMode::ShaderGxTev && mesh.gx_texture_stage_count > 0U) {
                auto effective_passes = mesh.material_passes;
                if (_btk_animation.has_value()) {
                    for (auto &pass : effective_passes) {
                        if (!pass.tex_matrix.has_value()) {
                            continue;
                        }

                        const auto srt =
                            j3d_evaluate_btk_texture_srt(*_btk_animation, mesh.material_name, pass.tex_matrix->slot, static_cast<float>(frame));
                        if (srt.has_value()) {
                            pass.tex_matrix->center = srt->center;
                            pass.tex_matrix->scale_s = srt->scale_s;
                            pass.tex_matrix->scale_t = srt->scale_t;
                            pass.tex_matrix->rotation = srt->rotation;
                            pass.tex_matrix->translate_s = srt->translate_s;
                            pass.tex_matrix->translate_t = srt->translate_t;
                        }
                    }
                }

                auto gx_vertices = std::vector<render::GxMaterialVertex2D>{};
                project_gx_material_source_mesh(mesh.source_vertices, mesh.source_indices, *mesh.material, effective_passes,
                                                renderer.logical_framebuffer_size(), matrix_context, camera_pose, gx_vertices, indices);
                if (gx_vertices.empty() || indices.empty()) {
                    return;
                }

                renderer.submit_gx_material_triangles(render::core::GxMaterialTriangleBatch2D{
                    .vertices = std::span<const render::GxMaterialVertex2D>(gx_vertices.data(), gx_vertices.size()),
                    .indices = std::span<const std::uint16_t>(indices.data(), indices.size()),
                    .texture_stages = std::span<const render::GxTextureStage2D>(mesh.gx_texture_stages.data(), mesh.gx_texture_stage_count),
                    .tev_stages = std::span<const render::GxTevStage2D>(mesh.gx_tev_stages.data(), mesh.gx_tev_stage_count),
                    .initial_tev_registers = mesh.gx_initial_tev_registers,
                    .alpha_compare = mesh.gx_alpha_compare,
                    .blend = mesh.gx_blend,
                    .depth_test = mesh.depth_test,
                    .depth_write = mesh.depth_write,
                    .depth_compare = mesh.depth_compare,
                    .cull_mode = mesh.cull_mode,
                    .fog = gx_fog_for_mesh(mesh.material),
                });
                return;
            }
            if (mesh.evaluate_material_per_vertex && mesh.material.has_value()) {
                auto effective_passes = mesh.material_passes;
                if (_btk_animation.has_value()) {
                    for (auto &pass : effective_passes) {
                        if (!pass.tex_matrix.has_value()) {
                            continue;
                        }

                        const auto srt =
                            j3d_evaluate_btk_texture_srt(*_btk_animation, mesh.material_name, pass.tex_matrix->slot, static_cast<float>(frame));
                        if (srt.has_value()) {
                            pass.tex_matrix->center = srt->center;
                            pass.tex_matrix->scale_s = srt->scale_s;
                            pass.tex_matrix->scale_t = srt->scale_t;
                            pass.tex_matrix->rotation = srt->rotation;
                            pass.tex_matrix->translate_s = srt->translate_s;
                            pass.tex_matrix->translate_t = srt->translate_t;
                        }
                    }
                }

                project_material_source_mesh(mesh.source_vertices, mesh.source_indices, *mesh.material, _textures, effective_passes,
                                             renderer.logical_framebuffer_size(), matrix_context, camera_pose, vertices, indices);
                if (vertices.empty() || indices.empty()) {
                    return;
                }

                renderer.submit_textured_triangles(mesh.texture,
                                                   render::core::TexturedTriangleBatch2D{
                                                       .vertices = std::span<const render::TexturedVertex2D>(vertices.data(), vertices.size()),
                                                       .indices = std::span<const std::uint16_t>(indices.data(), indices.size()),
                                                       .wrap_u = mesh.wrap_u,
                                                       .wrap_v = mesh.wrap_v,
                                                       .blend = mesh.blend,
                                                       .blend_mode = mesh.blend_mode,
                                                       .depth_test = mesh.depth_test,
                                                       .depth_write = mesh.depth_write,
                                                       .depth_compare = mesh.depth_compare,
                                                       .cull_mode = mesh.cull_mode,
                                                       .fog = gx_fog_for_mesh(mesh.material),
                                                   });
                return;
            }

            project_source_mesh(mesh.source_vertices, mesh.source_indices, mesh.material_color, renderer.logical_framebuffer_size(), matrix_context,
                                camera_pose, tex_coord_gen, tex_matrix, vertices, indices);
            if (vertices.empty() || indices.empty()) {
                return;
            }

            renderer.submit_textured_triangles(mesh.texture,
                                               render::core::TexturedTriangleBatch2D{
                                                   .vertices = std::span<const render::TexturedVertex2D>(vertices.data(), vertices.size()),
                                                   .indices = std::span<const std::uint16_t>(indices.data(), indices.size()),
                                                   .wrap_u = mesh.wrap_u,
                                                   .wrap_v = mesh.wrap_v,
                                                   .blend = mesh.blend,
                                                   .blend_mode = mesh.blend_mode,
                                                   .depth_test = mesh.depth_test,
                                                   .depth_write = mesh.depth_write,
                                                   .depth_compare = mesh.depth_compare,
                                                   .cull_mode = mesh.cull_mode,
                                                   .fog = gx_fog_for_mesh(mesh.material),
                                               });
            return;
        }

        renderer.submit_textured_triangles(mesh.texture,
                                           render::core::TexturedTriangleBatch2D{
                                               .vertices = std::span<const render::TexturedVertex2D>(mesh.vertices.data(), mesh.vertices.size()),
                                               .indices = std::span<const std::uint16_t>(mesh.indices.data(), mesh.indices.size()),
                                               .wrap_u = mesh.wrap_u,
                                               .wrap_v = mesh.wrap_v,
                                               .blend = mesh.blend,
                                               .blend_mode = mesh.blend_mode,
                                               .depth_test = mesh.depth_test,
                                               .depth_write = mesh.depth_write,
                                               .depth_compare = mesh.depth_compare,
                                               .cull_mode = mesh.cull_mode,
                                               .fog = gx_fog_for_mesh(mesh.material),
                                           });
    }

    J3dMatrix3x4 j3d_matrix_from_translation_scale(const CameraParamVec3 &translation, float scale) {
        return J3dMatrix3x4{
            .m =
                {
                    scale,
                    0.0F,
                    0.0F,
                    translation.x,
                    0.0F,
                    scale,
                    0.0F,
                    translation.y,
                    0.0F,
                    0.0F,
                    scale,
                    translation.z,
                },
        };
    }

}  // namespace smgpc::game
