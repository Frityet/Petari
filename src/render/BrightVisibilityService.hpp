#pragma once

#include "JSystem/JGeometry/TMatrix.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace smgpc::render {

    using BrightVisibilitySourceId = std::uint64_t;
    inline constexpr BrightVisibilitySourceId kInvalidBrightVisibilitySourceId = 0U;

    struct BrightVisibilitySphere {
        TVec3f center{};
        float radius = 0.0F;
    };

    inline constexpr std::size_t kBrightVisibilityProbeCount = 17U;

    struct BrightVisibilityProbe {
        bool on_screen = false;
        TVec2f screen_position{};
        TVec2f framebuffer_position{};
    };

    // One value-owned copy of BrightObjBase's exact seventeen-probe disk.  It
    // deliberately retains no camera, actor, or matrix pointers: Aurora depth
    // snapshots complete asynchronously and must be evaluated with the
    // projection that produced this batch, never a later frame's camera.
    struct BrightVisibilityBatch {
        BrightVisibilitySourceId source = kInvalidBrightVisibilitySourceId;
        std::uint16_t draw_token = 0U;
        BrightVisibilitySphere sphere{};
        float center_ndc_z = 0.0F;
        TVec2f real_center{};
        std::array< BrightVisibilityProbe, kBrightVisibilityProbeCount > probes{};
    };

    struct BrightVisibilityResult {
        std::uint32_t sample_count = 0U;
        std::uint32_t visible_count = 0U;
        TVec2f visible_position_sum{};
        TVec2f real_center{};
        std::uint64_t capture_id = 0U;
    };

    class BrightVisibilityService {
    public:
        virtual ~BrightVisibilityService() = default;

        // Opens the exact scheduler draw pass in which BrightObj disks may be
        // submitted. Implementations must discard eligibility from any older
        // prepared pass so a skipped draw-sync callback cannot leak a disk
        // marker into a later frame.
        virtual void begin_draw_pass(std::uint16_t draw_token) = 0;

        // The scheduler opens exactly one capture at the retail BrightSun draw
        // boundary. Every BrightObj in that pass contributes a batch to the
        // same tagged depth snapshot.
        virtual void begin_capture(std::uint16_t draw_token) = 0;
        virtual void submit_batch(const BrightVisibilityBatch& batch) = 0;
        virtual void end_capture() = 0;

        // Records that the source actually submitted its retail bright disk in
        // this pass. This keeps hidden/dead/clipped actors from manufacturing a
        // visibility result merely because their projected coordinates happen
        // to be unobstructed.
        virtual void submit_sphere(BrightVisibilitySourceId source,
                                   const BrightVisibilitySphere& sphere) = 0;

        // Results are published only after the matching tagged snapshot has
        // completed. Implementations must never regress a source to an older
        // capture when GPU callbacks finish out of order.
        [[nodiscard]] virtual bool take_result(BrightVisibilitySourceId source,
                                               BrightVisibilityResult& result) = 0;
        virtual void forget_source(BrightVisibilitySourceId source) noexcept = 0;
    };

    [[nodiscard]] BrightVisibilitySourceId allocate_bright_visibility_source_id() noexcept;
    [[nodiscard]] BrightVisibilityService* try_bright_visibility_service() noexcept;
    void begin_bright_visibility_draw_pass(std::uint16_t draw_token);
    void begin_bright_visibility_capture(std::uint16_t draw_token);
    void submit_bright_visibility_batch(const BrightVisibilityBatch& batch);
    void end_bright_visibility_capture();
    void submit_bright_visibility_sphere(BrightVisibilitySourceId source,
                                         const BrightVisibilitySphere& sphere);
    [[nodiscard]] bool take_bright_visibility_result(BrightVisibilitySourceId source,
                                                     BrightVisibilityResult& result);
    void forget_bright_visibility_source(BrightVisibilitySourceId source) noexcept;

    class ScopedBrightVisibilityServiceOverride final {
    public:
        explicit ScopedBrightVisibilityServiceOverride(BrightVisibilityService& service) noexcept;
        ~ScopedBrightVisibilityServiceOverride();

        ScopedBrightVisibilityServiceOverride(const ScopedBrightVisibilityServiceOverride&) = delete;
        ScopedBrightVisibilityServiceOverride& operator=(const ScopedBrightVisibilityServiceOverride&) = delete;

    private:
        BrightVisibilityService* _previous = nullptr;
    };

}  // namespace smgpc::render
