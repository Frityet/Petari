#include "Game/Util/StarPointerUtil.hpp"

#include <revolution.h>

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/compat/RuntimeContext.hpp"

#include <array>
#include <cmath>

namespace {
    std::array< TVec2f, WPAD_MAX_CONTROLLERS > sScreenPositions{};
    std::array< TVec2f, WPAD_MAX_CONTROLLERS > sScreenVelocities{};
    constexpr auto cStarPointerModeTitle = smgpc::game::StarPointerMode::ScreenMenu;
    constexpr auto cStarPointerModeFileSelect = smgpc::game::StarPointerMode::TargetSelection;
    constexpr auto cStarPointerModeSaveLoad = smgpc::game::StarPointerMode::SystemModal;
    constexpr auto cStarPointerModePictureBook = smgpc::game::StarPointerMode::DocumentViewer;
    constexpr auto cFileSelectGuidanceRequest = smgpc::game::StarPointerGuidanceRequest::Primary;
    constexpr auto cFileSelectCopyGuidanceRequest = smgpc::game::StarPointerGuidanceRequest::Secondary;

    [[nodiscard]] bool is_valid_channel(s32 channel) {
        return channel >= 0 && channel < WPAD_MAX_CONTROLLERS;
    }

    [[nodiscard]] TVec2f pointer_screen_position(s32 channel) {
        const auto* runtime = smgpc::game::RuntimeContext::try_instance();
        if (runtime == nullptr || !is_valid_channel(channel)) {
            return {};
        }

        const auto pointer = runtime->wpad().pointer(channel);
        return TVec2f{
            .x = pointer.x,
            .y = pointer.y,
        };
    }

    [[nodiscard]] TVec2f pointer_screen_velocity(s32 channel) {
        const auto* runtime = smgpc::game::RuntimeContext::try_instance();
        if (runtime == nullptr || !is_valid_channel(channel) || runtime->wpad().pointer_history_count(channel) < 2U) {
            return {};
        }

        const auto current = runtime->wpad().past_pointer(channel, 0U);
        const auto previous = runtime->wpad().past_pointer(channel, 1U);
        if (!current.valid || !previous.valid) {
            return {};
        }

        return TVec2f{
            .x = current.x - previous.x,
            .y = current.y - previous.y,
        };
    }
}  // namespace

namespace MR {
    bool isStarPointerPointingPane(const LayoutActor* pLayout, const char* pPaneName, s32, bool, const char*) {
        if (!MR::isCorePadPointInScreen(WPAD_CHAN0)) {
            return false;
        }

        const auto* manager = pLayout != nullptr ? pLayout->getLayoutManager() : nullptr;
        auto pointer = TVec2f{};
        MR::getCorePadPointingPosBasedOnScreen(&pointer, WPAD_CHAN0);
        return manager != nullptr && manager->isPointingPane(pPaneName, pointer.x, pointer.y);
    }

    bool isStarPointerPointingPaneForMeterLayout(const LayoutActor* pLayout, const char* pPaneName, s32 param3, bool param4, const char* pParam5) {
        return isStarPointerPointingPane(pLayout, pPaneName, param3, param4, pParam5);
    }

    void initStarPointerTarget(LiveActor* pActor, f32 radius, const TVec3f& rOffset) {
        if (pActor == nullptr) {
            return;
        }

        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().register_target(*pActor, radius, smgpc::game::CameraParamVec3{.x = rOffset.x, .y = rOffset.y, .z = rOffset.z});
        }
    }

    bool isStarPointerPointing1PWithoutCheckZ(const LiveActor* pActor, const char*, bool, bool) {
        auto* runtime = smgpc::game::RuntimeContext::try_instance();
        return runtime != nullptr && pActor != nullptr && runtime->sample_star_pointer_target(*pActor, false);
    }

    bool isStarPointerPointingFileSelect(const LiveActor* pActor) {
        auto* runtime = smgpc::game::RuntimeContext::try_instance();
        return runtime != nullptr && pActor != nullptr && runtime->sample_star_pointer_target(*pActor, true);
    }

    bool isExistStarPointerTarget(const LiveActor* pActor) {
        const auto* runtime = smgpc::game::RuntimeContext::try_instance();
        return runtime != nullptr && pActor != nullptr && runtime->star_pointer().has_target(*pActor);
    }

    void setStarPointerTargetRadius3d(LiveActor* pActor, f32 radius) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance(); runtime != nullptr && pActor != nullptr) {
            runtime->star_pointer().set_target_radius(*pActor, radius);
        }
    }

    TVec2f* getStarPointerScreenPosition(s32 channel) {
        if (!is_valid_channel(channel)) {
            return nullptr;
        }

        sScreenPositions[static_cast< std::size_t >(channel)] = pointer_screen_position(channel);
        return &sScreenPositions[static_cast< std::size_t >(channel)];
    }

    TVec2f* getStarPointerScreenVelocity(s32 channel) {
        if (!is_valid_channel(channel)) {
            return nullptr;
        }

        sScreenVelocities[static_cast< std::size_t >(channel)] = pointer_screen_velocity(channel);
        return &sScreenVelocities[static_cast< std::size_t >(channel)];
    }

    f32 getStarPointerScreenSpeed(s32 channel) {
        const auto velocity = pointer_screen_velocity(channel);
        return std::sqrt((velocity.x * velocity.x) + (velocity.y * velocity.y));
    }

    bool isStarPointerInScreen(s32 channel) {
        const auto* runtime = smgpc::game::RuntimeContext::try_instance();
        if (runtime == nullptr || !is_valid_channel(channel)) {
            return false;
        }

        return runtime->wpad().pointer(channel).valid;
    }

    void startStarPointerModeTitle(void*) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().start_mode(cStarPointerModeTitle);
        }
    }

    void startStarPointerModeFileSelect(void*) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().start_mode(cStarPointerModeFileSelect);
        }
    }

    void requestStarPointerModeSaveLoad(void*) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().start_mode(cStarPointerModeSaveLoad);
        }
    }

    void requestStarPointerModePictureBook(void*) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().start_mode(cStarPointerModePictureBook);
        }
    }

    void activeStarPointerGuidance() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().set_guidance_active(true);
        }
    }

    void deactiveStarPointerGuidance() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().set_guidance_active(false);
        }
    }

    bool requestFileSelectGuidance() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().request_guidance(cFileSelectGuidanceRequest);
        }
        return true;
    }

    bool requestFileSelectCopyGuidance() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().request_guidance(cFileSelectCopyGuidanceRequest);
        }
        return true;
    }
}  // namespace MR
