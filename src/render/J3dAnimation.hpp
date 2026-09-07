#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class J3DAnmTransformKey;

namespace smgpc::render {

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
        // Shared immutable resource; each actor supplies its own playback frame
        // to the original sampler rather than changing the resource's mFrame.
        std::shared_ptr<const J3DAnmTransformKey> transform_animation;
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
        struct RegisterTrack {
            std::string material_name;
            std::uint16_t stored_material_id = 0xffffU;
            std::uint8_t register_id = 0xffU;
            std::array<J3dAnimationKeyTableSummary, 4U> channels{};
        };
        std::array<std::vector<std::int16_t>, 4U> color_values;
        std::array<std::vector<std::int16_t>, 4U> konst_values;
        std::vector<RegisterTrack> color_tracks;
        std::vector<RegisterTrack> konst_tracks;
    };

    struct J3dBtpMaterialAnimationSummary {
        std::string material_name;
        std::uint16_t material_id = 0xffffU;
        std::uint8_t texture_slot = 0xffU;
        std::uint16_t max_frame = 0U;
        std::uint16_t texture_index_offset = 0U;
    };

    struct J3dBtpAnimationSummary {
        std::uint8_t attribute = 0U;
        std::int16_t frame_max = 0;
        std::uint16_t material_count = 0U;
        std::uint16_t texture_index_count = 0U;
        std::vector<std::uint16_t> texture_indices;
        std::vector<J3dBtpMaterialAnimationSummary> materials;
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
        std::optional<J3dBtpAnimationSummary> btp;
    };

    [[nodiscard]] J3dAnimationSummary inspect_j3d_animation(std::span<const std::uint8_t> animation_data);
    [[nodiscard]] float j3d_animation_frame(std::uint8_t attribute, std::int16_t frame_max, float elapsed_frame);
    [[nodiscard]] bool j3d_animation_stopped(std::uint8_t attribute, std::int16_t frame_max, float elapsed_frame);
    [[nodiscard]] bool j3d_animation_check_pass(std::uint8_t attribute, std::int16_t frame_max, float elapsed_frame, float pass_frame);
    [[nodiscard]] std::optional<J3dJointTransformValue> j3d_evaluate_bck_joint_transform(const J3dBckAnimationSummary &bck,
                                                                                         std::uint16_t joint_index, float frame);
    [[nodiscard]] std::optional<J3dTextureSrtAnimationValue>
    j3d_evaluate_btk_texture_srt(const J3dBtkAnimationSummary &btk, std::string_view material_name, std::uint8_t tex_matrix_id, float frame);
    [[nodiscard]] std::optional<std::uint16_t>
    j3d_evaluate_btp_texture_index(const J3dBtpAnimationSummary &btp, std::string_view material_name, std::uint8_t texture_slot, float frame);
    [[nodiscard]] std::array<std::int16_t, 4U>
    j3d_evaluate_brk_color_track(const J3dBrkAnimationSummary &brk,
                                 std::size_t track_index, float raw_frame);
    [[nodiscard]] std::array<std::uint8_t, 4U>
    j3d_evaluate_brk_konst_track(const J3dBrkAnimationSummary &brk,
                                 std::size_t track_index, float raw_frame);

}  // namespace smgpc::render
