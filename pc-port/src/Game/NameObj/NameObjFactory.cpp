#include "Game/NameObj/NameObjFactory.hpp"

#include "Game/Demo/PrologueDirector.hpp"
#include "Game/Effect/SimpleEffectObj.hpp"
#include "Game/Map/FileSelector.hpp"
#include "Game/Map/GroupSwitchWatcher.hpp"
#include "Game/Map/SwitchSynchronizer.hpp"
#include "Game/MapObj/CollisionBlocker.hpp"
#include "Game/MapObj/Coin.hpp"
#include "Game/MapObj/PurpleCoinStarter.hpp"
#include "Game/MapObj/RailCoin.hpp"
#include "Game/NPC/DemoRabbit.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"

#include <array>
#include <string_view>

namespace {
    template <typename T>
    NameObj* createNameObj(const char* pName) {
        return new T(pName);
    }

    constexpr auto cName2CreateFuncTable = std::array<NameObjFactory::Name2CreateFunc, 12>{
        NameObjFactory::Name2CreateFunc{
            "PrologueDirector",
            createNameObj<PrologueDirector>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "FileSelector",
            createNameObj<FileSelector>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GroupSwitchWatcher",
            createNameObj<GroupSwitchWatcher>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "SwitchSynchronizerReverse",
            createNameObj<SwitchSynchronizer>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "CollisionBlocker",
            createNameObj<CollisionBlocker>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "Steam",
            createNameObj<SimpleEffectObj>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "Coin",
            MR::createDirectSetCoin,
            "Coin",
        },
        NameObjFactory::Name2CreateFunc{
            "PurpleCoin",
            MR::createDirectSetPurpleCoin,
            "PurpleCoin",
        },
        NameObjFactory::Name2CreateFunc{
            "RailCoin",
            MR::createRailCoin,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "PurpleRailCoin",
            MR::createRailPurpleCoin,
            "PurpleCoin",
        },
        NameObjFactory::Name2CreateFunc{
            "PurpleCoinStarter",
            createNameObj<PurpleCoinStarter>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "DemoRabbit",
            createNameObj<DemoRabbit>,
            nullptr,
        },
    };

    constexpr auto cName2MakeArchiveListFuncTable = std::array<NameObjFactory::Name2MakeArchiveListFunc, 1>{
        NameObjFactory::Name2MakeArchiveListFunc{
            "DemoRabbit",
            DemoRabbit::makeArchiveList,
        },
    };

    [[nodiscard]] const NameObjFactory::Name2CreateFunc* findEntry(const char* pName) {
        const auto name = pName != nullptr ? std::string_view(pName) : std::string_view{};
        for (const auto& entry : cName2CreateFuncTable) {
            if (entry.mName == name) {
                return &entry;
            }
        }

        return nullptr;
    }
}  // namespace

namespace NameObjFactory {
    CreatorFuncPtr getCreator(const char* pName) {
        const auto* entry = findEntry(pName);
        return entry != nullptr ? entry->mCreateFunc : nullptr;
    }

    bool canCreate(const char* pName) {
        return getCreator(pName) != nullptr;
    }

    void getMountObjectArchiveList(NameObjArchiveListCollector* pArchiveList, const char* pName, const JMapInfoIter& rIter) {
        if (pArchiveList == nullptr) {
            return;
        }
        if (const auto* entry = findEntry(pName); entry != nullptr && entry->mArchiveName != nullptr) {
            pArchiveList->addArchive(entry->mArchiveName);
        }
        const auto name = pName != nullptr ? std::string_view(pName) : std::string_view{};
        for (const auto& entry : cName2MakeArchiveListFuncTable) {
            if (entry.mName == name && entry.mArchiveFunc != nullptr) {
                entry.mArchiveFunc(pArchiveList, rIter);
            }
        }
    }
}  // namespace NameObjFactory
