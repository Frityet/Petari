#include "Game/Util/PlayerUtil.hpp"

#include "Game/compat/RuntimeContext.hpp"

namespace MR {
    void hidePlayer() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->player_system().hide_player();
        }
    }

    void setPlayerBaseMtx(MtxPtr matrix) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->player_system().set_base_matrix(matrix);
        }
    }
}  // namespace MR
