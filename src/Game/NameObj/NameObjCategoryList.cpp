#include "Game/NameObj/NameObjCategoryList.hpp"
#include "Game/Util/Functor.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include <aurora/allocation.hpp>
#include <algorithm>
#include <vector>

namespace {
    std::shared_ptr<bool> createNativeLifetime() {
        const aurora::allocation::HostAllocationScope host;
        return std::make_shared<bool>(true);
    }
}

NameObjCategoryList::NameObjCategoryList(u32 count, const CategoryListInitialTable* pTable, NameObjMethod pMethod, bool a4,
                                         const char* /* unused */) : mNativeLifetime(createNativeLifetime()) {
    NameObjMethod method;
    method = pMethod;
    mDelegator = new NameObjRealDelegator< NameObjMethod >(method);
    _D = a4;
    _C = 0;
    initTable(count, pTable);
}

NameObjCategoryList::NameObjCategoryList(u32 count, const CategoryListInitialTable* pTable, NameObjMethodConst pMethod, bool a4,
                                         const char* /* unused */) : mNativeLifetime(createNativeLifetime()) {
    NameObjMethodConst method;
    method = pMethod;
    mDelegatorConst = new NameObjRealDelegator< NameObjMethodConst >(method);
    _D = a4;
    _C = 0;
    initTable(count, pTable);
}

NameObjCategoryList::~NameObjCategoryList() {
    *mNativeLifetime = false;
    delete mDelegator;
}

void NameObjCategoryList::execute(int idx) {
    const auto lifetime = mNativeLifetime;
    CategoryInfo* pCategoryInfo = &mCategoryInfo[idx];

    if (pCategoryInfo->mNameObjArr.size() == 0) {
        return;
    }

    if (pCategoryInfo->_C != nullptr) {
        (*pCategoryInfo->_C)();
        if (!*lifetime) {
            return;
        }
    }

    struct Entry {
        NameObj* mObject;
        std::uint64_t mGeneration;
        s16 mExecutorIdx;
    };
    std::vector<Entry> entries;
    {
        const aurora::allocation::HostAllocationScope host;
        const auto& objects = mCategoryInfo[idx].mNameObjArr;
        entries.reserve(objects.size());
        for (s32 i = 0; i < objects.size(); i++) {
            NameObj* object = objects[i];
            const auto generation = smgpc::compat::name_obj_runtime_generation(object);
            if (generation != 0) {
                entries.push_back({object, generation, object->mExecutorIdx});
            }
        }
    }

    // Native retirement can remove a member or destroy this list during a callback.
    // Keep the original batch order, and consult the actual list before each call.
    for (const Entry& entry : entries) {
        if (!*lifetime) {
            return;
        }
        if (smgpc::compat::name_obj_runtime_generation(entry.mObject) != entry.mGeneration) {
            continue;
        }
        if (entry.mObject->mExecutorIdx != entry.mExecutorIdx) {
            continue;
        }
        const auto& objects = mCategoryInfo[idx].mNameObjArr;
        if (objects.size() == 0 || std::find(objects.begin(), objects.end(), entry.mObject) == objects.end()) {
            continue;
        }
        (*mDelegator)(entry.mObject);
    }
}

void NameObjCategoryList::incrementCheck(NameObj* /*unused*/, int index) {
    mCategoryInfo[index].mCheck++;
}

void NameObjCategoryList::allocateBuffer() {
    if (_D) {
        for (int i = 0; i < mCategoryInfo.size(); i++) {
            NameObjCategoryList::CategoryInfo* inf = &mCategoryInfo[i];
            u32 size = inf->mCheck;
            NameObj** nameObjArr = new NameObj*[size];
            MR::Vector< MR::AssignableArray< NameObj* > >* arr = &mCategoryInfo[i].mNameObjArr;
            arr->mArray.mArr = nameObjArr;
            arr->mArray.mMaxSize = size;
        }

        _C = 1;
    }
}

void NameObjCategoryList::add(NameObj* pObj, int idx) {
    mCategoryInfo[idx].mNameObjArr.push_back(pObj);
}

void NameObjCategoryList::remove(NameObj* pObj, int idx) {
    MR::Vector< MR::AssignableArray< NameObj* > >& array = mCategoryInfo[idx].mNameObjArr;
    array[std::find(array.begin(), array.end(), pObj) - array.begin()] = array[array.mCount - 1];
    array.mCount--;
}

void NameObjCategoryList::registerExecuteBeforeFunction(const MR::FunctorBase& rFunc, int idx) {
    NameObjCategoryList::CategoryInfo* pCategoryInfo = &mCategoryInfo[idx];

    pCategoryInfo->_C = rFunc.clone(nullptr);
}

void NameObjCategoryList::initTable(u32 count, const CategoryListInitialTable* pTable) {
    mCategoryInfo.init(count);

    for (CategoryInfo* pCategoryInfo = mCategoryInfo.begin(); pCategoryInfo != mCategoryInfo.end(); pCategoryInfo++) {
        pCategoryInfo->_C = nullptr;
    }

    for (const CategoryListInitialTable* pEntry = &pTable[0]; pEntry->mIndex != -1; pEntry++) {
        if (!_D) {
            u32 size = pEntry->mCount;
            NameObj** arr = new NameObj*[size];
            NameObjCategoryList::CategoryInfo* inf = &mCategoryInfo[pEntry->mIndex];
            inf->mNameObjArr.mArray.mArr = arr;
            inf->mNameObjArr.mArray.mMaxSize = size;
            _C = 1;
        }

        mCategoryInfo[pEntry->mIndex].mCheck = 0;
    }
}

NameObjCategoryList::CategoryInfo::CategoryInfo() : mNameObjArr() {
}

NameObjCategoryList::CategoryInfo::~CategoryInfo() {
}
