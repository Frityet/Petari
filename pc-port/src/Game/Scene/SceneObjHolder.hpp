#pragma once

#include <array>

enum SceneObjId {
    SceneObj_StageSwitchContainer = 0x0A,
    SceneObj_SwitchWatcherHolder = 0x0B,
    SceneObj_SleepControllerHolder = 0x0C,
    SceneObj_CoinHolder = 0x36,
    SceneObj_PurpleCoinHolder = 0x37,
    SceneObj_CoinRotater = 0x38,
    SceneObj_MiiFacePartsHolder = 0x6C,
    SceneObj_PrologueHolder = 0x79,
    SceneObj_NumMax = 0x7B,
};

class MiiFacePartsHolder;
class NameObj;
class SceneObjHolder {
public:
    SceneObjHolder() = default;
    ~SceneObjHolder();

    [[nodiscard]] NameObj* create(int id);
    [[nodiscard]] void* getObj(int id);

private:
    [[nodiscard]] NameObj* newEachObj(int id);

    MiiFacePartsHolder* _mii_face_parts_holder = nullptr;
    std::array< NameObj*, SceneObj_NumMax > mObjects{};
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
