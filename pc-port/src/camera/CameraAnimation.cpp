#include "camera/CameraAnimation.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace smgpc::camera {
    namespace {

        [[nodiscard]] std::uint32_t read_be_u32(
            std::span<const std::uint8_t> bytes, std::size_t offset) {
            if (offset + 4U > bytes.size()) {
                throw std::runtime_error(
                    "Camera animation field is outside the CANM resource.");
            }
            return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
                   (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
                   (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) |
                   static_cast<std::uint32_t>(bytes[offset + 3U]);
        }

        [[nodiscard]] float read_be_float(
            std::span<const std::uint8_t> bytes, std::size_t offset) {
            return std::bit_cast<float>(read_be_u32(bytes, offset));
        }

        [[nodiscard]] bool has_tag(std::span<const std::uint8_t> bytes,
                                   std::size_t offset,
                                   std::string_view tag) {
            return offset + tag.size() <= bytes.size() &&
                   std::equal(tag.begin(), tag.end(), bytes.begin() + offset);
        }

        [[nodiscard]] float hermite(float frame, float first_frame,
                                    float first_value, float first_tangent,
                                    float second_frame, float second_value,
                                    float second_tangent) {
            const auto duration = second_frame - first_frame;
            if (!(duration > 0.0F)) {
                return first_value;
            }
            const auto t = std::clamp((frame - first_frame) / duration, 0.0F,
                                      1.0F);
            const auto t2 = t * t;
            const auto t3 = t2 * t;
            const auto h00 = 2.0F * t3 - 3.0F * t2 + 1.0F;
            const auto h10 = t3 - 2.0F * t2 + t;
            const auto h01 = -2.0F * t3 + 3.0F * t2;
            const auto h11 = t3 - t2;
            // Retail camera-key tangents are authored in 30 Hz units.
            return h00 * first_value +
                   h10 * duration * (first_tangent / 30.0F) +
                   h01 * second_value +
                   h11 * duration * (second_tangent / 30.0F);
        }

    }  // namespace

    CameraAnimation CameraAnimation::from_bytes(
        std::span<const std::uint8_t> bytes) {
        constexpr auto cHeaderSize = std::size_t{0x20U};
        if (bytes.size() < cHeaderSize || !has_tag(bytes, 0U, "ANDO")) {
            throw std::runtime_error(
                "Camera animation resource does not have an ANDO header.");
        }

        auto result = CameraAnimation{};
        auto component_size = std::size_t{};
        if (has_tag(bytes, 4U, "CANM")) {
            result._format = CameraAnimationFormat::Canm;
            component_size = 8U;
        } else if (has_tag(bytes, 4U, "CKAN")) {
            result._format = CameraAnimationFormat::Ckan;
            component_size = 12U;
        } else {
            throw std::runtime_error(
                "Camera animation resource is neither CANM nor CKAN.");
        }
        if (read_be_u32(bytes, 0x08U) == 0U) {
            throw std::runtime_error(
                "Camera animation resource has an unsupported zero version.");
        }

        result._frame_count = read_be_u32(bytes, 0x18U);
        const auto value_offset =
            cHeaderSize + static_cast<std::size_t>(read_be_u32(bytes, 0x1cU));
        const auto component_table_size =
            result._components.size() * component_size;
        if (cHeaderSize + component_table_size > bytes.size() ||
            value_offset + 4U > bytes.size()) {
            throw std::runtime_error(
                "Camera animation component or value table is truncated.");
        }

        const auto value_byte_count =
            static_cast<std::size_t>(read_be_u32(bytes, value_offset));
        if ((value_byte_count % sizeof(float)) != 0U ||
            value_offset + 4U + value_byte_count > bytes.size()) {
            throw std::runtime_error(
                "Camera animation float table has an invalid extent.");
        }

        for (auto index = std::size_t{}; index < result._components.size();
             ++index) {
            const auto offset = cHeaderSize + index * component_size;
            auto &component = result._components[index];
            component.count = read_be_u32(bytes, offset);
            component.offset = read_be_u32(bytes, offset + 4U);
            if (result._format == CameraAnimationFormat::Ckan) {
                component.type = read_be_u32(bytes, offset + 8U);
                if (component.type > 1U) {
                    throw std::runtime_error(
                        "CKAN component uses an unsupported tangent layout.");
                }
            }
        }

        result._values.reserve(value_byte_count / sizeof(float));
        for (auto offset = value_offset + 4U;
             offset < value_offset + 4U + value_byte_count; offset += 4U) {
            const auto value = read_be_float(bytes, offset);
            if (!std::isfinite(value)) {
                throw std::runtime_error(
                    "Camera animation contains a non-finite authored value.");
            }
            result._values.push_back(value);
        }

        for (const auto &component : result._components) {
            if (component.count == 0U) {
                throw std::runtime_error(
                    "Camera animation component has no authored samples.");
            }
            const auto stride = result._format == CameraAnimationFormat::Canm ? 1U : component.type == 0U ? 3U :
                                                                                                            4U;
            const auto required = component.count == 1U ? std::size_t{1U} : static_cast<std::size_t>(component.count) * stride;
            if (static_cast<std::size_t>(component.offset) + required >
                result._values.size()) {
                throw std::runtime_error(
                    "Camera animation component exceeds its float table.");
            }
        }
        return result;
    }

    CameraAnimationFormat CameraAnimation::format() const noexcept {
        return _format;
    }

    std::uint32_t CameraAnimation::frame_count() const noexcept {
        return _frame_count;
    }

    CameraAnimationSample CameraAnimation::sample(float frame) const {
        const auto bounded_frame =
            std::clamp(frame, 0.0F, static_cast<float>(_frame_count));
        return CameraAnimationSample{
            .eye = {sample_component(_components[0], bounded_frame),
                    sample_component(_components[1], bounded_frame),
                    sample_component(_components[2], bounded_frame)},
            .watch = {sample_component(_components[3], bounded_frame),
                      sample_component(_components[4], bounded_frame),
                      sample_component(_components[5], bounded_frame)},
            .twist_degrees =
                sample_component(_components[6], bounded_frame),
            .fovy_degrees = sample_component(_components[7], bounded_frame),
        };
    }

    float CameraAnimation::sample_component(const Component &component,
                                            float frame) const {
        const auto offset = static_cast<std::size_t>(component.offset);
        if (component.count == 1U) {
            return _values[offset];
        }

        if (_format == CameraAnimationFormat::Canm) {
            const auto last = component.count - 1U;
            if (frame >= static_cast<float>(last)) {
                return _values[offset + last];
            }
            const auto first = static_cast<std::uint32_t>(std::floor(frame));
            const auto fraction = frame - static_cast<float>(first);
            const auto first_value = _values[offset + first];
            const auto second_value = _values[offset + first + 1U];
            return first_value + (second_value - first_value) * fraction;
        }

        const auto stride = component.type == 0U ? std::size_t{3U} : std::size_t{4U};
        const auto frame_at = [&](std::size_t index) {
            return _values[offset + index * stride];
        };
        if (frame <= frame_at(0U)) {
            return _values[offset + 1U];
        }
        const auto last = static_cast<std::size_t>(component.count - 1U);
        if (frame >= frame_at(last)) {
            return _values[offset + last * stride + 1U];
        }

        auto upper = std::size_t{1U};
        while (upper < component.count && frame_at(upper) <= frame) {
            ++upper;
        }
        const auto lower = upper - 1U;
        const auto lower_base = offset + lower * stride;
        const auto upper_base = offset + upper * stride;
        const auto lower_tangent =
            _values[lower_base + (component.type == 0U ? 2U : 3U)];
        const auto upper_tangent = _values[upper_base + 2U];
        return hermite(frame, _values[lower_base], _values[lower_base + 1U],
                       lower_tangent, _values[upper_base],
                       _values[upper_base + 1U], upper_tangent);
    }

}  // namespace smgpc::camera
