#include "J3dAnimation.hpp"
#include "resource/J3dTransformAnimation.hpp"

#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3DGraphBase/J3DTransform.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace smgpc::render {
    namespace {

        constexpr auto J3D1_MAGIC = std::uint32_t {0x4a334431U};

        [[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 2U > data.size()) {
                throw std::runtime_error("J3D animation read past end of buffer");
            }

            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8U) | static_cast<std::uint16_t>(data[offset + 1U]));
        }

        [[nodiscard]] std::int16_t read_be_s16(std::span<const std::uint8_t> data, std::size_t offset) {
            return std::bit_cast<std::int16_t>(read_be16(data, offset));
        }

        [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 4U > data.size()) {
                throw std::runtime_error("J3D animation read past end of buffer");
            }

            return (static_cast<std::uint32_t>(data[offset]) << 24U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
                   (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) | static_cast<std::uint32_t>(data[offset + 3U]);
        }

        [[nodiscard]] float read_be_float(std::span<const std::uint8_t> data, std::size_t offset) {
            return std::bit_cast<float>(read_be32(data, offset));
        }

        [[nodiscard]] std::string read_tag(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 4U > data.size()) {
                throw std::runtime_error("J3D animation tag read past end of buffer");
            }

            return std::string(reinterpret_cast<const char *>(data.data() + offset), 4U);
        }

        [[nodiscard]] std::string read_string(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset >= data.size()) {
                throw std::runtime_error("J3D animation string offset outside buffer");
            }

            auto end = offset;
            while (end < data.size() && data[end] != 0U) {
                ++end;
            }
            if (end == data.size()) {
                throw std::runtime_error("J3D animation string is not null terminated");
            }

            return std::string(reinterpret_cast<const char *>(data.data() + offset), end - offset);
        }

        [[nodiscard]] std::vector<std::string> read_name_table(std::span<const std::uint8_t> data, std::size_t section_offset,
                                                               std::uint32_t table_relative_offset) {
            if (table_relative_offset == 0U) {
                return {};
            }

            const auto table_offset = section_offset + table_relative_offset;
            if (table_offset + 4U > data.size()) {
                throw std::runtime_error("J3D animation name table outside buffer");
            }

            const auto count = read_be16(data, table_offset);
            auto names = std::vector<std::string>{};
            names.reserve(count);
            for (auto i = 0U; i < count; ++i) {
                const auto entry_offset = table_offset + 4U + i * 4U;
                names.push_back(read_string(data, table_offset + read_be16(data, entry_offset + 2U)));
            }

            return names;
        }

        [[nodiscard]] J3dAnimationKeyTableSummary read_key_table(std::span<const std::uint8_t> data, std::size_t offset) {
            return J3dAnimationKeyTableSummary {
                .max_frame = read_be16(data, offset),
                .offset = read_be16(data, offset + 2U),
                .type = read_be16(data, offset + 4U),
            };
        }

        [[nodiscard]] J3dAnimationTransformTrackSummary read_transform_track(std::span<const std::uint8_t> data, std::size_t offset) {
            return J3dAnimationTransformTrackSummary {
                .scale = read_key_table(data, offset),
                .rotation = read_key_table(data, offset + 6U),
                .translation = read_key_table(data, offset + 12U),
            };
        }

        [[nodiscard]] std::vector<float> read_float_values(std::span<const std::uint8_t> data, std::size_t offset, std::uint16_t count) {
            auto values = std::vector<float>{};
            values.reserve(count);
            for (auto i = 0U; i < count; ++i) {
                values.push_back(read_be_float(data, offset + i * 4U));
            }
            return values;
        }

        [[nodiscard]] std::vector<std::int16_t> read_s16_values(std::span<const std::uint8_t> data, std::size_t offset, std::uint16_t count) {
            auto values = std::vector<std::int16_t>{};
            values.reserve(count);
            for (auto i = 0U; i < count; ++i) {
                values.push_back(read_be_s16(data, offset + i * 2U));
            }
            return values;
        }

        [[nodiscard]] float hermite(float frame, float frame0, float value0, float tangent0, float frame1, float value1, float tangent1) {
            const auto duration = frame1 - frame0;
            if (duration == 0.0F) {
                return value0;
            }

            const auto t = (frame - frame0) / duration;
            const auto t2 = t * t;
            const auto t3 = t2 * t;
            return (2.0F * t3 - 3.0F * t2 + 1.0F) * value0 + (t3 - 2.0F * t2 + t) * duration * tangent0 + (-2.0F * t3 + 3.0F * t2) * value1 +
                   (t3 - t2) * duration * tangent1;
        }

        template <typename T>
        [[nodiscard]] float value_at(const std::vector<T> &values, std::size_t index) {
            if (index >= values.size()) {
                throw std::runtime_error("J3D animation keyframe value outside value table");
            }

            return static_cast<float>(values[index]);
        }

        template <typename T>
        [[nodiscard]] float evaluate_key_table(float frame, const J3dAnimationKeyTableSummary &table, const std::vector<T> &values,
                                               float default_value) {
            if (table.max_frame == 0U) {
                return default_value;
            }
            if (table.max_frame == 1U) {
                return value_at(values, table.offset);
            }

            const auto stride = table.type == 0U ? 3U : 4U;
            const auto first = static_cast<std::size_t>(table.offset);
            const auto last = first + static_cast<std::size_t>(table.max_frame - 1U) * stride;
            if (frame < value_at(values, first)) {
                return value_at(values, first + 1U);
            }
            if (value_at(values, last) <= frame) {
                return value_at(values, last + 1U);
            }

            auto key = std::size_t {};
            while (key + 1U < table.max_frame && value_at(values, first + (key + 1U) * stride) <= frame) {
                ++key;
            }

            const auto current = first + key * stride;
            const auto next = current + stride;
            const auto tangent0 = table.type == 0U ? value_at(values, current + 2U) : value_at(values, current + 3U);
            const auto tangent1 = table.type == 0U ? value_at(values, next + 2U) : value_at(values, next + 2U);
            return hermite(frame, value_at(values, current), value_at(values, current + 1U), tangent0, value_at(values, next),
                           value_at(values, next + 1U), tangent1);
        }

        // BTK playback predates the actor-local BCK lifecycle. Keep texture SRT
        // animation on its independent renderer timeline until it gains an
        // explicit start origin of its own.
        [[nodiscard]] float loop_frame(float frame, std::int16_t frame_max) {
            if (frame_max <= 0 || !std::isfinite(frame)) {
                return 0.0F;
            }

            auto wrapped = std::fmod(frame, static_cast<float>(frame_max));
            if (wrapped < 0.0F) {
                wrapped += static_cast<float>(frame_max);
            }
            return wrapped;
        }

        [[nodiscard]] J3dBckAnimationSummary parse_ank1(std::span<const std::uint8_t> data, std::size_t section_offset) {
            auto summary = J3dBckAnimationSummary {};
            summary.attribute = data[section_offset + 0x08U];
            summary.rotation_fraction = data[section_offset + 0x09U];
            summary.frame_max = read_be_s16(data, section_offset + 0x0aU);
            summary.joint_count = read_be16(data, section_offset + 0x0cU);
            summary.scale_count = read_be16(data, section_offset + 0x0eU);
            summary.rotation_count = read_be16(data, section_offset + 0x10U);
            summary.translation_count = read_be16(data, section_offset + 0x12U);

            const auto table_relative = read_be32(data, section_offset + 0x14U);
            const auto scale_relative = read_be32(data, section_offset + 0x18U);
            const auto rotation_relative = read_be32(data, section_offset + 0x1cU);
            const auto translation_relative = read_be32(data, section_offset + 0x20U);
            if (scale_relative != 0U) {
                summary.scale_values = read_float_values(data, section_offset + scale_relative, summary.scale_count);
            }
            if (rotation_relative != 0U) {
                summary.rotation_values = read_s16_values(data, section_offset + rotation_relative, summary.rotation_count);
            }
            if (translation_relative != 0U) {
                summary.translation_values = read_float_values(data, section_offset + translation_relative, summary.translation_count);
            }

            if (table_relative != 0U) {
                const auto table_offset = section_offset + table_relative;
                summary.joints.reserve(summary.joint_count);
                for (auto joint = 0U; joint < summary.joint_count; ++joint) {
                    auto tracks = std::array<J3dAnimationTransformTrackSummary, 3U>{};
                    for (auto axis = 0U; axis < tracks.size(); ++axis) {
                        tracks[axis] = read_transform_track(data, table_offset + (joint * 3U + axis) * 0x12U);
                    }
                    summary.joints.push_back(tracks);
                }
            }

            return summary;
        }

        [[nodiscard]] J3dBtkAnimationSummary parse_ttk1(std::span<const std::uint8_t> data, std::size_t section_offset) {
            auto summary = J3dBtkAnimationSummary {};
            summary.attribute = data[section_offset + 0x08U];
            summary.rotation_fraction = data[section_offset + 0x09U];
            summary.frame_max = read_be_s16(data, section_offset + 0x0aU);
            summary.track_count = read_be16(data, section_offset + 0x0cU);
            summary.scale_count = read_be16(data, section_offset + 0x0eU);
            summary.rotation_count = read_be16(data, section_offset + 0x10U);
            summary.translation_count = read_be16(data, section_offset + 0x12U);

            const auto table_relative = read_be32(data, section_offset + 0x14U);
            const auto material_id_relative = read_be32(data, section_offset + 0x18U);
            const auto names_relative = read_be32(data, section_offset + 0x1cU);
            const auto tex_matrix_id_relative = read_be32(data, section_offset + 0x20U);
            const auto center_relative = read_be32(data, section_offset + 0x24U);
            const auto scale_relative = read_be32(data, section_offset + 0x28U);
            const auto rotation_relative = read_be32(data, section_offset + 0x2cU);
            const auto translation_relative = read_be32(data, section_offset + 0x30U);
            summary.tex_matrix_calc_type = read_be32(data, section_offset + 0x5cU);

            if (scale_relative != 0U) {
                summary.scale_values = read_float_values(data, section_offset + scale_relative, summary.scale_count);
            }
            if (rotation_relative != 0U) {
                summary.rotation_values = read_s16_values(data, section_offset + rotation_relative, summary.rotation_count);
            }
            if (translation_relative != 0U) {
                summary.translation_values = read_float_values(data, section_offset + translation_relative, summary.translation_count);
            }

            const auto names = read_name_table(data, section_offset, names_relative);
            const auto material_count = summary.track_count / 3U;
            summary.materials.reserve(material_count);
            for (auto material = 0U; material < material_count; ++material) {
                auto material_summary = J3dBtkMaterialAnimationSummary {};
                material_summary.material_name = material < names.size() ? names[material] : std::string {};
                if (material_id_relative != 0U) {
                    material_summary.material_id = read_be16(data, section_offset + material_id_relative + material * 2U);
                }
                if (tex_matrix_id_relative != 0U) {
                    material_summary.tex_matrix_id = data[section_offset + tex_matrix_id_relative + material];
                }
                if (center_relative != 0U) {
                    const auto center_offset = section_offset + center_relative + material * 12U;
                    material_summary.center = {
                        read_be_float(data, center_offset),
                        read_be_float(data, center_offset + 4U),
                        read_be_float(data, center_offset + 8U),
                    };
                }
                if (table_relative != 0U) {
                    const auto table_offset = section_offset + table_relative;
                    for (auto axis = 0U; axis < material_summary.tracks.size(); ++axis) {
                        material_summary.tracks[axis] = read_transform_track(data, table_offset + (material * 3U + axis) * 0x12U);
                    }
                }

                summary.materials.push_back(material_summary);
            }

            return summary;
        }

        [[nodiscard]] J3dBrkAnimationSummary parse_trk1(std::span<const std::uint8_t> data, std::size_t section_offset) {
            if (section_offset + 0x58U > data.size()) {
                throw std::runtime_error("TRK1 header is outside its section");
            }

            auto summary = J3dBrkAnimationSummary {
                .attribute = data[section_offset + 0x08U],
                .frame_max = read_be_s16(data, section_offset + 0x0aU),
            };
            const auto color_track_count = read_be16(data, section_offset + 0x0cU);
            const auto konst_track_count = read_be16(data, section_offset + 0x0eU);
            const auto color_counts = std::array<std::uint16_t, 4U>{
                read_be16(data, section_offset + 0x10U),
                read_be16(data, section_offset + 0x12U),
                read_be16(data, section_offset + 0x14U),
                read_be16(data, section_offset + 0x16U),
            };
            const auto konst_counts = std::array<std::uint16_t, 4U>{
                read_be16(data, section_offset + 0x18U),
                read_be16(data, section_offset + 0x1aU),
                read_be16(data, section_offset + 0x1cU),
                read_be16(data, section_offset + 0x1eU),
            };
            const auto color_table_relative = read_be32(data, section_offset + 0x20U);
            const auto konst_table_relative = read_be32(data, section_offset + 0x24U);
            const auto color_ids_relative = read_be32(data, section_offset + 0x28U);
            const auto konst_ids_relative = read_be32(data, section_offset + 0x2cU);
            const auto color_names_relative = read_be32(data, section_offset + 0x30U);
            const auto konst_names_relative = read_be32(data, section_offset + 0x34U);

            auto read_component_values = [&](auto& destination,
                                             const auto& counts,
                                             std::size_t relative_base) {
                for (auto channel = 0U; channel < destination.size(); ++channel) {
                    const auto relative = read_be32(
                        data, section_offset + relative_base + channel * 4U);
                    if (counts[channel] == 0U) {
                        destination[channel].clear();
                        continue;
                    }
                    if (relative == 0U) {
                        throw std::runtime_error(
                            "TRK1 value table is missing for a nonempty channel");
                    }
                    destination[channel] = read_s16_values(
                        data, section_offset + relative, counts[channel]);
                }
            };
            read_component_values(summary.color_values, color_counts, 0x38U);
            read_component_values(summary.konst_values, konst_counts, 0x48U);

            const auto validate_channel = [](
                                              const J3dAnimationKeyTableSummary& table,
                                              const std::vector<std::int16_t>& values) {
                if (table.max_frame == 0U) {
                    return;
                }
                if (table.max_frame == 1U) {
                    if (table.offset >= values.size()) {
                        throw std::runtime_error(
                            "TRK1 single-value channel is outside its value table");
                    }
                    return;
                }
                if (table.type > 1U) {
                    throw std::runtime_error("TRK1 uses an unsupported tangent type");
                }
                const auto stride = table.type == 0U ? 3U : 4U;
                const auto required = static_cast<std::size_t>(table.offset) +
                                      static_cast<std::size_t>(table.max_frame) * stride;
                if (required > values.size()) {
                    throw std::runtime_error(
                        "TRK1 keyframe channel is outside its value table");
                }
            };

            auto read_tracks = [&](std::uint16_t track_count,
                                   std::uint32_t table_relative,
                                   std::uint32_t ids_relative,
                                   std::uint32_t names_relative,
                                   const auto& values, std::uint8_t max_register,
                                   auto& destination) {
                if (track_count == 0U) {
                    return;
                }
                if (table_relative == 0U || ids_relative == 0U ||
                    names_relative == 0U) {
                    throw std::runtime_error("TRK1 track metadata is incomplete");
                }
                const auto names = read_name_table(data, section_offset,
                                                   names_relative);
                if (names.size() != track_count) {
                    throw std::runtime_error(
                        "TRK1 material-name table does not match its track count");
                }
                destination.reserve(track_count);
                for (auto track_index = 0U; track_index < track_count;
                     ++track_index) {
                    const auto track_offset = section_offset + table_relative +
                                              track_index * 0x1cU;
                    if (track_offset + 0x1cU > data.size()) {
                        throw std::runtime_error(
                            "TRK1 register track is outside its section");
                    }
                    auto track = J3dBrkAnimationSummary::RegisterTrack{
                        .material_name = names[track_index],
                        .stored_material_id = read_be16(
                            data, section_offset + ids_relative +
                                      track_index * 2U),
                        .register_id = data[track_offset + 0x18U],
                    };
                    if (track.material_name.empty() ||
                        track.register_id > max_register) {
                        throw std::runtime_error(
                            "TRK1 contains an invalid material/register binding");
                    }
                    for (auto channel = 0U; channel < track.channels.size();
                         ++channel) {
                        track.channels[channel] =
                            read_key_table(data, track_offset + channel * 6U);
                        validate_channel(track.channels[channel], values[channel]);
                    }
                    destination.push_back(std::move(track));
                }
            };

            read_tracks(color_track_count, color_table_relative,
                        color_ids_relative, color_names_relative,
                        summary.color_values, 2U, summary.color_tracks);
            read_tracks(konst_track_count, konst_table_relative,
                        konst_ids_relative, konst_names_relative,
                        summary.konst_values, 3U, summary.konst_tracks);
            return summary;
        }

        [[nodiscard]] J3dBtpAnimationSummary parse_tpt1(std::span<const std::uint8_t> data, std::size_t section_offset) {
            if (section_offset + 0x20U > data.size()) {
                throw std::runtime_error("TPT1 header is outside its section");
            }
            auto summary = J3dBtpAnimationSummary {};
            summary.attribute = data[section_offset + 0x08U];
            summary.frame_max = read_be_s16(data, section_offset + 0x0aU);
            summary.material_count = read_be16(data, section_offset + 0x0cU);
            summary.texture_index_count = read_be16(data, section_offset + 0x0eU);

            const auto table_relative = read_be32(data, section_offset + 0x10U);
            const auto values_relative = read_be32(data, section_offset + 0x14U);
            const auto material_id_relative = read_be32(data, section_offset + 0x18U);
            const auto names_relative = read_be32(data, section_offset + 0x1cU);
            if (summary.material_count == 0U || summary.texture_index_count == 0U || table_relative == 0U ||
                values_relative == 0U || material_id_relative == 0U || names_relative == 0U) {
                throw std::runtime_error("TPT1 contains no usable texture-pattern tracks");
            }

            summary.texture_indices.reserve(summary.texture_index_count);
            for (auto i = 0U; i < summary.texture_index_count; ++i) {
                summary.texture_indices.push_back(read_be16(data, section_offset + values_relative + i * 2U));
            }

            const auto names = read_name_table(data, section_offset, names_relative);
            if (names.size() != summary.material_count) {
                throw std::runtime_error("TPT1 material-name table does not match its track count");
            }

            summary.materials.reserve(summary.material_count);
            for (auto material = 0U; material < summary.material_count; ++material) {
                const auto table_offset = section_offset + table_relative + material * 8U;
                if (table_offset + 8U > data.size()) {
                    throw std::runtime_error("TPT1 texture-pattern table is outside its section");
                }
                auto track = J3dBtpMaterialAnimationSummary {
                    .material_name = names[material],
                    .material_id = read_be16(data, section_offset + material_id_relative + material * 2U),
                    .texture_slot = data[table_offset + 4U],
                    .max_frame = read_be16(data, table_offset),
                    .texture_index_offset = read_be16(data, table_offset + 2U),
                };
                if (track.material_name.empty() || track.texture_slot >= 8U || track.max_frame == 0U ||
                    static_cast<std::size_t>(track.texture_index_offset) + track.max_frame > summary.texture_indices.size()) {
                    throw std::runtime_error("TPT1 contains an invalid texture-pattern track");
                }
                summary.materials.push_back(std::move(track));
            }

            return summary;
        }

    }  // namespace

    J3dAnimationSummary inspect_j3d_animation(std::span<const std::uint8_t> animation_data) {
        if (animation_data.size() < 0x28U || read_be32(animation_data, 0U) != J3D1_MAGIC) {
            throw std::runtime_error("Not a J3D1 animation file");
        }

        auto summary = J3dAnimationSummary {};
        summary.type = read_tag(animation_data, 4U);
        summary.file_size = read_be32(animation_data, 0x08U);
        summary.block_count = read_be32(animation_data, 0x0cU);

        // Validate and own the transform resource at the endian boundary. The
        // summary arrays below remain available for inspection; skeletal poses
        // are evaluated by the original J3D object.
        std::shared_ptr<const J3DAnmTransformKey> transform_animation;
        if (summary.type == "bck1") {
            auto animation = resource::load_j3d_transform_animation(animation_data);
            transform_animation.reset(static_cast<J3DAnmTransformKey*>(animation.release()));
        }

        auto section_offset = std::size_t {0x20U};
        for (auto block = 0U; block < summary.block_count; ++block) {
            if (section_offset + 8U > animation_data.size()) {
                throw std::runtime_error("J3D animation section header is outside the file");
            }
            const auto tag = read_tag(animation_data, section_offset);
            const auto size = read_be32(animation_data, section_offset + 4U);
            if (size < 8U || section_offset >= animation_data.size() ||
                (block + 1U < summary.block_count && size > animation_data.size() - section_offset)) {
                throw std::runtime_error("J3D animation section is outside the file");
            }
            const auto available_size = std::min<std::size_t>(size, animation_data.size() - section_offset);
            summary.sections.push_back(J3dAnimationSectionInfo {
                .tag = tag,
                .offset = static_cast<std::uint32_t>(section_offset),
                .size = size,
            });

            if (tag == "ANK1") {
                summary.bck = parse_ank1(animation_data, section_offset);
                summary.bck->transform_animation = transform_animation;
            } else if (tag == "TTK1") {
                summary.btk = parse_ttk1(animation_data, section_offset);
            } else if (tag == "TRK1") {
                summary.brk = parse_trk1(
                    animation_data.subspan(section_offset, available_size), 0U);
            } else if (tag == "TPT1") {
                summary.btp = parse_tpt1(animation_data.subspan(section_offset, available_size), 0U);
            }

            section_offset += size;
        }

        return summary;
    }

    float j3d_animation_frame(std::uint8_t attribute, std::int16_t frame_max, float elapsed_frame) {
        if (frame_max <= 0 || !std::isfinite(elapsed_frame)) {
            return 0.0F;
        }

        const auto duration = static_cast<float>(frame_max);
        const auto elapsed = std::max(elapsed_frame, 0.0F);
        switch (attribute) {
        case 0U:  // LOOP_ONCE
            return std::min(elapsed, std::max(duration - 0.001F, 0.0F));
        case 1U:  // LOOP_ONCE_RESET
            return elapsed >= duration ? 0.0F : elapsed;
        case 2U: {  // LOOP_REPEAT
            return std::fmod(elapsed, duration);
        }
        case 3U:  // LOOP_MIRROR_ONCE
            if (elapsed > duration * 2.0F) {
                // With the original unit playback rate, the first update that
                // underflows reflects to frame 1 and then stops there.
                return std::min(1.0F, duration);
            }
            return elapsed <= duration ? elapsed : duration * 2.0F - elapsed;
        case 4U: {  // LOOP_MIRROR_REPEAT
            const auto turn_frame = std::max(duration - 1.0F, 0.0F);
            if (turn_frame == 0.0F) {
                return 0.0F;
            }
            const auto period = turn_frame * 2.0F;
            const auto mirrored = std::fmod(elapsed, period);
            return mirrored <= turn_frame ? mirrored : period - mirrored;
        }
        default:
            return std::min(elapsed, std::max(duration - 0.001F, 0.0F));
        }
    }

    bool j3d_animation_stopped(std::uint8_t attribute, std::int16_t frame_max, float elapsed_frame) {
        if (frame_max <= 0) {
            return true;
        }
        const auto elapsed = std::max(elapsed_frame, 0.0F);
        switch (attribute) {
        case 2U:
        case 4U:
            return false;
        case 3U:
            return elapsed > static_cast<float>(frame_max) * 2.0F;
        default:
            return elapsed >= static_cast<float>(frame_max);
        }
    }

    bool j3d_animation_check_pass(std::uint8_t attribute, std::int16_t frame_max, float elapsed_frame, float pass_frame) {
        if (frame_max <= 0 || !std::isfinite(elapsed_frame) || !std::isfinite(pass_frame)) {
            return false;
        }

        const auto end = static_cast<float>(frame_max);
        const auto elapsed = std::max(elapsed_frame, 0.0F);
        const auto current = j3d_animation_frame(attribute, frame_max, elapsed);
        auto rate = 1.0F;
        switch (attribute) {
        case 0U:
        case 1U:
            if (elapsed >= end) {
                rate = 0.0F;
            }
            break;
        case 2U:
            break;
        case 3U:
            rate = elapsed < end ? 1.0F : elapsed <= end * 2.0F ? -1.0F : 0.0F;
            break;
        case 4U: {
            const auto turn = std::max(end - 1.0F, 0.0F);
            if (turn == 0.0F) {
                rate = 0.0F;
                break;
            }
            const auto phase = std::fmod(elapsed, turn * 2.0F);
            rate = phase < turn && (elapsed == 0.0F || phase != 0.0F) ? 1.0F : -1.0F;
            break;
        }
        default:
            rate = 0.0F;
            break;
        }

        auto controller = J3DFrameCtrl{frame_max};
        controller.setAttribute(attribute);
        controller.setFrame(current);
        controller.setRate(rate);
        return controller.checkPass(pass_frame) != FALSE;
    }

    std::optional<J3dJointTransformValue> j3d_evaluate_bck_joint_transform(const J3dBckAnimationSummary &bck, std::uint16_t joint_index,
                                                                           float frame) {
        if (!bck.transform_animation || joint_index >= bck.transform_animation->field_0x1e) {
            return std::nullopt;
        }

        const auto normalized_frame = j3d_animation_frame(bck.attribute, bck.frame_max, frame);
        J3DTransformInfo transform;
        bck.transform_animation->calcTransform(normalized_frame, joint_index, &transform);
        return J3dJointTransformValue{
            .scale = {transform.mScale.x, transform.mScale.y, transform.mScale.z},
            .rotation = {transform.mRotation.x, transform.mRotation.y, transform.mRotation.z},
            .translation = {transform.mTranslate.x, transform.mTranslate.y, transform.mTranslate.z},
        };
    }

    std::optional<J3dTextureSrtAnimationValue> j3d_evaluate_btk_texture_srt(const J3dBtkAnimationSummary &btk, std::string_view material_name,
                                                                            std::uint8_t tex_matrix_id, float frame) {
        const auto it = std::ranges::find_if(btk.materials, [material_name, tex_matrix_id](const auto &material) {
            return material.material_name == material_name && material.tex_matrix_id == tex_matrix_id;
        });
        if (it == btk.materials.end()) {
            return std::nullopt;
        }

        const auto normalized_frame = loop_frame(frame, btk.frame_max);
        const auto &material = *it;
        const auto &track_s = material.tracks[0U];
        const auto &track_t = material.tracks[1U];
        const auto &track_rotation = material.tracks[2U];

        return J3dTextureSrtAnimationValue {
            .center = material.center,
            .scale_s = evaluate_key_table(normalized_frame, track_s.scale, btk.scale_values, 1.0F),
            .scale_t = evaluate_key_table(normalized_frame, track_t.scale, btk.scale_values, 1.0F),
            .rotation = static_cast<std::int16_t>(
                static_cast<int>(evaluate_key_table(normalized_frame, track_rotation.rotation, btk.rotation_values, 0.0F))
                << btk.rotation_fraction),
            .translate_s = evaluate_key_table(normalized_frame, track_s.translation, btk.translation_values, 0.0F),
            .translate_t = evaluate_key_table(normalized_frame, track_t.translation, btk.translation_values, 0.0F),
        };
    }

    std::optional<std::uint16_t> j3d_evaluate_btp_texture_index(const J3dBtpAnimationSummary &btp, std::string_view material_name,
                                                                 std::uint8_t texture_slot, float frame) {
        const auto it = std::ranges::find_if(btp.materials, [material_name, texture_slot](const auto &material) {
            return material.material_name == material_name && material.texture_slot == texture_slot;
        });
        if (it == btp.materials.end() || it->max_frame == 0U) {
            return std::nullopt;
        }

        const auto normalized_frame = j3d_animation_frame(btp.attribute, btp.frame_max, frame);
        const auto track_frame = normalized_frame < 0.0F ? 0U :
                                     normalized_frame >= static_cast<float>(it->max_frame) ? it->max_frame - 1U :
                                                                                           static_cast<std::uint16_t>(normalized_frame);
        const auto value_index = static_cast<std::size_t>(it->texture_index_offset) + track_frame;
        if (value_index >= btp.texture_indices.size()) {
            return std::nullopt;
        }
        return btp.texture_indices[value_index];
    }

    std::array<std::int16_t, 4U> j3d_evaluate_brk_color_track(
        const J3dBrkAnimationSummary& brk, std::size_t track_index,
        float raw_frame) {
        if (track_index >= brk.color_tracks.size() ||
            !std::isfinite(raw_frame)) {
            throw std::runtime_error("TRK1 color-track evaluation is invalid");
        }

        const auto& track = brk.color_tracks[track_index];
        auto result = std::array<std::int16_t, 4U>{};
        for (auto channel = 0U; channel < result.size(); ++channel) {
            const auto& table = track.channels[channel];
            const auto value = evaluate_key_table(raw_frame, table,
                                                  brk.color_values[channel],
                                                  0.0F);
            if (table.max_frame <= 1U) {
                result[channel] = static_cast<std::int16_t>(value);
            } else {
                result[channel] = static_cast<std::int16_t>(
                    std::clamp(value, -1024.0F, 1023.0F));
            }
        }
        return result;
    }

    std::array<std::uint8_t, 4U> j3d_evaluate_brk_konst_track(
        const J3dBrkAnimationSummary& brk, std::size_t track_index,
        float raw_frame) {
        if (track_index >= brk.konst_tracks.size() ||
            !std::isfinite(raw_frame)) {
            throw std::runtime_error("TRK1 konst-track evaluation is invalid");
        }

        const auto& track = brk.konst_tracks[track_index];
        auto result = std::array<std::uint8_t, 4U>{};
        for (auto channel = 0U; channel < result.size(); ++channel) {
            const auto& table = track.channels[channel];
            const auto value = evaluate_key_table(raw_frame, table,
                                                  brk.konst_values[channel],
                                                  0.0F);
            if (table.max_frame <= 1U) {
                result[channel] = static_cast<std::uint8_t>(
                    static_cast<std::int32_t>(value));
            } else {
                result[channel] = static_cast<std::uint8_t>(
                    std::clamp(value, 0.0F, 255.0F));
            }
        }
        return result;
    }

}  // namespace smgpc::render
