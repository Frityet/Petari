#include "compat/PlayerUtilCompat.hpp"
#include "runtime/RuntimeServices.hpp"

// Original MR player-state entry points whose host implementation belongs to
// the compatibility boundary while MarioActor is not part of the PC build.
namespace MR {

    bool isOnGroundPlayer() {
        const auto *player = smgpc::compat::active_player_system_for_player_util();
        return player != nullptr && player->is_on_ground();
    }

    void setPlayerSwingPermission(bool permitted) {
        if (auto *player = smgpc::compat::active_player_system_for_player_util()) {
            player->set_swing_permission(permitted);
        }
    }

    void offPlayerControl() {
        if (auto *player = smgpc::compat::active_player_system_for_player_util()) {
            player->disable_control();
        }
    }

    void onPlayerControl(bool resetCondition) {
        if (auto *player = smgpc::compat::active_player_system_for_player_util()) {
            player->enable_control(resetCondition);
        }
    }

    bool isOffPlayerControl() {
        const auto *player = smgpc::compat::active_player_system_for_player_util();
        return player != nullptr && !player->is_control_enabled();
    }

}  // namespace MR
