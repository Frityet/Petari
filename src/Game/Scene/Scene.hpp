#pragma once

#include "Game/Scene/SceneNameObjListExecutor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/System/NerveExecutor.hpp"
#include <memory>

class NameObjHolder;

#include <JSystem/JKernel/JKRHeap.hpp>

namespace smgpc::runtime {
    class SceneScheduler;
    class SceneSchedulerBinding;
    class SceneSchedulerAllocationBinding;
}

class Scene : public NerveExecutor {
public:
    Scene(const char*);

    virtual ~Scene();
    virtual void init();
    virtual void start();
    virtual void update();
    virtual void draw() const;
    virtual void calcAnim();

    void initNameObjListExecutor();
    void initSceneObjHolder();
    void prepareNativeRetirement() noexcept;
    void beginNativeFrame();
    void initializeNativeEffects(u32 particles, u32 emitters);

    SceneNameObjListExecutor* mListExecutor;  // 0x8
    u32 _C;
    SceneObjHolder* mSceneObjHolder;  // 0x10

private:
    void initializeNativeExecution();
    void retireNativeExecution() noexcept;

    JKRHeap::Handle mNativeHeap;
    std::unique_ptr< smgpc::runtime::SceneScheduler > mNativeScheduler;
    std::unique_ptr< smgpc::runtime::SceneSchedulerBinding > mNativeSchedulerBinding;
    std::unique_ptr< smgpc::runtime::SceneSchedulerAllocationBinding > mNativeAllocationBinding;
    NameObjHolder* mNativeNameObjHolder = nullptr;
    bool mNativeRetirementPrepared = false;
};
