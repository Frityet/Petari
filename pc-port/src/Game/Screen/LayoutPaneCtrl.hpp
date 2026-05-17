#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>
#include <revolution/types.h>

#include "Game/Util/GamePadUtil.hpp"

class LayoutManager;

#ifndef NDEBUG
struct LayoutPaneControlAnimationDebugState {
    u32 layer_index = 0U;
    std::string name;
    f32 frame = 0.0F;
    f32 end_frame = 0.0F;
    f32 rate = 0.0F;
    bool stopped = true;
    bool looping = false;
};
#endif

class LayoutPaneCtrl {
public:
    LayoutPaneCtrl(LayoutManager* pHost, const char* pPaneName, u32 animLayerNum);

    void movement();
    void calcAnim();
    void start(const char*, u32);
    void stop(u32);
    bool isAnimStopped(u32) const;
    void reflectFollowPos();
    J3DFrameCtrl* getFrameCtrl(u32) const;
    void recalcChildGlobalMtx(void*);

    void setFrame(f32 frame, u32 animLayer);
    void setRate(f32 rate, u32 animLayer);
    [[nodiscard]] f32 getFrame(u32 animLayer) const;
    [[nodiscard]] std::string_view paneName() const;
    [[nodiscard]] u32 animLayerCount() const;
#ifndef NDEBUG
    [[nodiscard]] std::vector< LayoutPaneControlAnimationDebugState > debugAnimations() const;
#endif

    /* 0x00 */ LayoutManager* mHost;
    /* 0x04 */ void* mPane;
    /* 0x08 */ s32 mPaneIndex;
    /* 0x0C */ u32 mFollowType;
    /* 0x10 */ const TVec2f* mFollowPos;

private:
    [[nodiscard]] u32 layerIndex(u32 animLayer) const;
    void syncLayoutFrame(u32 animLayer);

    std::string mPaneName;
    std::vector< J3DFrameCtrl > mFrameCtrls;
    std::vector< std::string > mAnimNames;
    std::vector< bool > mStopped;
};
