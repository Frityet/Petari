#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::game {

    struct J3dAnimationSectionInfo {
        std::string tag;
        std::uint32_t offset = 0U;
        std::uint32_t size = 0U;
    };

    struct J3dAnimationKeyTableSummary {
        std::uint16_t max_frame = 0U;
        std::uint16_t offset = 0U;
        std::uint16_t type = 0U;
    };

    struct J3dAnimationTransformTrackSummary {
        J3dAnimationKeyTableSummary scale;
        J3dAnimationKeyTableSummary rotation;
        J3dAnimationKeyTableSummary translation;
    };

    struct J3dBckAnimationSummary {
        std::uint8_t attribute = 0U;
        std::uint8_t rotation_fraction = 0U;
        std::int16_t frame_max = 0;
        std::uint16_t joint_count = 0U;
        std::uint16_t scale_count = 0U;
        std::uint16_t rotation_count = 0U;
        std::uint16_t translation_count = 0U;
        std::vector<float> scale_values;
        std::vector<std::int16_t> rotation_values;
        std::vector<float> translation_values;
        std::vector<std::array<J3dAnimationTransformTrackSummary, 3U>> joints;
    };

    struct J3dBtkMaterialAnimationSummary {
        std::string material_name;
        std::uint16_t material_id = 0xffffU;
        std::uint8_t tex_matrix_id = 0xffU;
        std::array<float, 3U> center{};
        std::array<J3dAnimationTransformTrackSummary, 3U> tracks{};
    };

    struct J3dBtkAnimationSummary {
        std::uint8_t attribute = 0U;
        std::uint8_t rotation_fraction = 0U;
        std::int16_t frame_max = 0;
        std::uint16_t track_count = 0U;
        std::uint16_t scale_count = 0U;
        std::uint16_t rotation_count = 0U;
        std::uint16_t translation_count = 0U;
        std::uint32_t tex_matrix_calc_type = 0U;
        std::vector<float> scale_values;
        std::vector<std::int16_t> rotation_values;
        std::vector<float> translation_values;
        std::vector<J3dBtkMaterialAnimationSummary> materials;
    };

    struct J3dBrkAnimationSummary {
        std::uint8_t attribute = 0U;
        std::int16_t frame_max = 0;
    };

    struct J3dTextureSrtAnimationValue {
        std::array<float, 3U> center{};
        float scale_s = 1.0F;
        float scale_t = 1.0F;
        std::int16_t rotation = 0;
        float translate_s = 0.0F;
        float translate_t = 0.0F;
    };

    struct J3dJointTransformValue {
        std::array<float, 3U> scale{1.0F, 1.0F, 1.0F};
        std::array<std::int16_t, 3U> rotation{};
        std::array<float, 3U> translation{};
    };

    struct J3dAnimationSummary {
        std::string type;
        std::uint32_t file_size = 0U;
        std::uint32_t block_count = 0U;
        std::vector<J3dAnimationSectionInfo> sections;
        std::optional<J3dBckAnimationSummary> bck;
        std::optional<J3dBtkAnimationSummary> btk;
        std::optional<J3dBrkAnimationSummary> brk;
    };

    [[nodiscard]] J3dAnimationSummary inspect_j3d_animation(std::span<const std::uint8_t> animation_data);
    [[nodiscard]] std::optional<J3dJointTransformValue> j3d_evaluate_bck_joint_transform(const J3dBckAnimationSummary &bck,
                                                                                         std::uint16_t joint_index, float frame);
    [[nodiscard]] std::optional<J3dTextureSrtAnimationValue>
    j3d_evaluate_btk_texture_srt(const J3dBtkAnimationSummary &btk, std::string_view material_name, std::uint8_t tex_matrix_id, float frame);

}  // namespace smgpc::game
