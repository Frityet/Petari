#pragma once

#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Scene/SceneNameObjListExecutor.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "runtime/SceneScheduler.hpp"
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "scene/SceneInitializationState.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include <aurora/allocation.hpp>
#include <algorithm>
#include <memory>
#include <vector>

namespace smgpc::test {
// Tests provide the actual GameSystem controller scene and use its original
// holder, executor and Game arena. No fixture factory or alternate scene is published.
class SceneExecutionFixture final {
public:
    SceneExecutionFixture(runtime::SceneScheduler& scheduler, std::shared_ptr<compat::JkrAllocationDomain> domain,
                          Scene* original_scene = nullptr)
        : _scheduler(scheduler), _domain(std::move(domain)), _original_scene(original_scene) {
        try {
            {
                const compat::JkrAllocationScope game(_domain);
                _holder = std::make_unique<SceneObjHolder>();
                _executor = std::make_unique<SceneNameObjListExecutor>();
                _executor->init();
            }
            if (_original_scene) {
                _original_scene->mSceneObjHolder = _holder.get();
                _original_scene->mListExecutor = _executor.get();
            }
            _allocation = std::make_unique<runtime::SceneSchedulerAllocationBinding>(_scheduler, _domain);
            _holder->initializeNative(_domain);
            _executor->bindNativeExecution(_scheduler, _domain);
        } catch (...) { retire(); throw; }
    }
    ~SceneExecutionFixture() { retire(); }
    SceneExecutionFixture(const SceneExecutionFixture&) = delete;
    SceneExecutionFixture& operator=(const SceneExecutionFixture&) = delete;
    void complete_initialization() {
        _executor->allocateDrawBufferActorList();
        _executor->nativeRequirements().initConnectting();
        _initialization.complete();
    }
    void init_after_placement() {
        const scene::SceneInitializationScope phase(SceneInitializeState_AfterPlacement);
        for (auto* object : compat::snapshot_name_obj_runtime_objects()) {
            if (!_holder->ownsNativeObject(object) || std::ranges::find(_completed, object) != _completed.end()) continue;
            {
                const aurora::allocation::ClientAllocationScope game({true, true});
                object->initAfterPlacement();
            }
            _completed.push_back(object);
        }
    }
    void apply_connections() {
        _scheduler.apply_execution_requirements(true, false);
        _scheduler.apply_execution_requirements(true, true);
        _scheduler.apply_execution_requirements(false, false);
        _scheduler.apply_execution_requirements(false, true);
    }
    void retire() {
        if (_executor) _executor->prepareNativeRetirement();
        if (_holder) _holder->retireNativeResources();
        if (_executor) _executor->unbindNativeExecution();
        _allocation.reset();
        if (_original_scene) {
            _original_scene->mSceneObjHolder = nullptr;
            _original_scene->mListExecutor = nullptr;
        }
        _executor.reset();
        _holder.reset();
        _domain.reset();
    }
    SceneNameObjListExecutor& executor() { return *_executor; }
    SceneObjHolder& holder() { return *_holder; }
private:
    runtime::SceneScheduler& _scheduler;
    std::shared_ptr<compat::JkrAllocationDomain> _domain;
    Scene* _original_scene;
    std::unique_ptr<SceneObjHolder> _holder;
    std::unique_ptr<SceneNameObjListExecutor> _executor;
    scene::SceneInitializationBinding _initialization;
    std::unique_ptr<runtime::SceneSchedulerAllocationBinding> _allocation;
    std::vector<NameObj*> _completed;
};
}
