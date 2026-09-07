#include "scene/SceneExecutionBinding.hpp"
#include "scene/SceneNameObjRegistry.hpp"
#include "runtime/SceneScheduler.hpp"
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/NameObj/NameObjListExecutor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>
#include <exception>

namespace smgpc::scene {
namespace { thread_local SceneExecutionBinding* current_binding = nullptr; }

SceneExecutionBinding::SceneExecutionBinding(runtime::SceneScheduler& scheduler, NameObjListExecutor& executor,
                                           std::shared_ptr<compat::JkrAllocationDomain> domain)
    : _scheduler(&scheduler), _executor(&executor), _domain(std::move(domain)), _previous(current_binding) {
    if (!_domain || !executor.mMovementList || !executor.mCalcAnimList || !executor.mDrawList || !executor.mBufferHolder)
        aurora::throw_host_exception<std::invalid_argument>("Scene execution requires its initialized original executor and Game domain");
    if (scheduler._execution)
        aurora::throw_host_exception<std::logic_error>("The scheduler already borrows a scene executor");
    current_binding = this;
    try {
        _registry = std::make_unique<SceneNameObjRegistry>(_domain);
        _scheduler->attach_execution(*this);
        const compat::JkrAllocationScope game(_domain);
        _requirements = static_cast<NameObjExecuteHolder*>(MR::createSceneObj(SceneObj_NameObjExecuteHolder));
        if (!_requirements)
            aurora::throw_host_exception<std::logic_error>("Scene initialization did not create its original execution requirement holder");
        if (!MR::createSceneObj(SceneObj_StopSceneController) ||
            !MR::createSceneObj(SceneObj_SceneNameObjMovementController))
            aurora::throw_host_exception<std::logic_error>("Scene initialization requires its original stop and movement controllers");
    } catch (...) {
        prepare_retirement();
        _scheduler->detach_execution(*this);
        current_binding = _previous;
        throw;
    }
}

SceneExecutionBinding::~SceneExecutionBinding() {
    if (current_binding != this) std::terminate();
    prepare_retirement();
    _scheduler->detach_execution(*this);
    current_binding = _previous;
}

void SceneExecutionBinding::complete_initialization() {
    if (_retiring || _initialized)
        aurora::throw_host_exception<std::logic_error>("Allocate original execution lists exactly once after placement");
    const compat::JkrAllocationScope game(_domain);
    _scheduler->allocate_draw_buffers();
    _initialized = true;
    requirements().initConnectting();
}

void SceneExecutionBinding::prepare_retirement() {
    if (_retiring) return;
    _retiring = true;
    // Each permanent removal applies only its own original requirements, so
    // no remaining object's pending operation is accidentally run early.
    _scheduler->clear();
}

NameObjListExecutor& SceneExecutionBinding::executor() const noexcept { return *_executor; }
NameObjExecuteHolder& SceneExecutionBinding::requirements() const {
    if (!_requirements)
        aurora::throw_host_exception<std::logic_error>("The scene execution requirement holder is unavailable");
    return *_requirements;
}
bool SceneExecutionBinding::initialized() const noexcept { return _initialized; }
bool SceneExecutionBinding::retiring() const noexcept { return _retiring; }
void SceneExecutionBinding::notify_object_retired(NameObj* object) noexcept {
    if (object == _requirements) _requirements = nullptr;
}
SceneExecutionBinding* current_scene_execution_binding() noexcept { return current_binding; }
NameObjListExecutor& current_scene_name_obj_list_executor() {
    if (!current_binding)
        aurora::throw_host_exception<std::logic_error>("Original execution requires the active scene's typed executor binding");
    return current_binding->executor();
}
}
