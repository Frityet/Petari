#pragma once

#include <array>
#include <memory>
#include <string_view>

#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>

#include "Game/NameObj/NameObj.hpp"
#include "Game/Screen/LayoutActorFlag.hpp"
#include "Game/Util/GamePadUtil.hpp"

class EffectSystem;
class LayoutManager;
class Nerve;
class PaneEffectKeeper;
class SimpleLayout;
class Spine;
class StarPointerLayoutTargetKeeper;

class LayoutActor : public NameObj {
public:
    LayoutActor(const char*, bool);
    ~LayoutActor() override;

    void movement() override;
    void draw() const override;
    void calcAnim() override;
    virtual void appear();
    virtual void kill();
    virtual void control() {
    }

    void setNerve(const Nerve*) const;
    bool isNerve(const Nerve*) const;
    s32 getNerveStep() const;
    TVec2f getTrans() const;
    void setTrans(const TVec2f&);
    LayoutManager* getLayoutManager() const;
    void createPaneMtxRef(const char*);
    MtxPtr getPaneMtxRef(const char*);
    void initLayoutManager(const char*, u32);
    void initLayoutManagerNoConvertFilename(const char*, u32);
    void initLayoutManagerWithTextBoxBufferLength(const char*, u32, u32);
    void initNerve(const Nerve*);
    void initEffectKeeper(int, const char*, const EffectSystem*);
    void initPointingTarget(int);
    void updateSpine();

    void drawLayout();
    [[nodiscard]] bool isDead() const;
    [[nodiscard]] SimpleLayout* getSimpleLayout();
    [[nodiscard]] const SimpleLayout* getSimpleLayout() const;
    [[nodiscard]] J3DFrameCtrl* getAnimCtrl(u32 animLayer);
    void startAnim(const char* pAnimName, u32 animLayer);
    void setAnimFrame(f32 frame, u32 animLayer);
    void setAnimFrameAndStop(f32 frame, u32 animLayer);
    void setAnimRate(f32 rate, u32 animLayer);
    [[nodiscard]] f32 getAnimFrame(u32 animLayer) const;
    [[nodiscard]] bool isAnimStopped(u32 animLayer);
    void setTextBoxNumberRecursive(const char* pPaneName, s32 number);
    void setTextBoxStringRecursive(const char* pPaneName, std::u16string_view text);

    /* 0x0C */ LayoutManager* mManager;
    /* 0x10 */ mutable Spine* mSpine;
    /* 0x14 */ PaneEffectKeeper* mPaneEffectKeeper;
    /* 0x18 */ StarPointerLayoutTargetKeeper* mStarPointerTargetKeeper;
    /* 0x1C */ LayoutActorFlag mFlag;

private:
    [[nodiscard]] J3DFrameCtrl& animCtrl(u32 animLayer);
    [[nodiscard]] const J3DFrameCtrl& animCtrl(u32 animLayer) const;
    void syncLayoutFromAnimCtrl(u32 animLayer);
    void syncAnimCtrlFromLayout(u32 animLayer);

    std::unique_ptr< LayoutManager > mManagerOwner;
    std::unique_ptr< SimpleLayout > mSimpleLayout;
    TVec2f mTrans{};
    std::array< J3DFrameCtrl, 4 > mAnimCtrls{};
    std::array< J3DFrameCtrl, 4 > mLastSyncedAnimCtrls{};
};
