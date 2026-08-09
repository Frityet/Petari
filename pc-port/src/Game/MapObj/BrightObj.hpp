#pragma once

#include "Game/LiveActor/LiveActor.hpp"
#include "render/BrightVisibilityService.hpp"

#include <JSystem/JGeometry/TMatrix.hpp>

class Sun;

class BrightInfo {
public:
    BrightInfo();

    void write(const TVec2f& brightnessCenter, const TVec2f& realCenter, f32 bright);
    void endRead();
    void reset();

    /* 0x00 */ u32 mWriteIndex;
    /* 0x04 */ u32 mReadIndex;
    /* 0x08 */ TVec2f mBrightnessCenter[3];
    /* 0x20 */ TVec2f mRealCenter[3];
    /* 0x38 */ f32 mBright[3];
};

class BrightDrawInfo {
public:
    BrightDrawInfo();

    void write(u16 token, const TVec3f& position, f32 radius);

    /* 0x00 */ TVec3f mPosition[2];
    /* 0x18 */ f32 mRadius[2];
};

class BrightCamInfo {
public:
    BrightCamInfo();

    void write(u16 token, const TPos3f& viewMtx, const TProj3f& projectionMtx, const TVec3f& cameraDir,
               const TVec3f& cameraPos);

    /* 0x00 */ TMtx34f mViewMtx[2];
    /* 0x60 */ TProj3f mProjectionMtx[2];
    /* 0xE0 */ TVec3f mCameraDir[2];
    /* 0xF8 */ TVec3f mCameraPos[2];
};

class BrightObjBase {
public:
    struct CheckArg {
        u32 mSampleCount;
        u32 mVisibleCount;
        TVec2f mVisiblePositionSum;
        TVec2f mRealCenter;
    };

    BrightObjBase();
    virtual ~BrightObjBase();

    virtual void calcBrightInfo(u16 token, const BrightCamInfo& camera) = 0;
    virtual f32 getBright() const;
    virtual const TVec2f* getBrightCenter() const;
    virtual const TVec2f* getCenter() const;
    virtual void endRead();
    virtual void getNowCenter(TVec2f* center) const;

protected:
    void checkVisibilityOfSphere(u16 token, const BrightCamInfo& camera);
    void setResult(const CheckArg& result);
    void drawSphere(const TVec3f& position, f32 radius) const;

    /* 0x04 */ BrightInfo mBrightInfo;
    /* 0x48 */ TVec2f mBrightnessCenter;
    /* 0x50 */ TVec2f mRealCenter;
    /* 0x58 */ f32 mBright;
    /* 0x5C */ bool mNoVisible;
    /* 0x60 */ BrightDrawInfo mDrawInfo;
    smgpc::render::BrightVisibilitySourceId mVisibilitySourceId;
};

class BrightObj : public LiveActor, public BrightObjBase {
public:
    explicit BrightObj(const char* name);
    ~BrightObj() override;

    void init(const JMapInfoIter& iter) override;
    void draw() const override;
    void control() override;
    void calcBrightInfo(u16 token, const BrightCamInfo& camera) override;
    void getNowCenter(TVec2f* center) const override;

private:
    f32 mRadius;
};

class BrightSun : public LiveActor, public BrightObjBase {
public:
    explicit BrightSun(const char* name);
    ~BrightSun() override;

    void init(const JMapInfoIter& iter) override;
    void draw() const override;
    void control() override;
    void calcBrightInfo(u16 token, const BrightCamInfo& camera) override;
    void getNowCenter(TVec2f* center) const override;

private:
    void controlSunModel();

    Sun* mSun;
};
