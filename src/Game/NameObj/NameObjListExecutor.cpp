#include "Game/NameObj/NameObjListExecutor.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/SceneJ3dScope.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "runtime/RuntimeContext.hpp"
#include <optional>

namespace {
    struct NativeExecutionScope {
        smgpc::compat::SceneJ3dScope mCommands;
        std::optional<smgpc::compat::JkrAllocationScope> mHeap;

        NativeExecutionScope() {
            if (auto domain = smgpc::scene::current_scene_allocation_domain()) {
                mHeap.emplace(std::move(domain));
            }
        }
    };
}


NameObjListExecutor::NameObjListExecutor() : mBufferHolder(), mMovementList(), mCalcAnimList(), mDrawList() {
}

NameObjListExecutor::~NameObjListExecutor() {
    delete mMovementList;
    delete mCalcAnimList;
    delete mDrawList;
    delete mBufferHolder;
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
    mMovementList->allocateBuffer();
    mCalcAnimList->allocateBuffer();
    mDrawList->allocateBuffer();
    mBufferHolder->allocateActorListBuffer();
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
    const NativeExecutionScope native;
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
    const NativeExecutionScope native;
    mCalcAnimList->execute(category);
}

void NameObjListExecutor::entryDrawBuffer2D() {
    const NativeExecutionScope native;
    mBufferHolder->entry(MR::CameraType_2D);
}

void NameObjListExecutor::entryDrawBuffer3D() {
    const NativeExecutionScope native;
    mBufferHolder->entry(MR::CameraType_3D);
}

void NameObjListExecutor::entryDrawBufferMirror() {
    const NativeExecutionScope native;
    mBufferHolder->entry(MR::CameraType_Mirror);
}

void NameObjListExecutor::drawOpa(int drawBufferType) {
    const NativeExecutionScope native;
    mBufferHolder->drawOpa(drawBufferType);
}

void NameObjListExecutor::drawXlu(int drawBufferType) {
    const NativeExecutionScope native;
    mBufferHolder->drawXlu(drawBufferType);
}

void NameObjListExecutor::executeDraw(int category) {
    const NativeExecutionScope native;
    mDrawList->execute(category);
}
