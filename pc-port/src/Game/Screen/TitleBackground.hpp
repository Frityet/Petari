#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Game/compat/J3dAnimation.hpp"
#include "Game/compat/J3dModel.hpp"
#include "RendererService.hpp"

namespace smgpc::game {

    class TitleBackground final {
    public:
        struct Layer {
            std::string_view texture_name;
            std::array< std::uint8_t, 4U > color{255U, 255U, 255U, 255U};
            float center_x = 0.0F;
            float center_y = 0.0F;
            float width = 0.0F;
            float height = 0.0F;
            bool wrap_u = false;
            bool wrap_v = false;
        };

        void draw(render::IRendererEngine& renderer);
        void draw(render::IRendererEngine& renderer, std::uint64_t file_select_sky_step);

    private:
        struct Texture {
            std::string name;
            render::TextureHandle handle{};
        };

        struct Mesh {
            std::string material_name;
            std::uint16_t shape_index = 0xffffU;
            std::uint16_t shape_draw_order = 0xffffU;
            std::uint16_t material_index = 0xffffU;
            std::uint16_t joint_index = 0xffffU;
            render::TextureHandle texture{};
            std::vector< render::TexturedVertex2D > vertices{};
            std::vector< std::uint16_t > indices{};
            std::vector< J3dMeshVertex > source_vertices{};
            std::vector< std::uint16_t > source_indices{};
            std::optional< J3dJointTransformValue > joint_transform{};
            std::optional< J3dTexCoordGenSummary > tex_coord_gen{};
            std::optional< J3dTexMatrixSummary > tex_matrix{};
            std::array< std::uint8_t, 4U > material_color{255U, 255U, 255U, 255U};
            std::uint8_t pass_order = 0U;
            bool wrap_u = false;
            bool wrap_v = false;
            bool blend = true;
            render::BlendMode blend_mode = render::BlendMode::Alpha;
            bool depth_test = false;
            bool depth_write = false;
            render::DepthCompare depth_compare = render::DepthCompare::LessEqual;
            bool project_source_vertices = false;
        };

        void ensure_loaded(render::IRendererEngine& renderer);
        [[nodiscard]] Mesh make_constant_backdrop(render::IRendererEngine& renderer, std::array< std::uint8_t, 4U > color) const;
        void submit_layer(render::IRendererEngine& renderer, const Layer& layer);
        void submit_mesh(render::IRendererEngine& renderer, const Mesh& mesh, std::uint64_t frame) const;
        [[nodiscard]] std::optional< render::TextureHandle > find_texture(std::string_view name) const;

        bool _load_attempted = false;
        bool _loaded_geometry = false;
        std::uint64_t _frame = 0U;
        std::vector< Texture > _textures{};
        std::vector< Mesh > _meshes{};
        std::optional< J3dBckAnimationSummary > _bck_animation{};
        std::optional< J3dBtkAnimationSummary > _btk_animation{};
    };

}  // namespace smgpc::game
