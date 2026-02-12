#include "Game/Player/RushEndInfo.hpp"

RushEndInfo::RushEndInfo(LiveActor* pActor, u32 type, const TVec3f& rPosition, bool isUnknown, u32 flags)
    : mMagic(0x12345678), mType(type), mPosition(rPosition), mIsUnknown(isUnknown), mFlags(flags), mActor(pActor), mUnused(0) {}
