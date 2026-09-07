#pragma once

class NameObjListExecutor;
namespace smgpc::scene {
// Borrows the actual executor owned by the scene. It owns no duplicate lists.
class SceneExecutionBinding final {
public:
    explicit SceneExecutionBinding(NameObjListExecutor&);
    ~SceneExecutionBinding();
    SceneExecutionBinding(const SceneExecutionBinding&) = delete;
    SceneExecutionBinding& operator=(const SceneExecutionBinding&) = delete;
private:
    NameObjListExecutor* _bound;
    NameObjListExecutor* _previous;
};
NameObjListExecutor& current_scene_name_obj_list_executor();
}
