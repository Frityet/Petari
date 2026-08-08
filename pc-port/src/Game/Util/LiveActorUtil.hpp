#pragma once

#include <memory>

#include "render/J3dMaterialRuntime.hpp"
#include <JSystem/JGeometry/TVec.hpp>
#include <revolution.h>

class LiveActor;
class LodCtrl;
class MirrorActor;
class PartsModel;
class ActorLightCtrl;
class J3DFrameCtrl;
class Nerve;
class JMapInfoIter;
class MsgSharedGroup;

class ProjmapEffectMtxSetter {
public:
    explicit ProjmapEffectMtxSetter(LiveActor* pActor);

    void updateMtxUseBaseMtx();
    void updateMtxUseBaseMtxWithLocalOffset(const TVec3f& offset);

private:
    LiveActor* mActor = nullptr;
};

namespace MR {
    MirrorActor* tryCreateMirrorActor(LiveActor* pActor, const char* pModelName);
    ProjmapEffectMtxSetter* initDLMakerProjmapEffectMtxSetter(LiveActor* pActor);
    void connectToSceneSky(LiveActor* pActor);
    void validateClipping(LiveActor* pActor);
    void invalidateClipping(LiveActor* pActor);
    void setClippingTypeSphere(LiveActor* pActor, f32 radius);
    void setClippingTypeSphere(LiveActor* pActor, f32 radius, const TVec3f* pCenter);
    void setClippingFar(LiveActor* pActor, f32 distance);
    void setGroupClipping(LiveActor* pActor, const JMapInfoIter& rIter, int groupSize);
    void setClippingFar50m(LiveActor* pActor);
    void setClippingFar100m(LiveActor* pActor);
    void setBinderExceptSensorType(LiveActor* pActor, const TVec3f* pCenter, f32 radius);
    bool isOnGround(const LiveActor* pActor);
    bool isBindedGround(const LiveActor* pActor);
    bool isBindedWall(const LiveActor* pActor);
    bool isBindedRoof(const LiveActor* pActor);
    bool isPressedRoofAndGround(const LiveActor* pActor);
    const TVec3f* getGroundNormal(const LiveActor* pActor);
    const TVec3f* getWallNormal(const LiveActor* pActor);
    const TVec3f* getRoofNormal(const LiveActor* pActor);
    bool isNoBind(const LiveActor* pActor);
    void onBind(LiveActor* pActor);
    void offBind(LiveActor* pActor);
    void offCalcGravity(LiveActor* pActor);
    void onCalcGravity(LiveActor* pActor);
    void calcGravityOrZero(LiveActor* pActor);
    void calcGravityOrZero(LiveActor* pActor, const TVec3f& position);
    MsgSharedGroup* joinToGroupArray(LiveActor* pActor, const JMapInfoIter& rIter, const char* pGroupName, s32 capacity);
    void startBck(LiveActor* pActor, const char* pName, const char* pFileName);
    void startBrk(LiveActor* pActor, const char* pName);
    void startBtk(LiveActor* pActor, const char* pName);
    void startAction(LiveActor* pActor, const char* pName);
    void setBrkFrame(LiveActor* pActor, f32 frame);
    void setBrkFrameAndStop(LiveActor* pActor, f32 frame);
    void setBrkFrameEndAndStop(LiveActor* pActor);
    J3DFrameCtrl* getBrkCtrl(LiveActor* pActor);
    void setBaseTRMtx(LiveActor* pActor, MtxPtr pMtx);
    void setBaseTRMtx(LiveActor* pActor, const smgpc::render::J3dMatrix3x4& matrix);
    PartsModel* createPartsModelMapObj(LiveActor* pHost, const char* pName, const char* pModelName, MtxPtr pMtx);
    PartsModel* createPartsModelNoSilhouettedMapObj(LiveActor* pHost, const char* pName, const char* pModelName, MtxPtr pMtx);
    LodCtrl* createLodCtrlNPC(LiveActor*, const JMapInfoIter&);
    void connectToDrawTemporarily(LiveActor* pActor);
    void disconnectToDrawTemporarily(LiveActor* pActor);
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
    void initDefaultPos(LiveActor* pActor, const JMapInfoIter& rIter);
    void initDefaultPosNoRepeat(LiveActor* pActor, const JMapInfoIter& rIter);
    bool isHiddenModel(const LiveActor* pActor);
    bool isClipped(const LiveActor* pActor);
    bool isNoEntryDrawBuffer(const LiveActor* pActor);
    void onEntryDrawBuffer(LiveActor* pActor);
    void offEntryDrawBuffer(LiveActor* pActor);
    void showModel(LiveActor* pActor);
    void hideModel(LiveActor* pActor);
    bool isNoCalcAnim(const LiveActor* pActor);
    bool isDead(const LiveActor* pActor);
    bool isStep(const LiveActor* pActor, s32 step);
    bool isFirstStep(const LiveActor* pActor);
    bool isGreaterEqualStep(const LiveActor* pActor, s32 step);
    void setNerveAtStep(LiveActor* pActor, const Nerve* pNerve, s32 step);
    bool isBtpStopped(const LiveActor* pActor);
    bool isBckStopped(const LiveActor* pActor);
    bool checkPassBckFrame(const LiveActor* pActor, f32 frame);
    f32 calcNerveValue(const LiveActor* pActor, s32 stepMax, f32 valueStart, f32 valueEnd);
    f32 calcNerveEaseInRate(const LiveActor* pActor, s32 stepMax);
    f32 calcHitPowerToWall(const LiveActor* pActor);
    f32 getBckFrameMax(const LiveActor* pActor);
    MtxPtr getJointMtx(const LiveActor* pActor, const char* pJointName);
    bool isBrkOneTimeAndStopped(const LiveActor* pActor);
}  // namespace MR
