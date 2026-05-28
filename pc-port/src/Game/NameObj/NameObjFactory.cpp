#include "Game/NameObj/NameObjFactory.hpp"

#include "Game/Demo/PrologueDirector.hpp"
#include "Game/Map/FileSelector.hpp"

#include <array>
#include <string_view>

namespace {
    template <typename T>
    NameObj* createNameObj(const char* pName) {
        return new T(pName);
    }

    constexpr auto cName2CreateFuncTable = std::array<NameObjFactory::Name2CreateFunc, 2>{
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
