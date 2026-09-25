#include "Game/MapObj/SpinDriverPathDrawer.hpp"
#include "Game/Scene/SceneObjHolder.hpp"

namespace MR {
    bool isDrawSpinDriverPathAtOpa() {
        if (!MR::isExistSceneObj(SceneObj_SpinDriverPathDrawInit)) {
            return false;
        }

        return MR::getSceneObj< SpinDriverPathDrawInit >(SceneObj_SpinDriverPathDrawInit)->mIsPathAtOpa;
    }
};  // namespace MR
