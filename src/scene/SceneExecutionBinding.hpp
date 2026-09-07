#pragma once

#include <memory>

class NameObj;
class NameObjExecuteHolder;
class NameObjListExecutor;
namespace smgpc::compat { class JkrAllocationDomain; }
namespace smgpc::runtime { class SceneScheduler; }

namespace smgpc::scene {
class SceneNameObjRegistry;
// The Scene owns executor; its SceneObjHolder owns requirements. This scope
// borrows both while retaining the Game arena through execution and teardown.
class SceneExecutionBinding final {
public:
    SceneExecutionBinding(runtime::SceneScheduler&, NameObjListExecutor&,
                          std::shared_ptr<compat::JkrAllocationDomain>);
    ~SceneExecutionBinding();
    SceneExecutionBinding(const SceneExecutionBinding&) = delete;
    SceneExecutionBinding& operator=(const SceneExecutionBinding&) = delete;
    void complete_initialization();
    void prepare_retirement();
    NameObjListExecutor& executor() const noexcept;
    NameObjExecuteHolder& requirements() const;
    bool initialized() const noexcept;
    bool retiring() const noexcept;
    void notify_object_retired(NameObj*) noexcept;
private:
    friend class runtime::SceneScheduler;
    runtime::SceneScheduler* _scheduler;
    NameObjListExecutor* _executor;
    std::shared_ptr<compat::JkrAllocationDomain> _domain;
    bool _owns_allocation_binding = false;
    NameObjExecuteHolder* _requirements = nullptr;
    SceneExecutionBinding* _previous = nullptr;
    bool _initialized = false;
    bool _retiring = false;
    std::unique_ptr<SceneNameObjRegistry> _registry;
};
SceneExecutionBinding* current_scene_execution_binding() noexcept;
NameObjListExecutor& current_scene_name_obj_list_executor();
}
