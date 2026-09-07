#include "Game/Screen/ImageEffectDirector.hpp"

#include "Game/Screen/ImageEffectSystemHolder.hpp"

#include "Game/Scene/SceneObjHolder.hpp"

#include "Game/Util/ScreenUtil.hpp"

#include "Game/Util/ObjUtil.hpp"

// Original optional image-effect owner queries and its actual subjective state.

void ImageEffectDirector::turnOffDOFInSubjective() {
    _F = true;
}

namespace MR {

ImageEffectSystemHolder* getImageEffectSystemHolder() {
        return getSceneObj< ImageEffectSystemHolder >(SceneObj_ImageEffectSystemHolder);
    }

bool isExistImageEffectDirector() {
        if (isExistSceneObj(SceneObj_ImageEffectSystemHolder)) {
            return getImageEffectDirector() != nullptr;
        }

        return false;
    }

ImageEffectDirector* getImageEffectDirector() {
        return getImageEffectSystemHolder()->mDirector;
    }

void turnOffDOFInSubjective() {
        if (isExistImageEffectDirector()) {
            getImageEffectDirector()->turnOffDOFInSubjective();
        }
    }

} // namespace MR

void ImageEffectDirector::turnOnDOFInSubjective() {
    _F = false;
}

namespace MR {
    void turnOnDOFInSubjective() {
        if (isExistImageEffectDirector()) {
            getImageEffectDirector()->turnOnDOFInSubjective();
        }
    }
}
