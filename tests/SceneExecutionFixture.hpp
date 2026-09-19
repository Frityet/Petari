#pragma once

#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Scene/SceneNameObjListExecutor.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/SceneExecutionBinding.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include <memory>

namespace smgpc::test {
// Standalone tests explicitly own the same original executor, SceneObj holder,
// requirements and Game arena as production scene initialization.
class SceneExecutionFixture final {
public:
    SceneExecutionFixture(runtime::SceneScheduler& scheduler, std::shared_ptr<compat::JkrAllocationDomain> domain,
                          scene::SceneObjFactoryOverride factory = nullptr, void* factory_context = nullptr,
                          Scene* original_scene = nullptr, NameObjHolder* original_names = nullptr)
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
            _objects = std::make_unique<scene::SceneObjHolderBinding>(*_holder, factory, factory_context, _domain);
            _execution = std::make_unique<scene::SceneExecutionBinding>(_scheduler, *_executor, _domain, original_names);
        } catch (...) { retire(); throw; }
    }
    ~SceneExecutionFixture() { retire(); }
    SceneExecutionFixture(const SceneExecutionFixture&) = delete;
    SceneExecutionFixture& operator=(const SceneExecutionFixture&) = delete;
    void complete_initialization() { _execution->complete_initialization(); }
    void apply_connections() {
        _scheduler.apply_execution_requirements(true, false);
        _scheduler.apply_execution_requirements(true, true);
        _scheduler.apply_execution_requirements(false, false);
        _scheduler.apply_execution_requirements(false, true);
    }
    void retire() {
        if (_execution) _execution->prepare_retirement();
        _objects.reset();
        _execution.reset();
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
    scene::SceneExecutionBinding& execution() { return *_execution; }
    scene::SceneObjHolderBinding& objects() { return *_objects; }
private:
    runtime::SceneScheduler& _scheduler;
    std::shared_ptr<compat::JkrAllocationDomain> _domain;
    Scene* _original_scene;
    std::unique_ptr<SceneObjHolder> _holder;
    std::unique_ptr<SceneNameObjListExecutor> _executor;
    std::unique_ptr<scene::SceneObjHolderBinding> _objects;
    std::unique_ptr<scene::SceneExecutionBinding> _execution;
};
}
