#include "Game/Util/ScreenUtil.hpp"
#include "render/core/RenderTypes.hpp"
#include "runtime/RuntimeContext.hpp"

namespace MR {

    s32 getScreenHeight() {
        if (const auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            return runtime->display().render_mode().efbHeight;
        }
        return smgpc::render::core::kWiiLogicalFramebufferHeight;
    }

    // MarioActor.cpp is the retail TU that happens to provide this helper.
    // Keep the compatibility provider weak so an exact Player slice can retain
    // that source definition while non-Player scenes still receive the shared
    // render-mode implementation.
    __attribute__((weak)) s32 getFrameBufferWidth() {
        if (const auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            return runtime->display().render_mode().fbWidth;
        }
        return smgpc::render::core::kWiiLogicalFramebufferWidth;
    }

    s32 getFrameBufferHeight() {
        if (const auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            return runtime->display().render_mode().efbHeight;
        }
        return smgpc::render::core::kWiiLogicalFramebufferHeight;
    }

}  // namespace MR
