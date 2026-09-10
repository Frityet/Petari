#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SceneUtil.hpp"

namespace MR {

    bool isStageSuddenDeathDodoryu() {
        return isEqualStageName("CosmosGardenGalaxy") && getCurrentScenarioNo() == 4;
    }
} // namespace MR
