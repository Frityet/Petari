#pragma once

#include "Game/Util/Array.hpp"

#if defined(TARGET_PC)
namespace smgpc::compat { class DemoDirectorOwnership; }
#endif

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

private:
#if defined(TARGET_PC)
    friend class smgpc::compat::DemoDirectorOwnership;
#endif

    MR::Vector< MR::AssignableArray< LiveActor* > > mLiveActors;      // 0x0
    MR::Vector< MR::AssignableArray< LayoutActor* > > mLayoutActors;  // 0xC
    MR::Vector< MR::AssignableArray< NameObj* > > mNameObjs;          // 0x18
};
