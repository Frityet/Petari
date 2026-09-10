#include "Game/Scene/SceneNameObjMovementController.hpp"
#include "Game/Util/SceneUtil.hpp"

namespace MR {
    void stopSceneForScenarioOpeningCamera() {
        getSceneNameObjMovementController()->requestStopSceneFor(MovementControlType_4, nullptr);
    }

    void playSceneForScenarioOpeningCamera() {
        getSceneNameObjMovementController()->requestPlaySceneFor(MovementControlType_4, nullptr);
    }
}  // namespace MR
