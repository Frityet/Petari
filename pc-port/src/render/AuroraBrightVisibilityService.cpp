#include "render/AuroraBrightVisibilityService.hpp"

#include <dolphin/gx/GXAurora.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace smgpc::render {
    namespace {

        constexpr std::uint64_t cMaxResultAge = 8U;
        constexpr std::uint32_t cMaximumZ24 = 0x00ffffffU;

        class AuroraDepthSnapshotBackend final : public BrightDepthSnapshotBackend {
        public:
            std::uint64_t request_snapshot() noexcept override {
                return GXAuroraRequestDepthSnapshot();
            }

            BrightDepthSnapshotStatus query_snapshot(
                std::uint64_t id, BrightDepthSnapshotInfo& info) noexcept override {
                AuroraDepthSnapshotInfo auroraInfo{};
                const auto status = GXAuroraGetDepthSnapshotInfo(id, &auroraInfo);
                info = {
                    .id = auroraInfo.id,
                    .frame_id = auroraInfo.frameId,
                    .width = auroraInfo.width,
                    .height = auroraInfo.height,
                    .viewport_near = auroraInfo.viewportNear,
                    .viewport_far = auroraInfo.viewportFar,
                };

                switch (status) {
                case AURORA_DEPTH_SNAPSHOT_PENDING:
                    return BrightDepthSnapshotStatus::Pending;
                case AURORA_DEPTH_SNAPSHOT_READY:
                    return BrightDepthSnapshotStatus::Ready;
                case AURORA_DEPTH_SNAPSHOT_DROPPED:
                    return BrightDepthSnapshotStatus::Dropped;
                case AURORA_DEPTH_SNAPSHOT_UNKNOWN:
                default:
                    return BrightDepthSnapshotStatus::Unknown;
                }
            }

            bool read_z24(std::uint64_t id, std::uint16_t x, std::uint16_t y,
                          std::uint32_t& z24) noexcept override {
                return GXAuroraReadDepthSnapshotZ(id, x, y, &z24) == GX_TRUE;
            }

            void release_snapshot(std::uint64_t id) noexcept override {
                GXAuroraReleaseDepthSnapshot(id);
            }
        };

        [[nodiscard]] bool finite_vec2(const TVec2f& value) noexcept {
            return std::isfinite(value.x) && std::isfinite(value.y);
        }

        [[nodiscard]] bool finite_sphere(
            const BrightVisibilitySphere& sphere) noexcept {
            return std::isfinite(sphere.center.x) &&
                   std::isfinite(sphere.center.y) &&
                   std::isfinite(sphere.center.z) &&
                   std::isfinite(sphere.radius) && sphere.radius >= 0.0F;
        }

        [[nodiscard]] bool same_sphere(const BrightVisibilitySphere& lhs,
                                       const BrightVisibilitySphere& rhs) noexcept {
            constexpr auto tolerance = 0.001F;
            return finite_sphere(lhs) && finite_sphere(rhs) &&
                   std::fabs(lhs.center.x - rhs.center.x) <= tolerance &&
                   std::fabs(lhs.center.y - rhs.center.y) <= tolerance &&
                   std::fabs(lhs.center.z - rhs.center.z) <= tolerance &&
                   std::fabs(lhs.radius - rhs.radius) <= tolerance;
        }

        [[nodiscard]] std::optional< std::uint32_t > expected_z24(
            const BrightVisibilityBatch& batch,
            const BrightDepthSnapshotInfo& info) noexcept {
            if (!std::isfinite(batch.center_ndc_z) ||
                !std::isfinite(info.viewport_near) ||
                !std::isfinite(info.viewport_far) ||
                batch.center_ndc_z < -1.0F || batch.center_ndc_z > 0.0F) {
                return std::nullopt;
            }

            const auto logicalDepth = std::clamp(
                info.viewport_far + batch.center_ndc_z *
                                             (info.viewport_far - info.viewport_near),
                0.0F, 1.0F);
            const auto quantized = static_cast< std::uint32_t >(
                logicalDepth * static_cast< float >(cMaximumZ24) + 0.5F);
            return std::min(quantized, cMaximumZ24);
        }

        [[nodiscard]] bool valid_snapshot_info(
            const BrightDepthSnapshotInfo& info,
            std::uint64_t expected_id) noexcept {
            constexpr auto maximumDimension =
                static_cast< std::uint32_t >(
                    std::numeric_limits< std::uint16_t >::max()) +
                1U;
            return info.id == expected_id && info.width != 0U &&
                   info.height != 0U && info.width <= maximumDimension &&
                   info.height <= maximumDimension &&
                   std::isfinite(info.viewport_near) &&
                   std::isfinite(info.viewport_far) &&
                   info.viewport_near >= 0.0F &&
                   info.viewport_near <= 1.0F &&
                   info.viewport_far >= 0.0F &&
                   info.viewport_far <= 1.0F;
        }

    }  // namespace

    struct AuroraBrightVisibilityService::Impl {
        struct SourceState {
            std::uint64_t last_completed_capture = 0U;
            std::uint64_t last_completed_sequence = 0U;
            std::uint64_t last_ready_sequence = 0U;
            std::uint64_t minimum_accepted_sequence = 0U;
            TVec2f last_real_center{};
            std::optional< BrightVisibilityResult > ready_result;
            bool stale_zero_published = false;
        };

        struct SubmittedBatch {
            BrightVisibilityBatch batch{};
            bool disk_was_drawn = false;
        };

        struct DrawSubmission {
            BrightVisibilitySphere sphere{};
            std::uint64_t generation = 0U;
            std::uint16_t draw_token = 0U;
        };

        struct DrawPass {
            std::uint64_t generation = 0U;
            std::uint16_t draw_token = 0U;
            std::unordered_map< BrightVisibilitySourceId, DrawSubmission >
                drawn_sources;
        };

        struct Capture {
            std::uint64_t snapshot_id = 0U;
            std::uint64_t sequence = 0U;
            std::uint64_t draw_generation = 0U;
            std::uint16_t draw_token = 0U;
            std::unordered_map< BrightVisibilitySourceId, DrawSubmission >
                drawn_sources;
            std::vector< SubmittedBatch > batches;
        };

        explicit Impl(BrightDepthSnapshotBackend& backendIn)
            : backend(&backendIn) {
        }

        Impl()
            : owned_backend(std::make_unique< AuroraDepthSnapshotBackend >()),
              backend(owned_backend.get()) {
        }

        ~Impl() {
            reset();
        }

        void reset() noexcept {
            if (backend != nullptr) {
                if (active_capture.has_value() &&
                    active_capture->snapshot_id != 0U) {
                    backend->release_snapshot(active_capture->snapshot_id);
                }
                for (const auto& capture : pending_captures) {
                    if (capture.snapshot_id != 0U) {
                        backend->release_snapshot(capture.snapshot_id);
                    }
                }
            }
            active_capture.reset();
            pending_captures.clear();
            prepared_draw_pass.reset();
            source_states.clear();
            capture_sequence = 0U;
            draw_generation = 0U;
        }

        void poll() {
            if (backend == nullptr) {
                return;
            }

            auto capture = pending_captures.begin();
            while (capture != pending_captures.end()) {
                auto info = BrightDepthSnapshotInfo{};
                const auto status =
                    backend->query_snapshot(capture->snapshot_id, info);
                if (status == BrightDepthSnapshotStatus::Pending) {
                    ++capture;
                    continue;
                }

                if (status == BrightDepthSnapshotStatus::Ready &&
                    valid_snapshot_info(info, capture->snapshot_id)) {
                    resolve(*capture, info);
                }

                backend->release_snapshot(capture->snapshot_id);
                capture = pending_captures.erase(capture);
            }
        }

        void resolve(const Capture& capture,
                     const BrightDepthSnapshotInfo& info) {
            for (const auto& submitted : capture.batches) {
                const auto source = submitted.batch.source;
                const auto stateFound = source_states.find(source);
                if (source == kInvalidBrightVisibilitySourceId ||
                    stateFound == source_states.end() ||
                    capture.sequence <
                        stateFound->second.minimum_accepted_sequence ||
                    capture.sequence <=
                        stateFound->second.last_completed_sequence) {
                    continue;
                }

                auto result = BrightVisibilityResult{
                    .sample_count = static_cast< std::uint32_t >(
                        kBrightVisibilityProbeCount),
                    .real_center = submitted.batch.real_center,
                    .capture_id = capture.snapshot_id,
                };

                const auto expectedDepth = expected_z24(submitted.batch, info);
                if (submitted.disk_was_drawn && expectedDepth.has_value()) {
                    for (const auto& probe : submitted.batch.probes) {
                        if (!probe.on_screen ||
                            !finite_vec2(probe.framebuffer_position) ||
                            !finite_vec2(probe.screen_position) ||
                            probe.framebuffer_position.x < 0.0F ||
                            probe.framebuffer_position.y < 0.0F ||
                            probe.framebuffer_position.x >=
                                static_cast< float >(info.width) ||
                            probe.framebuffer_position.y >=
                                static_cast< float >(info.height) ||
                            probe.framebuffer_position.x >
                                static_cast< float >(
                                    std::numeric_limits< std::uint16_t >::max()) ||
                            probe.framebuffer_position.y >
                                static_cast< float >(
                                    std::numeric_limits< std::uint16_t >::max())) {
                            continue;
                        }

                        // Retail converts projected EFB coordinates to integer
                        // GXPeekARGB coordinates by truncating toward zero.
                        const auto x = static_cast< std::uint16_t >(
                            probe.framebuffer_position.x);
                        const auto y = static_cast< std::uint16_t >(
                            probe.framebuffer_position.y);
                        auto sceneDepth = std::uint32_t{};
                        if (backend->read_z24(capture.snapshot_id, x, y,
                                              sceneDepth) &&
                            *expectedDepth <= sceneDepth) {
                            ++result.visible_count;
                            result.visible_position_sum.add(
                                probe.screen_position);
                        }
                    }
                }

                auto& state = stateFound->second;
                state.last_completed_capture = capture.snapshot_id;
                state.last_completed_sequence = capture.sequence;
                state.last_ready_sequence = capture.sequence;
                state.last_real_center.set(result.real_center);
                state.ready_result = result;
                state.stale_zero_published = false;
            }
        }

        void age_results() {
            for (auto& [source, state] : source_states) {
                (void)source;
                if (capture_sequence >= cMaxResultAge) {
                    state.minimum_accepted_sequence = std::max(
                        state.minimum_accepted_sequence,
                        capture_sequence - cMaxResultAge + 1U);
                }
                if (state.last_completed_capture == 0U ||
                    state.stale_zero_published ||
                    capture_sequence <= state.last_ready_sequence + cMaxResultAge) {
                    continue;
                }

                state.ready_result = BrightVisibilityResult{
                    .sample_count = static_cast< std::uint32_t >(
                        kBrightVisibilityProbeCount),
                    .real_center = state.last_real_center,
                    .capture_id = state.last_completed_capture,
                };
                state.stale_zero_published = true;
            }
        }

        void expire_pending_captures() noexcept {
            if (backend == nullptr || capture_sequence < cMaxResultAge) {
                return;
            }

            const auto minimumSequence =
                capture_sequence - cMaxResultAge + 1U;
            auto capture = pending_captures.begin();
            while (capture != pending_captures.end()) {
                if (capture->sequence >= minimumSequence) {
                    ++capture;
                    continue;
                }
                if (capture->snapshot_id != 0U) {
                    backend->release_snapshot(capture->snapshot_id);
                }
                capture = pending_captures.erase(capture);
            }
        }

        std::unique_ptr< BrightDepthSnapshotBackend > owned_backend;
        BrightDepthSnapshotBackend* backend = nullptr;
        std::uint64_t capture_sequence = 0U;
        std::uint64_t draw_generation = 0U;
        std::unordered_map< BrightVisibilitySourceId, SourceState > source_states;
        std::optional< DrawPass > prepared_draw_pass;
        std::optional< Capture > active_capture;
        std::vector< Capture > pending_captures;
    };

    AuroraBrightVisibilityService::AuroraBrightVisibilityService()
        : _impl(std::make_unique< Impl >()) {
    }

    AuroraBrightVisibilityService::AuroraBrightVisibilityService(
        BrightDepthSnapshotBackend& backend)
        : _impl(std::make_unique< Impl >(backend)) {
    }

    AuroraBrightVisibilityService::~AuroraBrightVisibilityService() = default;

    void AuroraBrightVisibilityService::reset() noexcept {
        _impl->reset();
    }

    void AuroraBrightVisibilityService::begin_draw_pass(
        std::uint16_t draw_token) {
        ++_impl->draw_generation;
        _impl->prepared_draw_pass = Impl::DrawPass{
            .generation = _impl->draw_generation,
            .draw_token = draw_token,
        };
    }

    void AuroraBrightVisibilityService::begin_capture(std::uint16_t draw_token) {
        _impl->poll();
        ++_impl->capture_sequence;
        _impl->age_results();
        _impl->expire_pending_captures();

        if (_impl->active_capture.has_value()) {
            end_capture();
        }

        auto capture = Impl::Capture{
            .snapshot_id = _impl->backend != nullptr
                               ? _impl->backend->request_snapshot()
                               : 0U,
            .sequence = _impl->capture_sequence,
            .draw_token = draw_token,
        };

        // A prepared pass is single-use even when the token mismatches. This
        // makes a skipped or misrouted callback incapable of donating its disk
        // markers to a future pass after the two-buffer token wraps around.
        if (_impl->prepared_draw_pass.has_value() &&
            _impl->prepared_draw_pass->draw_token == draw_token) {
            capture.draw_generation =
                _impl->prepared_draw_pass->generation;
            capture.drawn_sources =
                std::move(_impl->prepared_draw_pass->drawn_sources);
        }
        _impl->prepared_draw_pass.reset();
        _impl->active_capture = std::move(capture);
    }

    void AuroraBrightVisibilityService::submit_batch(
        const BrightVisibilityBatch& batch) {
        if (!_impl->active_capture.has_value() ||
            batch.source == kInvalidBrightVisibilitySourceId ||
            batch.draw_token != _impl->active_capture->draw_token) {
            return;
        }

        auto& state = _impl->source_states[batch.source];
        state.last_real_center.set(batch.real_center);
        const auto drawn = _impl->active_capture->drawn_sources.find(batch.source);
        _impl->active_capture->batches.push_back({
            .batch = batch,
            .disk_was_drawn =
                drawn != _impl->active_capture->drawn_sources.end() &&
                drawn->second.generation ==
                    _impl->active_capture->draw_generation &&
                drawn->second.draw_token ==
                    _impl->active_capture->draw_token &&
                same_sphere(drawn->second.sphere, batch.sphere),
        });
    }

    void AuroraBrightVisibilityService::end_capture() {
        if (!_impl->active_capture.has_value()) {
            return;
        }

        if (_impl->active_capture->snapshot_id != 0U) {
            _impl->pending_captures.push_back(
                std::move(*_impl->active_capture));
        }
        _impl->active_capture.reset();
    }

    void AuroraBrightVisibilityService::submit_sphere(
        BrightVisibilitySourceId source, const BrightVisibilitySphere& sphere) {
        if (source == kInvalidBrightVisibilitySourceId) {
            return;
        }
        if (!_impl->prepared_draw_pass.has_value()) {
            return;
        }

        _impl->source_states.try_emplace(source);
        _impl->prepared_draw_pass->drawn_sources.insert_or_assign(
            source, Impl::DrawSubmission{
                        .sphere = sphere,
                        .generation =
                            _impl->prepared_draw_pass->generation,
                        .draw_token =
                            _impl->prepared_draw_pass->draw_token,
                    });
    }

    bool AuroraBrightVisibilityService::take_result(
        BrightVisibilitySourceId source, BrightVisibilityResult& result) {
        if (source == kInvalidBrightVisibilitySourceId) {
            return false;
        }

        // begin_capture() is the publication barrier shared by every Bright
        // object in a retail draw-sync callback. Do not let GPU readiness
        // split that object set into different generations mid-callback.
        if (!_impl->active_capture.has_value()) {
            _impl->poll();
        }
        const auto found = _impl->source_states.find(source);
        if (found == _impl->source_states.end() ||
            !found->second.ready_result.has_value()) {
            return false;
        }

        result = *found->second.ready_result;
        found->second.ready_result.reset();
        return true;
    }

    void AuroraBrightVisibilityService::forget_source(
        BrightVisibilitySourceId source) noexcept {
        if (source == kInvalidBrightVisibilitySourceId) {
            return;
        }

        if (_impl->prepared_draw_pass.has_value()) {
            _impl->prepared_draw_pass->drawn_sources.erase(source);
        }
        if (_impl->active_capture.has_value()) {
            _impl->active_capture->drawn_sources.erase(source);
        }

        _impl->source_states.erase(source);
    }

}  // namespace smgpc::render
