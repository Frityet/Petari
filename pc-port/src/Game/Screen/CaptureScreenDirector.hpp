#pragma once

#include "Game/NameObj/NameObj.hpp"

class JUTTexture;
struct ResTIMG;

struct TimingInfo {
    /* 0x00 */ const char* mName;
    /* 0x04 */ s32 mTiming;
    /* 0x08 */ bool _8;
    /* 0x0C */ u32 _C;
};

class CaptureScreenDirector : public NameObj {
public:
    CaptureScreenDirector();

    void captureIfAllow(const char* pName);
    void capture();
    void requestCaptureTiming(const char* pName);
    void invalidateCaptureTiming(const char* pName);
    const ResTIMG* getResTIMG() const;
    u8* getTexImage() const;
    const TimingInfo* getUsingTiming() const;
    const TimingInfo* getCurrentTiming() const;
    const TimingInfo* findFromName(const char* pName) const;

private:
    /* 0x0C */ const char* _C;
    /* 0x10 */ const char* mTimingType;
    /* 0x14 */ JUTTexture* mTexture;
    /* 0x18 */ bool _18;
};

class CaptureScreenActor : public NameObj {
public:
    CaptureScreenActor(u32 drawType, const char* pCameraName);

    void draw() const override;

private:
    /* 0x0C */ const char* mCameraName;
};
