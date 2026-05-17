#include "Game/Screen/LayoutPaneCtrl.hpp"

#include <algorithm>
#include <cmath>

#include "Game/Screen/LayoutManager.hpp"

LayoutPaneCtrl::LayoutPaneCtrl(LayoutManager* pHost, const char* pPaneName, u32 animLayerNum)
    : mHost(pHost), mPane(nullptr), mPaneIndex(-1), mFollowType(0U), mFollowPos(nullptr), mPaneName(pPaneName != nullptr ? pPaneName : ""),
      mFrameCtrls(std::max<u32>(animLayerNum, 1U)), mAnimNames(mFrameCtrls.size()), mStopped(mFrameCtrls.size(), true) {
}

void LayoutPaneCtrl::movement() {
    for (auto layer = u32{}; layer < mFrameCtrls.size(); ++layer) {
        auto& ctrl = mFrameCtrls[layer];
        if (mAnimNames[layer].empty() || mStopped[layer]) {
            continue;
        }

        if (ctrl.mRate == 0.0F) {
            mStopped[layer] = true;
            syncLayoutFrame(layer);
            continue;
        }

        ctrl.mFrame += ctrl.mRate;
        const auto end_frame = static_cast< f32 >(ctrl.mEnd);
        if (ctrl.mLoop != 0 && end_frame > 0.0F) {
            ctrl.mFrame = std::fmod(ctrl.mFrame, end_frame);
        } else if (ctrl.mFrame >= end_frame || ctrl.mFrame <= static_cast< f32 >(ctrl.mStart)) {
            ctrl.mFrame = std::clamp(ctrl.mFrame, static_cast< f32 >(ctrl.mStart), end_frame);
            mStopped[layer] = true;
        }
        syncLayoutFrame(layer);
    }
}

void LayoutPaneCtrl::calcAnim() {
}

void LayoutPaneCtrl::start(const char* pAnimName, u32 animLayer) {
    const auto layer = layerIndex(animLayer);
    auto& ctrl = mFrameCtrls[layer];
    mAnimNames[layer] = pAnimName != nullptr ? pAnimName : "";
    ctrl.mStart = 0;
    ctrl.mEnd = static_cast< s16 >(mHost != nullptr ? mHost->getAnimFrameMax(pAnimName) : 1.0F);
    ctrl.mLoop = mHost != nullptr && mHost->isLoopingAnim(pAnimName) ? 1 : 0;
    ctrl.mRate = 1.0F;
    ctrl.mFrame = 0.0F;
    mStopped[layer] = false;
    if (mHost != nullptr) {
        mHost->startPaneAnim(mPaneName.c_str(), pAnimName, layer);
    }
}

void LayoutPaneCtrl::stop(u32 animLayer) {
    const auto layer = layerIndex(animLayer);
    mStopped[layer] = true;
    mFrameCtrls[layer].mRate = 0.0F;
    if (mHost != nullptr) {
        mHost->stopPaneAnim(mPaneName.c_str(), layer);
    }
}

bool LayoutPaneCtrl::isAnimStopped(u32 animLayer) const {
    return mStopped[layerIndex(animLayer)];
}

void LayoutPaneCtrl::reflectFollowPos() {
}

J3DFrameCtrl* LayoutPaneCtrl::getFrameCtrl(u32 animLayer) const {
    return const_cast< J3DFrameCtrl* >(&mFrameCtrls[layerIndex(animLayer)]);
}

void LayoutPaneCtrl::recalcChildGlobalMtx(void*) {
}

void LayoutPaneCtrl::setFrame(f32 frame, u32 animLayer) {
    const auto layer = layerIndex(animLayer);
    mFrameCtrls[layer].mFrame = frame;
    syncLayoutFrame(layer);
}

void LayoutPaneCtrl::setRate(f32 rate, u32 animLayer) {
    const auto layer = layerIndex(animLayer);
    mFrameCtrls[layer].mRate = rate;
    mStopped[layer] = rate == 0.0F;
    if (mHost != nullptr) {
        mHost->setPaneAnimRate(mPaneName.c_str(), rate, layer);
    }
}

f32 LayoutPaneCtrl::getFrame(u32 animLayer) const {
    return mFrameCtrls[layerIndex(animLayer)].mFrame;
}

std::string_view LayoutPaneCtrl::paneName() const {
    return mPaneName;
}

u32 LayoutPaneCtrl::animLayerCount() const {
    return static_cast< u32 >(mFrameCtrls.size());
}

#ifndef NDEBUG
std::vector< LayoutPaneControlAnimationDebugState > LayoutPaneCtrl::debugAnimations() const {
    auto out = std::vector< LayoutPaneControlAnimationDebugState >{};
    out.reserve(mFrameCtrls.size());
    for (auto layer = u32{}; layer < mFrameCtrls.size(); ++layer) {
        const auto& ctrl = mFrameCtrls[layer];
        out.push_back(LayoutPaneControlAnimationDebugState{
            .layer_index = layer,
            .name = mAnimNames[layer],
            .frame = ctrl.mFrame,
            .end_frame = static_cast< f32 >(ctrl.mEnd),
            .rate = ctrl.mRate,
            .stopped = mStopped[layer],
            .looping = ctrl.mLoop != 0,
        });
    }
    return out;
}
#endif

u32 LayoutPaneCtrl::layerIndex(u32 animLayer) const {
    return std::min< u32 >(animLayer, static_cast< u32 >(mFrameCtrls.size() - 1U));
}

void LayoutPaneCtrl::syncLayoutFrame(u32 animLayer) {
    if (mHost != nullptr) {
        mHost->setPaneAnimFrame(mPaneName.c_str(), mFrameCtrls[layerIndex(animLayer)].mFrame, layerIndex(animLayer));
    }
}
