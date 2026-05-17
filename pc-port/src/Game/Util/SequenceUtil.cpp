#include "Game/Util/SequenceUtil.hpp"

#include "Game/compat/RuntimeContext.hpp"

namespace MR {
    void requestChangeStageInGameAfterLoadingGameData() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->sequence_requests().request_change_stage_in_game_after_loading_game_data();
        }
    }
}  // namespace MR
