#include "Game/Util/SystemUtil.hpp"
#include "compat/StageSessionState.hpp"

namespace MR {

    JMapIdInfo *getPlayerRestartIdInfo() {
        return &smgpc::compat::require_active_stage_session().restart_id();
    }

    void setPlayerRestartIdInfo(const JMapIdInfo &restart_id) {
        smgpc::compat::require_active_stage_session().set_restart_id(restart_id);
    }

}  // namespace MR
