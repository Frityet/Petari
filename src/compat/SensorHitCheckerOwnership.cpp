#include "Game/LiveActor/SensorHitChecker.hpp"

// The original scene heap releases these raw children together. Native scenes
// retire their early-created checker after the actors have removed their sensors.
SensorHitChecker::~SensorHitChecker() {
    SensorGroup* groups[] = {mPlayerGroup, mRideGroup, mEyeGroup, mSimpleGroup, mMapObjGroup, mCharacterGroup};
    for (auto* group : groups) {
        delete[] group->mSensors;
        delete group;
    }
}
