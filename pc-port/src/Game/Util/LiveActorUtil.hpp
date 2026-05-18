#pragma once

#include <memory>

#include "Game/compat/J3dMaterialRuntime.hpp"
#include <revolution.h>

class LiveActor;
class PartsModel;
class ActorLightCtrl;
class J3DFrameCtrl;
class Nerve;

class ProjmapEffectMtxSetter {
public:
    explicit ProjmapEffectMtxSetter(LiveActor* pActor);

    void updateMtxUseBaseMtx();

private:
    LiveActor* mActor = nullptr;
};

namespace MR {
    ProjmapEffectMtxSetter* initDLMakerProjmapEffectMtxSetter(LiveActor* pActor);
    void connectToSceneSky(LiveActor* pActor);
    void invalidateClipping(LiveActor* pActor);
    void startBck(LiveActor* pActor, const char* pName, const char* pFileName);
    void startBrk(LiveActor* pActor, const char* pName);
    void startBtk(LiveActor* pActor, const char* pName);
    void startAction(LiveActor* pActor, const char* pName);
    void setBrkFrame(LiveActor* pActor, f32 frame);
    void setBrkFrameAndStop(LiveActor* pActor, f32 frame);
    void setBrkFrameEndAndStop(LiveActor* pActor);
    J3DFrameCtrl* getBrkCtrl(LiveActor* pActor);
    void setBaseTRMtx(LiveActor* pActor, MtxPtr pMtx);
    void setBaseTRMtx(LiveActor* pActor, const smgpc::game::J3dMatrix3x4& matrix);
    PartsModel* createPartsModelMapObj(LiveActor* pHost, const char* pName, const char* pModelName, MtxPtr pMtx);
    void emitEffect(LiveActor* pActor, const char* pEffectName);
    void deleteEffect(LiveActor* pActor, const char* pEffectName);
    void initLightCtrl(LiveActor* pActor);
    void initLightCtrlForPlayer(LiveActor* pActor);
    void initLightCtrlNoDrawEnemy(LiveActor* pActor);
    void initLightCtrlNoDrawMapObj(LiveActor* pActor);
    void updateLightCtrl(LiveActor* pActor);
    void updateLightCtrlDirect(LiveActor* pActor);
    void loadActorLight(const LiveActor* pActor);
    ActorLightCtrl* getLightCtrl(const LiveActor* pActor);
    bool isHiddenModel(const LiveActor* pActor);
    bool isDead(const LiveActor* pActor);
    bool isStep(const LiveActor* pActor, s32 step);
    bool isFirstStep(const LiveActor* pActor);
    bool isGreaterEqualStep(const LiveActor* pActor, s32 step);
    void setNerveAtStep(LiveActor* pActor, const Nerve* pNerve, s32 step);
    bool isBtpStopped(const LiveActor* pActor);
    bool isBckStopped(const LiveActor* pActor);
    f32 getBckFrameMax(const LiveActor* pActor);
    MtxPtr getJointMtx(const LiveActor* pActor, const char* pJointName);
    bool isBrkOneTimeAndStopped(const LiveActor* pActor);
}  // namespace MR
