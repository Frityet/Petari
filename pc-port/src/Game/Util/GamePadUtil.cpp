#include "Game/Util/GamePadUtil.hpp"

#include "Game/compat/RuntimeContext.hpp"

namespace MR {

bool testCorePadButtonA(s32 channel) {
    if (auto *runtime = smgpc::game::RuntimeContext::try_instance()) {
        return runtime->is_core_pad_button_a(channel);
    }

    return false;
}

bool testCorePadButtonB(s32 channel) {
    if (auto *runtime = smgpc::game::RuntimeContext::try_instance()) {
        return runtime->is_core_pad_button_b(channel);
    }

    return false;
}

}  // namespace MR

