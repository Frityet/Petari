#include "Game/NPC/NPCFunction.hpp"
#include "Game/NPC/NPCDirector.hpp"
#include "Game/NPC/NPCParameter.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/System/ResourceHolder.hpp"
#include <cstdio>

namespace NPCFunction {
    void createNPCData() {
    }

    void deleteNPCData() {
    }

    bool getNPCItemData(NPCActorItem* pItem, s32 itemType) {
        NPCDirector* pDirector = MR::getSceneObj< NPCDirector >(SceneObj_NPCDirector);
        ResourceHolder* pResourceHolder = pDirector->mDataResourceHolder;
        NPCItemParameterReader* pReader = pDirector->mItemParameterReader;
        char name[256];
        snprintf(name, sizeof(name), "%sItem.bcsv", pItem->mActor);

        if (!pResourceHolder->mFileInfoTable->isExistRes(name)) {
            return false;
        }

        pReader->copy(pItem);
        JMapInfo info;
        info.attach(pResourceHolder->mFileInfoTable->getRes(name));
        pReader->read(&info, itemType);
        *pItem = pReader->mItem;
        return true;
    }
};  // namespace NPCFunction
