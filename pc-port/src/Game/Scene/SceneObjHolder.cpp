#include "Game/Scene/SceneObjHolder.hpp"

#include "Game/NPC/MiiFacePartsHolder.hpp"

void* SceneObjHolder::getObj(int id) {
    if (id == SceneObj_MiiFacePartsHolder) {
        if (_mii_face_parts_holder == nullptr) {
            _mii_face_parts_holder = new MiiFacePartsHolder();
        }
        return _mii_face_parts_holder;
    }

    return nullptr;
}

namespace MR {
    SceneObjHolder* getSceneObjHolder() {
        static auto holder = SceneObjHolder();
        return &holder;
    }
}  // namespace MR
