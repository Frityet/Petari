#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class ProjmapEffectMtxSetter;

class FileSelectSky : public LiveActor {
public:
    FileSelectSky(const char* pName);
    ~FileSelectSky() override;

    void init(const JMapInfoIter& rIter) override;
    void calcAnim() override;
    void calcAndSetBaseMtx() override;
    bool receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver) override;

    void exeWait();

private:
    f32 _8C = 0.0F;
    f32 _90 = 0.0F;
    smgpc::render::J3dMatrix3x4 _94{};
    ProjmapEffectMtxSetter* mProjmapEffectMtxSetter = nullptr;
};
