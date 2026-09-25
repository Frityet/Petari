from edit import *
p='src/Game/NameObj/NameObjListExecutor.hpp';s=Path(p).read_text()
s=s.replace('class LiveActor;','class LiveActor;\nclass NameObjExecuteHolder;\nnamespace smgpc::compat { class JkrAllocationDomain; }\nnamespace smgpc::runtime { class SceneScheduler; }')
s=s.replace('    void init();','''    void bindNativeExecution(smgpc::runtime::SceneScheduler&, std::shared_ptr< smgpc::compat::JkrAllocationDomain >);
    void unbindNativeExecution();
    void prepareNativeRetirement();
    NameObjExecuteHolder& nativeRequirements() const;
    bool nativeInitialized() const noexcept { return mNativeInitialized; }
    bool nativeRetiring() const noexcept { return mNativeRetiring; }
    void notifyNativeObjectRetired(NameObj*) noexcept;
    std::shared_ptr< smgpc::compat::JkrAllocationDomain > nativeAllocationDomain() const noexcept { return mNativeDomain; }
    void retireNativeDrawBuffers();

    void init();''')
s=s.replace('    NameObjCategoryList* mDrawList;      // 0x10\n};','''    NameObjCategoryList* mDrawList;      // 0x10

private:
    smgpc::runtime::SceneScheduler* mNativeScheduler = nullptr;
    std::shared_ptr< smgpc::compat::JkrAllocationDomain > mNativeDomain;
    NameObjExecuteHolder* mNativeRequirements = nullptr;
    bool mNativeInitialized = false;
    bool mNativeRetiring = false;
};''')
write(p,s)
p='src/Game/NameObj/NameObjListExecutor.cpp';s=Path(p).read_text()
s=s.replace('#include "Game/NameObj/NameObjListExecutor.hpp"','#include "Game/NameObj/NameObjListExecutor.hpp"\n#include "Game/NameObj/NameObjExecuteHolder.hpp"')
s=s.replace('#include "runtime/RuntimeContext.hpp"','#include "runtime/RuntimeContext.hpp"\n#include "runtime/SceneScheduler.hpp"\n#include <aurora/exception.hpp>\n#include <stdexcept>')
s=s.replace('''        NativeExecutionScope() {
            if (auto* holder = MR::getSceneObjHolder()) {
                auto domain = holder->nativeAllocationDomain();
                if (!domain) return;
                mHeap.emplace(std::move(domain));
            }
        }''','''        explicit NativeExecutionScope(std::shared_ptr< smgpc::compat::JkrAllocationDomain > domain) {
            if (domain) {
                mHeap.emplace(std::move(domain));
            }
        }''')
s=s.replace('''NameObjListExecutor::~NameObjListExecutor() {
    delete mMovementList;
    delete mCalcAnimList;
    delete mDrawList;
    delete mBufferHolder;
}''','''NameObjListExecutor::~NameObjListExecutor() {
    unbindNativeExecution();
    delete mMovementList;
    delete mCalcAnimList;
    delete mDrawList;
    delete mBufferHolder;
}

void NameObjListExecutor::bindNativeExecution(smgpc::runtime::SceneScheduler& scheduler,
                                             std::shared_ptr< smgpc::compat::JkrAllocationDomain > domain) {
    if (!domain || !mMovementList || !mCalcAnimList || !mDrawList || !mBufferHolder) {
        aurora::throw_host_exception< std::invalid_argument >("Scene execution requires its initialized original executor and Game domain");
    }
    if (mNativeScheduler || mNativeRetiring || mNativeInitialized) {
        aurora::throw_host_exception< std::logic_error >("Bind the original executor once before scene placement");
    }

    mNativeDomain = std::move(domain);
    try {
        scheduler.attach_execution(*this);
        mNativeScheduler = &scheduler;
        const smgpc::compat::JkrAllocationScope game(mNativeDomain);
        mNativeRequirements = static_cast< NameObjExecuteHolder* >(MR::createSceneObj(SceneObj_NameObjExecuteHolder));
        if (!mNativeRequirements) {
            aurora::throw_host_exception< std::logic_error >("Scene initialization did not create its original execution requirement holder");
        }
        if (!MR::createSceneObj(SceneObj_StopSceneController) ||
            !MR::createSceneObj(SceneObj_SceneNameObjMovementController) ||
            !MR::createSceneObj(SceneObj_SensorHitChecker)) {
            aurora::throw_host_exception< std::logic_error >("Scene initialization requires its original stop, movement and sensor controllers");
        }
    } catch (...) {
        if (mNativeScheduler) {
            unbindNativeExecution();
        }
        throw;
    }
}

void NameObjListExecutor::prepareNativeRetirement() {
    if (mNativeRetiring) {
        return;
    }
    mNativeRetiring = true;
    if (mNativeScheduler) {
        mNativeScheduler->clear();
    }
    if (mMovementList) {
        mMovementList->clearNativeCallbacks();
    }
    if (mCalcAnimList) {
        mCalcAnimList->clearNativeCallbacks();
    }
    if (mDrawList) {
        mDrawList->clearNativeCallbacks();
    }
}

void NameObjListExecutor::unbindNativeExecution() {
    prepareNativeRetirement();
    if (mNativeScheduler) {
        mNativeScheduler->detach_execution(*this);
        mNativeScheduler = nullptr;
    }
    mNativeRequirements = nullptr;
    retireNativeDrawBuffers();
}

NameObjExecuteHolder& NameObjListExecutor::nativeRequirements() const {
    if (!mNativeRequirements) {
        aurora::throw_host_exception< std::logic_error >("The scene execution requirement holder is unavailable");
    }
    return *mNativeRequirements;
}

void NameObjListExecutor::notifyNativeObjectRetired(NameObj* object) noexcept {
    if (object == mNativeRequirements) {
        mNativeRequirements = nullptr;
    }
}

void NameObjListExecutor::retireNativeDrawBuffers() {
    if (!mBufferHolder) {
        return;
    }
    for (const auto& group : mBufferHolder->mBufferGroups) {
        for (const auto* executor : group.mExecutors) {
            if (executor->mNumActors != 0) {
                aurora::throw_host_exception< std::logic_error >("Disconnect original draw actors before retiring their buffers");
            }
        }
    }
    auto* holder = mBufferHolder;
    mBufferHolder = nullptr;
    delete holder;
}''')
s=s.replace('''void NameObjListExecutor::allocateDrawBufferActorList() {
    mMovementList->allocateBuffer();''','''void NameObjListExecutor::allocateDrawBufferActorList() {
    if (mNativeRetiring || mNativeInitialized) {
        aurora::throw_host_exception< std::logic_error >("Allocate original execution lists exactly once after placement");
    }
    const NativeExecutionScope native(mNativeDomain);
    mMovementList->allocateBuffer();''')
s=s.replace('    mBufferHolder->allocateActorListBuffer();\n}', '    mBufferHolder->allocateActorListBuffer();\n    mNativeInitialized = true;\n}')
s=s.replace('    const NativeExecutionScope native;', '    const NativeExecutionScope native(mNativeDomain);')
write(p,s)
