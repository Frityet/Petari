#include "Game/Util/SystemUtil.hpp"
#include "Game/System/FunctionAsyncExecutor.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include <aurora/guest_thread.hpp>

namespace {
    FunctionAsyncExecutor* getFunctionAsyncExecutor() {
        return MR::getGameSystemObjHolder()->mFunctionAsyncExecutor;
    }
}

namespace MR {
    void startFunctionAsyncExecute(const MR::FunctorBase& rFunc, int threadPriority, const char* pThreadName) {
        const aurora::os::GuestThreadExecutionScope execution;
        ::getFunctionAsyncExecutor()->start(rFunc, threadPriority, pThreadName);
    }

    bool startFunctionAsyncExecuteOnMainThread(const MR::FunctorBase& rFunc, const char* pThreadName) {
        const aurora::os::GuestThreadExecutionScope execution;
        return ::getFunctionAsyncExecutor()->startOnMainThread(rFunc, pThreadName);
    }

    void waitForEndFunctionAsyncExecute(const char* pThreadName) {
        const aurora::os::GuestThreadExecutionScope execution;
        ::getFunctionAsyncExecutor()->waitForEnd(pThreadName);
    }

    bool isEndFunctionAsyncExecute(const char* pThreadName) {
        const aurora::os::GuestThreadExecutionScope execution;
        return ::getFunctionAsyncExecutor()->isEnd(pThreadName);
    }

    bool tryEndFunctionAsyncExecute(const char* pThreadName) {
        const aurora::os::GuestThreadExecutionScope execution;
        if (isEndFunctionAsyncExecute(pThreadName)) {
            waitForEndFunctionAsyncExecute(pThreadName);
            return true;
        }

        return false;
    }

    void suspendAsyncExecuteThread(const char* pThreadName) {
        const aurora::os::GuestThreadExecutionScope execution;
        OSSuspendThread(::getFunctionAsyncExecutor()->getOSThread(pThreadName));
    }

    void resumeAsyncExecuteThread(const char* pThreadName) {
        const aurora::os::GuestThreadExecutionScope execution;
        OSThread* pThread = ::getFunctionAsyncExecutor()->getOSThread(pThreadName);
        OSResumeThread(::getFunctionAsyncExecutor()->getOSThread(pThreadName));
    }

    bool isSuspendedAsyncExecuteThread(const char* pThreadName) {
        const aurora::os::GuestThreadExecutionScope execution;
        OSThread* pThread = ::getFunctionAsyncExecutor()->getOSThread(pThreadName);
        if (pThread == nullptr) {
            return false;
        }

        return OSIsThreadSuspended(pThread);
    }
}
