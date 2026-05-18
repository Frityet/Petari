#include "Game/Camera/CameraTargetArg.hpp"

CameraTargetArg::CameraTargetArg() : mTargetObj(nullptr), mTargetMtx(nullptr), mLiveActor(nullptr), mMarioActor(nullptr) {
}

CameraTargetArg::CameraTargetArg(const LiveActor *pActor) : mTargetObj(nullptr), mTargetMtx(nullptr), mLiveActor(pActor), mMarioActor(nullptr) {
}

CameraTargetArg::CameraTargetArg(CameraTargetMtx *pTargetMtx)
    : mTargetObj(nullptr), mTargetMtx(pTargetMtx), mLiveActor(nullptr), mMarioActor(nullptr) {
}

CameraTargetArg::CameraTargetArg(CameraTargetObj *pTargetObj, CameraTargetMtx *pTargetMtx, const LiveActor *pActor, MarioActor *pMario)
    : mTargetObj(pTargetObj), mTargetMtx(pTargetMtx), mLiveActor(pActor), mMarioActor(pMario) {
}

void CameraTargetArg::setTarget() const {
}
