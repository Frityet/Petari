#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include "JSystem/J3DGraphBase/J3DTransform.hpp"
#include "render/J3dAnimation.hpp"
#include "resource/J3dTransformAnimation.hpp"
#include "runtime/RuntimeServices.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void near(float actual, float expected, std::string_view message) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > 0.00002F) {
            throw std::runtime_error(std::string(message) + ": actual=" + std::to_string(actual) +
                                     "; expected=" + std::to_string(expected));
        }
    }

    void write16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value) {
        bytes.at(offset) = static_cast<std::uint8_t>(value >> 8U);
        bytes.at(offset + 1U) = static_cast<std::uint8_t>(value);
    }

    void write32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
        write16(bytes, offset, static_cast<std::uint16_t>(value >> 16U));
        write16(bytes, offset + 2U, static_cast<std::uint16_t>(value));
    }

    void write_float(std::vector<std::uint8_t>& bytes, std::size_t offset, float value) {
        write32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }

    std::uint16_t read16(std::span<const std::uint8_t> bytes, std::size_t offset) {
        return static_cast<std::uint16_t>((bytes[offset] << 8U) | bytes[offset + 1U]);
    }

    std::uint32_t read32(std::span<const std::uint8_t> bytes, std::size_t offset) {
        return (static_cast<std::uint32_t>(read16(bytes, offset)) << 16U) | read16(bytes, offset + 2U);
    }

    void tag(std::vector<std::uint8_t>& bytes, std::size_t offset, std::string_view value) {
        require(value.size() == 4U, "fixture tags contain four bytes");
        std::copy(value.begin(), value.end(), bytes.begin() + offset);
    }

    struct Track {
        std::uint16_t count = 0;
        std::uint16_t offset = 0;
        std::uint16_t type = 0;
    };

    using Joint = std::array<std::array<Track, 3>, 3>;
    constexpr std::size_t Scale = 0, Rotation = 1, Translation = 2;
    constexpr std::size_t Block = 0x20, Table = Block + 0x24;

    struct Input {
        bool keyed = true;
        std::uint8_t attribute = 2;
        std::uint8_t shift = 2;
        std::int16_t frame_max = 4;
        std::vector<Joint> joints{1};
        std::vector<float> scales;
        std::vector<std::int16_t> rotations;
        std::vector<float> translations;
    };

    struct Encoded {
        std::vector<std::uint8_t> bytes;
        std::size_t scales;
        std::size_t rotations;
        std::size_t translations;
    };

    template <typename T>
    Track append(std::vector<T>& pool, std::uint16_t count, std::uint16_t type, std::initializer_list<T> values) {
        const auto offset = static_cast<std::uint16_t>(pool.size());
        pool.insert(pool.end(), values.begin(), values.end());
        return {count, offset, type};
    }

    Encoded encode(const Input& input) {
        const auto descriptor_size = input.keyed ? 6U : 4U;
        auto bytes = std::vector<std::uint8_t>(Table + input.joints.size() * 9U * descriptor_size, 0);
        tag(bytes, 0, "J3D1");
        tag(bytes, 4, input.keyed ? "bck1" : "bca1");
        write32(bytes, 0xC, 1);
        tag(bytes, Block, input.keyed ? "ANK1" : "ANF1");
        bytes[Block + 8] = input.attribute;
        bytes[Block + 9] = input.shift;
        write16(bytes, Block + 0xA, static_cast<std::uint16_t>(input.frame_max));
        write16(bytes, Block + 0xC, static_cast<std::uint16_t>(input.joints.size()));
        write16(bytes, Block + 0xE, static_cast<std::uint16_t>(input.scales.size()));
        write16(bytes, Block + 0x10, static_cast<std::uint16_t>(input.rotations.size()));
        write16(bytes, Block + 0x12, static_cast<std::uint16_t>(input.translations.size()));
        write32(bytes, Block + 0x14, Table - Block);

        auto cursor = Table;
        for (const auto& joint : input.joints) {
            for (const auto& axis : joint) {
                for (const auto& track : axis) {
                    write16(bytes, cursor, track.count);
                    write16(bytes, cursor + 2, track.offset);
                    if (input.keyed) {
                        write16(bytes, cursor + 4, track.type);
                    }
                    cursor += descriptor_size;
                }
            }
        }

        const auto align = [&] { bytes.resize((bytes.size() + 3U) & ~std::size_t{3U}, 0); };
        align();
        const auto scales = bytes.size();
        write32(bytes, Block + 0x18, static_cast<std::uint32_t>(scales - Block));
        for (float value : input.scales) {
            const auto offset = bytes.size();
            bytes.resize(offset + 4U);
            write_float(bytes, offset, value);
        }
        align();
        const auto rotations = bytes.size();
        write32(bytes, Block + 0x1C, static_cast<std::uint32_t>(rotations - Block));
        for (std::int16_t value : input.rotations) {
            const auto offset = bytes.size();
            bytes.resize(offset + 2U);
            write16(bytes, offset, static_cast<std::uint16_t>(value));
        }
        align();
        const auto translations = bytes.size();
        write32(bytes, Block + 0x20, static_cast<std::uint32_t>(translations - Block));
        for (float value : input.translations) {
            const auto offset = bytes.size();
            bytes.resize(offset + 4U);
            write_float(bytes, offset, value);
        }
        write32(bytes, 8, static_cast<std::uint32_t>(bytes.size()));
        write32(bytes, Block + 4, static_cast<std::uint32_t>(bytes.size() - Block));
        return {std::move(bytes), scales, rotations, translations};
    }

    Input keyed_input() {
        Input input;
        input.joints.resize(2);
        auto& first = input.joints[0];
        first[0][Scale] = {0, 0xFFFF, 0xFFFF};
        first[1][Scale] = append<float>(input.scales, 1, 99, {2.5F});
        first[2][Scale] = append<float>(input.scales, 2, 0, {0, 2, 1, 4, 6, 1});
        first[0][Rotation] = append<std::int16_t>(input.rotations, 1, 11, {-123});
        first[1][Rotation] = append<std::int16_t>(input.rotations, 2, 0, {0, 1, 0, 4, 5, 0});
        first[2][Rotation] = append<std::int16_t>(input.rotations, 2, 0, {0, -1, 0, 4, -5, 0});
        first[0][Translation] = append<float>(input.translations, 2, 1, {0, 10, 99, 2, 4, 30, -1, 88});
        first[1][Translation] = append<float>(input.translations, 1, 0, {7.25F});
        first[2][Translation] = {0, 0xFFFF, 0xFFFF};
        input.joints[1][0][Scale] = append<float>(input.scales, 1, 0, {9});
        input.joints[1][0][Translation] = append<float>(input.translations, 1, 0, {-99});
        input.joints[1][2][Rotation] = append<std::int16_t>(input.rotations, 1, 0, {16385});
        return input;
    }

    Input full_input() {
        Input input;
        input.keyed = false;
        input.frame_max = 2;
        auto& joint = input.joints[0];
        joint[0][Scale] = append<float>(input.scales, 3, 0, {2, 6, 10});
        joint[1][Scale] = append<float>(input.scales, 1, 0, {3});
        joint[2][Scale] = append<float>(input.scales, 2, 0, {4, 8});
        joint[0][Rotation] = append<std::int16_t>(input.rotations, 3, 0, {32760, -32760, -32740});
        joint[1][Rotation] = append<std::int16_t>(input.rotations, 2, 0, {10, -10});
        joint[2][Rotation] = append<std::int16_t>(input.rotations, 2, 0, {-10, 10});
        joint[0][Translation] = append<float>(input.translations, 3, 0, {10, 14, 22});
        joint[1][Translation] = append<float>(input.translations, 2, 0, {-2, 2});
        joint[2][Translation] = append<float>(input.translations, 1, 0, {99});
        return input;
    }

    J3DTransformInfo sample(J3DAnmTransform& animation, float frame, std::uint16_t joint = 0) {
        animation.setFrame(frame);
        J3DTransformInfo result{};
        animation.getTransform(joint, &result);
        require(animation.getFrame() == frame, "sampling must preserve the caller's raw frame");
        return result;
    }

    void test_keyed_metadata_and_owned_native_arrays() {
        auto encoded = encode(keyed_input());
        // Parsing must also work when the caller's bytes are not naturally aligned.
        auto unaligned = std::vector<std::uint8_t>(encoded.bytes.size() + 1, 0xEE);
        std::copy(encoded.bytes.begin(), encoded.bytes.end(), unaligned.begin() + 1);
        auto animation = smgpc::resource::load_j3d_transform_animation(std::span(unaligned).subspan(1));
        auto* key = dynamic_cast<J3DAnmTransformKey*>(animation.get());
        require(key != nullptr && key->getKind() == 8 && key->field_0x1e == 2 &&
                    key->getAttribute() == 2 && key->getFrameMax() == 4 && key->mDecShift == 2 && key->getFrame() == 0,
                "ANK1 must construct the original typed key animation and retain metadata");
        require(key->field_0x18 == 0 && key->field_0x1a == 0 && key->field_0x1c == 0,
                "loading must preserve original unused transform constructor fields");
        require(reinterpret_cast<std::uintptr_t>(key->mScaleData) % alignof(float) == 0 &&
                    reinterpret_cast<std::uintptr_t>(key->mRotData) % alignof(std::int16_t) == 0 &&
                    reinterpret_cast<std::uintptr_t>(key->mAnmTable) % alignof(J3DAnmTransformKeyTable) == 0,
                "decoded arrays must have native typed alignment");
        std::fill(unaligned.begin(), unaligned.end(), 0xCD);
        unaligned.clear();
        unaligned.shrink_to_fit();
        encoded.bytes.clear();
        encoded.bytes.shrink_to_fit();

        const auto first = sample(*animation, 1);
        near(first.mScale.x, 1, "a zero-key scale uses the original unit default");
        near(first.mScale.y, 2.5F, "a constant scale ignores its tangent type");
        near(first.mScale.z, 3, "linear Hermite scale survives input-buffer retirement");
        near(first.mTranslate.z, 0, "a zero-key translation ignores its unused out-of-range offset");
        require(first.mRotation.x == -492, "constant signed rotations retain their shift");
        const auto second = sample(*animation, 1, 1);
        near(second.mScale.x, 9, "joint indexing must reach the second joint's distinct scale");
        near(second.mTranslate.x, -99, "joint indexing must reach the second joint's translation");
        require(second.mRotation.z == 4, "shifted rotations retain their low sixteen bits");
    }

    void test_keyed_hermite_tangents_integer_rounding_and_endpoints() {
        auto encoded = encode(keyed_input());
        auto animation = smgpc::resource::load_j3d_transform_animation(encoded.bytes);
        auto value = sample(*animation, 1);
        // h00=.84375, h10=.140625, h01=.15625, h11=-.046875.
        // Values10/30 and duration-scaled outgoing8/incoming-4 give14.4375.
        near(value.mTranslate.x, 14.4375F, "type1 must use the first outgoing and second incoming tangent");
        require(value.mRotation.y == 4 && value.mRotation.z == -4,
                "s16 Hermite values +1.625/-1.625 truncate toward zero before the two-bit shift");
        value = sample(*animation, 2);
        near(value.mTranslate.x, 21.5F, "asymmetric tangents must affect the middle of the segment");
        near(value.mScale.z, 4, "type0 retains its one tangent per key");
        for (float frame : {-20.0F, -0.5F, 0.0F}) {
            value = sample(*animation, frame);
            near(value.mTranslate.x, 10, "frames before the first key hold the first value");
            require(value.mRotation.y == 4 && value.mRotation.z == -4, "first signed key values remain shifted");
        }
        for (float frame : {4.0F, 4.5F, 8.0F, 100.0F}) {
            value = sample(*animation, frame);
            near(value.mTranslate.x, 30, "raw keyed frames at and past the end must hold the final key without wrapping");
            near(value.mScale.z, 6, "each float channel independently holds its final key");
            require(value.mRotation.y == 20 && value.mRotation.z == -20, "final signed key values remain shifted");
        }

        encoded.bytes[Block + 9] = 64;
        auto shifted = smgpc::resource::load_j3d_transform_animation(encoded.bytes);
        require(dynamic_cast<J3DAnmTransformKey*>(shifted.get())->mDecShift == 64,
                "the loader must preserve the stored shift byte");
        require(sample(*shifted, 1).mRotation.x == -123, "PPC SLW uses six shift bits, so shift64 acts as shift0");
        for (std::uint8_t shift : {std::uint8_t{31}, std::uint8_t{32}, std::uint8_t{255}}) {
            encoded.bytes[Block + 9] = shift;
            shifted = smgpc::resource::load_j3d_transform_animation(encoded.bytes);
            const auto rotated = sample(*shifted, 1);
            require(rotated.mRotation.x == 0 && rotated.mRotation.y == 0 && rotated.mRotation.z == 0,
                    "large shift counts must preserve PPC zero/low-word behavior without native undefined shifts");
        }
    }

    void test_keyed_original_type_predicate_and_duplicate_times() {
        auto encoded = encode(keyed_input());
        write16(encoded.bytes, Table + Translation * 6 + 4, 7);
        auto animation = smgpc::resource::load_j3d_transform_animation(encoded.bytes, true);
        auto* key = dynamic_cast<J3DAnmTransformKey*>(animation.get());
        require(key != nullptr && key->mAnmTable[0].mTranslateInfo.mType == 7,
                "every nonzero tangent type is a preserved four-tuple and the full-lerp option does not change BCK kind");
        near(sample(*animation, 1).mTranslate.x, 14.4375F, "nonzero tangent types must use original four-value semantics");

        Input input;
        input.shift = 0;
        input.joints[0][0][Translation] = append<float>(input.translations, 3, 0, {0, 1, 0, 0, 5, 0, 4, 9, 0});
        animation = smgpc::resource::load_j3d_transform_animation(encode(input).bytes);
        near(sample(*animation, 0).mTranslate.x, 5, "search must select the last duplicate key at the initial time");
        near(sample(*animation, 2).mTranslate.x, 7, "the selected duplicate starts a valid positive-length segment");

        input.translations.clear();
        input.joints[0][0][Translation] = append<float>(input.translations, 4, 0, {0, 1, 0, 2, 5, 0, 2, 9, 0, 4, 13, 0});
        animation = smgpc::resource::load_j3d_transform_animation(encode(input).bytes);
        near(sample(*animation, 2).mTranslate.x, 9, "search must select the last duplicate at an interior key time");
        near(sample(*animation, 3).mTranslate.x, 11, "interior duplicates must not create a zero-duration interpolation");

        input.translations.clear();
        input.joints[0][0][Translation] = append<float>(input.translations, 2, 0, {2, 1, 0, 2, 5, 0});
        animation = smgpc::resource::load_j3d_transform_animation(encode(input).bytes);
        near(sample(*animation, 0).mTranslate.x, 1, "all-equal times retain the original before-first endpoint");
        near(sample(*animation, 2).mTranslate.x, 5, "all-equal times retain the original final endpoint");
    }

    void test_full_nearest_sample_and_independent_channel_lengths() {
        auto animation = smgpc::resource::load_j3d_transform_animation(encode(full_input()).bytes);
        require(dynamic_cast<J3DAnmTransformFull*>(animation.get()) != nullptr &&
                    dynamic_cast<J3DAnmTransformFullWithLerp*>(animation.get()) == nullptr && animation->getKind() == 9 &&
                    animation->field_0x1e == 1 && animation->getFrameMax() == 2 && animation->getAttribute() == 2,
                "ANF1 without the flag must retain the original full animation type and metadata");
        for (float frame : {-10.0F, -0.25F, 0.0F, 0.499F}) {
            const auto value = sample(*animation, frame);
            near(value.mScale.x, 2, "negative/early full frames select the first sample");
            near(value.mTranslate.x, 10, "nearest-sample rounding changes at one half");
            require(value.mRotation.x == 32760, "full rotations have no BCK shift");
        }
        auto value = sample(*animation, 0.5F);
        near(value.mScale.x, 6, "frame .5 rounds to the second full sample");
        near(value.mScale.y, 3, "a shorter full channel independently holds its only sample");
        near(value.mTranslate.x, 14, "full mode performs nearest sampling, not interpolation");
        require(value.mRotation.x == -32760, "nearest full rotation selects the stored signed sample");
        for (float frame : {1.5F, 2.0F, 2.5F, 50.0F}) {
            value = sample(*animation, frame);
            near(value.mScale.x, 10, "full frames use channel lengths rather than wrapping by header frameMax");
            near(value.mScale.z, 8, "a two-sample scale clamps independently");
            near(value.mTranslate.x, 22, "full samples at and beyond frameMax retain the final value");
            near(value.mTranslate.y, 2, "a shorter translation independently clamps");
        }

        // J3DAnmFullLoader_v15::setAnmTransform never reads these three
        // unnamed header fields; each channel descriptor determines its reads.
        for (std::uint16_t unused_metadata : {std::uint16_t{0}, std::uint16_t{1}}) {
            auto encoded = encode(full_input());
            write16(encoded.bytes, Block + 0xE, unused_metadata);
            write16(encoded.bytes, Block + 0x10, unused_metadata);
            write16(encoded.bytes, Block + 0x12, unused_metadata);
            for (bool interpolate : {false, true}) {
                auto original = smgpc::resource::load_j3d_transform_animation(encoded.bytes, interpolate);
                value = sample(*original, 1.5F);
                near(value.mScale.x, interpolate ? 8 : 10,
                     "ANF1 scale reads are independent of unused header metadata");
                require(value.mRotation.x == (interpolate ? -32750 : -32740),
                        "ANF1 rotation reads are independent of unused header metadata");
                near(value.mTranslate.x, interpolate ? 18 : 22,
                     "ANF1 translation reads are independent of unused header metadata");
            }
        }
    }

    void test_full_lerp_and_rotation_wraps() {
        auto animation = smgpc::resource::load_j3d_transform_animation(encode(full_input()).bytes, true);
        require(dynamic_cast<J3DAnmTransformFullWithLerp*>(animation.get()) != nullptr && animation->getKind() == 16,
                "the explicit lerp flag must construct the original FullWithLerp class");
        auto value = sample(*animation, 0.25F);
        near(value.mScale.x, 3, "full lerp interpolates fractional scale");
        near(value.mTranslate.x, 11, "full lerp interpolates fractional translation");
        require(value.mRotation.y == 5 && value.mRotation.z == -5,
                "full rotations cross unsigned wrap in both directions by the shorter delta");
        value = sample(*animation, 0.5F);
        near(value.mScale.x, 4, "lerp must differ from nearest-sample mode at frame .5");
        near(value.mTranslate.x, 12, "lerp must preserve the actual half-frame translation");
        require(value.mRotation.x == -32768 && value.mRotation.y == 0 && value.mRotation.z == 0,
                "full lerp must cross both the signed boundary and the zero-angle wrap");
        value = sample(*animation, 1.5F);
        near(value.mScale.x, 8, "full lerp selects a later interpolation segment");
        near(value.mScale.z, 8, "full lerp holds short channels once the next sample is unavailable");
        near(value.mTranslate.x, 18, "full lerp interpolates a later translation segment");
        for (float frame : {-1.0F, 0.0F}) {
            near(sample(*animation, frame).mScale.x, 2, "full lerp retains the first endpoint");
        }
        for (float frame : {2.0F, 2.75F, 100.0F}) {
            near(sample(*animation, frame).mTranslate.x, 22, "full lerp holds its final endpoint without wrapping");
        }

        auto half_turn = full_input();
        half_turn.rotations[0] = 0;
        half_turn.rotations[1] = -32768;
        half_turn.rotations[3] = -32768;
        half_turn.rotations[4] = 0;
        animation = smgpc::resource::load_j3d_transform_animation(encode(half_turn).bytes, true);
        value = sample(*animation, 0.5F);
        require(value.mRotation.x == 16384 && value.mRotation.y == 16384,
                "exact half-turn deltas retain the original strict greater-than wrap threshold");
    }

    void check_renderer_sample(const smgpc::render::J3dBckAnimationSummary& summary,
                               J3DAnmTransform& original, float frame, std::uint16_t joint) {
        const auto normalized = smgpc::render::j3d_animation_frame(summary.attribute, summary.frame_max, frame);
        const auto expected = sample(original, normalized, joint);
        const auto actual = smgpc::render::j3d_evaluate_bck_joint_transform(summary, joint, frame);
        require(actual.has_value() && actual->scale == std::array<float, 3>{expected.mScale.x, expected.mScale.y, expected.mScale.z} &&
                    actual->rotation == std::array<std::int16_t, 3>{expected.mRotation.x, expected.mRotation.y, expected.mRotation.z} &&
                    actual->translation == std::array<float, 3>{expected.mTranslate.x, expected.mTranslate.y, expected.mTranslate.z},
                "renderer joint transforms must equal original sampling at the renderer's normalized frame");
    }

    void test_renderer_summary_copy_ownership_and_delegation() {
        std::weak_ptr<const J3DAnmTransformKey> lifetime;
        auto retained = [&] {
            auto bytes = encode(keyed_input()).bytes;
            auto parsed = smgpc::render::inspect_j3d_animation(bytes);
            require(parsed.bck.has_value() && parsed.bck->transform_animation,
                    "BCK inspection must retain an actual original transform object");
            lifetime = parsed.bck->transform_animation;
            auto copied = *parsed.bck;
            std::fill(bytes.begin(), bytes.end(), 0xDD);
            return copied;
        }();
        auto second_copy = retained;
        require(retained.transform_animation == second_copy.transform_animation && !lifetime.expired(),
                "copied renderer summaries share ownership after parser and source bytes disappear");
        auto original = smgpc::resource::load_j3d_transform_animation(encode(keyed_input()).bytes);
        for (std::uint16_t joint = 0; joint < original->field_0x1e; ++joint) {
            for (float frame : {-1.0F, 0.0F, 1.0F, 1.5F, 4.0F, 5.0F, 100.0F}) {
                check_renderer_sample(second_copy, *original, frame, joint);
            }
        }
        near(sample(*original, 4).mTranslate.x, 30, "direct original sampling retains the raw final key");
        const auto rendered_end = smgpc::render::j3d_evaluate_bck_joint_transform(second_copy, 0, 4);
        require(rendered_end.has_value(), "the normalized renderer endpoint must remain available");
        near(rendered_end->translation[0], 10, "the renderer retains its existing loop policy before original sampling");
        retained.transform_animation.reset();
        require(!lifetime.expired(), "another summary copy must keep the typed object alive");
        second_copy.transform_animation.reset();
        require(lifetime.expired(), "the typed transform owner must retire with its final summary reference");
    }

    void rejected(const std::vector<std::uint8_t>& bytes, std::string_view message) {
        bool threw = false;
        try {
            static_cast<void>(smgpc::resource::load_j3d_transform_animation(bytes));
        } catch (const std::runtime_error&) {
            threw = true;
        }
        require(threw, message);
    }

    void test_file_structure_and_rejected_extents() {
        const auto valid = encode(keyed_input());
        auto with_unknown = valid.bytes;
        with_unknown.insert(with_unknown.begin() + Block, 8, 0);
        tag(with_unknown, Block, "EXT1");
        write32(with_unknown, Block + 4, 8);
        write32(with_unknown, 8, static_cast<std::uint32_t>(with_unknown.size()));
        write32(with_unknown, 0xC, 2);
        auto animation = smgpc::resource::load_j3d_transform_animation(with_unknown);
        near(sample(*animation, 1).mTranslate.x, 14.4375F, "unknown blocks must be skipped before the actual transform block");

        auto bad = valid.bytes;
        bad.resize(31);
        rejected(bad, "a truncated file header must be rejected");
        bad = valid.bytes;
        tag(bad, 0, "J3D2");
        rejected(bad, "an unsupported file magic must be rejected");
        bad = valid.bytes;
        write32(bad, 8, static_cast<std::uint32_t>(bad.size() + 4));
        rejected(bad, "a declared file length beyond the supplied span must be rejected");
        bad = valid.bytes;
        write32(bad, 8, static_cast<std::uint32_t>(bad.size() - 4));
        rejected(bad, "blocks cannot read beyond the declared file length even if caller bytes exist");
        bad = valid.bytes;
        write32(bad, Block + 4, 0);
        rejected(bad, "a zero-size block cannot advance the original block chain");
        bad = valid.bytes;
        write32(bad, Block + 4, 0xFFFFFFFC);
        rejected(bad, "a block outside the resource span must be rejected without integer overflow");
        bad = valid.bytes;
        tag(bad, Block, "EXT1");
        rejected(bad, "a transform file needs an actual matching transform block");
        bad = valid.bytes;
        write32(bad, Block + 0x14, 0xFFFFFFFC);
        rejected(bad, "joint tables must fit within the actual transform block");
        bad = valid.bytes;
        write16(bad, Block + 0xC, 0xFFFF);
        rejected(bad, "the declared joint count must fit its complete axis table");
        bad = valid.bytes;
        write16(bad, Table + 18 + 2, 0xFFFF);
        rejected(bad, "a single-value channel must reference one available pool element");
        bad = valid.bytes;
        write16(bad, Table + 2 * 18 + 4, 1);
        rejected(bad, "four-value key records must fit their entire declared value extent");
        bad = valid.bytes;
        write_float(bad, valid.scales + 4, std::numeric_limits<float>::quiet_NaN());
        rejected(bad, "an active key time must be finite");
        bad = valid.bytes;
        write_float(bad, valid.scales + 4 * 4, -1);
        rejected(bad, "decreasing key times violate the original binary-search contract");
        bad = encode(full_input()).bytes;
        write16(bad, Table, 0);
        rejected(bad, "full channels require a sample because the original full sampler has no zero-count fallback");
        bad = encode(full_input()).bytes;
        write16(bad, Table + 4 + 2, 0xFFFF);
        rejected(bad, "full channel reads must fit the actual transform block");
    }

    void check_real_native_arrays(const J3DAnmTransformKey& key, std::span<const std::uint8_t> data) {
        require(data.size() >= Table && read32(data, Block) == 0x414E4B31U,
                "retail fixture must have its authored ANK1 transform block");
        require(key.field_0x1e == read16(data, Block + 0xC) && key.getAttribute() == data[Block + 8] &&
                    key.getFrameMax() == std::bit_cast<std::int16_t>(read16(data, Block + 0xA)) && key.mDecShift == data[Block + 9],
                "retail typed animation metadata must match its actual file");
        const auto compare_floats = [&](const float* native, std::size_t count_field, std::size_t offset_field) {
            const auto offset = Block + read32(data, Block + offset_field);
            for (std::size_t i = 0; i < read16(data, Block + count_field); ++i) {
                require(std::bit_cast<std::uint32_t>(native[i]) == read32(data, offset + i * 4),
                        "retail float pools must preserve every big-endian value in native storage");
            }
        };
        compare_floats(key.mScaleData, 0xE, 0x18);
        compare_floats(key.mTransData, 0x12, 0x20);
        const auto rotation_offset = Block + read32(data, Block + 0x1C);
        for (std::size_t i = 0; i < read16(data, Block + 0x10); ++i) {
            require(static_cast<std::uint16_t>(key.mRotData[i]) == read16(data, rotation_offset + i * 2),
                    "retail signed rotation pools must preserve every original value");
        }
        const auto table_offset = Block + read32(data, Block + 0x14);
        for (std::size_t axis = 0; axis < key.field_0x1e * 3U; ++axis) {
            const auto& row = key.mAnmTable[axis];
            const std::array<const J3DAnmKeyTableBase*, 3> channels{&row.mScaleInfo, &row.mRotationInfo, &row.mTranslateInfo};
            for (std::size_t channel = 0; channel < channels.size(); ++channel) {
                const auto offset = table_offset + axis * 18 + channel * 6;
                require(channels[channel]->mMaxFrame == read16(data, offset) &&
                            channels[channel]->mOffset == read16(data, offset + 2) && channels[channel]->mType == read16(data, offset + 4),
                        "retail key descriptors must retain count, value index and tangent type");
            }
        }
    }

    void test_optional_real_disc() {
        const auto* path = std::getenv("SMGPC_REAL_DISC");
        if (path == nullptr || path[0] == '\0') {
            std::cout << "[skip] real Mario transform resources (set SMGPC_REAL_DISC)\n";
            return;
        }
        aurora_dvd_close();
        require(aurora_dvd_open(path), "SMGPC_REAL_DISC must be a readable original game image");
        struct DiscOwner {
            ~DiscOwner() { aurora_dvd_close(); }
        } disc;
        DVDInit();
        std::vector<std::unique_ptr<J3DAnmTransform>> animations;
        std::vector<smgpc::render::J3dBckAnimationSummary> renderer_summaries;
        std::vector<std::unique_ptr<J3DAnmTransform>> full_animations;
        {
            smgpc::runtime::DvdFileSystemService dvd{"/"};
            // Original MarioActorDraw::initDraw names this animation archive;
            // these three files also appear in the authored MarioAnimatorData tables.
            const auto archive_path = dvd.find_object_archive("MarioAnime");
            require(archive_path.has_value(), "the real Mario animation archive must be available");
            const auto& archive = dvd.archive_for_path(*archive_path);
            for (std::string_view name : {"wait.bck", "run.bck", "jump.bck"}) {
                const auto* entry = archive.find_by_basename(name);
                require(entry != nullptr, std::string("MarioAnime is missing ") + std::string(name));
                const auto bytes = archive.file_data(*entry);
                auto animation = smgpc::resource::load_j3d_transform_animation(bytes);
                auto* key = dynamic_cast<J3DAnmTransformKey*>(animation.get());
                require(key != nullptr && key->field_0x1e != 0 && key->getFrameMax() > 0,
                        "each actual Mario motion must load as a nonempty original Key animation");
                check_real_native_arrays(*key, bytes);
                const auto parsed = smgpc::render::inspect_j3d_animation(bytes);
                require(parsed.bck.has_value() && parsed.bck->transform_animation,
                        "each real renderer summary must retain its original transform owner");
                renderer_summaries.push_back(*parsed.bck);
                std::cout << "[resource] " << name << ": joints=" << key->field_0x1e << ", frames=" << key->getFrameMax() << '\n';
                animations.push_back(std::move(animation));
            }
            for (const auto& entry : archive.entries()) {
                const auto data = archive.file_data(entry);
                if (data.size() < 8 || read32(data, 0) != 0x4A334431U || read32(data, 4) != 0x62636131U) {
                    continue;
                }
                for (bool interpolate : {false, true}) {
                    auto animation = smgpc::resource::load_j3d_transform_animation(data, interpolate);
                    require(dynamic_cast<J3DAnmTransformFull*>(animation.get()) != nullptr &&
                                animation->field_0x1e == read16(data, Block + 0xC),
                            "an authored BCA must load its original typed full transform and joint count");
                    full_animations.push_back(std::move(animation));
                }
                std::cout << "[resource] full animation " << entry.path << '\n';
            }
        }
        // The archive and its DVD service have now been destroyed.
        for (std::size_t index = 0; index < animations.size(); ++index) {
            auto& animation = animations[index];
            bool changed = false;
            for (std::uint16_t joint = 0; joint < animation->field_0x1e; ++joint) {
                const auto initial = sample(*animation, 0, joint);
                for (float frame : {-1.0F, 1.25F, animation->getFrameMax() * 0.5F,
                                    static_cast<float>(animation->getFrameMax()), animation->getFrameMax() + 10.0F}) {
                    const auto value = sample(*animation, frame, joint);
                    check_renderer_sample(renderer_summaries[index], *animation, frame, joint);
                    for (float component : {value.mScale.x, value.mScale.y, value.mScale.z,
                                            value.mTranslate.x, value.mTranslate.y, value.mTranslate.z}) {
                        require(std::isfinite(component), "retired-archive sampling must remain finite for every real joint");
                    }
                    changed = changed || value.mScale.x != initial.mScale.x || value.mScale.y != initial.mScale.y ||
                              value.mScale.z != initial.mScale.z || value.mTranslate.x != initial.mTranslate.x ||
                              value.mTranslate.y != initial.mTranslate.y || value.mTranslate.z != initial.mTranslate.z ||
                              value.mRotation.x != initial.mRotation.x || value.mRotation.y != initial.mRotation.y ||
                              value.mRotation.z != initial.mRotation.z;
                }
            }
            require(changed, "each authored Wait/Run/Jump resource must produce a real changing joint pose");
        }
        if (full_animations.empty()) {
            std::cout << "[skip] no authored BCA in MarioAnime.arc; full sampling is covered by byte fixtures\n";
        }
        for (std::size_t index = 0; index < full_animations.size(); index += 2) {
            auto& nearest = *full_animations[index];
            auto& interpolated = *full_animations[index + 1];
            for (std::uint16_t joint = 0; joint < nearest.field_0x1e; ++joint) {
                for (float frame : {0.0F, 1.0F, static_cast<float>(nearest.getFrameMax())}) {
                    const auto a = sample(nearest, frame, joint);
                    const auto b = sample(interpolated, frame, joint);
                    require(a.mScale.x == b.mScale.x && a.mScale.y == b.mScale.y && a.mScale.z == b.mScale.z &&
                                a.mTranslate.x == b.mTranslate.x && a.mTranslate.y == b.mTranslate.y && a.mTranslate.z == b.mTranslate.z &&
                                a.mRotation.x == b.mRotation.x && a.mRotation.y == b.mRotation.y && a.mRotation.z == b.mRotation.z,
                            "authored full and full-lerp owners agree on integer frames after archive retirement");
                }
            }
        }
    }

}  // namespace

int main() {
    try {
        test_keyed_metadata_and_owned_native_arrays();
        test_keyed_hermite_tangents_integer_rounding_and_endpoints();
        test_keyed_original_type_predicate_and_duplicate_times();
        test_full_nearest_sample_and_independent_channel_lengths();
        test_full_lerp_and_rotation_wraps();
        test_renderer_summary_copy_ownership_and_delegation();
        test_file_structure_and_rejected_extents();
        test_optional_real_disc();
        std::cout << "7/7 original J3D transform-animation groups passed, plus available real-disc resources\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
