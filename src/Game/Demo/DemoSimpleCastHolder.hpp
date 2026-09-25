#pragma once

#include "Game/Util/Array.hpp"
#include <cstddef>


class LayoutActor;
class LiveActor;
class NameObj;

class DemoSimpleCastHolder {
public:
    DemoSimpleCastHolder(s32, s32, s32);

    void registerActor(LiveActor*);
    void registerActor(LayoutActor*);
    void registerNameObj(NameObj*);
    void movementOnAllCasts();
    void releaseNativeReference(const NameObj*) noexcept;
    std::size_t nativeRegistrationCount(const NameObj* = nullptr) const noexcept;

private:

    MR::Vector< MR::AssignableArray< LiveActor* > > mLiveActors;      // 0x0
    MR::Vector< MR::AssignableArray< LayoutActor* > > mLayoutActors;  // 0xC
    MR::Vector< MR::AssignableArray< NameObj* > > mNameObjs;          // 0x18
};
