#include "resource/TextEncoding.hpp"
#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class FileSelectEffect : public LiveActor {
public:
    /// @brief Creates a new `FileSelectEffect`.
    /// @param pName A pointer to the null-terminated name of the object.
    FileSelectEffect(const char* pName = CP932("選択時エフェクト"));

    virtual void init(const JMapInfoIter&);
    virtual void appear();
    virtual void calcAndSetBaseMtx();

    void disappear();

    void exeAppear();
    void exeWait();
    void exeDisappear();

    /* 0x8C */ f32 mEffectFrame;
};
