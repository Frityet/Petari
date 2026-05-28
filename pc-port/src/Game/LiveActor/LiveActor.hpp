#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <string_view>

#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>
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

struct TVec3f {
    f32 x = 0.0F;
    f32 y = 0.0F;
    f32 z = 0.0F;

    constexpr TVec3f() = default;

    constexpr TVec3f(f32 newX, f32 newY, f32 newZ) : x(newX), y(newY), z(newZ) {
    }

    template < typename T >
    void set(T newX, T newY, T newZ) {
        x = static_cast< f32 >(newX);
        y = static_cast< f32 >(newY);
        z = static_cast< f32 >(newZ);
    }

    void set(const TVec3f& value) {
        x = value.x;
        y = value.y;
        z = value.z;
    }

    void add(const TVec3f& value) {
        x += value.x;
        y += value.y;
        z += value.z;
    }

    void sub(const TVec3f& value) {
        x -= value.x;
        y -= value.y;
        z -= value.z;
    }

    void scale(f32 value) {
        x *= value;
        y *= value;
        z *= value;
    }

    [[nodiscard]] f32 dot(const TVec3f& value) const {
        return (x * value.x) + (y * value.y) + (z * value.z);
    }

    [[nodiscard]] f32 length() const {
        return std::sqrt(dot(*this));
    }

    [[nodiscard]] f32 squareDistance(const TVec3f& value) const {
        const auto dx = x - value.x;
        const auto dy = y - value.y;
        const auto dz = z - value.z;
        return (dx * dx) + (dy * dy) + (dz * dz);
    }

    [[nodiscard]] f32 distance(const TVec3f& value) const {
        return std::sqrt(squareDistance(value));
    }
};

[[nodiscard]] constexpr TVec3f operator+(const TVec3f& lhs, const TVec3f& rhs) {
    return TVec3f{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

[[nodiscard]] constexpr TVec3f operator-(const TVec3f& lhs, const TVec3f& rhs) {
    return TVec3f{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[nodiscard]] constexpr TVec3f operator*(const TVec3f& value, f32 scale) {
    return TVec3f{value.x * scale, value.y * scale, value.z * scale};
}

[[nodiscard]] constexpr TVec3f operator*(f32 scale, const TVec3f& value) {
    return value * scale;
}

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
    void drawModel(smgpc::render::IRendererEngine& renderer, const smgpc::camera::CameraPose& camera_pose, std::uint64_t frame,
                   smgpc::render::live_actor::LiveActorModel::DrawPass pass = smgpc::render::live_actor::LiveActorModel::DrawPass::All);
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
    ActorLightCtrl* mActorLightCtrl = nullptr;

private:
    bool mIsDead = true;
    Spine* mSpine = nullptr;
    smgpc::render::J3dMatrix3x4 mBaseMatrix{};
    J3DFrameCtrl mBrkCtrl{};
    bool mBrkActive = false;
    std::string mCurrentBckName{};
    std::string mCurrentBrkName{};
    std::string mCurrentBtkName{};
    std::unique_ptr< smgpc::render::live_actor::LiveActorModel > mModel{};
};
