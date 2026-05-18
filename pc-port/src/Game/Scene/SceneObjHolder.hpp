#pragma once

enum SceneObjId {
    SceneObj_MiiFacePartsHolder = 0x6C,
    SceneObj_PrologueHolder = 0x79,
};

class MiiFacePartsHolder;
class NameObj;
class PrologueHolder;

class SceneObjHolder {
public:
    [[nodiscard]] NameObj* create(int id);
    [[nodiscard]] void* getObj(int id);

private:
    MiiFacePartsHolder* _mii_face_parts_holder = nullptr;
    PrologueHolder* _prologue_holder = nullptr;
};

namespace MR {
    NameObj* createSceneObj(int id);
    SceneObjHolder* getSceneObjHolder();
    void setCurrentSceneObjHolder(SceneObjHolder* pHolder);

    template < class T >
    T* getSceneObj(int id) {
        return static_cast< T* >(getSceneObjHolder()->getObj(id));
    }
}  // namespace MR
