// Original GCapture.cpp query while that actor translation unit is not compiled.
#include "Game/MapObj/GCapture.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/PlayerUtil.hpp"

namespace MR {
    bool isPlayerGCaptured() {
        if (!MR::isExistSceneObj(SceneObj_GCapture)) {
            return false;
        }
        GCapture* gCapture = static_cast< GCapture* >(MR::getSceneObjHolder()->getObj(SceneObj_GCapture));
        if (gCapture == nullptr) {
            return false;
        }

        return gCapture->_108;
    }
}
