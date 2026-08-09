#pragma once

#include "render/BrightVisibilityService.hpp"

#include <cstdint>
#include <memory>

namespace smgpc::render {

    enum class BrightDepthSnapshotStatus {
        Pending,
        Ready,
        Dropped,
        Unknown,
    };

    struct BrightDepthSnapshotInfo {
        std::uint64_t id = 0U;
        std::uint64_t frame_id = 0U;
        std::uint32_t width = 0U;
        std::uint32_t height = 0U;
        float viewport_near = 0.0F;
        float viewport_far = 1.0F;
    };

    // Narrow backend seam for the tagged Aurora snapshots.  Keeping the
    // visibility aggregator independent of WebGPU makes out-of-order, dropped,
    // and pointer-reuse behavior deterministic in focused tests.
    class BrightDepthSnapshotBackend {
    public:
        virtual ~BrightDepthSnapshotBackend() = default;

        [[nodiscard]] virtual std::uint64_t request_snapshot() noexcept = 0;
        [[nodiscard]] virtual BrightDepthSnapshotStatus query_snapshot(
            std::uint64_t id, BrightDepthSnapshotInfo& info) noexcept = 0;
        [[nodiscard]] virtual bool read_z24(std::uint64_t id, std::uint16_t x,
                                            std::uint16_t y,
                                            std::uint32_t& z24) noexcept = 0;
        virtual void release_snapshot(std::uint64_t id) noexcept = 0;
    };

    class AuroraBrightVisibilityService final : public BrightVisibilityService {
    public:
        AuroraBrightVisibilityService();
        explicit AuroraBrightVisibilityService(BrightDepthSnapshotBackend& backend);
        ~AuroraBrightVisibilityService() override;

        AuroraBrightVisibilityService(const AuroraBrightVisibilityService&) = delete;
        AuroraBrightVisibilityService& operator=(const AuroraBrightVisibilityService&) = delete;

        void reset() noexcept;

        void begin_draw_pass(std::uint16_t draw_token) override;
        void begin_capture(std::uint16_t draw_token) override;
        void submit_batch(const BrightVisibilityBatch& batch) override;
        void end_capture() override;
        void submit_sphere(BrightVisibilitySourceId source,
                           const BrightVisibilitySphere& sphere) override;
        [[nodiscard]] bool take_result(BrightVisibilitySourceId source,
                                       BrightVisibilityResult& result) override;
        void forget_source(BrightVisibilitySourceId source) noexcept override;

    private:
        struct Impl;
        std::unique_ptr< Impl > _impl;
    };

}  // namespace smgpc::render
