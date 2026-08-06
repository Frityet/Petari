#include "Game/System/GameDataFunction.hpp"

#include "Game/System/GameDataHolder.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/System/UserFile.hpp"

namespace GameDataFunction {

    void onGameEventFlag(const char* pName) {
        if (auto* file = smgpc::game::save_data_handle_sequence().getCurrentUserFile()) {
            file->mGameDataHolder->tryOnGameEventFlag(pName);
        }
    }

    bool isOnGameEventFlag(const char* pName) {
        if (const auto* file = smgpc::game::save_data_handle_sequence().getCurrentUserFile()) {
            return file->mGameDataHolder->isOnGameEventFlag(pName);
        }
        return false;
    }

    bool isPassedStoryEvent(const char* pStoryEventName) {
        return isOnGameEventFlag(pStoryEventName);
    }

    void followStoryEventByName(const char* pStoryEventName) {
        onGameEventFlag(pStoryEventName);
    }

}  // namespace GameDataFunction
