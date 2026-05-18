#include "Game/compat/EffectResourceCompat.hpp"

#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

#include "Game/compat/BcsvTable.hpp"
#include "Game/compat/RarcArchive.hpp"

namespace smgpc::game {
    namespace {
        constexpr auto JPAC_MAGIC = std::uint32_t{0x4a504143U};
        constexpr auto JPAC_VERSION_210 = std::uint32_t{0x322d3130U};
        constexpr auto BEM1_MAGIC = std::uint32_t{0x42454d31U};
        constexpr auto BSP1_MAGIC = std::uint32_t{0x42535031U};
        constexpr auto KFA1_MAGIC = std::uint32_t{0x4b464131U};
        constexpr auto SSP1_MAGIC = std::uint32_t{0x53535031U};
        constexpr auto TDB1_MAGIC = std::uint32_t{0x54444231U};

        [[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 2U > data.size()) {
                throw std::runtime_error("JPC read past end of buffer");
            }

            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8U) | static_cast<std::uint16_t>(data[offset + 1U]));
        }

        [[nodiscard]] std::int16_t read_be_s16(std::span<const std::uint8_t> data, std::size_t offset) {
            return static_cast<std::int16_t>(read_be16(data, offset));
        }

        [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 4U > data.size()) {
                throw std::runtime_error("JPC read past end of buffer");
            }

            return (static_cast<std::uint32_t>(data[offset]) << 24U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
                   (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) | static_cast<std::uint32_t>(data[offset + 3U]);
        }

        [[nodiscard]] float read_be_float(std::span<const std::uint8_t> data, std::size_t offset) {
            return std::bit_cast<float>(read_be32(data, offset));
        }

        [[nodiscard]] JpcVec3f read_be_vec3f(std::span<const std::uint8_t> data, std::size_t offset) {
            return JpcVec3f{
                .x = read_be_float(data, offset),
                .y = read_be_float(data, offset + 0x04U),
                .z = read_be_float(data, offset + 0x08U),
            };
        }

        [[nodiscard]] JpcVec3s read_be_vec3s(std::span<const std::uint8_t> data, std::size_t offset) {
            return JpcVec3s{
                .x = read_be_s16(data, offset),
                .y = read_be_s16(data, offset + 0x02U),
                .z = read_be_s16(data, offset + 0x04U),
            };
        }

        [[nodiscard]] std::string read_fixed_string(std::span<const std::uint8_t> data, std::size_t offset, std::size_t capacity) {
            if (offset + capacity > data.size()) {
                throw std::runtime_error("JPC fixed string outside buffer");
            }

            auto size = std::size_t{};
            while (size < capacity && data[offset + size] != 0U) {
                ++size;
            }
            return std::string(reinterpret_cast<const char *>(data.data() + offset), size);
        }

        [[nodiscard]] std::string block_tag_name(std::uint32_t tag) {
            auto text = std::string(4U, '\0');
            text[0] = static_cast<char>((tag >> 24U) & 0xffU);
            text[1] = static_cast<char>((tag >> 16U) & 0xffU);
            text[2] = static_cast<char>((tag >> 8U) & 0xffU);
            text[3] = static_cast<char>(tag & 0xffU);
            return text;
        }

        [[nodiscard]] bool has_trailing_two_digits(std::string_view text) {
            return text.size() >= 2U && text[text.size() - 2U] >= '0' && text[text.size() - 2U] <= '9' && text[text.size() - 1U] >= '0' &&
                   text[text.size() - 1U] <= '9';
        }

        [[nodiscard]] std::vector<std::string_view> split_space_tokens(std::string_view text) {
            auto tokens = std::vector<std::string_view>{};
            auto offset = std::size_t{};
            while (offset < text.size()) {
                while (offset < text.size() && text[offset] == ' ') {
                    ++offset;
                }
                const auto begin = offset;
                while (offset < text.size() && text[offset] != ' ') {
                    ++offset;
                }
                if (begin != offset) {
                    tokens.emplace_back(text.substr(begin, offset - begin));
                }
            }
            return tokens;
        }

        [[nodiscard]] std::string numbered_name(std::string_view base, std::uint32_t index) {
            auto out = std::string(base);
            out.push_back(static_cast<char>('0' + ((index / 10U) % 10U)));
            out.push_back(static_cast<char>('0' + (index % 10U)));
            return out;
        }

        [[nodiscard]] std::string group_unique_key(std::string_view group_name, std::string_view unique_name) {
            auto out = std::string(group_name);
            out.push_back('\x1f');
            out.append(unique_name);
            return out;
        }

        [[nodiscard]] std::string get_string_or_empty(const BcsvTable &table, std::size_t row, std::string_view name) {
            return table.get_string(row, name).value_or(std::string{});
        }

        [[nodiscard]] std::int32_t get_s32_or(const BcsvTable &table, std::size_t row, std::string_view name, std::int32_t fallback) {
            return table.get_s32(row, name).value_or(fallback);
        }

        [[nodiscard]] float get_float_or(const BcsvTable &table, std::size_t row, std::string_view name, float fallback) {
            return table.get_float(row, name).value_or(fallback);
        }

        [[nodiscard]] JpcDynamicsBlockMetadata read_bem1_dynamics_metadata(std::span<const std::uint8_t> data, std::size_t offset,
                                                                           std::uint32_t block_size) {
            constexpr auto DYNAMICS_BLOCK_DATA_SIZE = std::uint32_t{0x7cU};

            if (block_size < DYNAMICS_BLOCK_DATA_SIZE || offset + DYNAMICS_BLOCK_DATA_SIZE > data.size()) {
                throw std::runtime_error("JPC BEM1 block too small for JPADynamicsBlockData");
            }

            const auto flags = read_be32(data, offset + 0x08U);
            return JpcDynamicsBlockMetadata{
                .flags = flags,
                .resource_user_work = read_be32(data, offset + 0x0cU),
                .emitter_scale = read_be_vec3f(data, offset + 0x10U),
                .emitter_translation = read_be_vec3f(data, offset + 0x1cU),
                .emitter_direction = read_be_vec3f(data, offset + 0x28U),
                .initial_velocity_omni = read_be_float(data, offset + 0x34U),
                .initial_velocity_axis = read_be_float(data, offset + 0x38U),
                .initial_velocity_random = read_be_float(data, offset + 0x3cU),
                .initial_velocity_direction = read_be_float(data, offset + 0x40U),
                .spread = read_be_float(data, offset + 0x44U),
                .initial_velocity_ratio = read_be_float(data, offset + 0x48U),
                .rate = read_be_float(data, offset + 0x4cU),
                .rate_random = read_be_float(data, offset + 0x50U),
                .lifetime_random = read_be_float(data, offset + 0x54U),
                .volume_sweep = read_be_float(data, offset + 0x58U),
                .volume_min_radius = read_be_float(data, offset + 0x5cU),
                .air_resistance = read_be_float(data, offset + 0x60U),
                .moment = read_be_float(data, offset + 0x64U),
                .emitter_rotation = read_be_vec3s(data, offset + 0x68U),
                .max_frame = read_be_s16(data, offset + 0x6eU),
                .start_frame = read_be_s16(data, offset + 0x70U),
                .lifetime = read_be_s16(data, offset + 0x72U),
                .volume_size = read_be16(data, offset + 0x74U),
                .div_number = read_be16(data, offset + 0x76U),
                .rate_step = data[offset + 0x78U],
                .volume_type = static_cast<std::uint8_t>((flags >> 8U) & 0x07U),
                .fixed_density = (flags & 0x01U) != 0U,
                .fixed_interval = (flags & 0x02U) != 0U,
                .inherit_scale = (flags & 0x04U) != 0U,
                .follow_emitter = (flags & 0x08U) != 0U,
                .follow_emitter_child = (flags & 0x10U) != 0U,
            };
        }

        void read_bsp1_shape_metadata(std::span<const std::uint8_t> data, std::size_t offset, std::uint32_t block_size,
                                      JpcResourceMetadata &resource) {
            constexpr auto BASE_SHAPE_DATA_SIZE = std::uint32_t{0x34U};
            constexpr auto BASE_SHAPE_TEX_IDX_OFFSET = std::size_t{0x20U};

            if (block_size < BASE_SHAPE_DATA_SIZE || offset + BASE_SHAPE_DATA_SIZE > data.size()) {
                return;
            }

            const auto flags = read_be32(data, offset + 0x08U);
            const auto texture_slot = data[offset + BASE_SHAPE_TEX_IDX_OFFSET];
            resource.base_shape = JpcBaseShapeMetadata{
                .flags = flags,
                .base_size_x = read_be_float(data, offset + 0x10U),
                .base_size_y = read_be_float(data, offset + 0x14U),
                .blend_mode_config = read_be16(data, offset + 0x18U),
                .alpha_compare_config = data[offset + 0x1aU],
                .alpha_ref0 = data[offset + 0x1bU],
                .alpha_ref1 = data[offset + 0x1cU],
                .z_mode_config = data[offset + 0x1dU],
                .texture_flags = data[offset + 0x1eU],
                .texture_count = data[offset + 0x1fU],
                .texture_slot = texture_slot,
                .color_flags = data[offset + 0x21U],
                .color_animation_frame_max = read_be_s16(data, offset + 0x24U),
                .prm_color = {data[offset + 0x26U], data[offset + 0x27U], data[offset + 0x28U], data[offset + 0x29U]},
                .env_color = {data[offset + 0x2aU], data[offset + 0x2bU], data[offset + 0x2cU], data[offset + 0x2dU]},
                .shape_type = static_cast<std::uint8_t>(flags & 0x0fU),
                .direction_type = static_cast<std::uint8_t>((flags >> 4U) & 0x07U),
                .rotation_type = static_cast<std::uint8_t>((flags >> 7U) & 0x07U),
                .base_plane_type = static_cast<std::uint8_t>((flags >> 10U) & 0x01U),
                .texture_coordinate_animation = (flags & 0x01000000U) != 0U,
            };
            resource.base_shape_texture_slot = texture_slot;
        }

        [[nodiscard]] JpcChildShapeMetadata read_ssp1_child_shape_metadata(std::span<const std::uint8_t> data, std::size_t offset,
                                                                           std::uint32_t block_size) {
            constexpr auto CHILD_SHAPE_DATA_SIZE = std::uint32_t{0x48U};

            if (block_size < CHILD_SHAPE_DATA_SIZE || offset + CHILD_SHAPE_DATA_SIZE > data.size()) {
                throw std::runtime_error("JPC SSP1 block too small for JPAChildShapeData");
            }

            const auto flags = read_be32(data, offset + 0x08U);
            return JpcChildShapeMetadata{
                .flags = flags,
                .position_random = read_be_float(data, offset + 0x0cU),
                .base_velocity = read_be_float(data, offset + 0x10U),
                .base_velocity_random = read_be_float(data, offset + 0x14U),
                .velocity_inherit_rate = read_be_float(data, offset + 0x18U),
                .gravity = read_be_float(data, offset + 0x1cU),
                .scale_x = read_be_float(data, offset + 0x20U),
                .scale_y = read_be_float(data, offset + 0x24U),
                .inherit_scale = read_be_float(data, offset + 0x28U),
                .inherit_alpha = read_be_float(data, offset + 0x2cU),
                .inherit_rgb = read_be_float(data, offset + 0x30U),
                .prm_color = {data[offset + 0x34U], data[offset + 0x35U], data[offset + 0x36U], data[offset + 0x37U]},
                .env_color = {data[offset + 0x38U], data[offset + 0x39U], data[offset + 0x3aU], data[offset + 0x3bU]},
                .timing = read_be_float(data, offset + 0x3cU),
                .lifetime = read_be_s16(data, offset + 0x40U),
                .rate = read_be_s16(data, offset + 0x42U),
                .step = data[offset + 0x44U],
                .texture_slot = data[offset + 0x45U],
                .rotation_speed = read_be_s16(data, offset + 0x46U),
                .shape_type = static_cast<std::uint8_t>(flags & 0x0fU),
                .direction_type = static_cast<std::uint8_t>((flags >> 4U) & 0x07U),
                .rotation_type = static_cast<std::uint8_t>((flags >> 7U) & 0x07U),
                .base_plane_type = static_cast<std::uint8_t>((flags >> 10U) & 0x01U),
                .scale_inherited = (flags & 0x00010000U) != 0U,
                .alpha_inherited = (flags & 0x00020000U) != 0U,
                .color_inherited = (flags & 0x00040000U) != 0U,
                .clip_enabled = (flags & 0x00100000U) != 0U,
                .field_affected = (flags & 0x00200000U) != 0U,
                .scale_out_enabled = (flags & 0x00400000U) != 0U,
                .alpha_out_enabled = (flags & 0x00800000U) != 0U,
                .rotate_enabled = (flags & 0x01000000U) != 0U,
            };
        }

        [[nodiscard]] JpcKeyBlockMetadata read_kfa1_key_metadata(std::span<const std::uint8_t> data, std::size_t offset,
                                                                 std::uint32_t block_size) {
            constexpr auto KEY_BLOCK_HEADER_SIZE = std::uint32_t{0x0cU};
            constexpr auto FLOATS_PER_KEY = std::size_t{4U};
            constexpr auto KEY_FRAME_SIZE = FLOATS_PER_KEY * sizeof(float);

            if (block_size < KEY_BLOCK_HEADER_SIZE || offset + KEY_BLOCK_HEADER_SIZE > data.size()) {
                throw std::runtime_error("JPC KFA1 block too small for JPAKeyBlock");
            }

            const auto key_count = data[offset + 0x09U];
            const auto key_data_size = static_cast<std::size_t>(key_count) * KEY_FRAME_SIZE;
            if (offset + KEY_BLOCK_HEADER_SIZE + key_data_size > data.size() || KEY_BLOCK_HEADER_SIZE + key_data_size > block_size) {
                throw std::runtime_error("JPC KFA1 key data outside buffer");
            }

            auto key_block = JpcKeyBlockMetadata{
                .id = data[offset + 0x08U],
                .loop = data[offset + 0x0bU] != 0U,
                .keys = {},
            };
            key_block.keys.reserve(key_count);
            for (auto key_index = std::size_t{}; key_index < key_count; ++key_index) {
                const auto key_offset = offset + KEY_BLOCK_HEADER_SIZE + key_index * KEY_FRAME_SIZE;
                key_block.keys.push_back(JpcKeyFrameMetadata{
                    .time = read_be_float(data, key_offset + 0x00U),
                    .value = read_be_float(data, key_offset + 0x04U),
                    .tangent_in = read_be_float(data, key_offset + 0x08U),
                    .tangent_out = read_be_float(data, key_offset + 0x0cU),
                });
            }
            return key_block;
        }

    }  // namespace

    EffectResourceLibrary EffectResourceLibrary::from_archive(const RarcArchive &archive) {
        auto library = EffectResourceLibrary{};
        library.parse_particle_names(archive.file_data("particlenames.bcsv"));
        library.parse_auto_effect_list(archive.file_data("autoeffectlist.bcsv"));
        library.parse_jpc(archive.file_data("particles.jpc"));
        return library;
    }

    std::size_t EffectResourceLibrary::particle_name_count() const {
        return _particle_name_to_user_index.size();
    }

    std::size_t EffectResourceLibrary::auto_effect_count() const {
        return _auto_effects.size();
    }

    std::size_t EffectResourceLibrary::resource_count() const {
        return _resources.size();
    }

    std::size_t EffectResourceLibrary::texture_count() const {
        return _textures.size();
    }

    std::optional<std::uint16_t> EffectResourceLibrary::find_particle_user_index(std::string_view name) const {
        if (const auto it = _particle_name_to_user_index.find(name); it != _particle_name_to_user_index.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    std::optional<std::string_view> EffectResourceLibrary::particle_name_for_user_index(std::uint16_t user_index) const {
        if (const auto it = _particle_name_by_user_index.find(user_index); it != _particle_name_by_user_index.end()) {
            return std::string_view(it->second);
        }

        return std::nullopt;
    }

    const JpcResourceMetadata *EffectResourceLibrary::resource_by_user_index(std::uint16_t user_index) const {
        if (const auto it = _resource_index_by_user_index.find(user_index); it != _resource_index_by_user_index.end()) {
            return &_resources[it->second];
        }

        return nullptr;
    }

    const JpcTextureMetadata *EffectResourceLibrary::texture(std::uint16_t texture_index) const {
        if (texture_index >= _textures.size()) {
            return nullptr;
        }

        return &_textures[texture_index];
    }

    std::vector<ResolvedEffectResource> EffectResourceLibrary::resolve_particle_name(std::string_view name) const {
        auto out = std::vector<ResolvedEffectResource>{};
        for (const auto token : split_space_tokens(name)) {
            auto token_resources = resolve_particle_token(token, nullptr);
            out.insert(out.end(), token_resources.begin(), token_resources.end());
        }
        if (out.empty() && !name.empty() && split_space_tokens(name).empty()) {
            return resolve_particle_token(name, nullptr);
        }
        return out;
    }

    std::vector<ResolvedEffectResource> EffectResourceLibrary::resolve_auto_effect(std::string_view group_name, std::string_view unique_name) const {
        auto out = std::vector<ResolvedEffectResource>{};
        if (const auto it = _auto_effects_by_group_unique_name.find(group_unique_key(group_name, unique_name));
            it != _auto_effects_by_group_unique_name.end()) {
            for (const auto auto_effect_index : it->second) {
                const auto &auto_effect = _auto_effects[auto_effect_index];
                auto resources = resolve_particle_token(auto_effect.effect_name, &auto_effect);
                out.insert(out.end(), resources.begin(), resources.end());
            }
        }
        return out;
    }

    std::vector<ResolvedEffectResource> EffectResourceLibrary::resolve_effect_request(std::string_view effect_name) const {
        auto out = std::vector<ResolvedEffectResource>{};
        for (const auto token : split_space_tokens(effect_name)) {
            append_particle_resolution(out, token, token, nullptr);
        }
        if (!out.empty()) {
            return out;
        }

        if (const auto it = _auto_effects_by_unique_name.find(effect_name); it != _auto_effects_by_unique_name.end()) {
            for (const auto auto_effect_index : it->second) {
                const auto &auto_effect = _auto_effects[auto_effect_index];
                auto resources = resolve_particle_token(auto_effect.effect_name, &auto_effect);
                out.insert(out.end(), resources.begin(), resources.end());
            }
        }
        if (!out.empty()) {
            return out;
        }

        out = resolve_particle_name(effect_name);
        return out;
    }

    void EffectResourceLibrary::parse_particle_names(std::span<const std::uint8_t> data) {
        const auto table = BcsvTable::from_bytes(data);
        for (auto row = std::size_t{}; row < table.entry_count(); ++row) {
            const auto name = table.get_string(row, "name");
            const auto id = table.get_s32(row, "id");
            if (!name.has_value() || name->empty() || !id.has_value() || *id < 0 || *id > 0xffff) {
                continue;
            }

            const auto user_index = static_cast<std::uint16_t>(*id);
            _particle_name_to_user_index.emplace(*name, user_index);
            _particle_name_by_user_index.emplace(user_index, *name);
        }
    }

    void EffectResourceLibrary::parse_auto_effect_list(std::span<const std::uint8_t> data) {
        const auto table = BcsvTable::from_bytes(data);
        _auto_effects.reserve(table.entry_count());
        for (auto row = std::size_t{}; row < table.entry_count(); ++row) {
            auto auto_effect = AutoEffectInfoCompat{
                .row_index = static_cast<std::uint32_t>(row),
                .group_name = get_string_or_empty(table, row, "GroupName"),
                .unique_name = get_string_or_empty(table, row, "UniqueName"),
                .anim_name = get_string_or_empty(table, row, "AnimName"),
                .continue_anim_end = get_string_or_empty(table, row, "ContinueAnimEnd"),
                .effect_name = get_string_or_empty(table, row, "EffectName"),
                .parent_name = get_string_or_empty(table, row, "ParentName"),
                .joint_name = get_string_or_empty(table, row, "JointName"),
                .offset_x = get_float_or(table, row, "OffsetX", 0.0F),
                .offset_y = get_float_or(table, row, "OffsetY", 0.0F),
                .offset_z = get_float_or(table, row, "OffsetZ", 0.0F),
                .start_frame = get_s32_or(table, row, "StartFrame", 0),
                .end_frame = get_s32_or(table, row, "EndFrame", -1),
                .affect = get_string_or_empty(table, row, "Affect"),
                .follow = get_string_or_empty(table, row, "Follow"),
                .scale_value = get_float_or(table, row, "ScaleValue", 1.0F),
                .rate_value = get_float_or(table, row, "RateValue", 1.0F),
                .prm_color = get_string_or_empty(table, row, "PrmColor"),
                .env_color = get_string_or_empty(table, row, "EnvColor"),
                .light_affect_value = get_float_or(table, row, "LightAffectValue", 0.0F),
                .draw_order = get_string_or_empty(table, row, "DrawOrder"),
            };

            const auto index = _auto_effects.size();
            if (!auto_effect.unique_name.empty() && !auto_effect.effect_name.empty()) {
                _auto_effects_by_unique_name[auto_effect.unique_name].push_back(index);
            }
            if (!auto_effect.group_name.empty() && !auto_effect.unique_name.empty() && !auto_effect.effect_name.empty()) {
                _auto_effects_by_group_unique_name[group_unique_key(auto_effect.group_name, auto_effect.unique_name)].push_back(index);
            }
            _auto_effects.push_back(std::move(auto_effect));
        }
    }

    void EffectResourceLibrary::parse_jpc(std::span<const std::uint8_t> data) {
        if (data.size() < 0x10U || read_be32(data, 0U) != JPAC_MAGIC || read_be32(data, 4U) != JPAC_VERSION_210) {
            throw std::runtime_error("Effect particles.jpc is not JPAC2-10");
        }

        const auto resource_count = read_be16(data, 0x08U);
        const auto texture_count = read_be16(data, 0x0aU);
        const auto texture_offset = read_be32(data, 0x0cU);

        auto offset = std::size_t{0x10U};
        _resources.reserve(resource_count);
        for (auto i = std::uint16_t{}; i < resource_count; ++i) {
            if (offset + 8U > data.size()) {
                throw std::runtime_error("JPC resource header outside buffer");
            }

            auto resource = JpcResourceMetadata{};
            resource.user_index = read_be16(data, offset);
            resource.block_count = read_be16(data, offset + 0x02U);
            resource.field_block_count = data[offset + 0x04U];
            resource.key_block_count = data[offset + 0x05U];
            resource.texture_reference_count = data[offset + 0x06U];
            offset += 8U;

            resource.block_tags.reserve(resource.block_count);
            resource.key_blocks.reserve(resource.key_block_count);
            resource.texture_indices.reserve(resource.texture_reference_count);
            for (auto block_index = std::uint16_t{}; block_index < resource.block_count; ++block_index) {
                if (offset + 8U > data.size()) {
                    throw std::runtime_error("JPC block header outside buffer");
                }
                const auto block_tag = read_be32(data, offset);
                const auto block_size = read_be32(data, offset + 0x04U);
                if (block_size < 8U || offset + block_size > data.size()) {
                    throw std::runtime_error("JPC block size outside buffer");
                }

                resource.block_tags.push_back(block_tag_name(block_tag));
                if (block_tag == BEM1_MAGIC) {
                    resource.dynamics = read_bem1_dynamics_metadata(data, offset, block_size);
                }
                if (block_tag == BSP1_MAGIC) {
                    read_bsp1_shape_metadata(data, offset, block_size, resource);
                }
                if (block_tag == KFA1_MAGIC) {
                    resource.key_blocks.push_back(read_kfa1_key_metadata(data, offset, block_size));
                }
                if (block_tag == SSP1_MAGIC) {
                    resource.child_shape = read_ssp1_child_shape_metadata(data, offset, block_size);
                }
                if (block_tag == TDB1_MAGIC) {
                    const auto texture_entry_count = std::min<std::uint32_t>(resource.texture_reference_count, (block_size - 8U) / 2U);
                    for (auto texture_entry = std::uint32_t{}; texture_entry < texture_entry_count; ++texture_entry) {
                        resource.texture_indices.push_back(read_be16(data, offset + 8U + texture_entry * 2U));
                    }
                }

                offset += block_size;
            }

            if (resource.base_shape_texture_slot.has_value() && *resource.base_shape_texture_slot < resource.texture_indices.size()) {
                resource.primary_texture_index = resource.texture_indices[*resource.base_shape_texture_slot];
            } else if (!resource.texture_indices.empty()) {
                resource.primary_texture_index = resource.texture_indices.front();
            }
            if (resource.child_shape.has_value() && resource.child_shape->texture_slot < resource.texture_indices.size()) {
                resource.child_texture_index = resource.texture_indices[resource.child_shape->texture_slot];
            }

            _resource_index_by_user_index.emplace(resource.user_index, _resources.size());
            _resources.push_back(std::move(resource));
        }

        if (texture_offset >= data.size()) {
            throw std::runtime_error("JPC texture table outside buffer");
        }

        offset = texture_offset;
        _textures.reserve(texture_count);
        for (auto texture_index = std::uint16_t{}; texture_index < texture_count; ++texture_index) {
            if (offset + 0x40U > data.size()) {
                throw std::runtime_error("JPC texture header outside buffer");
            }

            const auto texture_size = read_be32(data, offset + 0x04U);
            if (texture_size < 0x40U || offset + texture_size > data.size()) {
                throw std::runtime_error("JPC texture size outside buffer");
            }

            const auto format = static_cast<TplTextureFormat>(data[offset + 0x20U]);
            const auto width = read_be16(data, offset + 0x22U);
            const auto height = read_be16(data, offset + 0x24U);
            auto image = DecodedTexture{};
            if (offset + 0x40U <= offset + texture_size) {
                image = decode_raw_gx_texture(data.subspan(offset + 0x40U, texture_size - 0x40U), width, height, format);
            }

            _textures.push_back(JpcTextureMetadata{
                .index = texture_index,
                .name = read_fixed_string(data, offset + 0x0cU, 0x14U),
                .format = format,
                .width = width,
                .height = height,
                .wrap_s = data[offset + 0x26U],
                .wrap_t = data[offset + 0x27U],
                .min_filter = data[offset + 0x34U],
                .mag_filter = data[offset + 0x35U],
                .image = std::move(image),
            });
            offset += texture_size;
        }
    }

    void EffectResourceLibrary::append_particle_resolution(std::vector<ResolvedEffectResource> &out, std::string_view requested_name,
                                                           std::string_view particle_name, const AutoEffectInfoCompat *auto_effect) const {
        const auto user_index = find_particle_user_index(particle_name);
        if (!user_index.has_value()) {
            return;
        }

        const auto *resource = resource_by_user_index(*user_index);
        auto textures = std::vector<JpcTextureMetadata>{};
        if (resource != nullptr) {
            textures.reserve(resource->texture_indices.size());
            for (const auto texture_index : resource->texture_indices) {
                if (const auto *metadata = texture(texture_index); metadata != nullptr) {
                    textures.push_back(*metadata);
                }
            }
        }

        out.push_back(ResolvedEffectResource{
            .requested_name = std::string(requested_name),
            .particle_name = std::string(particle_name),
            .user_index = *user_index,
            .auto_effect_group_name = auto_effect != nullptr ? auto_effect->group_name : std::string{},
            .auto_effect_unique_name = auto_effect != nullptr ? auto_effect->unique_name : std::string{},
            .auto_effect_parent_name = auto_effect != nullptr ? auto_effect->parent_name : std::string{},
            .auto_effect_joint_name = auto_effect != nullptr ? auto_effect->joint_name : std::string{},
            .auto_effect_draw_order = auto_effect != nullptr ? auto_effect->draw_order : std::string{},
            .auto_effect_offset_x = auto_effect != nullptr ? auto_effect->offset_x : 0.0F,
            .auto_effect_offset_y = auto_effect != nullptr ? auto_effect->offset_y : 0.0F,
            .auto_effect_offset_z = auto_effect != nullptr ? auto_effect->offset_z : 0.0F,
            .auto_effect_scale_value = auto_effect != nullptr ? auto_effect->scale_value : 1.0F,
            .auto_effect_rate_value = auto_effect != nullptr ? auto_effect->rate_value : 1.0F,
            .primary_texture_index = resource != nullptr ? resource->primary_texture_index : std::optional<std::uint16_t>{},
            .resource = resource,
            .textures = std::move(textures),
        });
    }

    std::vector<ResolvedEffectResource> EffectResourceLibrary::resolve_particle_token(std::string_view requested_name,
                                                                                      const AutoEffectInfoCompat *auto_effect) const {
        auto out = std::vector<ResolvedEffectResource>{};
        append_particle_resolution(out, requested_name, requested_name, auto_effect);
        if (!out.empty() || has_trailing_two_digits(requested_name)) {
            return out;
        }

        for (auto index = std::uint32_t{}; index < 100U; ++index) {
            const auto particle_name = numbered_name(requested_name, index);
            const auto before = out.size();
            append_particle_resolution(out, requested_name, particle_name, auto_effect);
            if (out.size() == before) {
                break;
            }
        }
        return out;
    }

}  // namespace smgpc::game
