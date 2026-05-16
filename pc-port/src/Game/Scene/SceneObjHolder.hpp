#pragma once

enum SceneObjId {
    SceneObj_MiiFacePartsHolder = 0x6C,
};

class MiiFacePartsHolder;

class SceneObjHolder {
public:
    [[nodiscard]] void* getObj(int id);

private:
    MiiFacePartsHolder* _mii_face_parts_holder = nullptr;
};

namespace MR {
    SceneObjHolder* getSceneObjHolder();

    template < class T >
    T* getSceneObj(int id) {
        return static_cast< T* >(getSceneObjHolder()->getObj(id));
    }
}  // namespace MR
