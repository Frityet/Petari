#include "Brlan.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>

#include "Binary.hpp"

namespace smgpc::assets::layout {
namespace {

[[nodiscard]] AssetError make_error(std::string message) {
    return AssetError {
        .code = AssetErrorCode::InvalidFormat,
        .message = std::move(message),
    };
}

[[nodiscard]] std::string read_c_string_bounded(std::span<const std::byte> bytes, std::size_t offset, std::size_t max_length) {
    if (offset >= bytes.size()) {
        return {};
    }

    std::string out {};
    out.reserve(max_length);
    for (std::size_t i = 0; i < max_length && offset + i < bytes.size(); ++i) {
        const auto c = static_cast<char>(binary::read_u8(bytes, offset + i));
        if (c == '\0') {
            break;
        }
        out.push_back(c);
    }
    return out;
}

[[nodiscard]] AssetResult<void> validate_block(std::span<const std::byte> bytes, std::size_t block_offset, std::size_t block_size) {
    if (block_size < 8U) {
        return AssetResult<void>(make_error("BRLAN block size is smaller than block header."));
    }
    if (not binary::has_bytes(bytes, block_offset, block_size)) {
        return AssetResult<void>(make_error("BRLAN block exceeds file bounds."));
    }
    return {};
}

}  // namespace

float BrlanTrack::sample(float frame) const {
    if (keys.empty()) {
        return 0.0F;
    }

    if (keys.size() == 1U || frame <= keys.front().frame) {
        return keys.front().value;
    }

    if (frame >= keys.back().frame) {
        return keys.back().value;
    }

    for (std::size_t i = 0; i + 1U < keys.size(); ++i) {
        const auto &key0 = keys[i];
        const auto &key1 = keys[i + 1U];
        if (frame < key0.frame || frame > key1.frame) {
            continue;
        }

        if (curve_type == BrlanCurveType::Step) {
            return key0.value;
        }

        const float span = key1.frame - key0.frame;
        if (std::fabs(span) < 0.00001F) {
            return key1.value;
        }

        const float t = (frame - key0.frame) / span;
        const float t2 = t * t;
        const float t3 = t2 * t;

        const float h00 = 2.0F * t3 - 3.0F * t2 + 1.0F;
        const float h10 = t3 - 2.0F * t2 + t;
        const float h01 = -2.0F * t3 + 3.0F * t2;
        const float h11 = t3 - t2;

        return h00 * key0.value + h10 * span * key0.slope + h01 * key1.value + h11 * span * key1.slope;
    }

    return keys.back().value;
}

float BrlanAnimation::normalize_frame(float frame) const {
    if (not loop || frame_size == 0U) {
        return frame;
    }

    const float loop_frame = static_cast<float>(frame_size);
    const float wrapped = std::fmod(frame, loop_frame);
    if (wrapped < 0.0F) {
        return wrapped + loop_frame;
    }
    return wrapped;
}

AssetResult<BrlanAnimation> parse_brlan(std::span<const std::byte> bytes, std::string default_name) {
    using namespace binary;

    if (bytes.size() < 0x10U) {
        return make_error("BRLAN file is too small.");
    }
    if (not fourcc_equals(bytes, 0U, "RLAN")) {
        return make_error("BRLAN signature mismatch.");
    }

    BrlanAnimation animation {};
    animation.name = std::move(default_name);

    const auto header_size = static_cast<std::size_t>(read_u16_be(bytes, 0x0CU));
    const auto block_count = static_cast<std::size_t>(read_u16_be(bytes, 0x0EU));
    if (header_size < 0x10U || header_size > bytes.size()) {
        return make_error("BRLAN header size is invalid.");
    }

    std::size_t block_offset = header_size;
    for (std::size_t block_index = 0; block_index < block_count; ++block_index) {
        if (not has_bytes(bytes, block_offset, 8U)) {
            return make_error("BRLAN block header exceeds file bounds.");
        }

        const auto block_size = static_cast<std::size_t>(read_u32_be(bytes, block_offset + 4U));
        const auto block_valid = validate_block(bytes, block_offset, block_size);
        if (not block_valid) {
            return block_valid.failure();
        }

        const auto block = subspan(bytes, block_offset, block_size);
        const auto kind = read_fourcc(block, 0U);
        const auto kind_string = std::string(kind.begin(), kind.end());

        if (kind_string == "pat1") {
            if (not has_bytes(block, 12U, 12U)) {
                return make_error("pat1 block is too small.");
            }

            const auto name_offset = static_cast<std::size_t>(read_u32_be(block, 12U));
            if (name_offset > 0U && has_bytes(block, name_offset, 1U)) {
                animation.name = read_c_string(block, name_offset);
            }

            const auto start_frame = static_cast<int>(read_s32_be(block, 16U) >> 16U);
            const auto end_frame = static_cast<int>(read_s32_be(block, 20U) >> 16U);
            if (animation.frame_size == 0U && end_frame >= start_frame) {
                animation.frame_size = static_cast<std::uint16_t>(end_frame - start_frame);
            }
        } else if (kind_string == "pai1") {
            if (not has_bytes(block, 8U, 12U)) {
                return make_error("pai1 block is too small.");
            }

            animation.frame_size = read_u16_be(block, 8U);
            animation.loop = read_u8(block, 10U) != 0U;

            const auto anim_content_count = static_cast<std::size_t>(read_u16_be(block, 14U));
            const auto anim_content_offsets_offset = static_cast<std::size_t>(read_u32_be(block, 16U));
            if (not has_bytes(block, anim_content_offsets_offset, anim_content_count * 4U)) {
                return make_error("pai1 content offset table exceeds block bounds.");
            }

            for (std::size_t content_index = 0; content_index < anim_content_count; ++content_index) {
                const auto content_offset_rel = static_cast<std::size_t>(read_u32_be(block, anim_content_offsets_offset + content_index * 4U));
                if (not has_bytes(block, content_offset_rel, 24U)) {
                    return make_error("pai1 animation content header exceeds block bounds.");
                }

                const auto pane_name = read_c_string_bounded(block, content_offset_rel, 20U);
                const auto anim_type_count = static_cast<std::size_t>(read_u8(block, content_offset_rel + 20U));
                const auto content_layer = read_u8(block, content_offset_rel + 21U);

                const std::size_t anim_type_offsets_base = content_offset_rel + 24U;
                if (not has_bytes(block, anim_type_offsets_base, anim_type_count * 4U)) {
                    return make_error("pai1 anim-type offset table exceeds block bounds.");
                }

                for (std::size_t anim_type_index = 0; anim_type_index < anim_type_count; ++anim_type_index) {
                    const auto anim_type_offset_rel = static_cast<std::size_t>(read_u32_be(block, anim_type_offsets_base + anim_type_index * 4U));
                    const auto anim_type_offset = content_offset_rel + anim_type_offset_rel;
                    if (not has_bytes(block, anim_type_offset, 8U)) {
                        return make_error("pai1 anim-type header exceeds block bounds.");
                    }

                    const auto anim_kind = read_fourcc(block, anim_type_offset);
                    const auto anim_kind_string = std::string(anim_kind.begin(), anim_kind.end());
                    const auto channel_count = static_cast<std::size_t>(read_u8(block, anim_type_offset + 4U));
                    const auto channel_offsets_base = anim_type_offset + 8U;

                    if (not has_bytes(block, channel_offsets_base, channel_count * 4U)) {
                        return make_error("pai1 channel offset table exceeds block bounds.");
                    }

                    for (std::size_t channel_index = 0; channel_index < channel_count; ++channel_index) {
                        const auto channel_offset_rel = static_cast<std::size_t>(read_u32_be(block, channel_offsets_base + channel_index * 4U));
                        const auto channel_offset = anim_type_offset + channel_offset_rel;
                        if (not has_bytes(block, channel_offset, 12U)) {
                            return make_error("pai1 channel header exceeds block bounds.");
                        }

                        BrlanTrack track {};
                        track.pane_name = pane_name;
                        track.kind = anim_kind_string;
                        track.layer = content_layer;
                        track.target = read_u16_be(block, channel_offset + 0U);
                        track.curve_type = static_cast<BrlanCurveType>(read_u8(block, channel_offset + 2U));

                        const auto key_count = static_cast<std::size_t>(read_u16_be(block, channel_offset + 4U));
                        const auto key_offset_rel = static_cast<std::size_t>(read_u32_be(block, channel_offset + 8U));
                        const auto key_offset = channel_offset + key_offset_rel;

                        if (track.curve_type == BrlanCurveType::Step) {
                            if (not has_bytes(block, key_offset, key_count * 8U)) {
                                return make_error("pai1 step key array exceeds block bounds.");
                            }
                        } else if (track.curve_type == BrlanCurveType::Hermite) {
                            if (not has_bytes(block, key_offset, key_count * 12U)) {
                                return make_error("pai1 Hermite key array exceeds block bounds.");
                            }
                        } else {
                            return make_error("BRLAN contains unsupported curve type.");
                        }

                        track.keys.reserve(key_count);
                        for (std::size_t key_index = 0; key_index < key_count; ++key_index) {
                            BrlanKey key {};
                            if (track.curve_type == BrlanCurveType::Step) {
                                const auto key_start = key_offset + key_index * 8U;
                                key.frame = read_f32_be(block, key_start + 0U);
                                key.value = static_cast<float>(read_u16_be(block, key_start + 4U));
                                key.slope = 0.0F;
                            } else {
                                const auto key_start = key_offset + key_index * 12U;
                                key.frame = read_f32_be(block, key_start + 0U);
                                key.value = read_f32_be(block, key_start + 4U);
                                key.slope = read_f32_be(block, key_start + 8U);
                            }
                            track.keys.push_back(key);
                        }

                        animation.tracks.push_back(std::move(track));
                    }
                }
            }
        }

        block_offset += block_size;
    }

    if (animation.name.empty()) {
        animation.name = "unknown";
    }

    return animation;
}

}  // namespace smgpc::assets::layout
