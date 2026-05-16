#include "BrlanAnimation.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <string_view>

namespace smgpc::game {
namespace {

constexpr auto CURVE_STEP = std::uint8_t {1U};
constexpr auto CURVE_HERMITE = std::uint8_t {2U};

[[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) {
    if (offset + 2U > data.size()) {
        throw std::runtime_error("BRLAN read_be16 out of range");
    }

    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8U) | data[offset + 1U]);
}

[[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
    if (offset + 4U > data.size()) {
        throw std::runtime_error("BRLAN read_be32 out of range");
    }

    return (static_cast<std::uint32_t>(data[offset]) << 24U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) | (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) | data[offset + 3U];
}

[[nodiscard]] float read_be_float(std::span<const std::uint8_t> data, std::size_t offset) {
    return std::bit_cast<float>(read_be32(data, offset));
}

[[nodiscard]] bool has_magic(std::span<const std::uint8_t> data, std::size_t offset, std::string_view magic) {
    if (offset + magic.size() > data.size()) {
        return false;
    }

    for (std::size_t i = 0U; i < magic.size(); ++i) {
        if (data[offset + i] != static_cast<std::uint8_t>(magic[i])) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] std::string read_fixed_string(std::span<const std::uint8_t> data, std::size_t offset, std::size_t capacity) {
    if (offset + capacity > data.size()) {
        throw std::runtime_error("BRLAN fixed string out of range");
    }

    auto length = 0U;
    while (length < capacity && data[offset + length] != 0U) {
        ++length;
    }

    return std::string(reinterpret_cast<const char *>(data.data() + offset), length);
}

[[nodiscard]] float evaluate_hermite(std::span<const BrlanAnimation::HermiteKey> keys, float frame) {
    if (keys.empty()) {
        return 0.0F;
    }
    if (keys.size() == 1U || frame <= keys.front().frame) {
        return keys.front().value;
    }
    if (frame >= keys.back().frame) {
        return keys.back().value;
    }

    auto right = std::ranges::upper_bound(keys, frame, {}, &BrlanAnimation::HermiteKey::frame);
    if (right == keys.begin()) {
        return keys.front().value;
    }

    auto left = right;
    --left;
    while (right != keys.end() && right->frame == left->frame) {
        ++right;
    }
    if (right == keys.end()) {
        return left->value;
    }

    const auto duration = right->frame - left->frame;
    if (std::abs(duration) <= 0.00001F) {
        return right->value;
    }

    const auto t = (frame - left->frame) / duration;
    const auto t2 = t * t;
    const auto t3 = t2 * t;
    const auto h00 = 2.0F * t3 - 3.0F * t2 + 1.0F;
    const auto h10 = t3 - 2.0F * t2 + t;
    const auto h01 = -2.0F * t3 + 3.0F * t2;
    const auto h11 = t3 - t2;
    return h00 * left->value + h10 * duration * left->slope + h01 * right->value + h11 * duration * right->slope;
}

[[nodiscard]] std::uint16_t evaluate_step(std::span<const BrlanAnimation::StepKey> keys, float frame) {
    if (keys.empty()) {
        return 0U;
    }

    auto value = keys.front().value;
    for (const auto &key : keys) {
        if (frame < key.frame) {
            break;
        }
        value = key.value;
    }
    return value;
}

[[nodiscard]] std::optional<float> evaluate_float_target(const BrlanAnimation::Target &target, float frame) {
    if (target.curve_type == CURVE_HERMITE) {
        return evaluate_hermite(target.hermite_keys, frame);
    }
    if (target.curve_type == CURVE_STEP) {
        return static_cast<float>(evaluate_step(target.step_keys, frame));
    }

    return std::nullopt;
}

void apply_target(BrlanPaneFrame &frame_values, std::string_view kind, const BrlanAnimation::Target &target, float frame) {
    if (kind == "RLPA") {
        const auto value = evaluate_float_target(target, frame);
        if (!value.has_value()) {
            return;
        }

        switch (target.target) {
        case 0U:
            frame_values.translate_x = *value;
            break;
        case 1U:
            frame_values.translate_y = *value;
            break;
        case 6U:
            frame_values.scale_x = *value;
            break;
        case 7U:
            frame_values.scale_y = *value;
            break;
        default:
            break;
        }
    } else if (kind == "RLVC" && target.target == 16U) {
        if (const auto value = evaluate_float_target(target, frame)) {
            frame_values.alpha = *value;
        }
    } else if (kind == "RLVI" && target.target == 0U) {
        frame_values.visible = evaluate_step(target.step_keys, frame) != 0U;
    }
}

void apply_texture_target(BrlanTextureFrame &frame_values, std::string_view kind, const BrlanAnimation::Target &target, float frame) {
    if (kind != "RLTS") {
        return;
    }

    const auto value = evaluate_float_target(target, frame);
    if (!value.has_value()) {
        return;
    }

    switch (target.target) {
    case 0U:
        frame_values.translate_s = *value;
        break;
    case 1U:
        frame_values.translate_t = *value;
        break;
    case 2U:
        frame_values.rotate = *value;
        break;
    case 3U:
        frame_values.scale_s = *value;
        break;
    case 4U:
        frame_values.scale_t = *value;
        break;
    default:
        break;
    }
}

[[nodiscard]] BrlanAnimation::Target parse_target(std::span<const std::uint8_t> data, std::size_t base) {
    if (base + 12U > data.size()) {
        throw std::runtime_error("BRLAN target is truncated");
    }

    auto target = BrlanAnimation::Target {
        .target = read_be16(data, base),
        .curve_type = data[base + 2U],
        .step_keys = {},
        .hermite_keys = {},
    };

    const auto key_count = read_be16(data, base + 4U);
    const auto key_offset = read_be32(data, base + 8U);
    const auto key_base = base + key_offset;
    if (target.curve_type == CURVE_STEP) {
        if (key_base + static_cast<std::size_t>(key_count) * 8U > data.size()) {
            throw std::runtime_error("BRLAN step keys are truncated");
        }
        target.step_keys.reserve(key_count);
        for (auto i = 0U; i < key_count; ++i) {
            const auto offset = key_base + static_cast<std::size_t>(i) * 8U;
            target.step_keys.push_back(BrlanAnimation::StepKey {
                .frame = read_be_float(data, offset),
                .value = read_be16(data, offset + 4U),
            });
        }
    } else if (target.curve_type == CURVE_HERMITE) {
        if (key_base + static_cast<std::size_t>(key_count) * 12U > data.size()) {
            throw std::runtime_error("BRLAN hermite keys are truncated");
        }
        target.hermite_keys.reserve(key_count);
        for (auto i = 0U; i < key_count; ++i) {
            const auto offset = key_base + static_cast<std::size_t>(i) * 12U;
            target.hermite_keys.push_back(BrlanAnimation::HermiteKey {
                .frame = read_be_float(data, offset),
                .value = read_be_float(data, offset + 4U),
                .slope = read_be_float(data, offset + 8U),
            });
        }
    }

    return target;
}

[[nodiscard]] BrlanAnimation::Info parse_info(std::span<const std::uint8_t> data, std::size_t base) {
    if (base + 8U > data.size()) {
        throw std::runtime_error("BRLAN animation info is truncated");
    }

    const auto target_count = data[base + 4U];
    if (base + 8U + static_cast<std::size_t>(target_count) * 4U > data.size()) {
        throw std::runtime_error("BRLAN target offset table is truncated");
    }

    auto info = BrlanAnimation::Info {
        .kind = std::string(reinterpret_cast<const char *>(data.data() + base), 4U),
        .targets = {},
    };
    info.targets.reserve(target_count);
    for (auto i = 0U; i < target_count; ++i) {
        info.targets.push_back(parse_target(data, base + read_be32(data, base + 8U + static_cast<std::size_t>(i) * 4U)));
    }

    return info;
}

[[nodiscard]] BrlanAnimation::Content parse_content(std::span<const std::uint8_t> data, std::size_t base) {
    if (base + 24U > data.size()) {
        throw std::runtime_error("BRLAN animation content is truncated");
    }

    const auto info_count = data[base + 20U];
    if (base + 24U + static_cast<std::size_t>(info_count) * 4U > data.size()) {
        throw std::runtime_error("BRLAN animation info offset table is truncated");
    }

    auto content = BrlanAnimation::Content {
        .name = read_fixed_string(data, base, 20U),
        .infos = {},
    };
    content.infos.reserve(info_count);
    for (auto i = 0U; i < info_count; ++i) {
        content.infos.push_back(parse_info(data, base + read_be32(data, base + 24U + static_cast<std::size_t>(i) * 4U)));
    }

    return content;
}

void parse_animation_block(BrlanAnimation &animation, std::span<const std::uint8_t> block) {
    animation.frame_size = read_be16(block, 8U);
    animation.loop = block[10U] != 0U;
    const auto content_count = read_be16(block, 14U);
    const auto content_offsets_offset = read_be32(block, 16U);
    if (content_offsets_offset + static_cast<std::size_t>(content_count) * 4U > block.size()) {
        throw std::runtime_error("BRLAN animation content table is truncated");
    }

    animation.contents.clear();
    animation.contents.reserve(content_count);
    for (auto i = 0U; i < content_count; ++i) {
        animation.contents.push_back(parse_content(block, read_be32(block, content_offsets_offset + static_cast<std::size_t>(i) * 4U)));
    }
}

}  // namespace

BrlanPaneFrame BrlanAnimation::pane_frame(std::string_view pane_name, float frame) const {
    auto result = BrlanPaneFrame {};
    for (const auto &content : contents) {
        if (content.name != pane_name) {
            continue;
        }

        for (const auto &info : content.infos) {
            for (const auto &target : info.targets) {
                apply_target(result, info.kind, target, frame);
            }
        }
    }

    return result;
}

BrlanTextureFrame BrlanAnimation::texture_frame(std::string_view material_name, float frame) const {
    auto result = BrlanTextureFrame {};
    for (const auto &content : contents) {
        if (content.name != material_name) {
            continue;
        }

        for (const auto &info : content.infos) {
            for (const auto &target : info.targets) {
                apply_texture_target(result, info.kind, target, frame);
            }
        }
    }

    return result;
}

BrlanAnimation parse_brlan_animation(std::span<const std::uint8_t> data) {
    if (!has_magic(data, 0U, "RLAN")) {
        throw std::runtime_error("BRLAN file is missing RLAN magic");
    }
    if (read_be16(data, 4U) != 0xFEFFU) {
        throw std::runtime_error("BRLAN file is not big-endian");
    }

    const auto header_size = read_be16(data, 12U);
    const auto block_count = read_be16(data, 14U);
    auto cursor = static_cast<std::size_t>(header_size);
    auto animation = BrlanAnimation {};

    for (auto i = 0U; i < block_count; ++i) {
        if (cursor + 8U > data.size()) {
            throw std::runtime_error("BRLAN data block header is truncated");
        }

        const auto block_size = read_be32(data, cursor + 4U);
        if (block_size < 8U || cursor + block_size > data.size()) {
            throw std::runtime_error("BRLAN data block size is invalid");
        }

        const auto block = data.subspan(cursor, block_size);
        if (has_magic(block, 0U, "pai1")) {
            parse_animation_block(animation, block);
        }

        cursor += block_size;
    }

    return animation;
}

}  // namespace smgpc::game
