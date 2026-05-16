#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <vector>

#include "AssetServices.hpp"
#include "layout/J3dModel.hpp"
#include "layout/LayoutDrawList.hpp"
#include "core/RenderCommandBuffer.hpp"

namespace smgpc::game::compat {

enum class FileSelectSkyCameraMode {
    Title,
    Far,
    Near,
};

class FileSelectSkyJ3d {
public:
    [[nodiscard]] static assets::AssetResult< FileSelectSkyJ3d > parse(std::span< const std::byte > bdlBytes,
                                                                        std::span< const std::byte > bckBytes = {},
                                                                        std::span< const std::byte > btkBytes = {});

    void appendDrawCommands(render::layout::LayoutDrawList* pDrawList, float frame, std::size_t selectedFileIndex = 0U,
                            bool useNearCamera = false) const;
    void appendDrawCommands(render::layout::LayoutDrawList* pDrawList, float frame, std::size_t selectedFileIndex,
                            FileSelectSkyCameraMode cameraMode) const;
    void appendJ3dDrawCommands(render::core::RenderCommandBuffer* pCommands, float frame, std::uint16_t framebufferWidth,
                               std::uint16_t framebufferHeight, std::size_t selectedFileIndex,
                               FileSelectSkyCameraMode cameraMode) const;

    [[nodiscard]] std::size_t triangleCount() const;
    [[nodiscard]] bool empty() const;

private:
    struct AnimationKey {
        float frame{};
        float value{};
        float tangent_in{};
        float tangent_out{};
    };

    struct AnimationTrack {
        bool valid{};
        std::vector< AnimationKey > keys{};
    };

    struct JointAnimation {
        std::array< AnimationTrack, 3U > scale{};
        std::array< AnimationTrack, 3U > rotation{};
        std::array< AnimationTrack, 3U > translation{};
    };

    struct TextureMatrixAnimation {
        std::size_t material_index{};
        std::uint8_t texture_matrix_index{};
        assets::layout::J3dVec3 center{0.5F, 0.5F, 0.5F};
        AnimationTrack scale_x{};
        AnimationTrack scale_y{};
        AnimationTrack rotation{};
        AnimationTrack translation_x{};
        AnimationTrack translation_y{};
    };

    struct ProjectedTriangle {
        render::layout::QuadCommand quad{};
        render::layout::TriangleTextureCombineMode secondary_texture_mode{render::layout::TriangleTextureCombineMode::None};
        std::uint8_t tev_stage_count{};
        std::array< render::layout::TriangleTevStage, 2U > tev_stages{};
        float depth{};
    };

    assets::layout::J3dModel _model{};
    std::vector< JointAnimation > _jointAnimations{};
    std::vector< TextureMatrixAnimation > _textureMatrixAnimations{};
    float _jointAnimationFrameMax{};
    float _textureMatrixAnimationFrameMax{};
};

}  // namespace smgpc::game::compat
