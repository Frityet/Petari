#include "compat/GameDataOwnership.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/System/BinaryDataContentAccessor.hpp"
#include "Game/System/GameDataGalaxyStorage.hpp"
#include "Game/System/GameEventFlagChecker.hpp"
#include "Game/System/GameEventFlagStorage.hpp"
#include "Game/System/GameEventFlagTable.hpp"
#include "Game/System/GameEventValueChecker.hpp"
#include "Game/System/ScenarioProgressTestRun.hpp"
#include "Game/System/SpinDriverPathStorage.hpp"
#include "Game/System/StarPieceAlmsStorage.hpp"
#include "Game/Util/BitArray.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/SingletonHolder.hpp"

#include <memory>

namespace smgpc::compat::game_data {
namespace {
struct EventTableDeleter {
    void operator()(GameEventFlagTableInstance* table) const {
        delete[] table->mSortTable;
        delete table;
    }
};
}

void initialize_event_table() {
    JkrHostAllocationScope host;
    static const auto owner = [] {
        SingletonHolder<GameEventFlagTableInstance>::init();
        return std::unique_ptr<GameEventFlagTableInstance, EventTableDeleter>(
            SingletonHolder<GameEventFlagTableInstance>::get());
    }();
}

void destroy_holder(GameDataHolder& holder) {
    // Plain arrays that have no original owner destructor are reclaimed by
    // the profile's solid heap. Run every nontrivial child destructor here;
    // JMapInfo unregisters its retained decoded resource during disposal.
    if (auto* galaxies = holder.mAllGalaxyStorage) {
        for (s32 index = 0; index < galaxies->getGalaxyNum(); ++index)
            delete galaxies->getGalaxyStorage(index);
        if (galaxies->mHeaderSerializer) {
            delete[] static_cast<u8*>(galaxies->mHeaderSerializer->getBuffer());
            delete galaxies->mHeaderSerializer;
        }
        delete galaxies;
        holder.mAllGalaxyStorage = nullptr;
    }
    if (auto* paths = holder.mSpinDriverPathStorage) {
        for (s32 index = 0; index < paths->mGalaxyStorage.size(); ++index) {
            auto* galaxy = paths->mGalaxyStorage[index];
            delete[] galaxy->mScenarioStorage;
            delete galaxy;
        }
        delete paths;
        holder.mSpinDriverPathStorage = nullptr;
    }
    if (holder.mEventFlagChecker) {
        delete holder.mEventFlagChecker->mFlagStorage->mFlagBitArray;
        delete holder.mEventFlagChecker->mFlagStorage;
        delete holder.mEventFlagChecker;
        holder.mEventFlagChecker = nullptr;
    }
    delete holder.mEventValueChecker;
    holder.mEventValueChecker = nullptr;
    delete holder.mStarPieceAlmsStorage;
    holder.mStarPieceAlmsStorage = nullptr;
    delete holder.mPlayerStatus;
    holder.mPlayerStatus = nullptr;
    delete holder.mMapInfo;
    holder.mMapInfo = nullptr;
    delete holder.mScenarioProgressTestRun;
    holder.mScenarioProgressTestRun = nullptr;
    if (holder.mChunkHolder) {
        delete[] holder.mChunkHolder->mChunks;
        delete[] static_cast<u8*>(holder.mChunkHolder->mData);
        delete holder.mChunkHolder;
        holder.mChunkHolder = nullptr;
    }
}
}
