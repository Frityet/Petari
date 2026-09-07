#include "Game/Scene/SceneFunction.hpp"
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "runtime/SceneScheduler.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>
namespace MR {
void connectToScene(LiveActor* actor, int movement, int animation, int buffer, int draw) {
    registerNameObjToExecuteHolder(actor, movement, animation, buffer, draw);
}
void connectToScene(NameObj* object, int movement, int animation, int buffer, int draw) {
    registerNameObjToExecuteHolder(object, movement, animation, buffer, draw);
    connectToSceneTemporarily(object);
    connectToDrawTemporarily(object);
}
void disconnectToScene(NameObj* object) {
    if (auto* scheduler = smgpc::runtime::try_active_scene_scheduler()) scheduler->disconnect_name_obj(*object);
    else if (object->mExecutorIdx >= 0) aurora::throw_host_exception<std::logic_error>("Retiring a registered object needs its active scene scheduler");
}
}
