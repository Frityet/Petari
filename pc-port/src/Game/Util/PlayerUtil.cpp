#include "Game/Util/PlayerUtil.hpp"

#include "Game/compat/RuntimeContext.hpp"

namespace MR {
    void hidePlayer() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_debug_event("FileSelector hid the player for title mode");
        }
    }

    void setPlayerBaseMtx(MtxPtr) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_debug_event("FileSelector reset the player base matrix");
        }
    }
}  // namespace MR
