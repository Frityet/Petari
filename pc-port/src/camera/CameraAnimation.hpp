#pragma once

#include "camera/CameraParam.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace smgpc::camera {

    enum class CameraAnimationFormat {
        Canm,
        Ckan,
    };

    struct CameraAnimationSample {
        CameraParamVec3 eye{};
        CameraParamVec3 watch{};
        float twist_degrees = 0.0F;
        float fovy_degrees = 45.0F;
    };

    class CameraAnimation final {
    public:
        [[nodiscard]] static CameraAnimation from_bytes(
            std::span<const std::uint8_t> bytes);

        [[nodiscard]] CameraAnimationFormat format() const noexcept;
        [[nodiscard]] std::uint32_t frame_count() const noexcept;
        [[nodiscard]] CameraAnimationSample sample(float frame) const;

    private:
        struct Component {
            std::uint32_t count = 0U;
            std::uint32_t offset = 0U;
            std::uint32_t type = 0U;
        };

        [[nodiscard]] float sample_component(const Component &component,
                                             float frame) const;

        CameraAnimationFormat _format = CameraAnimationFormat::Canm;
        std::uint32_t _frame_count = 0U;
        std::array<Component, 8U> _components{};
        std::vector<float> _values{};
    };

}  // namespace smgpc::camera
