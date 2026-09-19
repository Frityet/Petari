#include "compat/Cp932Literal.hpp"
#include "Game/MapObj/PowerStarAppearPoint.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util.hpp"
#include "Game/Util/ActorCameraUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"

PowerStarAppearPoint::PowerStarAppearPoint(const char* pName) : LiveActor(pName), mCameraInfo() {
}

PowerStarAppearPoint::~PowerStarAppearPoint() {
}

void PowerStarAppearPoint::init(const JMapInfoIter& rIter) {
    MR::initDefaultPos(this, rIter);
    MR::joinToGroupArray(this, rIter, CP932("パワースター出現ポイントグループ"), 16);
    MR::initActorCamera(this, rIter, &mCameraInfo);
    makeActorAppeared();
}
