#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class ProjmapEffectMtxSetter;

class FileSelectSky : public LiveActor {
public:
    FileSelectSky(const char* pName);
    ~FileSelectSky() override;
    using LiveActor::draw;

    void init(const JMapInfoIter& rIter) override;
    void calcAnim() override;
    void calcAndSetBaseMtx() override;
    bool receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver) override;

    void exeWait();
    void draw(smgpc::render::IRendererEngine& renderer, const smgpc::game::CameraPoseCompat& camera_pose);

private:
    f32 _8C = 0.0F;
    f32 _90 = 0.0F;
    smgpc::game::J3dMatrix3x4 _94{};
    ProjmapEffectMtxSetter* mProjmapEffectMtxSetter = nullptr;
};
