#pragma once

class CameraTargetMtx;
class CameraTargetObj;
class LiveActor;
class MarioActor;

class CameraTargetArg {
public:
    CameraTargetArg();
    explicit CameraTargetArg(const LiveActor *pActor);
    explicit CameraTargetArg(CameraTargetMtx *pTargetMtx);
    CameraTargetArg(CameraTargetObj *pTargetObj, CameraTargetMtx *pTargetMtx, const LiveActor *pActor, MarioActor *pMario);

    void setTarget() const;

    CameraTargetObj *mTargetObj;
    CameraTargetMtx *mTargetMtx;
    const LiveActor *mLiveActor;
    MarioActor *mMarioActor;
};
