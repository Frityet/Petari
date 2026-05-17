#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>
#include <revolution.h>

#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/compat/CameraPose.hpp"
#include "Game/compat/J3dMaterialRuntime.hpp"
#include "Game/compat/LiveActorModelCompat.hpp"
#include "RendererService.hpp"

class ActorLightCtrl;
class HitSensor;
class Nerve;
class Spine;

struct TVec3f {
    f32 x = 0.0F;
    f32 y = 0.0F;
    f32 z = 0.0F;
};

class LiveActor : public NameObj {
public:
    LiveActor(const char* pName);
    virtual ~LiveActor();

    void init(const JMapInfoIter& rIter) override;
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
    virtual void calcAndSetBaseMtx();

    void initNerve(const Nerve* pNerve);
    void updateNerve();
    void setNerve(const Nerve* pNerve);
    bool isNerve(const Nerve* pNerve) const;
    s32 getNerveStep() const;
    void initModelManagerWithAnm(const char* pModelArcName, const char* pAnimArcName, bool);
    void initEffectKeeper(int effectNum, const char* pEffectName, bool);
    void initActorLightCtrl();
    void loadActorLight() const;
    void setBaseMatrix(const smgpc::game::J3dMatrix3x4& matrix);
    void drawModel(smgpc::render::IRendererEngine& renderer, const smgpc::game::CameraPoseCompat& camera_pose, std::uint64_t frame,
                   LiveActorModelCompat::DrawPass pass = LiveActorModelCompat::DrawPass::All);
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
    [[nodiscard]] const smgpc::game::J3dMatrix3x4& getBaseMatrix() const;

    TVec3f mPosition{};
    TVec3f mRotation{};
    TVec3f mScale{1.0F, 1.0F, 1.0F};
    ActorLightCtrl* mActorLightCtrl = nullptr;

private:
    bool mIsDead = true;
    Spine* mSpine = nullptr;
    smgpc::game::J3dMatrix3x4 mBaseMatrix{};
    J3DFrameCtrl mBrkCtrl{};
    bool mBrkActive = false;
    std::string mCurrentBckName{};
    std::string mCurrentBrkName{};
    std::string mCurrentBtkName{};
    std::unique_ptr< LiveActorModelCompat > mModel{};
};
