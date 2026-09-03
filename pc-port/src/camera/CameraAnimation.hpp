#pragma once

#include "camera/CameraParam.hpp"

#include <cstdint>
#include <memory>
#include <span>

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

    // A copy retains the aligned native block even after its source animation
    // is destroyed. Original CameraAnim accessors borrow this block read-only.
    class NativeCameraAnimationData final {
    public:
        [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept;

    private:
        friend class CameraAnimation;
        struct Storage;
        std::shared_ptr<const Storage> _storage{};
    };

    class CameraAnimation final {
    public:
        [[nodiscard]] static CameraAnimation from_bytes(
            std::span<const std::uint8_t> bytes);

        [[nodiscard]] CameraAnimationFormat format() const noexcept;
        [[nodiscard]] std::uint32_t frame_count() const noexcept;
        [[nodiscard]] NativeCameraAnimationData native_data() const noexcept;
        [[nodiscard]] CameraAnimationSample sample(float frame) const;

    private:
        CameraAnimationFormat _format = CameraAnimationFormat::Canm;
        std::uint32_t _frame_count = 0U;
        NativeCameraAnimationData _native_data{};
    };

}  // namespace smgpc::camera
