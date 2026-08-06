#pragma once

enum SceneObjId {
    SceneObj_StageSwitchContainer = 0x0A,
    SceneObj_SwitchWatcherHolder = 0x0B,
    SceneObj_SleepControllerHolder = 0x0C,
    SceneObj_MiiFacePartsHolder = 0x6C,
    SceneObj_PrologueHolder = 0x79,
};

class MiiFacePartsHolder;
class NameObj;
class PrologueHolder;
class StageSwitchContainer;
class SleepControllerHolder;
class SwitchWatcherHolder;

class SceneObjHolder {
public:
    SceneObjHolder() = default;
    ~SceneObjHolder();

    [[nodiscard]] NameObj* create(int id);
    [[nodiscard]] void* getObj(int id);

private:
    MiiFacePartsHolder* _mii_face_parts_holder = nullptr;
    PrologueHolder* _prologue_holder = nullptr;
    StageSwitchContainer* _stage_switch_container = nullptr;
    SwitchWatcherHolder* _switch_watcher_holder = nullptr;
    SleepControllerHolder* _sleep_controller_holder = nullptr;
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
