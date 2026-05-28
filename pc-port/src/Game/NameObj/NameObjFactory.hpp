#pragma once

class NameObj;

using CreatorFuncPtr = NameObj* (*)(const char*);

namespace NameObjFactory {
    struct Name2CreateFunc {
        const char* mName;
        CreatorFuncPtr mCreateFunc;
        const char* mArchiveName;
    };

    [[nodiscard]] CreatorFuncPtr getCreator(const char* pName);
    [[nodiscard]] bool canCreate(const char* pName);
}  // namespace NameObjFactory
