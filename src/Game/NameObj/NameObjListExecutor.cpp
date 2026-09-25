#include "Game/NameObj/NameObjListExecutor.hpp"
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <aurora/allocation.hpp>
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/SceneScheduler.hpp"
#include <aurora/exception.hpp>
#include <optional>
#include <stdexcept>

namespace {
    struct NativeExecutionScope {
        J3DSys::ContextScope mCommands;
        std::optional<JKRHeap::CurrentHeapScope> mHeap;
        std::optional<aurora::allocation::ClientAllocationScope> mRouting;

        explicit NativeExecutionScope(JKRHeap::Handle domain) {
            if (domain) {
                mHeap.emplace(*domain);
                mRouting.emplace(aurora::allocation::RoutingState{true, true});
            }
        }
    };
}


NameObjListExecutor::NameObjListExecutor() : mBufferHolder(), mMovementList(), mCalcAnimList(), mDrawList() {
}

NameObjListExecutor::~NameObjListExecutor() {
    unbindNativeExecution();
    delete mMovementList;
    delete mCalcAnimList;
    delete mDrawList;
    delete mBufferHolder;
}

void NameObjListExecutor::bindNativeExecution(smgpc::runtime::SceneScheduler& scheduler,
                                             JKRHeap::Handle domain) {
    if (!domain || !mMovementList || !mCalcAnimList || !mDrawList || !mBufferHolder) {
        aurora::throw_host_exception< std::invalid_argument >("Scene execution requires its initialized original executor and Game domain");
    }
    if (mNativeScheduler || mNativeRetiring || mNativeInitialized) {
        aurora::throw_host_exception< std::logic_error >("Bind the original executor once before scene placement");
    }

    mNativeHeap = std::move(domain);
    try {
        scheduler.attach_execution(*this);
        mNativeScheduler = &scheduler;
        const JKRHeap::CurrentHeapScope game(*mNativeHeap);
        const aurora::allocation::ClientAllocationScope game_routing({true, true});
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
}

void NameObjListExecutor::init() {
    initMovementList();
    initCalcAnimList();
    initCalcViewAndEntryList();
    initDrawList();
}

s32 NameObjListExecutor::registerDrawBuffer(LiveActor* pActor, int drawBufferType) {
    return mBufferHolder->registerDrawBuffer(pActor, drawBufferType);
}

void NameObjListExecutor::allocateDrawBufferActorList() {
    if (mNativeRetiring || mNativeInitialized) {
        aurora::throw_host_exception< std::logic_error >("Allocate original execution lists exactly once after placement");
    }
    const NativeExecutionScope native(mNativeHeap);
    mMovementList->allocateBuffer();
    mCalcAnimList->allocateBuffer();
    mDrawList->allocateBuffer();
    mBufferHolder->allocateActorListBuffer();
    mNativeInitialized = true;
}

void NameObjListExecutor::registerPreDrawFunction(const MR::FunctorBase& rFunc, int drawType) {
    mDrawList->registerExecuteBeforeFunction(rFunc, drawType);
}

void NameObjListExecutor::findLightInfo(LiveActor* pActor, int drawBufferType, int executorIndex) const {
    mBufferHolder->findLightInfo(pActor, drawBufferType, executorIndex);
}

void NameObjListExecutor::incrementCheckMovement(NameObj* pObj, int category) {
    mMovementList->incrementCheck(pObj, category);
}

void NameObjListExecutor::incrementCheckCalcAnim(NameObj* pObj, int category) {
    mCalcAnimList->incrementCheck(pObj, category);
}

void NameObjListExecutor::incrementCheckDraw(NameObj* pObj, int category) {
    mDrawList->incrementCheck(pObj, category);
}

void NameObjListExecutor::addToMovement(NameObj* pObj, int category) {
    mMovementList->add(pObj, category);
}

void NameObjListExecutor::addToCalcAnim(NameObj* pObj, int category) {
    mCalcAnimList->add(pObj, category);
}

void NameObjListExecutor::addToDrawBuffer(LiveActor* pActor, int drawBufferType, int executorIndex) {
    mBufferHolder->active(pActor, drawBufferType, executorIndex);
}

void NameObjListExecutor::addToDraw(NameObj* pObj, int category) {
    mDrawList->add(pObj, category);
}

void NameObjListExecutor::removeToMovement(NameObj* pObj, int category) {
    mMovementList->remove(pObj, category);
}

void NameObjListExecutor::removeToCalcAnim(NameObj* pObj, int category) {
    mCalcAnimList->remove(pObj, category);
}

void NameObjListExecutor::removeToDrawBuffer(LiveActor* pActor, int drawBufferType, int executorIndex) {
    mBufferHolder->deactive(pActor, drawBufferType, executorIndex);
}

void NameObjListExecutor::removeToDraw(NameObj* pObj, int category) {
    mDrawList->remove(pObj, category);
}

void NameObjListExecutor::executeMovement(int category) {
    const NativeExecutionScope native(mNativeHeap);
    mMovementList->execute(category);
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        if (category == MR::MovementType_Camera) {
            runtime->refresh_scene_camera_pose();
        }
        if (category == MR::MovementType_Player && runtime->player_system().attached_actor()) {
            runtime->player_system().synchronize_attached_actor();
        }
    }
}

void NameObjListExecutor::executeCalcAnim(int category) {
    const NativeExecutionScope native(mNativeHeap);
    mCalcAnimList->execute(category);
}

void NameObjListExecutor::entryDrawBuffer2D() {
    const NativeExecutionScope native(mNativeHeap);
    mBufferHolder->entry(MR::CameraType_2D);
}

void NameObjListExecutor::entryDrawBuffer3D() {
    const NativeExecutionScope native(mNativeHeap);
    mBufferHolder->entry(MR::CameraType_3D);
}

void NameObjListExecutor::entryDrawBufferMirror() {
    const NativeExecutionScope native(mNativeHeap);
    mBufferHolder->entry(MR::CameraType_Mirror);
}

void NameObjListExecutor::drawOpa(int drawBufferType) {
    const NativeExecutionScope native(mNativeHeap);
    mBufferHolder->drawOpa(drawBufferType);
}

void NameObjListExecutor::drawXlu(int drawBufferType) {
    const NativeExecutionScope native(mNativeHeap);
    mBufferHolder->drawXlu(drawBufferType);
}

void NameObjListExecutor::executeDraw(int category) {
    const NativeExecutionScope native(mNativeHeap);
    mDrawList->execute(category);
}
