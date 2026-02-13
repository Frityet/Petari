#include "Game/Util/SystemUtil.hpp"

#include "compat/RuntimeContext.hpp"

namespace MR {

bool isDisplayEncouragePal60Window() {
    return false;
}

bool isScreen16Per9() {
    return smgpc::game::compat::runtime_context().is_widescreen;
}

}  // namespace MR
