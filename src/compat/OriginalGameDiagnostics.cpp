#include "compat/OriginalGameDiagnostics.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"

#include <aurora/exception.hpp>
#include <stdexcept>
#include <string>

namespace smgpc::compat {
    NameObj* require_scene_object_for_debug(int id) {
        auto* holder = MR::getSceneObjHolder();
        auto* object = holder != nullptr ? holder->getObj(id) : nullptr;
        if (object == nullptr) {
            aurora::throw_host_exception<std::logic_error>(
                "Original game utility requires active SceneObj " + std::to_string(id));
        }
        return object;
    }

    void validate_player_owner_for_debug() {
        auto* holder = static_cast<MarioHolder*>(require_scene_object_for_debug(SceneObj_MarioHolder));
        if (holder->getMarioActor() == nullptr) {
            aurora::throw_host_exception<std::logic_error>(
                "Original player utility requires MarioHolder's actual MarioActor");
        }
    }
}
