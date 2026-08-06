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

#include <array>
#include <string_view>

namespace {
    template <typename T>
    NameObj* createNameObj(const char* pName) {
        return new T(pName);
    }

    constexpr auto cName2CreateFuncTable = std::array<NameObjFactory::Name2CreateFunc, 11>{
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
}  // namespace NameObjFactory
