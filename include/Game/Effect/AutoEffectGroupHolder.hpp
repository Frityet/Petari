#pragma once

#include "Game/Effect/AutoEffectGroup.hpp"
#include "Game/Util/Array.hpp"

class AutoEffectGroupHolder {
public:
    AutoEffectGroupHolder();
    AutoEffectGroup* find(const char*) const;
    bool isExist(const char*) const;

    /* 0x000 */ MR::Vector< MR::FixedArray< AutoEffectGroup*, 256 > > mGroups;
};

namespace MR {
    namespace Effect {
        bool createAndAddAutoEffectGroup(AutoEffectGroupHolder*, const char*);
        void registerAutoEffectInfos(AutoEffectGroupHolder*, EffectKeeper*, const LiveActor*, const char*);
        void registerAutoEffectInfos(AutoEffectGroupHolder*, PaneEffectKeeper*, const LayoutActor*, const char*);
        void registerAutoEffectInfos(AutoEffectGroupHolder*, MultiSceneEffectKeeper*, const MultiSceneActor*, const char*);
    }
}
