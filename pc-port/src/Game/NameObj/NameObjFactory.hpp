#pragma once

class NameObj;
class NameObjArchiveListCollector;
class JMapInfoIter;

using CreatorFuncPtr = NameObj* (*)(const char*);
using ArchiveFuncPtr = void (*)(NameObjArchiveListCollector*, const JMapInfoIter&);

namespace NameObjFactory {
    struct Name2CreateFunc {
        const char* mName;
        CreatorFuncPtr mCreateFunc;
        const char* mArchiveName;
    };

    struct Name2MakeArchiveListFunc {
        const char* mName;
        ArchiveFuncPtr mArchiveFunc;
    };

    [[nodiscard]] CreatorFuncPtr getCreator(const char* pName);
    [[nodiscard]] bool canCreate(const char* pName);
    void getMountObjectArchiveList(NameObjArchiveListCollector* pArchiveList, const char* pName, const JMapInfoIter& rIter);
}  // namespace NameObjFactory
