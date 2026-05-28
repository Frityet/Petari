#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "resource/TplTexture.hpp"

namespace smgpc::compat {

    class RarcArchive;

    struct JpcTextureMetadata {
        std::uint16_t index = 0U;
        std::string name;
        TplTextureFormat format = TplTextureFormat::I4;
        std::uint16_t width = 0U;
        std::uint16_t height = 0U;
        std::uint8_t wrap_s = 0U;
        std::uint8_t wrap_t = 0U;
        std::uint8_t min_filter = 0U;
        std::uint8_t mag_filter = 0U;
        DecodedTexture image;
    };

    struct JpcVec3f {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
    };

    struct JpcVec3s {
        std::int16_t x = 0;
        std::int16_t y = 0;
        std::int16_t z = 0;
    };

    struct JpcDynamicsBlockMetadata {
        std::uint32_t flags = 0U;
        std::uint32_t resource_user_work = 0U;
        JpcVec3f emitter_scale{};
        JpcVec3f emitter_translation{};
        JpcVec3f emitter_direction{};
        float initial_velocity_omni = 0.0F;
        float initial_velocity_axis = 0.0F;
        float initial_velocity_random = 0.0F;
        float initial_velocity_direction = 0.0F;
        float spread = 0.0F;
        float initial_velocity_ratio = 0.0F;
        float rate = 0.0F;
        float rate_random = 0.0F;
        float lifetime_random = 0.0F;
        float volume_sweep = 0.0F;
        float volume_min_radius = 0.0F;
        float air_resistance = 0.0F;
        float moment = 0.0F;
        JpcVec3s emitter_rotation{};
        std::int16_t max_frame = 0;
        std::int16_t start_frame = 0;
        std::int16_t lifetime = 0;
        std::uint16_t volume_size = 0U;
        std::uint16_t div_number = 0U;
        std::uint8_t rate_step = 0U;
        std::uint8_t volume_type = 0U;
        bool fixed_density = false;
        bool fixed_interval = false;
        bool inherit_scale = false;
        bool follow_emitter = false;
        bool follow_emitter_child = false;
    };

    struct JpcBaseShapeMetadata {
        std::uint32_t flags = 0U;
        float base_size_x = 1.0F;
        float base_size_y = 1.0F;
        std::uint16_t blend_mode_config = 0U;
        std::uint8_t alpha_compare_config = 0U;
        std::uint8_t alpha_ref0 = 0U;
        std::uint8_t alpha_ref1 = 0U;
        std::uint8_t z_mode_config = 0U;
        std::uint8_t texture_flags = 0U;
        std::uint8_t texture_count = 0U;
        std::uint8_t texture_slot = 0U;
        std::uint8_t color_flags = 0U;
        std::int16_t color_animation_frame_max = 0;
        std::array<std::uint8_t, 4U> prm_color = {255U, 255U, 255U, 255U};
        std::array<std::uint8_t, 4U> env_color = {0U, 0U, 0U, 255U};
        std::uint8_t shape_type = 0U;
        std::uint8_t direction_type = 0U;
        std::uint8_t rotation_type = 0U;
        std::uint8_t base_plane_type = 0U;
        bool texture_coordinate_animation = false;
    };

    struct JpcChildShapeMetadata {
        std::uint32_t flags = 0U;
        float position_random = 0.0F;
        float base_velocity = 0.0F;
        float base_velocity_random = 0.0F;
        float velocity_inherit_rate = 0.0F;
        float gravity = 0.0F;
        float scale_x = 1.0F;
        float scale_y = 1.0F;
        float inherit_scale = 0.0F;
        float inherit_alpha = 0.0F;
        float inherit_rgb = 0.0F;
        std::array<std::uint8_t, 4U> prm_color = {255U, 255U, 255U, 255U};
        std::array<std::uint8_t, 4U> env_color = {0U, 0U, 0U, 255U};
        float timing = 0.0F;
        std::int16_t lifetime = 0;
        std::int16_t rate = 0;
        std::uint8_t step = 0U;
        std::uint8_t texture_slot = 0U;
        std::int16_t rotation_speed = 0;
        std::uint8_t shape_type = 0U;
        std::uint8_t direction_type = 0U;
        std::uint8_t rotation_type = 0U;
        std::uint8_t base_plane_type = 0U;
        bool scale_inherited = false;
        bool alpha_inherited = false;
        bool color_inherited = false;
        bool clip_enabled = false;
        bool field_affected = false;
        bool scale_out_enabled = false;
        bool alpha_out_enabled = false;
        bool rotate_enabled = false;
    };

    struct JpcKeyFrameMetadata {
        float time = 0.0F;
        float value = 0.0F;
        float tangent_in = 0.0F;
        float tangent_out = 0.0F;
    };

    struct JpcKeyBlockMetadata {
        std::uint8_t id = 0U;
        bool loop = false;
        std::vector<JpcKeyFrameMetadata> keys;
    };

    struct JpcResourceMetadata {
        std::uint16_t user_index = 0U;
        std::uint16_t block_count = 0U;
        std::uint8_t field_block_count = 0U;
        std::uint8_t key_block_count = 0U;
        std::uint8_t texture_reference_count = 0U;
        std::optional<JpcDynamicsBlockMetadata> dynamics;
        std::optional<JpcBaseShapeMetadata> base_shape;
        std::optional<JpcChildShapeMetadata> child_shape;
        std::optional<std::uint8_t> base_shape_texture_slot;
        std::optional<std::uint16_t> primary_texture_index;
        std::optional<std::uint16_t> child_texture_index;
        std::vector<std::string> block_tags;
        std::vector<JpcKeyBlockMetadata> key_blocks;
        std::vector<std::uint16_t> texture_indices;
    };

    struct AutoEffectInfoCompat {
        std::uint32_t row_index = 0U;
        std::string group_name;
        std::string unique_name;
        std::string anim_name;
        std::string continue_anim_end;
        std::string effect_name;
        std::string parent_name;
        std::string joint_name;
        float offset_x = 0.0F;
        float offset_y = 0.0F;
        float offset_z = 0.0F;
        std::int32_t start_frame = 0;
        std::int32_t end_frame = -1;
        std::string affect;
        std::string follow;
        float scale_value = 1.0F;
        float rate_value = 1.0F;
        std::string prm_color;
        std::string env_color;
        float light_affect_value = 0.0F;
        std::string draw_order;
    };

    struct ResolvedEffectResource {
        std::string requested_name;
        std::string particle_name;
        std::uint16_t user_index = 0U;
        std::string auto_effect_group_name;
        std::string auto_effect_unique_name;
        std::string auto_effect_parent_name;
        std::string auto_effect_joint_name;
        std::string auto_effect_draw_order;
        float auto_effect_offset_x = 0.0F;
        float auto_effect_offset_y = 0.0F;
        float auto_effect_offset_z = 0.0F;
        float auto_effect_scale_value = 1.0F;
        float auto_effect_rate_value = 1.0F;
        std::optional<std::uint16_t> primary_texture_index;
        const JpcResourceMetadata *resource = nullptr;
        std::vector<JpcTextureMetadata> textures;
    };

    class EffectResourceLibrary final {
    public:
        [[nodiscard]] static EffectResourceLibrary from_archive(const RarcArchive &archive);

        [[nodiscard]] std::size_t particle_name_count() const;
        [[nodiscard]] std::size_t auto_effect_count() const;
        [[nodiscard]] std::size_t resource_count() const;
        [[nodiscard]] std::size_t texture_count() const;

        [[nodiscard]] std::optional<std::uint16_t> find_particle_user_index(std::string_view name) const;
        [[nodiscard]] std::optional<std::string_view> particle_name_for_user_index(std::uint16_t user_index) const;
        [[nodiscard]] const JpcResourceMetadata *resource_by_user_index(std::uint16_t user_index) const;
        [[nodiscard]] const JpcTextureMetadata *texture(std::uint16_t texture_index) const;
        [[nodiscard]] std::vector<ResolvedEffectResource> resolve_particle_name(std::string_view name) const;
        [[nodiscard]] std::vector<ResolvedEffectResource> resolve_auto_effect(std::string_view group_name, std::string_view unique_name) const;
        [[nodiscard]] std::vector<ResolvedEffectResource> resolve_effect_request(std::string_view effect_name) const;

    private:
        void parse_particle_names(std::span<const std::uint8_t> data);
        void parse_auto_effect_list(std::span<const std::uint8_t> data);
        void parse_jpc(std::span<const std::uint8_t> data);
        void append_particle_resolution(std::vector<ResolvedEffectResource> &out, std::string_view requested_name,
                                        std::string_view particle_name, const AutoEffectInfoCompat *auto_effect) const;
        [[nodiscard]] std::vector<ResolvedEffectResource> resolve_particle_token(std::string_view requested_name,
                                                                                 const AutoEffectInfoCompat *auto_effect) const;

        std::map<std::string, std::uint16_t, std::less<>> _particle_name_to_user_index;
        std::map<std::uint16_t, std::string> _particle_name_by_user_index;
        std::vector<AutoEffectInfoCompat> _auto_effects;
        std::map<std::string, std::vector<std::size_t>, std::less<>> _auto_effects_by_unique_name;
        std::map<std::string, std::vector<std::size_t>, std::less<>> _auto_effects_by_group_unique_name;
        std::vector<JpcResourceMetadata> _resources;
        std::map<std::uint16_t, std::size_t> _resource_index_by_user_index;
        std::vector<JpcTextureMetadata> _textures;
    };

}  // namespace smgpc::compat
