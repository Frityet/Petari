#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace smgpc::layout {

    struct BrlanPaneFrame {
        std::optional<float> translate_x;
        std::optional<float> translate_y;
        std::optional<float> scale_x;
        std::optional<float> scale_y;
        std::optional<float> rotate_z;
        std::optional<float> alpha;
        std::optional<bool> visible;
    };

    struct BrlanTextureFrame {
        std::optional<float> translate_s;
        std::optional<float> translate_t;
        std::optional<float> rotate;
        std::optional<float> scale_s;
        std::optional<float> scale_t;
    };

    struct BrlanAnimation {
        struct GroupRef {
            std::string name;
            std::uint8_t flag = 0U;
        };

        struct ShareInfo {
            std::string source_pane_name;
            std::string target_group_name;
        };

        struct StepKey {
            float frame = 0.0F;
            std::uint16_t value = 0U;
        };

        struct HermiteKey {
            float frame = 0.0F;
            float value = 0.0F;
            float slope = 0.0F;
        };

        struct Target {
            std::uint16_t target = 0U;
            std::uint8_t curve_type = 0U;
            std::vector<StepKey> step_keys;
            std::vector<HermiteKey> hermite_keys;
        };

        struct Info {
            std::string kind;
            std::vector<Target> targets;
        };

        struct Content {
            std::string name;
            std::vector<Info> infos;
        };

        std::uint16_t frame_size = 0U;
        bool loop = false;
        std::string tag_name;
        std::uint16_t tag_order = 0U;
        std::int16_t tag_start_frame = 0;
        std::int16_t tag_end_frame = 0;
        std::uint8_t tag_flag = 0U;
        std::vector<GroupRef> group_refs;
        std::vector<ShareInfo> share_infos;
        std::vector<Content> contents;

        [[nodiscard]] BrlanPaneFrame pane_frame(std::string_view pane_name, float frame) const;
        [[nodiscard]] BrlanTextureFrame texture_frame(std::string_view material_name, float frame) const;
    };

    [[nodiscard]] BrlanAnimation parse_brlan_animation(std::span<const std::uint8_t> data);

}  // namespace smgpc::layout
