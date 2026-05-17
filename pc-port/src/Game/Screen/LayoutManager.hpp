#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <revolution/types.h>

#include "Game/Screen/LayoutPaneCtrl.hpp"

class ButtonPaneController;
class LayoutActor;

#ifndef NDEBUG
struct LayoutPaneControlDebugState {
    std::string pane_name;
    bool exists_in_layout = false;
    bool visible = true;
    std::vector< LayoutPaneControlAnimationDebugState > animations;
};

struct LayoutButtonControllerDebugState {
    std::string pane_name;
    std::string bounding_pane_name;
    std::string nerve;
    u32 anim_layer = 0U;
    bool active = false;
    bool selected = false;
    bool pointing = false;
    bool appearance_enabled = true;
    bool decide_enabled = true;
    f32 pointing_anim_start_frame = 0.0F;
};
#endif

class LayoutManager {
public:
    LayoutManager(LayoutActor* pHost);
    LayoutManager(const char*, bool, u32, u32);
    ~LayoutManager();

    void movement();
    void calcAnim();
    void draw() const;
    void addPaneCtrl(LayoutPaneCtrl*);
    LayoutPaneCtrl* createAndAddRootPaneCtrl(u32);
    LayoutPaneCtrl* createAndAddPaneCtrl(const char*, u32);
    LayoutPaneCtrl* getPaneCtrl(const char*) const;
    s32 getIndexOfPane(const char*) const;
    bool isExistPaneCtrl(const char*) const;
    void showPane(const char*);
    void hidePane(const char*);
    [[nodiscard]] bool isPaneVisible(const char*) const;
    [[nodiscard]] bool isPointingPane(const char*, f32, f32) const;
    void startPaneAnim(const char*, const char*, u32);
    void stopPaneAnim(const char*, u32);
    void setPaneAnimFrame(const char*, f32, u32);
    void setPaneAnimRate(const char*, f32, u32);
    [[nodiscard]] f32 getPaneAnimFrame(const char*, u32) const;
    [[nodiscard]] bool isPaneAnimStopped(const char*, u32) const;
    [[nodiscard]] f32 getPaneAnimFrameMax(const char*, u32) const;
    [[nodiscard]] f32 getAnimFrameMax(const char*) const;
    [[nodiscard]] bool isLoopingAnim(const char*) const;
    void registerButtonController(ButtonPaneController*);
    void unregisterButtonController(ButtonPaneController*);
#ifndef NDEBUG
    [[nodiscard]] std::vector< LayoutPaneControlDebugState > debugPaneControls() const;
    [[nodiscard]] std::vector< LayoutButtonControllerDebugState > debugButtonControllers() const;
#endif

    /* 0x00 */ LayoutActor* mHost;
    /* 0x04 */ bool mIsScreenHidden;
    /* 0x05 */ bool _61;

private:
    [[nodiscard]] std::string normalizedPaneName(const char*) const;
    [[nodiscard]] LayoutPaneCtrl* findPaneCtrl(std::string_view) const;

    std::vector< std::unique_ptr< LayoutPaneCtrl > > mPaneCtrls;
    std::vector< ButtonPaneController* > mButtonControllers;
};
