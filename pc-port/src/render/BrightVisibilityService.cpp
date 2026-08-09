#include "render/BrightVisibilityService.hpp"

#include <atomic>
#include <utility>

namespace {

    thread_local smgpc::render::BrightVisibilityService* sBrightVisibilityService = nullptr;
    std::atomic< smgpc::render::BrightVisibilitySourceId > sNextBrightVisibilitySourceId{1U};

}  // namespace

namespace smgpc::render {

    BrightVisibilitySourceId allocate_bright_visibility_source_id() noexcept {
        auto id = sNextBrightVisibilitySourceId.fetch_add(1U, std::memory_order_relaxed);
        if (id == kInvalidBrightVisibilitySourceId) {
            id = sNextBrightVisibilitySourceId.fetch_add(1U, std::memory_order_relaxed);
        }
        return id;
    }

    BrightVisibilityService* try_bright_visibility_service() noexcept {
        return sBrightVisibilityService;
    }

    void begin_bright_visibility_draw_pass(std::uint16_t draw_token) {
        if (auto* service = try_bright_visibility_service(); service != nullptr) {
            service->begin_draw_pass(draw_token);
        }
    }

    void begin_bright_visibility_capture(std::uint16_t draw_token) {
        if (auto* service = try_bright_visibility_service(); service != nullptr) {
            service->begin_capture(draw_token);
        }
    }

    void submit_bright_visibility_batch(const BrightVisibilityBatch& batch) {
        if (auto* service = try_bright_visibility_service(); service != nullptr) {
            service->submit_batch(batch);
        }
    }

    void end_bright_visibility_capture() {
        if (auto* service = try_bright_visibility_service(); service != nullptr) {
            service->end_capture();
        }
    }

    void submit_bright_visibility_sphere(BrightVisibilitySourceId source,
                                         const BrightVisibilitySphere& sphere) {
        if (auto* service = try_bright_visibility_service(); service != nullptr) {
            service->submit_sphere(source, sphere);
        }
    }

    bool take_bright_visibility_result(BrightVisibilitySourceId source,
                                       BrightVisibilityResult& result) {
        auto* service = try_bright_visibility_service();
        return service != nullptr && service->take_result(source, result);
    }

    void forget_bright_visibility_source(BrightVisibilitySourceId source) noexcept {
        if (auto* service = try_bright_visibility_service(); service != nullptr) {
            service->forget_source(source);
        }
    }

    ScopedBrightVisibilityServiceOverride::ScopedBrightVisibilityServiceOverride(BrightVisibilityService& service) noexcept
        : _previous(std::exchange(sBrightVisibilityService, &service)) {
    }

    ScopedBrightVisibilityServiceOverride::~ScopedBrightVisibilityServiceOverride() {
        sBrightVisibilityService = _previous;
    }

}  // namespace smgpc::render
