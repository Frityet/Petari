#include "Game/MapObj/StarPieceDirector.hpp"
#include "Game/Scene/SceneObjHolder.hpp"

StarPieceDirector* MR::getStarPieceDirector() {
    return MR::getSceneObj< StarPieceDirector >(SceneObj_StarPieceDirector);
}

void StarPieceDirector::initCSDelay() {
    for (int i = 0; i < 16; i++) {
        mGetSoundArray[i] = false;
    }
}

void StarPieceDirector::initCSSound() {
    mQueueNewGetSound = false;
    initCSDelay();
    mSoundIndex = 0;
}
