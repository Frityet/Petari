#include "scene/SceneExecutionBinding.hpp"
#include "Game/NameObj/NameObjListExecutor.hpp"
#include <aurora/exception.hpp>
#include <exception>
#include <stdexcept>
namespace smgpc::scene {
namespace { NameObjListExecutor* current_executor = nullptr; }
SceneExecutionBinding::SceneExecutionBinding(NameObjListExecutor& executor)
    : _bound(&executor), _previous(current_executor) {
    if (!executor.mMovementList || !executor.mCalcAnimList || !executor.mDrawList || !executor.mBufferHolder)
        aurora::throw_host_exception<std::logic_error>("Initialize the original execution lists before publishing their scene binding");
    current_executor = _bound;
}
SceneExecutionBinding::~SceneExecutionBinding() {
    if (current_executor != _bound) std::terminate();
    current_executor = _previous;
}
NameObjListExecutor& current_scene_name_obj_list_executor() {
    if (!current_executor)
        aurora::throw_host_exception<std::logic_error>("Original scene execution requires its actual scene-owned list executor");
    return *current_executor;
}
}
