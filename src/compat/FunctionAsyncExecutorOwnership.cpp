#include "compat/FunctionAsyncExecutorOwnership.hpp"
#include "Game/System/FunctionAsyncExecutor.hpp"
#include "JSystem/JKernel/JKRExpHeap.hpp"
#include "JSystem/JKernel/JKRUnitHeap.hpp"
#include <aurora/guest_thread.hpp>

namespace smgpc::compat {

void destroy_function_async_executor(FunctionAsyncExecutor* executor) {
    if (executor == nullptr) return;
    const aurora::os::GuestThreadExecutionScope execution;
    for (auto*& thread : executor->mThreads) {
        delete thread;
        thread = nullptr;
    }
    for (auto* info : executor->mHolders) delete info;
    executor->mHolders.mCount = 0;
    delete executor->mMainThreadExec;
    executor->mMainThreadExec = nullptr;
    if (executor->_410 != nullptr) executor->_410->destroy();
    if (executor->_414 != nullptr) executor->_414->destroy();
    delete executor;
}

}
