#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "RendererService.hpp"
#include "camera/CameraPose.hpp"
#include "render/J3dAnimation.hpp"
#include "render/J3dMaterialRuntime.hpp"

namespace smgpc::render {

    struct J3dModelRendererLoadOptions {
        std::uint16_t min_material_index = 0U;
        std::uint16_t max_material_index = 0xffffU;
        std::optional<std::uint16_t> constant_backdrop_material_index = {};
        bool use_cpu_tev = true;
        bool use_gx_blend_mode = false;
    };

    struct J3dModelRendererDrawOptions {
        std::string_view material_filter = {};
        std::optional<bool> translucent_filter = {};
        std::optional<bool> gx_color_update = {};
        std::optional<bool> gx_alpha_update = {};
        std::span<const GXLightState> scene_lights = {};
        std::optional<J3dMatrix3x4> projmap_effect_matrix = {};
    };

    enum class J3dRendererPacketMode {
        ConstantBackdrop,
        ConstantMaterial,
        ComposedMaterial,
        CpuTevPerVertex,
        ShaderGxTev,
        TexturePass,
    };

    struct J3dRendererTextureState {
        std::uint8_t slot = 0U;
        std::uint16_t texture_index = 0xffffU;
        std::uint8_t wrap_s = 0xffU;
        std::uint8_t wrap_t = 0xffU;
        std::uint8_t min_filter = 0xffU;
        std::uint8_t mag_filter = 0xffU;
        std::string name;
        std::uint16_t width = 0U;
        std::uint16_t height = 0U;
        smgpc::resource::TplTextureFormat format = smgpc::resource::TplTextureFormat::I4;
        bool has_source_texture = false;
        bool has_sampler_metadata = false;
        std::uint8_t transparency = 0U;
        std::uint8_t palette_format = 0U;
        std::uint16_t palette_entry_count = 0U;
        std::uint32_t palette_data_offset = 0U;
        bool mipmap = false;
        bool do_edge_lod = false;
        bool bias_clamp = false;
        std::uint8_t max_anisotropy = 0U;
        std::uint8_t min_lod = 0U;
        std::uint8_t max_lod = 0U;
        std::uint8_t image_count = 0U;
        std::int16_t lod_bias = 0;
        std::uint32_t image_data_offset = 0U;
        render::TextureHandle host_handle = {};
    };

    // TODO: Split up
    struct J3dRendererPacketState {
        std::string material_name;
        std::uint16_t shape_index = 0xffffU;
        std::uint16_t shape_draw_order = 0xffffU;
        std::uint16_t material_index = 0xffffU;
        std::uint8_t material_mode = 0U;
        bool draw_buffer_opaque = true;
        std::uint16_t joint_index = 0xffffU;
        std::uint16_t matrix_group_index = 0xffffU;
        std::uint16_t matrix_group_count = 0U;
        std::uint16_t use_matrix_index = 0xffffU;
        std::uint16_t use_matrix_count = 0U;
        std::uint32_t first_matrix_table_index = 0U;
        std::size_t matrix_table_count = 0U;
        std::uint32_t display_list_offset = 0U;
        std::uint32_t display_list_size = 0U;
        std::uint32_t parsed_display_list_bytes = 0U;
        std::uint32_t draw_packet_triangle_count = 0U;
        std::uint8_t pass_order = 0U;
        J3dRendererPacketMode packet_mode = J3dRendererPacketMode::TexturePass;
        std::string packet_mode_reason;
        bool packet_mode_fallback = false;
        std::size_t material_pass_count = 0U;
        std::size_t shader_texture_stage_count = 0U;
        std::uint8_t color_channel_count = 0U;
        std::array<GXColorValue, 2U> color_channel_material_colors = {};
        std::array<GXColorValue, 2U> color_channel_ambient_colors = {};
        std::array<GXColorChannelControlState, 2U> color_channel_controls = {};
        std::array<GXColorChannelControlState, 2U> alpha_channel_controls = {};
        std::uint8_t loaded_light_mask = 0U;
        std::uint8_t material_loaded_light_mask = 0U;
        std::uint8_t scene_loaded_light_mask = 0U;
        std::uint8_t requested_light_mask = 0U;
        std::uint8_t unsatisfied_light_mask = 0U;
        std::array<GXLightState, 8U> lights = {};
        std::vector<J3dRendererTextureState> texture_bindings = {};
        std::vector<GXTexCoordGenState> tex_coord_gens = {};
        std::array<GXTexCoordScaleState, 8U> tex_coord_scales = {};
        GXSULinePointState su_line_point = {};
        std::vector<GXTexMatrixState> tex_matrices = {};
        std::vector<GXTevOrderState> tev_orders = {};
        std::vector<GXTevStageState> tev_stages = {};
        std::array<GXColorValue, 4U> tev_k_colors = {};
        GXIndirectState indirect = {};
        std::uint8_t declared_tev_stage_count = 0U;
        std::size_t active_tev_stage_count = 0U;
        std::size_t tev_order_count = 0U;
        std::size_t tev_stage_count = 0U;
        std::size_t texgen_count = 0U;
        std::uint8_t indirect_stage_count = 0U;
        std::size_t active_indirect_tev_stage_count = 0U;
        std::size_t indirect_matrix_count = 0U;
        std::size_t indirect_texture_order_count = 0U;
        std::size_t indirect_texture_scale_count = 0U;
        std::size_t mdl3_packet_bytes = 0U;
        std::uint32_t mdl3_command_count = 0U;
        std::uint32_t mdl3_bp_load_count = 0U;
        std::uint32_t mdl3_xf_load_count = 0U;
        std::size_t source_vertex_count = 0U;
        std::size_t source_triangle_count = 0U;
        bool project_source_vertices = false;
        bool evaluate_material_per_vertex = false;
        bool blend = false;
        render::BlendMode blend_mode = render::BlendMode::Alpha;
        render::GxBlendMode2D gx_blend = {};
        render::GxAlphaCompare2D gx_alpha_compare = {};
        GXZModeState gx_z_mode = {};
        GXFogState gx_fog = {};
        std::array<render::GxTevRegisterColor2D, 4U> gx_initial_tev_registers = {};
        std::vector<GXRegisterLoadState> mdl3_register_loads = {};
        bool depth_test = false;
        bool depth_write = false;
        render::DepthCompare depth_compare = render::DepthCompare::LessEqual;
        render::CullMode cull_mode = render::CullMode::None;
        bool fog_enabled = false;
        std::uint8_t fog_type = 0U;
        std::uint8_t fog_projection = 0U;
        bool fog_range_adjust_enabled = false;
        std::array<std::uint8_t, 4U> fog_color = {};
        bool bck_active = false;
        float bck_frame = 0.0F;
        float bck_normalized_frame = 0.0F;
        std::int16_t bck_frame_max = 0;
        std::uint16_t bck_joint_count = 0U;
        bool btk_active = false;
        float btk_frame = 0.0F;
        float btk_normalized_frame = 0.0F;
        std::int16_t btk_frame_max = 0;
        std::uint16_t btk_material_count = 0U;
    };

    class J3dModelRenderer final {
    public:
        J3dModelRenderer();
        ~J3dModelRenderer();

        void load(render::IRendererEngine &renderer, std::span<const std::uint8_t> model_data, const J3dModelRendererLoadOptions &options = {});
        void set_bck_animation(const J3dBckAnimationSummary &animation);
        void set_btk_animation(const J3dBtkAnimationSummary &animation);
        void clear_animations();

        void draw(render::IRendererEngine &renderer, const smgpc::camera::CameraPose &camera_pose, const J3dMatrix3x4 &actor_matrix, std::uint64_t frame,
                  const J3dModelRendererDrawOptions &options = {}) const;

        [[nodiscard]] bool is_loaded() const;
        [[nodiscard]] std::size_t mesh_count() const;
        [[nodiscard]] std::span<const J3dRendererPacketState> render_packets() const;
        [[nodiscard]] std::vector<J3dRendererPacketState> render_packets(std::uint64_t frame,
                                                                         std::span<const GXLightState> scene_lights = {},
                                                                         const J3dModelRendererDrawOptions &options = {}) const;

    private:
        struct Mesh {
            std::string material_name;
            std::uint16_t shape_index = 0xffffU;
            std::uint16_t shape_draw_order = 0xffffU;
            std::uint16_t material_index = 0xffffU;
            std::uint8_t material_mode = 0U;
            bool draw_buffer_opaque = true;
            std::uint16_t joint_index = 0xffffU;
            std::uint16_t matrix_group_count = 0U;
            J3dShapeMatrixGroupSummary matrix_group = {};
            render::TextureHandle texture = {};
            std::vector<render::TexturedVertex2D> vertices = {};
            std::vector<std::uint16_t> indices = {};
            std::vector<J3dMeshVertex> source_vertices = {};
            std::vector<std::uint16_t> source_indices = {};
            std::optional<J3dJointTransformValue> joint_transform = {};
            std::optional<J3dTexCoordGenSummary> tex_coord_gen = {};
            std::optional<J3dTexMatrixSummary> tex_matrix = {};
            std::optional<J3dMaterialSummary> material = {};
            std::vector<J3dMaterialTexturePass> material_passes = {};
            std::array<render::GxTextureStage2D, render::core::kMaxGxMaterialTextureStages2D> gx_texture_stages = {};
            std::array<render::GxTevStage2D, render::core::kMaxGxMaterialTevStages2D> gx_tev_stages = {};
            std::size_t gx_texture_stage_count = 0U;
            std::size_t gx_tev_stage_count = 0U;
            J3dRendererPacketMode packet_mode = J3dRendererPacketMode::TexturePass;
            std::string packet_mode_reason;
            bool packet_mode_fallback = false;
            std::array<std::uint8_t, 4U> material_color{255U, 255U, 255U, 255U};
            std::uint8_t pass_order = 0U;
            std::uint8_t wrap_u = 0U;
            std::uint8_t wrap_v = 0U;
            std::uint8_t min_filter = 1U;
            std::uint8_t mag_filter = 1U;
            bool blend = true;
            render::BlendMode blend_mode = render::BlendMode::Alpha;
            render::GxBlendMode2D gx_blend = {};
            render::GxAlphaCompare2D gx_alpha_compare = {};
            std::array<render::GxTevRegisterColor2D, 4U> gx_initial_tev_registers = {};
            bool depth_test = false;
            bool depth_write = false;
            render::DepthCompare depth_compare = render::DepthCompare::LessEqual;
            render::CullMode cull_mode = render::CullMode::None;
            bool project_source_vertices = false;
            bool evaluate_material_per_vertex = false;
        };

        struct DrawScratch;
        struct DrawScratchDeleter {
            void operator()(DrawScratch *scratch) const;
        };

        [[nodiscard]] Mesh make_constant_backdrop(render::IRendererEngine &renderer, std::array<std::uint8_t, 4U> color) const;
        [[nodiscard]] J3dRendererPacketState packet_state_for_mesh(const Mesh &mesh, std::span<const GXLightState> scene_lights = {}) const;
        [[nodiscard]] J3dRendererPacketState packet_state_for_mesh(const Mesh &mesh, std::uint64_t frame,
                                                                   std::span<const GXLightState> scene_lights = {}) const;
        void submit_mesh(render::IRendererEngine &renderer, const Mesh &mesh, const J3dMatrix3x4 &actor_matrix, std::uint64_t frame,
                         DrawScratch &scratch, std::span<const GXLightState> scene_lights,
                         const J3dModelRendererDrawOptions &options) const;

        bool _loaded = false;
        std::vector<J3dTexture> _textures = {};
        std::vector<render::TextureHandle> _texture_handles = {};
        std::vector<J3dJointTransformValue> _joint_transforms = {};
        std::vector<std::uint16_t> _joint_parent_indices = {};
        std::vector<J3dDrawMatrixSummary> _draw_matrices = {};
        std::optional<J3dEnvelopeBlockSummary> _envelopes = {};
        std::vector<Mesh> _meshes = {};
        std::vector<J3dRendererPacketState> _render_packets = {};
        std::optional<J3dBckAnimationSummary> _bck_animation = {};
        std::optional<J3dBtkAnimationSummary> _btk_animation = {};
        mutable std::unique_ptr<DrawScratch, DrawScratchDeleter> _draw_scratch = {};
    };

    [[nodiscard]] J3dMatrix3x4 j3d_matrix_from_translation_scale(const smgpc::camera::CameraParamVec3 &translation, float scale);

}  // namespace smgpc::render
