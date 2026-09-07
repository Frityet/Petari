#include "Game/Scene/SceneNameObjMovementController.hpp"

// The original scene arena reclaimed this non-NameObj child wholesale. Native
// scene teardown must also run its NerveExecutor/Spine destructors before that
// arena is released. Registered NameObj children retain their separate owners.
SceneNameObjMovementController::~SceneNameObjMovementController() {
    delete mStopSceneStateControl;
}
