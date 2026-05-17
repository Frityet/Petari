#pragma once

#include <revolution.h>

using Pattern = u8 (*)[2];

class JUTVideo {
public:
    using Callback = void (*)(u32);

    explicit JUTVideo(const GXRenderModeObj* render_mode);
    virtual ~JUTVideo();

    static JUTVideo* createManager(const GXRenderModeObj* render_mode);
    static void destroyManager();
    static void drawDoneStart();
    static void dummyNoDrawWait();
    void setRenderMode(const GXRenderModeObj* render_mode);
    void waitRetraceIfNeed();

    static void preRetraceProc(u32);
    static void postRetraceProc(u32);
    static void drawDoneCallback();

    [[nodiscard]] u16 getFbWidth() const { return mRenderObj->fbWidth; }
    [[nodiscard]] u16 getEfbHeight() const { return mRenderObj->efbHeight; }
    void getBounds(u16& width, u16& height) const {
        width = getFbWidth();
        height = getEfbHeight();
    }
    [[nodiscard]] u16 getXfbHeight() const { return mRenderObj->xfbHeight; }
    [[nodiscard]] u32 isAntiAliasing() const { return mRenderObj->aa; }
    [[nodiscard]] Pattern getSamplePattern() const { return mRenderObj->sample_pattern; }
    [[nodiscard]] u8* getVFilter() const { return mRenderObj->vfilter; }

    static JUTVideo* getManager();

    [[nodiscard]] GXRenderModeObj* getRenderMode() const { return const_cast<GXRenderModeObj*>(&mRenderObjStorage); }

private:
    GXRenderModeObj mRenderObjStorage{};
    GXRenderModeObj* mRenderObj = &mRenderObjStorage;
};

inline JUTVideo* JUTGetVideoManager() {
    return JUTVideo::getManager();
}

inline void JUTDestroyVideoManager() {
    JUTVideo::destroyManager();
}
