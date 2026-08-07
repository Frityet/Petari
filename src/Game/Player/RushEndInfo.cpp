#include "Game/Player/RushEndInfo.hpp"

RushEndInfo::RushEndInfo(LiveActor* pActor, u32 i1, const TVec3f& rVec, bool b1, u32 i2)
    : mMagic(0x12345678), mType(i1), mVec(rVec), mUseVec(b1), mTimer(i2), mActor(pActor), mFlags() {
}
