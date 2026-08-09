#include "Game/Util/ScreenUtil.hpp"

#include "Game/Screen/InformationMessage.hpp"
#include "compat/InformationMessageCompat.hpp"
#include "compat/MessageUtilCompat.hpp"
#include "render/core/RenderTypes.hpp"
#include "runtime/RuntimeContext.hpp"

#include <cmath>
#include <stdexcept>

namespace {

    void appear_information_message(bool has_button_layout, bool is_center) {
        auto& message = smgpc::compat::require_information_message();
        message.setCenter(is_center);
        if (has_button_layout) {
            message.appearWithButtonLayout();
        } else {
            message.appear();
        }
    }

}  // namespace

namespace MR {

    u32 getViWidth() {
        if (const auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            return runtime->wii_video().render_mode().viWidth;
        }
        return smgpc::render::core::kWiiLogicalFramebufferWidth;
    }

    s32 getScreenWidth() {
        constexpr auto retail_4x3_aspect = 608.0F / 456.0F;
        constexpr auto retail_16x9_aspect = 16.0F / 9.0F;
        constexpr auto aspect_epsilon = 0.00001F;

        const auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        const auto* pose = runtime != nullptr && runtime->scene_camera_pose().has_value()
                               ? &*runtime->scene_camera_pose()
                               : runtime != nullptr && runtime->last_camera_pose().has_value()
                                     ? &*runtime->last_camera_pose()
                                     : nullptr;
        if (pose == nullptr || std::abs(pose->aspect_ratio - retail_4x3_aspect) <= aspect_epsilon) {
            return 608;
        }
        if (std::abs(pose->aspect_ratio - retail_16x9_aspect) <= aspect_epsilon) {
            return 832;
        }
        throw std::logic_error("Screen coordinates require an exact retail 4:3 or 16:9 camera aspect.");
    }

    s32 getScreenHeight() {
        if (const auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            return runtime->wii_video().render_mode().efbHeight;
        }
        return smgpc::render::core::kWiiLogicalFramebufferHeight;
    }

    // MarioActor.cpp is the retail TU that happens to provide this helper.
    // Keep the compatibility provider weak so an exact Player slice can retain
    // that source definition while non-Player scenes still receive the shared
    // render-mode implementation.
    __attribute__((weak)) s32 getFrameBufferWidth() {
        if (const auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            return runtime->wii_video().render_mode().fbWidth;
        }
        return smgpc::render::core::kWiiLogicalFramebufferWidth;
    }

    s32 getFrameBufferHeight() {
        if (const auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            return runtime->wii_video().render_mode().efbHeight;
        }
        return smgpc::render::core::kWiiLogicalFramebufferHeight;
    }

    void convertFrameBufferPosToScreenPos(TVec2f* screen_position, const TVec2f& framebuffer_position) {
        const auto ratio = framebuffer_position.x / static_cast<f32>(getFrameBufferWidth());
        screen_position->set(ratio * static_cast<f32>(getScreenWidth()), framebuffer_position.y);
    }

    void convertScreenPosToFrameBufferPos(TVec2f* framebuffer_position, const TVec2f& screen_position) {
        const auto ratio = screen_position.x / static_cast<f32>(getScreenWidth());
        framebuffer_position->set(ratio * static_cast<f32>(getFrameBufferWidth()), screen_position.y);
    }

    void appearInformationMessage(const char* message_id,
                                  bool has_button_layout) {
        smgpc::compat::require_information_message().setMessage(message_id);
        appear_information_message(has_button_layout, false);
    }

    void appearInformationMessageCenter(const char* message_id,
                                        bool has_button_layout) {
        smgpc::compat::require_information_message().setMessage(message_id);
        appear_information_message(has_button_layout, true);
    }

    void appearInformationMessage(const wchar_t* message,
                                  bool has_button_layout) {
        auto& information_message =
            smgpc::compat::require_information_message();
        if (const auto* message_id =
                smgpc::compat::layout_message_id_for_pointer(message);
            message_id != nullptr) {
            information_message.setMessage(message_id);
        } else {
            information_message.setMessage(message);
        }
        appear_information_message(has_button_layout, false);
    }

    void setInformationMessageReplaceString(const wchar_t* message,
                                            s32 index) {
        smgpc::compat::require_information_message().setReplaceString(message,
                                                                      index);
    }

    void disappearInformationMessage() {
        smgpc::compat::require_information_message().disappear();
    }

    bool isYesNoSelected() {
        throw std::logic_error(
            "Yes/No selection state is unavailable without a scene-owned selector.");
    }

    bool isYesNoSelectedYes() {
        throw std::logic_error(
            "Yes/No selection state is unavailable without a scene-owned selector.");
    }

}  // namespace MR
