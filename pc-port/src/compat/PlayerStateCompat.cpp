#include "compat/PlayerUtilCompat.hpp"
#include "runtime/RuntimeServices.hpp"

#include <stdexcept>

namespace {
    smgpc::runtime::PlayerSystemService& require_attached_player() {
        auto* player = smgpc::compat::active_player_system_for_player_util();
        if (player == nullptr || player->attached_actor() == nullptr) {
            throw std::logic_error("Player state is unavailable without an attached player actor.");
        }
        return *player;
    }
}  // namespace

// Original MR player-state entry points whose host implementation belongs to
// the compatibility boundary while MarioActor is not part of the PC build.
namespace MR {

    bool isOnGroundPlayer() {
        return require_attached_player().is_on_ground();
    }

    void setPlayerSwingPermission(bool permitted) {
        require_attached_player().set_swing_permission(permitted);
    }

    void offPlayerControl() {
        require_attached_player().disable_control();
    }

    void onPlayerControl(bool resetCondition) {
        require_attached_player().enable_control(resetCondition);
    }

    bool isOffPlayerControl() {
        return !require_attached_player().is_control_enabled();
    }

}  // namespace MR
