#include "Game/Effect/AutoEffectGroupHolder.hpp"
#include "Game/Util/StringUtil.hpp"
#include <algorithm>

namespace MR {
    template < class T >
    struct eq_ptr_case {
        eq_ptr_case(const char* pName) : mName(pName) {}
        bool operator()(T pItem) const {
            return MR::strcasecmp(pItem->getName(), mName) == 0;
        }
        const char* mName;
    };
}

AutoEffectGroupHolder::AutoEffectGroupHolder() {
}

AutoEffectGroup* AutoEffectGroupHolder::find(const char* pName) const {
    AutoEffectGroup* const* pGroup = std::find_if(mGroups.begin(), mGroups.end(), MR::eq_ptr_case< AutoEffectGroup* >(pName));
    return pGroup != mGroups.end() ? *pGroup : nullptr;
}

bool AutoEffectGroupHolder::isExist(const char* pName) const {
    AutoEffectGroup* const* pGroup = std::find_if(mGroups.begin(), mGroups.end(), MR::eq_ptr_case< AutoEffectGroup* >(pName));
    return pGroup != mGroups.end();
}

namespace MR {
    namespace Effect {
        bool createAndAddAutoEffectGroup(AutoEffectGroupHolder* pHolder, const char* pName) {
            if (pHolder->isExist(pName)) {
                return false;
            }
            AutoEffectGroup* pGroup = createAutoEffectGroup(pName);
            if (pGroup == nullptr) {
                return false;
            }
            pHolder->mGroups.push_back(pGroup);
            return true;
        }

        void registerAutoEffectInfos(AutoEffectGroupHolder* pHolder, EffectKeeper* pKeeper, const LiveActor* pActor, const char* pName) {
            AutoEffectGroup* pGroup = pHolder->find(pName);
            if (pGroup != nullptr) {
                addAutoEffectsFromGroup(pGroup, pKeeper, pActor);
            }
        }

        void registerAutoEffectInfos(AutoEffectGroupHolder* pHolder, PaneEffectKeeper* pKeeper, const LayoutActor* pActor, const char* pName) {
            AutoEffectGroup* pGroup = pHolder->find(pName);
            if (pGroup != nullptr) {
                addAutoEffectsFromGroup(pGroup, pKeeper, pActor);
            }
        }

        void registerAutoEffectInfos(AutoEffectGroupHolder* pHolder, MultiSceneEffectKeeper* pKeeper, const MultiSceneActor* pActor, const char* pName) {
            AutoEffectGroup* pGroup = pHolder->find(pName);
            if (pGroup != nullptr) {
                addAutoEffectsFromGroup(pGroup, pKeeper, pActor);
            }
        }
    }
}
