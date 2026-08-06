#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/JGeometry/TVec.hpp>
#include <revolution.h>

#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "RendererService.hpp"
#include "camera/CameraPose.hpp"
#include "render/J3dMaterialRuntime.hpp"
#include "render/live_actor/LiveActorModel.hpp"

class ActorLightCtrl;
class HitSensor;
class Nerve;
class Spine;
class StageSwitchCtrl;

class LiveActor : public NameObj {
public:
    LiveActor(const char* pName);
    virtual ~LiveActor();

    void init(const JMapInfoIter& rIter) override;
    virtual void initAfterPlacement() override;
    void movement() override;
    void calcAnim() override;
    void calcViewAndEntry() override;
    virtual void appear();
    virtual void kill();
    virtual void makeActorAppeared();
    virtual void makeActorDead();
    virtual bool receiveMessage(u32 msg, HitSensor* pSender, HitSensor* pReceiver);
    virtual bool receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver);
    virtual void control();
    virtual void attackSensor(HitSensor* pSender, HitSensor* pReceiver);
    virtual void startClipped();
    virtual void endClipped();
    virtual void calcAndSetBaseMtx();

    void initNerve(const Nerve* pNerve);
    void updateNerve();
    void setNerve(const Nerve* pNerve);
    bool isNerve(const Nerve* pNerve) const;
    s32 getNerveStep() const;
    void initSound(s32 soundCount, bool usesCallback);
    void initModelManagerWithAnm(const char* pModelArcName, const char* pAnimArcName, bool);
    void initEffectKeeper(int effectNum, const char* pEffectName, bool);
    void initActorLightCtrl();
    void loadActorLight() const;
    void setBaseMatrix(const smgpc::render::J3dMatrix3x4& matrix);
    void setProjmapEffectMatrix(const smgpc::render::J3dMatrix3x4& matrix);
    void drawModel(const smgpc::camera::CameraPose& camera_pose, std::uint64_t frame,
                   smgpc::render::live_actor::LiveActorModel::DrawPass pass = smgpc::render::live_actor::LiveActorModel::DrawPass::All);
    void initHitSensor(s32 sensorCount);
    void initStageSwitch(const JMapInfoIter& rIter);
    HitSensor* addHitSensor(const char* pName, u32 type, u16 groupSize, f32 radius, const TVec3f& offset);
    [[nodiscard]] HitSensor* getSensor(const char* pName);
    [[nodiscard]] const HitSensor* getSensor(const char* pName) const;
    [[nodiscard]] const char* getSensorName(const HitSensor* pSensor) const;
    void collectHitSensors(std::vector< HitSensor* >& sensors);
    void validateHitSensors();
    void invalidateHitSensors();
    void updateHitSensors();
    void startBck(const char* pName, const char* pFileName);
    void startBrk(const char* pName);
    void startBtk(const char* pName);
    void setBrkFrame(f32 frame);
    void setBrkFrameAndStop(f32 frame);
    void setBrkFrameEndAndStop();
    [[nodiscard]] J3DFrameCtrl* getBrkCtrl();
    [[nodiscard]] const J3DFrameCtrl* getBrkCtrl() const;
    [[nodiscard]] bool isBrkOneTimeAndStopped() const;
    [[nodiscard]] std::string_view currentBckName() const;
    [[nodiscard]] std::string_view currentBrkName() const;
    [[nodiscard]] std::string_view currentBtkName() const;

    [[nodiscard]] bool isDead() const;
    [[nodiscard]] const smgpc::render::J3dMatrix3x4& getBaseMatrix() const;

    TVec3f mPosition{};
    TVec3f mRotation{};
    TVec3f mScale{1.0F, 1.0F, 1.0F};
    StageSwitchCtrl* mStageSwitchCtrl = nullptr;
    ActorLightCtrl* mActorLightCtrl = nullptr;

private:
    struct ActorHitSensor {
        std::string name{};
        TVec3f offset{};
        std::unique_ptr< HitSensor > sensor{};
    };

    bool mIsDead = true;
    Spine* mSpine = nullptr;
    smgpc::render::J3dMatrix3x4 mBaseMatrix{};
    J3DFrameCtrl mBrkCtrl{};
    bool mBrkActive = false;
    std::string mCurrentBckName{};
    std::string mCurrentBrkName{};
    std::string mCurrentBtkName{};
    std::vector< ActorHitSensor > mHitSensors{};
    std::unique_ptr< smgpc::render::live_actor::LiveActorModel > mModel{};
};
