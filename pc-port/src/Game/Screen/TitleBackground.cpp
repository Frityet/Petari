#include "TitleBackground.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Game/compat/CameraPose.hpp"
#include "Game/compat/FileSelectSkyRuntime.hpp"
#include "Game/compat/J3dAnimation.hpp"
#include "Game/compat/J3dMaterialRuntime.hpp"
#include "Game/compat/J3dModel.hpp"
#include "Game/compat/J3dTexture.hpp"
#include "Game/compat/RarcArchive.hpp"
#include "Game/compat/RuntimeContext.hpp"

namespace smgpc::game {
    namespace {

        constexpr std::array< TitleBackground::Layer, 5U > TITLE_SKY_LAYERS{
            TitleBackground::Layer{
                .texture_name = "Skyk",
                .color = {5U, 64U, 96U, 255U},
                .center_x = 0.0F,
                .center_y = 0.0F,
                .width = 920.0F,
                .height = 690.0F,
                .wrap_u = true,
                .wrap_v = false,
            },
            TitleBackground::Layer{
                .texture_name = "OrbitUniverseL",
                .color = {40U, 150U, 235U, 235U},
                .center_x = 0.0F,
                .center_y = -70.0F,
                .width = 920.0F,
                .height = 460.0F,
                .wrap_u = true,
                .wrap_v = true,
            },
            TitleBackground::Layer{
                .texture_name = "EarthKsMM",
                .color = {90U, 245U, 255U, 230U},
                .center_x = 0.0F,
                .center_y = 350.0F,
                .width = 980.0F,
                .height = 300.0F,
                .wrap_u = true,
                .wrap_v = false,
            },
            TitleBackground::Layer{
                .texture_name = "PlanetSun",
                .color = {190U, 255U, 255U, 220U},
                .center_x = 255.0F,
                .center_y = 170.0F,
                .width = 245.0F,
                .height = 105.0F,
            },
            TitleBackground::Layer{
                .texture_name = "CometHalo",
                .color = {35U, 95U, 255U, 95U},
                .center_x = 0.0F,
                .center_y = -30.0F,
                .width = 620.0F,
                .height = 360.0F,
            },
        };

        [[nodiscard]] render::core::TexturedVertex2D vertex(float x, float y, float u, float v, std::array< std::uint8_t, 4U > color) {
            return {
                .x = x,
                .y = y,
                .z = 0.0F,
                .u = u,
                .v = v,
                .color = color,
            };
        }

        [[nodiscard]] std::array< std::uint8_t, 4U > modulate_color(std::array< std::uint8_t, 4U > color, std::array< std::uint8_t, 4U > tint) {
            return {
                static_cast< std::uint8_t >((static_cast< std::uint16_t >(color[0U]) * tint[0U]) / 255U),
                static_cast< std::uint8_t >((static_cast< std::uint16_t >(color[1U]) * tint[1U]) / 255U),
                static_cast< std::uint8_t >((static_cast< std::uint16_t >(color[2U]) * tint[2U]) / 255U),
                static_cast< std::uint8_t >((static_cast< std::uint16_t >(color[3U]) * tint[3U]) / 255U),
            };
        }

        [[nodiscard]] bool is_title_sky_material(std::uint16_t material_index) {
            return material_index >= 4U && material_index <= 8U;
        }

        [[nodiscard]] bool texture_needs_blending(const DecodedTexture& texture) {
            for (auto offset = std::size_t{3U}; offset < texture.rgba.size(); offset += 4U) {
                if (texture.rgba[offset] != 255U) {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] const J3dTevStageSummary* find_tev_stage(const J3dMaterialSummary& material, std::uint8_t stage_index) {
            const auto it = std::ranges::find_if(material.tev_stages, [stage_index](const auto& stage) { return stage.stage == stage_index; });
            return it == material.tev_stages.end() ? nullptr : &*it;
        }

        [[nodiscard]] bool tev_stage_accumulates_color(const J3dTevStageSummary* stage) {
            if (stage == nullptr || stage->color_op != 0U || stage->color_bias != 0U || stage->color_out != 0U) {
                return false;
            }

            constexpr auto gx_cc_cprev = 0U;
            constexpr auto gx_cc_konst = 14U;
            return stage->color_in[3U] == gx_cc_cprev || stage->color_in[3U] == gx_cc_konst;
        }

        [[nodiscard]] bool title_sky_debug_uses_gx_blend_mode() {
            const auto* value = std::getenv("SMGPC_J3D_GX_BLEND");
            return value != nullptr && std::string_view(value) == "1";
        }

        [[nodiscard]] render::BlendMode blend_mode_for_material_pass(const J3dMaterialSummary& material, bool texture_has_alpha,
                                                                     const J3dTevStageSummary* stage, bool is_overlay_pass) {
            if (title_sky_debug_uses_gx_blend_mode() && material.blend.enabled) {
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

        [[nodiscard]] bool token_matches_material(std::string_view token, const std::string& material_name, std::uint16_t material_index) {
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

            auto index_text = std::to_string(material_index);
            return token == index_text;
        }

        [[nodiscard]] bool title_sky_debug_allows_mesh(const std::string& material_name, std::uint16_t material_index) {
            const auto* filter = std::getenv("SMGPC_TITLE_SKY_MATERIAL_FILTER");
            if (filter == nullptr || *filter == '\0') {
                return true;
            }

            auto value = std::string_view(filter);
            while (!value.empty()) {
                const auto comma = value.find(',');
                const auto token = comma == std::string_view::npos ? value : value.substr(0U, comma);
                if (token_matches_material(token, material_name, material_index)) {
                    return true;
                }
                if (comma == std::string_view::npos) {
                    break;
                }
                value.remove_prefix(comma + 1U);
            }

            return false;
        }

        [[nodiscard]] bool title_sky_debug_uses_cpu_tev() {
            const auto* value = std::getenv("SMGPC_J3D_CPU_TEV");
            return value != nullptr && std::string_view(value) == "1";
        }

        struct ProjectedVertex {
            float x = 0.0F;
            float y = 0.0F;
            float z = 0.0F;
            float u = 0.0F;
            float v = 0.0F;
            std::array< float, 4U > color{255.0F, 255.0F, 255.0F, 255.0F};
        };

        [[nodiscard]] J3dJointTransformValue joint_transform_from_summary(const J3dJointSummary& joint) {
            return J3dJointTransformValue{
                .scale = joint.scale,
                .rotation = joint.rotation,
                .translation = joint.translation,
            };
        }

        [[nodiscard]] J3dMatrix3x4 matrix_from_joint_transform(const J3dJointTransformValue* transform) {
            if (transform == nullptr) {
                return J3dMatrix3x4{};
            }

            constexpr auto pi = 3.14159265358979323846F;
            const auto rx = static_cast< float >(transform->rotation[0U]) * pi / 32768.0F;
            const auto ry = static_cast< float >(transform->rotation[1U]) * pi / 32768.0F;
            const auto rz = static_cast< float >(transform->rotation[2U]) * pi / 32768.0F;
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

        [[nodiscard]] J3dMatrix3x4 concat_matrix(const J3dMatrix3x4& left, const J3dMatrix3x4& right) {
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

        [[nodiscard]] std::array< float, 3U > transform_point(const J3dMatrix3x4& matrix, std::array< float, 3U > position) {
            return {
                matrix.m[0U] * position[0U] + matrix.m[1U] * position[1U] + matrix.m[2U] * position[2U] + matrix.m[3U],
                matrix.m[4U] * position[0U] + matrix.m[5U] * position[1U] + matrix.m[6U] * position[2U] + matrix.m[7U],
                matrix.m[8U] * position[0U] + matrix.m[9U] * position[1U] + matrix.m[10U] * position[2U] + matrix.m[11U],
            };
        }

        [[nodiscard]] std::array< float, 4U > color_to_float(std::array< std::uint8_t, 4U > color) {
            return {
                static_cast< float >(color[0U]),
                static_cast< float >(color[1U]),
                static_cast< float >(color[2U]),
                static_cast< float >(color[3U]),
            };
        }

        [[nodiscard]] std::array< std::uint8_t, 4U > color_to_u8(std::array< float, 4U > color) {
            return {
                static_cast< std::uint8_t >(std::clamp(color[0U], 0.0F, 255.0F)),
                static_cast< std::uint8_t >(std::clamp(color[1U], 0.0F, 255.0F)),
                static_cast< std::uint8_t >(std::clamp(color[2U], 0.0F, 255.0F)),
                static_cast< std::uint8_t >(std::clamp(color[3U], 0.0F, 255.0F)),
            };
        }

        [[nodiscard]] ProjectedVertex interpolate_vertex(const ProjectedVertex& a, const ProjectedVertex& b, float t) {
            const auto mix = [t](float left, float right) { return left + (right - left) * t; };

            auto color = std::array< float, 4U >{};
            for (auto i = 0U; i < color.size(); ++i) {
                color[i] = mix(a.color[i], b.color[i]);
            }

            return ProjectedVertex{
                .x = mix(a.x, b.x),
                .y = mix(a.y, b.y),
                .z = mix(a.z, b.z),
                .u = mix(a.u, b.u),
                .v = mix(a.v, b.v),
                .color = color,
            };
        }

        [[nodiscard]] std::vector< ProjectedVertex > clip_to_near_plane(std::span< const ProjectedVertex > polygon, float near_plane) {
            auto clipped = std::vector< ProjectedVertex >{};
            if (polygon.empty()) {
                return clipped;
            }

            auto previous = polygon.back();
            auto previous_inside = previous.z >= near_plane;
            for (const auto& current : polygon) {
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

        [[nodiscard]] ProjectedVertex file_select_sky_view_vertex(const J3dMeshVertex& source, std::array< std::uint8_t, 4U > material_color,
                                                                  const J3dMatrix3x4& model_matrix, const CameraPoseCompat& camera_pose,
                                                                  const J3dTexCoordGenSummary* tex_coord_gen, const J3dTexMatrixSummary* tex_matrix) {
            const auto world = transform_point(model_matrix, {source.x, source.y, source.z});
            const auto camera = transform_world_to_camera(camera_pose, {world[0U], world[1U], world[2U]});
            const auto tex_coord = j3d_transform_tex_coord(source, tex_coord_gen, tex_matrix, &model_matrix);

            return ProjectedVertex{
                .x = camera.x,
                .y = camera.y,
                .z = camera.z,
                .u = tex_coord.u,
                .v = tex_coord.v,
                .color = color_to_float(modulate_color(source.color, material_color)),
            };
        }

        void append_projected_triangle(std::vector< render::TexturedVertex2D >& vertices, std::vector< std::uint16_t >& indices,
                                       const ProjectedVertex& a, const ProjectedVertex& b, const ProjectedVertex& c,
                                       const render::core::FramebufferInfo& framebuffer, const CameraPoseCompat& camera_pose) {
            constexpr auto pi = 3.14159265358979323846F;
            const auto fovy = camera_pose.fovy_degrees * pi / 180.0F;
            const auto focal_y = 1.0F / std::tan(fovy * 0.5F);
            const auto aspect = camera_pose.aspect_ratio;
            const auto half_width = static_cast< float >(framebuffer.width) * 0.5F;
            const auto half_height = static_cast< float >(framebuffer.height) * 0.5F;
            const auto near_plane = camera_pose.near_clip;
            const auto far_plane = camera_pose.far_clip;

            const auto append_vertex = [&](const ProjectedVertex& projected) {
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

            if (vertices.size() + 3U > std::numeric_limits< std::uint16_t >::max()) {
                return;
            }

            const auto first = static_cast< std::uint16_t >(vertices.size());
            append_vertex(a);
            append_vertex(b);
            append_vertex(c);
            indices.push_back(first);
            indices.push_back(static_cast< std::uint16_t >(first + 1U));
            indices.push_back(static_cast< std::uint16_t >(first + 2U));
        }

        void project_source_mesh(std::span< const J3dMeshVertex > source_vertices, std::span< const std::uint16_t > source_indices,
                                 std::array< std::uint8_t, 4U > material_color, const render::core::FramebufferInfo& framebuffer,
                                 const J3dMatrix3x4& model_matrix, const CameraPoseCompat& camera_pose, const J3dTexCoordGenSummary* tex_coord_gen,
                                 const J3dTexMatrixSummary* tex_matrix, std::vector< render::TexturedVertex2D >& vertices,
                                 std::vector< std::uint16_t >& indices) {
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

                const std::array< ProjectedVertex, 3U > triangle{
                    file_select_sky_view_vertex(source_vertices[index_a], material_color, model_matrix, camera_pose, tex_coord_gen, tex_matrix),
                    file_select_sky_view_vertex(source_vertices[index_b], material_color, model_matrix, camera_pose, tex_coord_gen, tex_matrix),
                    file_select_sky_view_vertex(source_vertices[index_c], material_color, model_matrix, camera_pose, tex_coord_gen, tex_matrix),
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

    }  // namespace

    void TitleBackground::draw(render::IRendererEngine& renderer) {
        draw(renderer, _frame);
        ++_frame;
    }

    void TitleBackground::draw(render::IRendererEngine& renderer, std::uint64_t file_select_sky_step) {
        ensure_loaded(renderer);
        if (_loaded_geometry) {
            for (const auto& mesh : _meshes) {
                submit_mesh(renderer, mesh, file_select_sky_step);
            }
            return;
        }

        for (const auto& layer : TITLE_SKY_LAYERS) {
            submit_layer(renderer, layer);
        }
    }

    void TitleBackground::ensure_loaded(render::IRendererEngine& renderer) {
        if (_load_attempted) {
            return;
        }
        _load_attempted = true;

        auto* runtime = RuntimeContext::try_instance();
        if (runtime == nullptr) {
            return;
        }

        const auto archive_path = runtime->find_object_archive("CometNearOrbitSky");
        if (!archive_path.has_value()) {
            runtime->note_missing_object_archive("CometNearOrbitSky");
            return;
        }

        try {
            const auto archive = RarcArchive::from_file(*archive_path);
            const auto model = archive.file_data("cometnearorbitsky.bdl");
            if (archive.contains("cometnearorbitsky.bck")) {
                _bck_animation = inspect_j3d_animation(archive.file_data("cometnearorbitsky.bck")).bck;
            }
            if (archive.contains("cometnearorbitsky.btk")) {
                _btk_animation = inspect_j3d_animation(archive.file_data("cometnearorbitsky.btk")).btk;
            }

            const auto geometry = extract_j3d_model_geometry(model);
            auto texture_handles = std::vector< render::TextureHandle >{};
            texture_handles.reserve(geometry.textures.size());
            for (const auto& texture : geometry.textures) {
                auto handle = renderer.create_rgba8_texture(texture.image.width, texture.image.height, texture.image.rgba);
                texture_handles.push_back(handle);
                if (!handle.is_valid()) {
                    continue;
                }

                _textures.push_back(Texture{
                    .name = texture.name,
                    .handle = handle,
                });
            }

            if (geometry.materials.has_value()) {
                if (geometry.materials->materials.size() > 7U) {
                    _meshes.push_back(make_constant_backdrop(renderer, geometry.materials->materials[7U].tev_k_colors[0U]));
                }

                for (const auto& shape : geometry.shapes) {
                    if (shape.material_index == 0xffffU || shape.material_index >= geometry.materials->materials.size()) {
                        continue;
                    }
                    if (!is_title_sky_material(shape.material_index)) {
                        continue;
                    }

                    const auto& material = geometry.materials->materials[shape.material_index];
                    const auto passes = j3d_material_texture_passes(material);
                    if (passes.empty()) {
                        continue;
                    }

                    for (auto pass_index = std::size_t{}; pass_index < passes.size(); ++pass_index) {
                        const auto& pass = passes[pass_index];
                        const auto texture_index = pass.texture_index;
                        if (texture_index >= texture_handles.size() || !texture_handles[texture_index].is_valid()) {
                            continue;
                        }

                        auto texture_handle = texture_handles[texture_index];
                        auto material_color = material.material_colors[0U];
                        auto texture_has_alpha = texture_needs_blending(geometry.textures[texture_index].image);
                        if (passes.size() == 1U && title_sky_debug_uses_cpu_tev()) {
                            const auto composed_texture =
                                j3d_try_compose_material_texture(material, geometry.textures[texture_index].image, material_color, pass.tex_map_slot);
                            if (composed_texture.has_value()) {
                                const auto composed_handle = renderer.create_rgba8_texture(
                                    composed_texture->image.width, composed_texture->image.height, composed_texture->image.rgba);
                                if (composed_handle.is_valid()) {
                                    texture_handle = composed_handle;
                                    texture_has_alpha = texture_needs_blending(composed_texture->image);
                                    if (composed_texture->raster_color_baked) {
                                        material_color = {255U, 255U, 255U, 255U};
                                    }
                                }
                            }
                        }

                        const auto* tev_stage = find_tev_stage(material, pass.stage);
                        auto mesh = Mesh{};
                        mesh.material_name = material.name;
                        mesh.shape_index = shape.shape_index;
                        mesh.shape_draw_order = shape.draw_order;
                        mesh.material_index = shape.material_index;
                        mesh.joint_index = shape.joint_index;
                        mesh.texture = texture_handle;
                        if (geometry.joints.has_value() && shape.joint_index < geometry.joints->joints.size()) {
                            mesh.joint_transform = joint_transform_from_summary(geometry.joints->joints[shape.joint_index]);
                        }
                        mesh.tex_coord_gen = pass.tex_coord_gen;
                        mesh.tex_matrix = pass.tex_matrix;
                        mesh.material_color = material_color;
                        mesh.pass_order = pass.stage;
                        mesh.wrap_u = geometry.textures[texture_index].wrap_s != 0U;
                        mesh.wrap_v = geometry.textures[texture_index].wrap_t != 0U;
                        mesh.blend_mode = blend_mode_for_material_pass(material, texture_has_alpha, tev_stage, pass_index != 0U);
                        mesh.blend = mesh.blend_mode != render::BlendMode::Opaque;
                        if (material.z_mode.enabled) {
                            mesh.depth_test = material.z_mode.compare_enable != 0U;
                            mesh.depth_write = material.z_mode.update_enable != 0U;
                            mesh.depth_compare = depth_compare_from_gx(material.z_mode.function);
                        }
                        mesh.project_source_vertices = true;
                        mesh.source_vertices = shape.vertices;
                        mesh.source_indices = shape.indices;
                        if (!mesh.source_vertices.empty() && !mesh.source_indices.empty()) {
                            _meshes.push_back(std::move(mesh));
                        }
                    }
                }
            }

            std::ranges::sort(_meshes, [](const auto& a, const auto& b) {
                if (a.material_index == 0xfffeU || b.material_index == 0xfffeU) {
                    return a.material_index == 0xfffeU && b.material_index != 0xfffeU;
                }
                if (a.shape_draw_order != b.shape_draw_order) {
                    return a.shape_draw_order < b.shape_draw_order;
                }

                return a.pass_order < b.pass_order;
            });

            _loaded_geometry = !_meshes.empty();
            runtime->note_object_archive("CometNearOrbitSky", *archive_path);
        } catch (const std::exception& e) {
            runtime->note_object_texture_decode_failed("CometNearOrbitSky", e.what());
        }
    }

    TitleBackground::Mesh TitleBackground::make_constant_backdrop(render::IRendererEngine& renderer, std::array< std::uint8_t, 4U > color) const {
        const std::array< std::uint8_t, 4U > white_pixel{255U, 255U, 255U, 255U};
        const auto texture = renderer.create_rgba8_texture(1U, 1U, std::span< const std::uint8_t >(white_pixel.data(), white_pixel.size()));
        const auto framebuffer = renderer.logical_framebuffer_size();
        const auto half_width = static_cast< float >(framebuffer.width) * 0.5F;
        const auto half_height = static_cast< float >(framebuffer.height) * 0.5F;

        auto mesh = Mesh{};
        mesh.material_index = 0xfffeU;
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

    void TitleBackground::submit_layer(render::IRendererEngine& renderer, const Layer& layer) {
        const auto texture = find_texture(layer.texture_name);
        if (!texture.has_value()) {
            return;
        }

        const auto half_width = layer.width * 0.5F;
        const auto half_height = layer.height * 0.5F;
        renderer.submit_textured_quad(*texture,
                                      render::core::TexturedQuad2D{
                                          .vertices =
                                              {
                                                  vertex(layer.center_x - half_width, layer.center_y - half_height, 0.0F, 0.0F, layer.color),
                                                  vertex(layer.center_x + half_width, layer.center_y - half_height, 1.0F, 0.0F, layer.color),
                                                  vertex(layer.center_x + half_width, layer.center_y + half_height, 1.0F, 1.0F, layer.color),
                                                  vertex(layer.center_x - half_width, layer.center_y + half_height, 0.0F, 1.0F, layer.color),
                                              },
                                          .wrap_u = layer.wrap_u,
                                          .wrap_v = layer.wrap_v,
                                      });
    }

    void TitleBackground::submit_mesh(render::IRendererEngine& renderer, const Mesh& mesh, std::uint64_t frame) const {
        if (!title_sky_debug_allows_mesh(mesh.material_name, mesh.material_index)) {
            return;
        }

        if (mesh.project_source_vertices) {
            auto vertices = std::vector< render::TexturedVertex2D >{};
            auto indices = std::vector< std::uint16_t >{};
            const auto* tex_coord_gen = mesh.tex_coord_gen.has_value() ? &*mesh.tex_coord_gen : nullptr;
            auto effective_joint_transform = mesh.joint_transform;
            if (_bck_animation.has_value() && mesh.joint_index != 0xffffU) {
                const auto transform = j3d_evaluate_bck_joint_transform(*_bck_animation, mesh.joint_index, static_cast< float >(frame));
                if (transform.has_value()) {
                    effective_joint_transform = *transform;
                }
            }
            auto effective_tex_matrix = mesh.tex_matrix;
            if (_btk_animation.has_value() && effective_tex_matrix.has_value()) {
                const auto srt =
                    j3d_evaluate_btk_texture_srt(*_btk_animation, mesh.material_name, effective_tex_matrix->slot, static_cast< float >(frame));
                if (srt.has_value()) {
                    effective_tex_matrix->center = srt->center;
                    effective_tex_matrix->scale_s = srt->scale_s;
                    effective_tex_matrix->scale_t = srt->scale_t;
                    effective_tex_matrix->rotation = srt->rotation;
                    effective_tex_matrix->translate_s = srt->translate_s;
                    effective_tex_matrix->translate_t = srt->translate_t;
                }
            }
            const auto* joint_transform = effective_joint_transform.has_value() ? &*effective_joint_transform : nullptr;
            const auto* tex_matrix = effective_tex_matrix.has_value() ? &*effective_tex_matrix : nullptr;
            const auto model_matrix = concat_matrix(file_select_sky_actor_matrix(frame), matrix_from_joint_transform(joint_transform));
            const auto camera_pose = file_select_title_camera_pose();
            project_source_mesh(mesh.source_vertices, mesh.source_indices, mesh.material_color, renderer.logical_framebuffer_size(), model_matrix,
                                camera_pose, tex_coord_gen, tex_matrix, vertices, indices);
            if (vertices.empty() || indices.empty()) {
                return;
            }

            renderer.submit_textured_triangles(mesh.texture,
                                               render::core::TexturedTriangleBatch2D{
                                                   .vertices = std::span< const render::TexturedVertex2D >(vertices.data(), vertices.size()),
                                                   .indices = std::span< const std::uint16_t >(indices.data(), indices.size()),
                                                   .wrap_u = mesh.wrap_u,
                                                   .wrap_v = mesh.wrap_v,
                                                   .blend = mesh.blend,
                                                   .blend_mode = mesh.blend_mode,
                                                   .depth_test = mesh.depth_test,
                                                   .depth_write = mesh.depth_write,
                                                   .depth_compare = mesh.depth_compare,
                                               });
            return;
        }

        renderer.submit_textured_triangles(mesh.texture,
                                           render::core::TexturedTriangleBatch2D{
                                               .vertices = std::span< const render::TexturedVertex2D >(mesh.vertices.data(), mesh.vertices.size()),
                                               .indices = std::span< const std::uint16_t >(mesh.indices.data(), mesh.indices.size()),
                                               .wrap_u = mesh.wrap_u,
                                               .wrap_v = mesh.wrap_v,
                                               .blend = mesh.blend,
                                               .blend_mode = mesh.blend_mode,
                                               .depth_test = mesh.depth_test,
                                               .depth_write = mesh.depth_write,
                                               .depth_compare = mesh.depth_compare,
                                           });
    }

    std::optional< render::TextureHandle > TitleBackground::find_texture(std::string_view name) const {
        const auto it = std::ranges::find_if(_textures, [name](const auto& texture) { return texture.name == name; });

        if (it == _textures.end()) {
            return std::nullopt;
        }

        return it->handle;
    }

}  // namespace smgpc::game
