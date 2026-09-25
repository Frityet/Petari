#pragma once

#include "Game/NameObj/NameObjCategoryList.hpp"
#include "Game/System/DrawBufferHolder.hpp"

class LiveActor;
class NameObjExecuteHolder;
namespace smgpc::compat { class JkrAllocationDomain; }
namespace smgpc::runtime { class SceneScheduler; }

/// @brief Class that executes NameObjCategoryList instances.
class NameObjListExecutor {
public:
    NameObjListExecutor();

    virtual ~NameObjListExecutor();
    virtual void initMovementList() {
    }
    virtual void initCalcAnimList() {
    }
    virtual void initCalcViewAndEntryList() {
    }
    virtual void initDrawList() {
    }

    void bindNativeExecution(smgpc::runtime::SceneScheduler&, std::shared_ptr< smgpc::compat::JkrAllocationDomain >);
    void unbindNativeExecution();
    void prepareNativeRetirement();
    NameObjExecuteHolder& nativeRequirements() const;
    bool nativeInitialized() const noexcept { return mNativeInitialized; }
    bool nativeRetiring() const noexcept { return mNativeRetiring; }
    void notifyNativeObjectRetired(NameObj*) noexcept;
    std::shared_ptr< smgpc::compat::JkrAllocationDomain > nativeAllocationDomain() const noexcept { return mNativeDomain; }
    void retireNativeDrawBuffers();

    void init();
    s32 registerDrawBuffer(LiveActor*, int);
    void allocateDrawBufferActorList();
    void registerPreDrawFunction(const MR::FunctorBase&, int drawType);
    void findLightInfo(LiveActor*, int, int) const;
    void incrementCheckMovement(NameObj*, int);
    void incrementCheckCalcAnim(NameObj*, int);
    void incrementCheckDraw(NameObj*, int);
    void addToMovement(NameObj*, int);
    void addToCalcAnim(NameObj*, int);
    void addToDrawBuffer(LiveActor*, int, int);
    void addToDraw(NameObj*, int);
    void removeToMovement(NameObj*, int);
    void removeToCalcAnim(NameObj*, int);
    void removeToDrawBuffer(LiveActor*, int, int);
    void removeToDraw(NameObj*, int);
    void executeMovement(int);
    void executeCalcAnim(int);
    void entryDrawBuffer2D();
    void entryDrawBuffer3D();
    void entryDrawBufferMirror();
    void drawOpa(int);
    void drawXlu(int);
    void executeDraw(int);

    DrawBufferHolder* mBufferHolder;     // 0x4
    NameObjCategoryList* mMovementList;  // 0x8
    NameObjCategoryList* mCalcAnimList;  // 0xC
    NameObjCategoryList* mDrawList;      // 0x10

private:
    smgpc::runtime::SceneScheduler* mNativeScheduler = nullptr;
    std::shared_ptr< smgpc::compat::JkrAllocationDomain > mNativeDomain;
    NameObjExecuteHolder* mNativeRequirements = nullptr;
    bool mNativeInitialized = false;
    bool mNativeRetiring = false;
};
