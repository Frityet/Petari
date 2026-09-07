#include <aurora/exception.hpp>
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
            aurora::throw_host_exception<std::logic_error>(std::string(operation) +
                                   " requires the active game runtime.");
        }
        return *runtime;
    }

    void require_pointer_channel(s32 channel) {
        if (channel < 0 || channel >= WPAD_MAX_CONTROLLERS) {
            aurora::throw_host_exception<std::out_of_range>("The star-pointer channel is outside the retail WPAD table.");
        }
    }

}  // namespace

namespace MR {

    void activateDefaultGameLayout() {
        require_runtime("Default-game-layout activation")
            .game_layout()
            .activate_default_game_layout();
    }









}  // namespace MR
