#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "core/RenderTypes.hpp"
#include "runtime/RuntimeContext.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    smgpc::runtime::RuntimeContext &require_runtime(std::string_view operation) {
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) {
            throw std::logic_error(std::string(operation) +
                                   " requires the active game runtime.");
        }
        return *runtime;
    }

    void require_pointer_channel(s32 channel) {
        if (channel < 0 || channel >= WPAD_MAX_CONTROLLERS) {
            throw std::out_of_range("The star-pointer channel is outside the retail WPAD table.");
        }
    }

}  // namespace

namespace MR {

    void connectToSceneMapObjDecorationMovement(NameObj *object) {
        MR::connectToScene(object, MR::MovementType_MapObjDecoration, -1, -1, -1);
    }

    void activateDefaultGameLayout() {
        require_runtime("Default-game-layout activation")
            .game_layout()
            .activate_default_game_layout();
    }

    TVec2f getStarPointerScreenPositionOrEdge(s32 channel) {
        require_pointer_channel(channel);
        const auto pointer = require_runtime("Star-pointer screen-position query")
                                 .wpad()
                                 .pointer(channel);
        return TVec2f{
            std::clamp(pointer.x, 0.0F,
                       static_cast<f32>(smgpc::render::core::kWiiLayoutWidth)),
            std::clamp(pointer.y, 0.0F,
                       static_cast<f32>(smgpc::render::core::kWiiLogicalFramebufferHeight)),
        };
    }

    void startStarPointerModeSphereSelectorFinger(void *requester) {
        require_runtime("Sphere-selector finger mode")
            .star_pointer()
            .push_mode(requester, smgpc::runtime::StarPointerMode::SphereSelectorFinger);
    }

    void startStarPointerModeSphereSelectorOnReaction(void *requester) {
        require_runtime("Sphere-selector reaction mode")
            .star_pointer()
            .push_mode(requester, smgpc::runtime::StarPointerMode::SphereSelectorReaction);
    }

    void endStarPointerMode(void *requester) {
        require_runtime("Star-pointer mode release").star_pointer().pop_mode(requester);
    }

}  // namespace MR
