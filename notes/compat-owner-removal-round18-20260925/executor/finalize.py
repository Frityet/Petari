from edit import *
p='src/Game/NameObj/NameObjCategoryList.cpp';s=Path(p).read_text()
s=s.replace('#include <vector>\n#include <stdexcept>','#include <stdexcept>\n#include <vector>')
s=s.replace('    *mNativeLifetime = false;\n    clearNativeCallbacks();\n    delete mDelegator;','    *mNativeLifetime = false;\n    delete mDelegator;')
s=s.replace('''void NameObjCategoryList::registerExecuteBeforeFunction(const MR::FunctorBase& rFunc, int idx) {
    requireNativeCategory(idx);
    std::shared_ptr''','''void NameObjCategoryList::registerExecuteBeforeFunction(const MR::FunctorBase& rFunc, int idx) {
    requireNativeCategory(idx);
    const auto lifetime = mNativeLifetime;
    std::shared_ptr''')
s=s.replace('''    auto& category = mCategoryInfo[idx];
    auto previous = std::move(category.mNativeCallback);''','''    if (!*lifetime) {
        return;
    }
    auto& category = mCategoryInfo[idx];
    auto previous = std::move(category.mNativeCallback);''')
s=s.replace('    const auto callback = pCategoryInfo->mNativeCallback;','    // Keep both the clone and its caller heap alive if the callback replaces\n    // itself, clears registration, or destroys the actual list.\n    const auto callback = pCategoryInfo->mNativeCallback;')
write(p,s)
p='src/Game/NameObj/NameObjListExecutor.cpp';s=Path(p).read_text().replace('#include <stdexcept>\n#include <optional>','#include <optional>\n#include <stdexcept>')
write(p,s)
save()
