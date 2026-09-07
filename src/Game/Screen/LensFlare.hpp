#pragma once

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/Array.hpp"

class BrightCamInfo;
class BrightObjBase;
class TriggerChecker;

class LensFlareModel : public LiveActor {
public:
    LensFlareModel(const char* name, const char* archiveName);
    ~LensFlareModel() override;

    void appear() override;
    void control() override;
    virtual void appearAnim();
    virtual void controlAnim();

    void update(bool inArea, bool brightVisible);
    void exeKill();
    void exeHide();
    void exeShow();
    void exeFadeIn();
    void exeFadeOut();
    void notifyInArea();

protected:
    friend class LensFlareDirector;

    /* 0x8C */ f32 mIntensity;
    /* 0x90 */ f32 mFade;
    /* 0x94 */ f32 mFadeStep;
    /* 0x98 */ TriggerChecker* mAreaTrigger;
    /* 0x9C */ TriggerChecker* mBrightTrigger;
};

class LensFlareRing : public LensFlareModel {
public:
    LensFlareRing();

    void appearAnim() override;
    void controlAnim() override;

    /* 0xA0 */ f32 mDistanceFrame;
};

class LensFlareGlow : public LensFlareModel {
public:
    LensFlareGlow();

    void appearAnim() override;
    void controlAnim() override;
};

class LensFlareLine : public LensFlareModel {
public:
    LensFlareLine();

    void appearAnim() override;
    void controlAnim() override;
};

class LensFlareDirector : public NameObj {
public:
    LensFlareDirector();
    ~LensFlareDirector() override;

    void init(const JMapInfoIter& iter) override;
    void movement() override;
    virtual void drawSyncCallback(u16 token);

    void pauseOff();
    void setDrawSyncToken();
    s32 checkArea();
    bool checkBrightObj(bool check);
    void controlFlare(s32 areaFlags, bool hasBrightObj);

    void addBrightObj(BrightObjBase* brightObj);
    void removeBrightObj(BrightObjBase* brightObj) noexcept;

    /* 0x0C */ void* mDrawSyncCallbackHost;
    /* 0x10 */ LensFlareRing* mRing;
    /* 0x14 */ LensFlareGlow* mGlow;
    /* 0x18 */ LensFlareLine* mLine;
    /* 0x1C */ MR::Vector< MR::FixedArray< BrightObjBase*, 16 > > mBrightObjArray;
    /* 0x60 */ TVec2f mBrightnessCenter;
    /* 0x68 */ f32 mBright;
    /* 0x6C */ TVec2f mRealCenter;
    /* 0x74 */ TVec2f mNowCenter;
    /* 0x7C */ u16 mDrawSyncTokenBase;
    /* 0x7E */ u16 mDrawSyncTokenIndex;
    /* 0x80 */ BrightCamInfo* mBrightCamInfo;
};

namespace MR {
    void addBrightObj(BrightObjBase* brightObj);
    void removeBrightObj(BrightObjBase* brightObj) noexcept;
    void setLensFlareDrawSyncToken();
    u32 getLensFlareDrawSyncTokenIndex();
}  // namespace MR
