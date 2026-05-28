#pragma once

#include <revolution.h>

using Pattern = u8 (*)[2];

class JUTVideo {
public:
    using Callback = void (*)(u32);

    explicit JUTVideo(const GXRenderModeObj *render_mode);
    virtual ~JUTVideo();

    static JUTVideo *createManager(const GXRenderModeObj *render_mode);
    static void destroyManager();
    static void drawDoneStart();
    static void dummyNoDrawWait();
    void setRenderMode(const GXRenderModeObj *render_mode);
    void waitRetraceIfNeed();

    static void preRetraceProc(u32);
    static void postRetraceProc(u32);
    static void drawDoneCallback();

    [[nodiscard]] GXRenderModeObj *getRenderMode() const;
    [[nodiscard]] u16 getFbWidth() const {
        return getRenderMode()->fbWidth;
    }
    [[nodiscard]] u16 getEfbHeight() const {
        return getRenderMode()->efbHeight;
    }
    void getBounds(u16 &width, u16 &height) const {
        width = getFbWidth();
        height = getEfbHeight();
    }
    [[nodiscard]] u16 getXfbHeight() const {
        return getRenderMode()->xfbHeight;
    }
    [[nodiscard]] u32 isAntiAliasing() const {
        return getRenderMode()->aa;
    }
    [[nodiscard]] Pattern getSamplePattern() const {
        return getRenderMode()->sample_pattern;
    }
    [[nodiscard]] u8 *getVFilter() const {
        return getRenderMode()->vfilter;
    }

    static JUTVideo *getManager();
};

inline JUTVideo *JUTGetVideoManager() {
    return JUTVideo::getManager();
}

inline void JUTDestroyVideoManager() {
    JUTVideo::destroyManager();
}
