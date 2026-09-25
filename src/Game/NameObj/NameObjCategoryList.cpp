#include "Game/NameObj/NameObjCategoryList.hpp"
#include "Game/Util/Functor.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <algorithm>
#include <stdexcept>
#include <vector>

struct NameObjCategoryList::CategoryInfo::NativeCallback {
    std::shared_ptr< smgpc::compat::JkrAllocationDomain > mDomain;
    std::unique_ptr< MR::FunctorBase > mFunctor;
};

namespace {
    std::shared_ptr< std::vector< unsigned > > createNativeExecuting(u32 count) {
        const aurora::allocation::HostAllocationScope host;
        return std::make_shared< std::vector< unsigned > >(count, 0U);
    }

    struct NativeCategoryExecution {
        std::shared_ptr< std::vector< unsigned > > mExecuting;
        int mCategory;

        NativeCategoryExecution(std::shared_ptr< std::vector< unsigned > > executing, int category)
            : mExecuting(std::move(executing)), mCategory(category) {
            if ((*mExecuting)[category] != 0) {
                aurora::throw_host_exception< std::logic_error >("An execution category cannot rebuild its active batch recursively");
            }
            ++(*mExecuting)[category];
        }

        ~NativeCategoryExecution() {
            --(*mExecuting)[mCategory];
        }
    };

    std::shared_ptr<bool> createNativeLifetime() {
        const aurora::allocation::HostAllocationScope host;
        return std::make_shared<bool>(true);
    }
}

NameObjCategoryList::NameObjCategoryList(u32 count, const CategoryListInitialTable* pTable, NameObjMethod pMethod, bool a4,
                                         const char* /* unused */) : mNativeLifetime(createNativeLifetime()), mNativeExecuting(createNativeExecuting(count)) {
    NameObjMethod method;
    method = pMethod;
    auto delegator = std::make_unique< NameObjRealDelegator< NameObjMethod > >(method);
    _D = a4;
    _C = 0;
    initTable(count, pTable);
    mDelegator = delegator.release();
}

NameObjCategoryList::NameObjCategoryList(u32 count, const CategoryListInitialTable* pTable, NameObjMethodConst pMethod, bool a4,
                                         const char* /* unused */) : mNativeLifetime(createNativeLifetime()), mNativeExecuting(createNativeExecuting(count)) {
    NameObjMethodConst method;
    method = pMethod;
    auto delegator = std::make_unique< NameObjRealDelegator< NameObjMethodConst > >(method);
    _D = a4;
    _C = 0;
    initTable(count, pTable);
    mDelegator = delegator.release();
}

NameObjCategoryList::~NameObjCategoryList() {
    *mNativeLifetime = false;
    delete mDelegator;
}

void NameObjCategoryList::execute(int idx) {
    requireNativeCategory(idx);
    const auto lifetime = mNativeLifetime;
    const NativeCategoryExecution executing(mNativeExecuting, idx);
    CategoryInfo* pCategoryInfo = &mCategoryInfo[idx];

    if (pCategoryInfo->mNameObjArr.size() == 0) {
        return;
    }

    // Keep both the clone and its caller heap alive if the callback replaces
    // itself, clears registration, or destroys the actual list.
    const auto callback = pCategoryInfo->mNativeCallback;
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
    requireNativeCategory(index);
    mCategoryInfo[index].mCheck++;
}

void NameObjCategoryList::allocateBuffer() {
    if (_D) {
        if (_C) {
            aurora::throw_host_exception< std::logic_error >("Original execution category storage is allocated only once");
        }
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
    requireNativeCategory(idx);
    auto& objects = mCategoryInfo[idx].mNameObjArr;
    if (objects.size() >= objects.capacity()) {
        aurora::throw_host_exception< std::length_error >("Original execution category capacity exceeded");
    }
    objects.push_back(pObj);
}

void NameObjCategoryList::remove(NameObj* pObj, int idx) {
    requireNativeCategory(idx);
    MR::Vector< MR::AssignableArray< NameObj* > >& array = mCategoryInfo[idx].mNameObjArr;
    if (array.size() == 0) {
        return;
    }
    auto* found = std::find(array.begin(), array.end(), pObj);
    if (found != array.end()) {
        *found = array[--array.mCount];
        array[array.mCount] = nullptr;
    }
}

void NameObjCategoryList::registerExecuteBeforeFunction(const MR::FunctorBase& rFunc, int idx) {
    requireNativeCategory(idx);
    const auto lifetime = mNativeLifetime;
    std::shared_ptr< CategoryInfo::NativeCallback > callback;
    auto domain = smgpc::compat::current_jkr_allocation_domain();
    {
        const aurora::allocation::HostAllocationScope host;
        callback = std::make_shared< CategoryInfo::NativeCallback >();
    }
    callback->mDomain = std::move(domain);
    callback->mFunctor.reset(rFunc.clone(nullptr));
    if (!callback->mFunctor) {
        throw std::bad_alloc();
    }
    if (!callback->mDomain) {
        if (auto* heap = JKRHeap::findFromRoot(callback->mFunctor.get())) {
            callback->mDomain = smgpc::compat::JkrAllocationDomain::retain_heap(*heap);
        }
    }

    if (!*lifetime) {
        return;
    }
    auto& category = mCategoryInfo[idx];
    auto previous = std::move(category.mNativeCallback);
    category.mNativeCallback = std::move(callback);
    category._C = category.mNativeCallback->mFunctor.get();
}

void NameObjCategoryList::clearNativeCallbacks() {
    const auto lifetime = mNativeLifetime;
    for (int i = 0; i < mCategoryInfo.size(); i++) {
        auto& category = mCategoryInfo[i];
        category._C = nullptr;
        auto previous = std::move(category.mNativeCallback);
        previous.reset();
        if (!*lifetime) {
            return;
        }
    }
}

void NameObjCategoryList::requireNativeCategory(int category) const {
    if (category < 0 || category >= mCategoryInfo.size()) {
        aurora::throw_host_exception< std::out_of_range >("Execution category is outside the original scene table");
    }
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

NameObjCategoryList::CategoryInfo::CategoryInfo() : mNameObjArr(), _C(), mCheck() {
}

NameObjCategoryList::CategoryInfo::~CategoryInfo() {
}
