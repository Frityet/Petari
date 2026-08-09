#include "render/AuroraBrightVisibilityService.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    struct FakeDepthBackend final : smgpc::render::BrightDepthSnapshotBackend {
        struct Snapshot {
            smgpc::render::BrightDepthSnapshotStatus status =
                smgpc::render::BrightDepthSnapshotStatus::Pending;
            smgpc::render::BrightDepthSnapshotInfo info{};
            std::uint32_t default_z24 = 0x00ffffffU;
            std::unordered_map< std::uint32_t, std::uint32_t > pixels;
            bool released = false;
        };

        [[nodiscard]] std::uint64_t request_snapshot() noexcept override {
            const auto id = forced_id_index < forced_ids.size()
                                ? forced_ids[forced_id_index++]
                                : next_id++;
            auto& snapshot = snapshots[id];
            snapshot.info = {
                .id = id,
                .frame_id = id + 100U,
                .width = 640U,
                .height = 456U,
                .viewport_near = 0.0F,
                .viewport_far = 1.0F,
            };
            return id;
        }

        [[nodiscard]] smgpc::render::BrightDepthSnapshotStatus query_snapshot(
            std::uint64_t id,
            smgpc::render::BrightDepthSnapshotInfo& info) noexcept override {
            const auto found = snapshots.find(id);
            if (found == snapshots.end() || found->second.released) {
                return smgpc::render::BrightDepthSnapshotStatus::Unknown;
            }
            info = found->second.info;
            return found->second.status;
        }

        [[nodiscard]] bool read_z24(std::uint64_t id, std::uint16_t x,
                                    std::uint16_t y,
                                    std::uint32_t& z24) noexcept override {
            const auto found = snapshots.find(id);
            if (found == snapshots.end() || found->second.released ||
                found->second.status !=
                    smgpc::render::BrightDepthSnapshotStatus::Ready ||
                x >= found->second.info.width || y >= found->second.info.height) {
                return false;
            }

            read_coordinates.emplace_back(x, y);
            const auto pixel = found->second.pixels.find(
                (static_cast< std::uint32_t >(y) << 16U) | x);
            z24 = pixel == found->second.pixels.end()
                      ? found->second.default_z24
                      : pixel->second;
            return true;
        }

        void release_snapshot(std::uint64_t id) noexcept override {
            ++release_counts[id];
            if (const auto found = snapshots.find(id); found != snapshots.end()) {
                found->second.released = true;
            }
        }

        Snapshot& at(std::uint64_t id) {
            return snapshots.at(id);
        }

        std::uint64_t next_id = 1U;
        std::vector< std::uint64_t > forced_ids;
        std::size_t forced_id_index = 0U;
        std::unordered_map< std::uint64_t, Snapshot > snapshots;
        std::unordered_map< std::uint64_t, std::uint32_t > release_counts;
        std::vector< std::pair< std::uint16_t, std::uint16_t > >
            read_coordinates;
    };

    [[nodiscard]] smgpc::render::BrightVisibilityBatch make_batch(
        smgpc::render::BrightVisibilitySourceId source, float center_ndc_z,
        float screen_bias = 0.0F) {
        auto batch = smgpc::render::BrightVisibilityBatch{
            .source = source,
            .draw_token = 0U,
            .sphere = {
                .center = {0.0F, 0.0F, -10.0F},
                .radius = 3.0F,
            },
            .center_ndc_z = center_ndc_z,
            .real_center = {304.0F + screen_bias, 228.0F},
        };

        for (auto index = std::size_t{}; index < batch.probes.size(); ++index) {
            const auto x = 10.75F + static_cast< float >(index);
            batch.probes[index] = {
                .on_screen = true,
                .screen_position = {100.0F + screen_bias +
                                        static_cast< float >(index),
                                    200.0F},
                .framebuffer_position = {x, 20.875F},
            };
        }
        return batch;
    }

    void submit_capture(smgpc::render::AuroraBrightVisibilityService& service,
                        const smgpc::render::BrightVisibilityBatch& batch,
                        std::uint16_t token = 0U) {
        auto tagged_batch = batch;
        tagged_batch.draw_token = token;
        service.begin_draw_pass(token);
        service.submit_sphere(tagged_batch.source, tagged_batch.sphere);
        service.begin_capture(token);
        service.submit_batch(tagged_batch);
        service.end_capture();
    }

    void test_center_plane_depth_and_exact_probe_contract() {
        auto backend = FakeDepthBackend{};
        auto service = smgpc::render::AuroraBrightVisibilityService{backend};
        const auto source = smgpc::render::allocate_bright_visibility_source_id();

        // NDC -0.5 with the ordinary 0..1 GX viewport produces Z24 0.5.
        // A hypothetical sphere-front test would move this toward the camera;
        // the retail camera-facing disk keeps every probe at the center depth.
        const auto batch = make_batch(source, -0.5F);
        submit_capture(service, batch);
        auto& first = backend.at(1U);
        first.status = smgpc::render::BrightDepthSnapshotStatus::Ready;
        first.default_z24 = 0x00700000U;

        auto result = smgpc::render::BrightVisibilityResult{};
        require(service.take_result(source, result),
                "a ready tagged snapshot did not publish its batch");
        require(result.capture_id == 1U &&
                    result.sample_count ==
                        smgpc::render::kBrightVisibilityProbeCount &&
                    result.visible_count == 0U,
                "Bright visibility used a sphere-front depth or changed the exact denominator");

        submit_capture(service, batch, 1U);
        auto& second = backend.at(2U);
        second.status = smgpc::render::BrightDepthSnapshotStatus::Ready;
        second.default_z24 = 0x00800000U;
        require(service.take_result(source, result) && result.capture_id == 2U &&
                    result.visible_count ==
                        smgpc::render::kBrightVisibilityProbeCount,
                "retail LEQUAL disk visibility did not accept equal center depth");
        require(backend.read_coordinates.size() ==
                    smgpc::render::kBrightVisibilityProbeCount * 2U &&
                    backend.read_coordinates.front() ==
                        std::pair< std::uint16_t, std::uint16_t >{10U, 20U},
                "projected EFB coordinates were not truncated toward zero");

        auto expected_sum = TVec2f{};
        for (const auto& probe : batch.probes) {
            expected_sum.add(probe.screen_position);
        }
        require(std::fabs(result.visible_position_sum.x - expected_sum.x) <
                        0.001F &&
                    std::fabs(result.visible_position_sum.y - expected_sum.y) <
                        0.001F &&
                    std::fabs(result.real_center.x - batch.real_center.x) <
                        0.001F,
                "completed visibility did not retain the submitted camera-space centers");

        const auto before_near = make_batch(source, -1.01F);
        submit_capture(service, before_near, 0U);
        backend.at(3U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        backend.at(3U).default_z24 = 0x00ffffffU;
        require(service.take_result(source, result) &&
                    result.visible_count == 0U,
                "a disk clipped before the GX near plane manufactured visibility");

        const auto on_near = make_batch(source, -1.0F);
        submit_capture(service, on_near, 1U);
        backend.at(4U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        backend.at(4U).default_z24 = 0U;
        require(service.take_result(source, result) &&
                    result.visible_count ==
                        smgpc::render::kBrightVisibilityProbeCount,
                "the exact GX near-plane disk was rejected");

        const auto on_far = make_batch(source, 0.0F);
        submit_capture(service, on_far, 0U);
        backend.at(5U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        backend.at(5U).default_z24 = 0x00ffffffU;
        require(service.take_result(source, result) &&
                    result.visible_count ==
                        smgpc::render::kBrightVisibilityProbeCount,
                "the exact GX far-plane disk was rejected");
    }

    void test_capture_isolation_and_out_of_order_completion() {
        const auto source = smgpc::render::allocate_bright_visibility_source_id();

        const auto older = make_batch(source, -0.5F, 0.0F);
        const auto newer = make_batch(source, -0.5F, 50.0F);

        // Camera/batch A may complete after B has already been submitted. It
        // must still publish A's copied centers rather than consulting B's
        // camera or projection state.
        {
            auto backend = FakeDepthBackend{};
            auto service = smgpc::render::AuroraBrightVisibilityService{backend};
            submit_capture(service, older, 0U);
            submit_capture(service, newer, 1U);
            backend.at(1U).status =
                smgpc::render::BrightDepthSnapshotStatus::Ready;
            auto result = smgpc::render::BrightVisibilityResult{};
            require(service.take_result(source, result) && result.capture_id == 1U &&
                        std::fabs(result.real_center.x - older.real_center.x) <
                            0.001F,
                    "an asynchronous result mixed an older snapshot with a newer camera batch");
            backend.at(2U).status =
                smgpc::render::BrightDepthSnapshotStatus::Ready;
            require(service.take_result(source, result) && result.capture_id == 2U &&
                        std::fabs(result.real_center.x - newer.real_center.x) <
                            0.001F,
                    "the newer stored camera batch did not supersede its predecessor");
        }

        // GPU map callbacks may also finish in reverse order. Once B has
        // published, completion of A must never regress the source.
        {
            auto backend = FakeDepthBackend{};
            auto service = smgpc::render::AuroraBrightVisibilityService{backend};
            submit_capture(service, older, 0U);
            submit_capture(service, newer, 1U);
            backend.at(2U).status =
                smgpc::render::BrightDepthSnapshotStatus::Ready;
            backend.at(2U).default_z24 = 0x00ffffffU;
            auto result = smgpc::render::BrightVisibilityResult{};
            require(service.take_result(source, result) && result.capture_id == 2U &&
                        std::fabs(result.real_center.x - newer.real_center.x) <
                            0.001F,
                    "newer tagged camera batch did not publish independently");

            backend.at(1U).status =
                smgpc::render::BrightDepthSnapshotStatus::Ready;
            backend.at(1U).default_z24 = 0U;
            require(!service.take_result(source, result),
                    "an older out-of-order GPU completion regressed the source result");
        }

        // Snapshot IDs are opaque backend handles, not an ordering contract.
        // A newer capture may receive a numerically smaller handle.
        {
            auto backend = FakeDepthBackend{};
            backend.forced_ids = {900U, 3U};
            auto service = smgpc::render::AuroraBrightVisibilityService{backend};
            submit_capture(service, older, 0U);
            submit_capture(service, newer, 1U);
            backend.at(3U).status =
                smgpc::render::BrightDepthSnapshotStatus::Ready;
            auto result = smgpc::render::BrightVisibilityResult{};
            require(service.take_result(source, result) &&
                        result.capture_id == 3U &&
                        std::fabs(result.real_center.x - newer.real_center.x) <
                            0.001F,
                    "capture ordering incorrectly depended on opaque snapshot IDs");

            backend.at(900U).status =
                smgpc::render::BrightDepthSnapshotStatus::Ready;
            require(!service.take_result(source, result),
                    "an older capture with a larger opaque ID regressed the source");
        }
    }

    void test_stale_pending_capture_cannot_resurrect_visibility() {
        auto backend = FakeDepthBackend{};
        auto service = smgpc::render::AuroraBrightVisibilityService{backend};
        const auto source = smgpc::render::allocate_bright_visibility_source_id();
        const auto batch = make_batch(source, -0.5F);

        submit_capture(service, batch, 0U);
        backend.at(1U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        auto result = smgpc::render::BrightVisibilityResult{};
        require(service.take_result(source, result) &&
                    result.visible_count ==
                        smgpc::render::kBrightVisibilityProbeCount,
                "initial visible result was not established");

        // Leave sequence 2 pending, then advance beyond the bounded eight-
        // capture result age. Empty captures still represent real BrightSun
        // pass boundaries and must age both results and pending work.
        submit_capture(service, batch, 1U);
        for (auto index = 0U; index < 8U; ++index) {
            const auto token = static_cast< std::uint16_t >(index & 1U);
            service.begin_draw_pass(token);
            service.begin_capture(token);
            service.end_capture();
        }

        require(service.take_result(source, result) &&
                    result.visible_count == 0U,
                "stale Bright visibility was not retired after its bounded age");

        backend.at(2U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        require(!service.take_result(source, result),
                "a capture older than the bounded result age resurrected visibility");
    }

    void test_capture_begin_is_the_publication_barrier() {
        auto backend = FakeDepthBackend{};
        auto service = smgpc::render::AuroraBrightVisibilityService{backend};
        const auto first_source =
            smgpc::render::allocate_bright_visibility_source_id();
        const auto second_source =
            smgpc::render::allocate_bright_visibility_source_id();
        const auto first_batch = make_batch(first_source, -0.5F);
        const auto second_batch = make_batch(second_source, -0.5F, 20.0F);

        service.begin_draw_pass(0U);
        service.submit_sphere(first_source, first_batch.sphere);
        service.submit_sphere(second_source, second_batch.sphere);
        service.begin_capture(0U);
        service.submit_batch(first_batch);
        service.submit_batch(second_batch);
        service.end_capture();

        // The next draw-sync callback begins while capture 1 is pending. If
        // readiness changes between its actor callbacks, neither actor may
        // observe the new generation until the following shared boundary.
        service.begin_draw_pass(1U);
        service.begin_capture(1U);
        auto result = smgpc::render::BrightVisibilityResult{};
        require(!service.take_result(first_source, result),
                "a pending result appeared before the shared capture boundary");
        backend.at(1U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        require(!service.take_result(second_source, result),
                "GPU readiness split one Bright object set across generations");
        service.end_capture();

        service.begin_draw_pass(0U);
        service.begin_capture(0U);
        require(service.take_result(first_source, result) &&
                    result.capture_id == 1U,
                "the first source did not publish at the next capture boundary");
        require(service.take_result(second_source, result) &&
                    result.capture_id == 1U,
                "the second source did not publish at the same capture boundary");
        service.end_capture();
    }

    void test_shared_capture_lifetime_and_draw_eligibility() {
        auto backend = FakeDepthBackend{};
        auto service = smgpc::render::AuroraBrightVisibilityService{backend};
        const auto first_source =
            smgpc::render::allocate_bright_visibility_source_id();
        const auto second_source =
            smgpc::render::allocate_bright_visibility_source_id();
        const auto first_batch = make_batch(first_source, -0.5F);
        const auto second_batch = make_batch(second_source, -0.5F, 25.0F);

        service.begin_draw_pass(0U);
        service.submit_sphere(first_source, first_batch.sphere);
        service.begin_capture(0U);
        service.submit_batch(first_batch);
        service.submit_batch(second_batch);
        service.end_capture();
        require(backend.next_id == 2U,
                "multiple Bright objects did not share one pass-boundary snapshot");

        backend.at(1U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        auto result = smgpc::render::BrightVisibilityResult{};
        require(service.take_result(first_source, result) &&
                    result.visible_count ==
                        smgpc::render::kBrightVisibilityProbeCount,
                "the actually submitted bright disk was not evaluated");
        require(service.take_result(second_source, result) &&
                    result.visible_count == 0U,
                "an actor that submitted no bright disk manufactured visibility");

        // A pending result from a destroyed source must not attach to a new
        // object, even if its host allocation happens to reuse an address.
        submit_capture(service, first_batch, 1U);
        service.forget_source(first_source);
        backend.at(2U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        require(!service.take_result(first_source, result),
                "destroyed-source completion escaped its stable source identity");

        const auto replacement_source =
            smgpc::render::allocate_bright_visibility_source_id();
        const auto replacement_batch = make_batch(replacement_source, -0.5F);
        submit_capture(service, replacement_batch, 0U);
        backend.at(3U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        require(service.take_result(replacement_source, result) &&
                    result.capture_id == 3U,
                "a replacement source could not establish a fresh capture identity");
    }

    void test_draw_eligibility_is_scoped_to_one_prepared_pass() {
        auto backend = FakeDepthBackend{};
        auto service = smgpc::render::AuroraBrightVisibilityService{backend};
        const auto source = smgpc::render::allocate_bright_visibility_source_id();
        const auto original = make_batch(source, -0.5F);
        auto result = smgpc::render::BrightVisibilityResult{};

        // Pass 0 draws a disk, but its callback is skipped. Preparing pass 1
        // must retire that marker before the two-buffer token can wrap.
        service.begin_draw_pass(0U);
        service.submit_sphere(source, original.sphere);
        service.begin_draw_pass(1U);
        auto tokenOne = original;
        tokenOne.draw_token = 1U;
        service.begin_capture(1U);
        service.submit_batch(tokenOne);
        service.end_capture();
        backend.at(1U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        require(service.take_result(source, result) &&
                    result.visible_count == 0U,
                "a skipped callback leaked its disk marker into a later draw pass");

        // A callback carrying the wrong token consumes (and discards) the
        // prepared generation. Matching the batch to that callback cannot
        // make the old disk eligible.
        service.begin_draw_pass(0U);
        service.submit_sphere(source, original.sphere);
        service.begin_capture(1U);
        service.submit_batch(tokenOne);
        service.end_capture();
        backend.at(2U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        require(service.take_result(source, result) &&
                    result.visible_count == 0U,
                "a token-mismatched callback retained draw eligibility");

        // The disk geometry associated with the generated pass must match the
        // camera batch, not merely the stable source identity.
        service.begin_draw_pass(0U);
        service.submit_sphere(source, original.sphere);
        auto wrongSphere = original;
        wrongSphere.sphere.radius += 1.0F;
        service.begin_capture(0U);
        service.submit_batch(wrongSphere);
        service.end_capture();
        backend.at(3U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        require(service.take_result(source, result) &&
                    result.visible_count == 0U,
                "a sphere-mismatched camera batch inherited disk eligibility");

        // Batch tokens remain an independent gate even when the disk pass was
        // prepared correctly.
        service.begin_draw_pass(0U);
        service.submit_sphere(source, original.sphere);
        service.begin_capture(0U);
        service.submit_batch(tokenOne);
        service.end_capture();
        backend.at(4U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        require(!service.take_result(source, result),
                "a token-mismatched batch entered the active capture");
    }

    void test_terminal_snapshot_failures_retain_then_expire_visibility() {
        auto backend = FakeDepthBackend{};
        backend.forced_ids = {1U, 2U, 3U, 4U, 0U, 0U, 0U, 0U, 0U, 0U};
        auto service = smgpc::render::AuroraBrightVisibilityService{backend};
        const auto source = smgpc::render::allocate_bright_visibility_source_id();
        const auto batch = make_batch(source, -0.5F);
        auto result = smgpc::render::BrightVisibilityResult{};

        submit_capture(service, batch);
        backend.at(1U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        require(service.take_result(source, result) &&
                    result.visible_count ==
                        smgpc::render::kBrightVisibilityProbeCount,
                "the retention test did not establish an initial valid result");

        submit_capture(service, batch, 1U);
        backend.at(2U).status =
            smgpc::render::BrightDepthSnapshotStatus::Dropped;
        require(!service.take_result(source, result),
                "a dropped snapshot overwrote the last valid result");

        submit_capture(service, batch, 0U);
        backend.at(3U).status =
            smgpc::render::BrightDepthSnapshotStatus::Unknown;
        require(!service.take_result(source, result),
                "an unknown snapshot overwrote the last valid result");

        submit_capture(service, batch, 1U);
        backend.at(4U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        backend.at(4U).info.id = 404U;
        require(!service.take_result(source, result),
                "malformed ready metadata overwrote the last valid result");

        // Requests 5 through 10 return the backend's invalid zero handle.
        // It is never queried or released, and it must follow the same bounded
        // last-valid retention policy as every other failed capture.
        for (auto sequence = 5U; sequence <= 9U; ++sequence) {
            submit_capture(service, batch,
                           static_cast< std::uint16_t >(sequence & 1U));
        }
        require(!service.take_result(source, result),
                "failed snapshots retired visibility before the bounded age");

        submit_capture(service, batch, 0U);
        require(service.take_result(source, result) &&
                    result.capture_id == 1U && result.visible_count == 0U,
                "failed snapshots did not retire retained visibility at the bounded timeout");
        require(backend.release_counts[1U] == 1U &&
                    backend.release_counts[2U] == 1U &&
                    backend.release_counts[3U] == 1U &&
                    backend.release_counts[4U] == 1U &&
                    backend.release_counts[0U] == 0U,
                "terminal and zero-handle snapshots did not obey exact release ownership");
    }

    void test_pending_timeout_reset_and_release_are_idempotent() {
        {
            auto backend = FakeDepthBackend{};
            backend.forced_ids = {11U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
            auto service = smgpc::render::AuroraBrightVisibilityService{backend};
            const auto source =
                smgpc::render::allocate_bright_visibility_source_id();
            const auto batch = make_batch(source, -0.5F);
            submit_capture(service, batch);
            for (auto sequence = 2U; sequence <= 9U; ++sequence) {
                submit_capture(service, batch,
                               static_cast< std::uint16_t >(sequence & 1U));
            }
            require(backend.release_counts[11U] == 1U,
                    "an indefinitely pending snapshot outlived the bounded capture window");
            service.reset();
            service.reset();
            require(backend.release_counts[11U] == 1U,
                    "reset released an already expired snapshot twice");
        }

        {
            auto backend = FakeDepthBackend{};
            auto service = smgpc::render::AuroraBrightVisibilityService{backend};
            const auto source =
                smgpc::render::allocate_bright_visibility_source_id();
            const auto batch = make_batch(source, -0.5F);
            submit_capture(service, batch);
            service.begin_draw_pass(1U);
            service.submit_sphere(source, batch.sphere);
            service.begin_capture(1U);
            service.reset();
            service.reset();
            service.end_capture();
            service.forget_source(source);
            service.forget_source(source);
            require(backend.release_counts[1U] == 1U &&
                        backend.release_counts[2U] == 1U,
                    "reset did not release active and pending snapshots exactly once");
        }
    }

    void test_probe_bounds_and_non_finite_values_are_ignored() {
        auto backend = FakeDepthBackend{};
        auto service = smgpc::render::AuroraBrightVisibilityService{backend};
        const auto source = smgpc::render::allocate_bright_visibility_source_id();
        auto batch = make_batch(source, -0.5F);
        const auto nan = std::numeric_limits< float >::quiet_NaN();
        const auto infinity = std::numeric_limits< float >::infinity();

        batch.probes[0].on_screen = false;
        batch.probes[1].framebuffer_position.x = -1.0F;
        batch.probes[2].framebuffer_position.y = -0.25F;
        batch.probes[3].framebuffer_position.x = 640.0F;
        batch.probes[4].framebuffer_position.y = 456.0F;
        batch.probes[5].framebuffer_position.x = nan;
        batch.probes[6].screen_position.y = nan;
        batch.probes[7].framebuffer_position.x = infinity;
        batch.probes[8].screen_position.x = infinity;

        submit_capture(service, batch);
        backend.at(1U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        auto result = smgpc::render::BrightVisibilityResult{};
        require(service.take_result(source, result) &&
                    result.sample_count ==
                        smgpc::render::kBrightVisibilityProbeCount &&
                    result.visible_count == 8U &&
                    backend.read_coordinates.size() == 8U,
                "off-screen, out-of-bounds, or non-finite probes reached the depth backend");

        auto nonFiniteSphere = make_batch(source, -0.5F);
        nonFiniteSphere.sphere.center.x = nan;
        service.begin_draw_pass(1U);
        service.submit_sphere(source, make_batch(source, -0.5F).sphere);
        nonFiniteSphere.draw_token = 1U;
        service.begin_capture(1U);
        service.submit_batch(nonFiniteSphere);
        service.end_capture();
        backend.at(2U).status =
            smgpc::render::BrightDepthSnapshotStatus::Ready;
        require(service.take_result(source, result) &&
                    result.visible_count == 0U &&
                    backend.read_coordinates.size() == 8U,
                "a non-finite sphere manufactured draw eligibility or depth reads");
    }

}  // namespace

int main() {
    try {
        test_center_plane_depth_and_exact_probe_contract();
        test_capture_isolation_and_out_of_order_completion();
        test_stale_pending_capture_cannot_resurrect_visibility();
        test_capture_begin_is_the_publication_barrier();
        test_shared_capture_lifetime_and_draw_eligibility();
        test_draw_eligibility_is_scoped_to_one_prepared_pass();
        test_terminal_snapshot_failures_retain_then_expire_visibility();
        test_pending_timeout_reset_and_release_are_idempotent();
        test_probe_bounds_and_non_finite_values_are_ignored();
        std::cout << "[ok] tagged Bright visibility batch contract passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] tagged Bright visibility batch contract: "
                  << error.what() << '\n';
        return 1;
    }
}
