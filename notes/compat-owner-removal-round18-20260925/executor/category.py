from edit import *
p='src/Game/NameObj/NameObjCategoryList.hpp';s=Path(p).read_text().replace('#include <memory>','#include <memory>\n#include <vector>')
s=s.replace('        u32 mCheck;  // 0x10','        u32 mCheck;  // 0x10\n\n    private:\n        friend class NameObjCategoryList;\n        struct NativeCallback;\n        std::shared_ptr< NativeCallback > mNativeCallback;')
s=s.replace('    void initTable(u32, const CategoryListInitialTable*);','    void initTable(u32, const CategoryListInitialTable*);\n    void clearNativeCallbacks();\n    std::shared_ptr< bool > nativeLifetime() const noexcept { return mNativeLifetime; }')
s=s.replace('    std::shared_ptr<bool> mNativeLifetime;','    void requireNativeCategory(int) const;\n    std::shared_ptr< bool > mNativeLifetime;\n    std::shared_ptr< std::vector< unsigned > > mNativeExecuting;')
write(p,s)
p='src/Game/NameObj/NameObjCategoryList.cpp';s=Path(p).read_text()
s=s.replace('#include "compat/ActorRuntimeRegistry.hpp"','#include "compat/ActorRuntimeRegistry.hpp"\n#include "compat/JkrAllocationDomain.hpp"')
s=s.replace('#include <aurora/allocation.hpp>','#include <aurora/allocation.hpp>\n#include <aurora/exception.hpp>')
s=s.replace('#include <vector>','#include <vector>\n#include <stdexcept>')
s=s.replace('namespace {\n    std::shared_ptr<bool> createNativeLifetime()', '''struct NameObjCategoryList::CategoryInfo::NativeCallback {
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

    std::shared_ptr<bool> createNativeLifetime()''')
s=s.replace(': mNativeLifetime(createNativeLifetime()) {',': mNativeLifetime(createNativeLifetime()), mNativeExecuting(createNativeExecuting(count)) {')
s=s.replace('    mDelegator = new NameObjRealDelegator< NameObjMethod >(method);','    auto delegator = std::make_unique< NameObjRealDelegator< NameObjMethod > >(method);')
s=s.replace('    mDelegatorConst = new NameObjRealDelegator< NameObjMethodConst >(method);','    auto delegator = std::make_unique< NameObjRealDelegator< NameObjMethodConst > >(method);')
s=s.replace('    initTable(count, pTable);\n}', '    initTable(count, pTable);\n    mDelegator = delegator.release();\n}')
s=s.replace('    *mNativeLifetime = false;\n    delete mDelegator;', '    *mNativeLifetime = false;\n    clearNativeCallbacks();\n    delete mDelegator;')
s=s.replace('void NameObjCategoryList::execute(int idx) {\n    const auto lifetime = mNativeLifetime;', 'void NameObjCategoryList::execute(int idx) {\n    requireNativeCategory(idx);\n    const auto lifetime = mNativeLifetime;\n    const NativeCategoryExecution executing(mNativeExecuting, idx);')
s=s.replace('    if (pCategoryInfo->_C != nullptr) {\n        (*pCategoryInfo->_C)();','    const auto callback = pCategoryInfo->mNativeCallback;\n    if (pCategoryInfo->_C != nullptr) {\n        (*pCategoryInfo->_C)();')
s=s.replace('void NameObjCategoryList::incrementCheck(NameObj* /*unused*/, int index) {\n    mCategoryInfo[index]', 'void NameObjCategoryList::incrementCheck(NameObj* /*unused*/, int index) {\n    requireNativeCategory(index);\n    mCategoryInfo[index]')
s=s.replace('    if (_D) {\n        for (int i', '    if (_D) {\n        if (_C) {\n            aurora::throw_host_exception< std::logic_error >("Original execution category storage is allocated only once");\n        }\n        for (int i')
s=s.replace('void NameObjCategoryList::add(NameObj* pObj, int idx) {\n    mCategoryInfo[idx].mNameObjArr.push_back(pObj);\n}', '''void NameObjCategoryList::add(NameObj* pObj, int idx) {
    requireNativeCategory(idx);
    auto& objects = mCategoryInfo[idx].mNameObjArr;
    if (objects.size() >= objects.capacity()) {
        aurora::throw_host_exception< std::length_error >("Original execution category capacity exceeded");
    }
    objects.push_back(pObj);
}''')
s=s.replace('    MR::Vector< MR::AssignableArray< NameObj* > >& array = mCategoryInfo[idx].mNameObjArr;\n    array[std::find(array.begin(), array.end(), pObj) - array.begin()] = array[array.mCount - 1];\n    array.mCount--;', '''    requireNativeCategory(idx);
    MR::Vector< MR::AssignableArray< NameObj* > >& array = mCategoryInfo[idx].mNameObjArr;
    if (array.size() == 0) {
        return;
    }
    auto* found = std::find(array.begin(), array.end(), pObj);
    if (found != array.end()) {
        *found = array[--array.mCount];
        array[array.mCount] = nullptr;
    }''')
start=s.index('void NameObjCategoryList::registerExecuteBeforeFunction(');end=s.index('\nvoid NameObjCategoryList::initTable',start)
s=s[:start]+'''void NameObjCategoryList::registerExecuteBeforeFunction(const MR::FunctorBase& rFunc, int idx) {
    requireNativeCategory(idx);
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
''' + s[end:]
s=s.replace('NameObjCategoryList::CategoryInfo::CategoryInfo() : mNameObjArr() {','NameObjCategoryList::CategoryInfo::CategoryInfo() : mNameObjArr(), _C(), mCheck() {')
write(p,s)
