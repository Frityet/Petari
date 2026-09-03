#include "Game/Camera/CamTranslatorSpiral.hpp"
#include "Game/Camera/CameraParamChunk.hpp"

void CamTranslatorSpiral::setParam(const CameraParamChunk* pChunk) {
    CameraGeneralParam* general = pChunk->mGeneralParam;

    s32 startTime = general->getNum1Low();
    s32 endTime = general->getNum1High();

    mCamera->setParam(general->mNum2, startTime, endTime, general->mWPoint.y, general->mAxis.y, general->mWPoint.z, general->mAxis.z,
                      general->mWPoint.x, general->mAxis.x);
}

Camera* CamTranslatorSpiral::getCamera() const {
    return mCamera;
}
