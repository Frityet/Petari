// Use external TVec3f ctor/set definitions to match original codegen.
#define TVEC3F_EXTERN_CALLS
#include "Game/Camera/CamTranslatorCharmedTripodBoss.hpp"
#include "Game/Camera/CameraParamChunk.hpp"
#include "Game/Util/MathUtil.hpp"
#undef TVEC3F_EXTERN_CALLS

void CamTranslatorCharmedTripodBoss::setParam(const CameraParamChunk* pChunk) {
    CameraGeneralParam* general = pChunk->mGeneralParam;

    const f32 upOne = 1.0f;
    const f32 upZero = 0.0f;
    const f32 nearZero = 0.001f;

    TVec3f up = general->mUp;

    if (MR::isNearZero(up, nearZero)) {
        up.set(upZero, upOne, upZero);
    } else {
        MR::normalize(&up);
    }

    TVec2f axis = TVec2f(general->mAxis.x, general->mAxis.y);

    mCamera->setParam(general->mNum1, up, general->mWPoint, axis);
}

Camera* CamTranslatorCharmedTripodBoss::getCamera() const {
    return mCamera;
}
