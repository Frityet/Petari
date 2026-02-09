#pragma once

#include "assets/MenuAssetPipeline.hpp"
#include "game/TitleSequenceProduct.hpp"
#include "layout/BrlanRuntime.hpp"
#include "layout/BrlytRuntime.hpp"

#include <cstdint>
#include <memory>

namespace pcport {

class BgfxRenderer;

enum class MenuState {
    Boot,
    TitleAppear,
    TitleLoop,
    TitleDecide,
    TitleDone,
};

const char* ToString(MenuState state);

struct MenuDirectorConfig {
    std::uint32_t bootDurationMs = 350;
    std::uint32_t autoAdvanceMs = 9000;
};

class MenuDirector {
public:
    static MenuDirector Create(const PreparedMenuAssets& assets, const MenuDirectorConfig& config);

    void SetButtonState(bool aPressed, bool bPressed);
    void RequestAdvance();
    void Update(float deltaMs);
    void Render(BgfxRenderer& renderer) const;

    MenuState GetState() const;
    float GetStateFrame() const;

    const BrlytLayout& GetPressStartLayout() const;
    const BrlytLayout& GetTitleLogoLayout() const;

private:
    void EnterState(MenuState state);

    static const BrlanAnimation* RequireAnimation(const BrlanBundle& bundle, const char* name);

    MenuDirectorConfig mConfig;
    MenuState mState = MenuState::Boot;

    float mStateTimeMs = 0.0F;
    float mStateFrame = 0.0F;

    bool mInputA = false;
    bool mInputB = false;
    bool mAdvanceRequested = false;

    BrlytLayout mPressStartLayout;
    BrlytLayout mTitleLogoLayout;

    BrlanBundle mPressStartAnimations;
    BrlanBundle mTitleLogoAnimations;

    std::unique_ptr<game::TitleSequenceProduct> mTitleSequence;
};

}  // namespace pcport
