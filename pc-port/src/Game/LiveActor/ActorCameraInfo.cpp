#include "Game/LiveActor/ActorCameraInfo.hpp"

ActorCameraInfo::ActorCameraInfo(const JMapInfoIter &) : mCameraSetID(-1), mZoneID(0) {
}

ActorCameraInfo::ActorCameraInfo(s32 cameraSetID, s32 zoneID) : mCameraSetID(cameraSetID), mZoneID(zoneID) {
}
