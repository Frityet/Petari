#include "J3dAnimation.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace smgpc::render {
    namespace {

        constexpr auto J3D1_MAGIC = std::uint32_t{0x4a334431U};

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
            return J3dAnimationKeyTableSummary{
                .max_frame = read_be16(data, offset),
                .offset = read_be16(data, offset + 2U),
                .type = read_be16(data, offset + 4U),
            };
        }

        [[nodiscard]] J3dAnimationTransformTrackSummary read_transform_track(std::span<const std::uint8_t> data, std::size_t offset) {
            return J3dAnimationTransformTrackSummary{
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

            auto key = std::size_t{};
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

        [[nodiscard]] float loop_frame(float frame, std::int16_t frame_max) {
            if (frame_max <= 0) {
                return frame;
            }

            auto wrapped = std::fmod(frame, static_cast<float>(frame_max));
            if (wrapped < 0.0F) {
                wrapped += static_cast<float>(frame_max);
            }
            return wrapped;
        }

        [[nodiscard]] J3dBckAnimationSummary parse_ank1(std::span<const std::uint8_t> data, std::size_t section_offset) {
            auto summary = J3dBckAnimationSummary{};
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
            auto summary = J3dBtkAnimationSummary{};
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
                auto material_summary = J3dBtkMaterialAnimationSummary{};
                material_summary.material_name = material < names.size() ? names[material] : std::string{};
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
            return J3dBrkAnimationSummary{
                .attribute = data[section_offset + 0x08U],
                .frame_max = read_be_s16(data, section_offset + 0x0aU),
            };
        }

    }  // namespace

    J3dAnimationSummary inspect_j3d_animation(std::span<const std::uint8_t> animation_data) {
        if (animation_data.size() < 0x28U || read_be32(animation_data, 0U) != J3D1_MAGIC) {
            throw std::runtime_error("Not a J3D1 animation file");
        }

        auto summary = J3dAnimationSummary{};
        summary.type = read_tag(animation_data, 4U);
        summary.file_size = read_be32(animation_data, 0x08U);
        summary.block_count = read_be32(animation_data, 0x0cU);

        auto section_offset = std::size_t{0x20U};
        for (auto block = 0U; block < summary.block_count; ++block) {
            const auto tag = read_tag(animation_data, section_offset);
            const auto size = read_be32(animation_data, section_offset + 4U);
            summary.sections.push_back(J3dAnimationSectionInfo{
                .tag = tag,
                .offset = static_cast<std::uint32_t>(section_offset),
                .size = size,
            });

            if (tag == "ANK1") {
                summary.bck = parse_ank1(animation_data, section_offset);
            } else if (tag == "TTK1") {
                summary.btk = parse_ttk1(animation_data, section_offset);
            } else if (tag == "TRK1") {
                summary.brk = parse_trk1(animation_data, section_offset);
            }

            section_offset += size;
        }

        return summary;
    }

    std::optional<J3dJointTransformValue> j3d_evaluate_bck_joint_transform(const J3dBckAnimationSummary &bck, std::uint16_t joint_index,
                                                                           float frame) {
        if (joint_index >= bck.joints.size()) {
            return std::nullopt;
        }

        const auto normalized_frame = loop_frame(frame, bck.frame_max);
        const auto &tracks = bck.joints[joint_index];
        auto transform = J3dJointTransformValue{};
        for (auto axis = 0U; axis < tracks.size(); ++axis) {
            transform.scale[axis] = evaluate_key_table(normalized_frame, tracks[axis].scale, bck.scale_values, 1.0F);
            transform.rotation[axis] = static_cast<std::int16_t>(
                static_cast<int>(evaluate_key_table(normalized_frame, tracks[axis].rotation, bck.rotation_values, 0.0F)) << bck.rotation_fraction);
            transform.translation[axis] = evaluate_key_table(normalized_frame, tracks[axis].translation, bck.translation_values, 0.0F);
        }

        return transform;
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

        return J3dTextureSrtAnimationValue{
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

}  // namespace smgpc::render
