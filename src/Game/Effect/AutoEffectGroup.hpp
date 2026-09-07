#pragma once

#include "Game/Util/Array.hpp"

class AutoEffectInfo;
class EffectKeeper;
class JMapInfoIter;
class LayoutActor;
class LiveActor;
class MultiSceneActor;
class MultiSceneEffectKeeper;
class PaneEffectKeeper;

class AutoEffectGroup {
public:
    AutoEffectGroup(const char*, int);
    void add(const JMapInfoIter&);
    const char* getName() const { return mName; }

    /* 0x00 */ const char* mName;
    /* 0x04 */ MR::Vector< MR::AssignableArray< AutoEffectInfo* > > mInfos;
};

namespace MR {
    namespace Effect {
        void addAutoEffectsFromGroup(const AutoEffectGroup*, EffectKeeper*, const LiveActor*);
        void addAutoEffectsFromGroup(const AutoEffectGroup*, PaneEffectKeeper*, const LayoutActor*);
        void addAutoEffectsFromGroup(const AutoEffectGroup*, MultiSceneEffectKeeper*, const MultiSceneActor*);
        AutoEffectGroup* createAutoEffectGroup(const char*);
    }
}
