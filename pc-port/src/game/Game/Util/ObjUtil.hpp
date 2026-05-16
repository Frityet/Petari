#pragma once

namespace MR {

void connectToSceneLayout(void *pActor);
void connectToSceneLayoutDecoration(void *pActor);
void connectToSceneTalkLayout(void *pActor);
void connectToSceneLayoutOnPause(void *pActor);
void requestMovementOn(void *pActor);
void tryRumblePadMiddle(void *pActor, int intensity);

}  // namespace MR
