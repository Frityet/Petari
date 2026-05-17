#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class FileSelectEffect : public LiveActor {
public:
    FileSelectEffect(const char* pName);

    ~FileSelectEffect() override;
    void init(const JMapInfoIter& rIter) override;
    void appear() override;
    void calcAndSetBaseMtx() override;

    void disappear();
    void exeAppear();
    void exeWait();
    void exeDisappear();

    f32 mEffectFrame = 0.0F;
};
