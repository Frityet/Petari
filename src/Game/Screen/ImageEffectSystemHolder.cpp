#include "compat/Cp932Literal.hpp"
#include "Game/Screen/ImageEffectSystemHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/ImageEffectDirector.hpp"
#include "Game/Screen/ImageEffectResource.hpp"
#include "Game/Util/ObjUtil.hpp"
#include <memory>

ImageEffectSystemHolder::ImageEffectSystemHolder() : NameObj(CP932("画像効果管理")) {
    std::unique_ptr< ImageEffectResource > resource(new ImageEffectResource());
    mResource = resource.get();
    mDirector = nullptr;
    mDirector = new ImageEffectDirector(CP932("全画面エフェクト管理"));
    resource.release();
}

ImageEffectSystemHolder::~ImageEffectSystemHolder() {
    // The director is a separately registered scene NameObj.
    delete mResource;
}

void ImageEffectSystemHolder::pauseOff() {
    if (mDirector != nullptr) {
        MR::requestMovementOn(mDirector);
    }
}

namespace MR {
    void createImageEffectSystemHolder() {
        createSceneObj(SceneObj_ImageEffectSystemHolder);
    }

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

    ImageEffectResource* getImageEffectResource() {
        return getImageEffectSystemHolder()->mResource;
    }
};  // namespace MR