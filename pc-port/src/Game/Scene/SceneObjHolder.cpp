#include "Game/Scene/SceneObjHolder.hpp"

#include "Game/Demo/PrologueDirector.hpp"
#include "Game/NPC/MiiFacePartsHolder.hpp"

namespace {
    SceneObjHolder* sCurrentSceneObjHolder = nullptr;

    SceneObjHolder* fallback_scene_obj_holder() {
        static auto holder = SceneObjHolder();
        return &holder;
    }
}  // namespace

NameObj* SceneObjHolder::create(int id) {
    if (id == SceneObj_PrologueHolder) {
        if (_prologue_holder == nullptr) {
            _prologue_holder = new PrologueHolder("プロローグ保持");
        }
        return _prologue_holder;
    }

    static_cast<void>(getObj(id));
    return nullptr;
}

void* SceneObjHolder::getObj(int id) {
    if (id == SceneObj_MiiFacePartsHolder) {
        if (_mii_face_parts_holder == nullptr) {
            _mii_face_parts_holder = new MiiFacePartsHolder();
        }
        return _mii_face_parts_holder;
    }

    if (id == SceneObj_PrologueHolder) {
        return create(id);
    }

    return nullptr;
}

namespace MR {
    NameObj* createSceneObj(int id) {
        return getSceneObjHolder()->create(id);
    }

    SceneObjHolder* getSceneObjHolder() {
        return sCurrentSceneObjHolder != nullptr ? sCurrentSceneObjHolder : fallback_scene_obj_holder();
    }

    void setCurrentSceneObjHolder(SceneObjHolder* pHolder) {
        sCurrentSceneObjHolder = pHolder;
    }
}  // namespace MR
