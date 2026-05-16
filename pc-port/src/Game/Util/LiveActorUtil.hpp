#pragma once

#include <memory>

#include "Game/compat/J3dMaterialRuntime.hpp"
#include <revolution.h>

class LiveActor;

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
    void startBtk(LiveActor* pActor, const char* pName);
    void setBaseTRMtx(LiveActor* pActor, const smgpc::game::J3dMatrix3x4& matrix);
    bool isDead(const LiveActor* pActor);
    bool isStep(const LiveActor* pActor, s32 step);
    bool isFirstStep(const LiveActor* pActor);
}  // namespace MR
