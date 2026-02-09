#include "menu/MenuDirector.hpp"

#include "core/Logger.hpp"
#include "render/BgfxRenderer.hpp"

#include <stdexcept>
#include <string>

namespace pcport {

const char* ToString(MenuState state) {
    switch (state) {
    case MenuState::Boot:
        return "Boot";
    case MenuState::TitleAppear:
        return "TitleAppear";
    case MenuState::TitleLoop:
        return "TitleLoop";
    case MenuState::TitleDecide:
        return "TitleDecide";
    case MenuState::TitleDone:
        return "TitleDone";
    }
    return "Unknown";
}

const BrlanAnimation* MenuDirector::RequireAnimation(const BrlanBundle& bundle, const char* name) {
    const BrlanAnimation* animation = bundle.FindByName(name);
    if (animation == nullptr) {
        throw std::runtime_error(std::string("Missing required animation: ") + name);
    }
    return animation;
}

MenuDirector MenuDirector::Create(const PreparedMenuAssets& assets, const MenuDirectorConfig& config) {
    MenuDirector director;
    director.mConfig = config;

    director.mPressStartLayout = BrlytLayout::LoadFromDirectory(assets.pressStartDir);
    director.mTitleLogoLayout = BrlytLayout::LoadFromDirectory(assets.titleLogoDir);

    director.mPressStartAnimations = BrlanBundle::LoadFromDirectory(assets.pressStartDir / "anim");
    director.mTitleLogoAnimations = BrlanBundle::LoadFromDirectory(assets.titleLogoDir / "anim");

    game::TitleSequenceProduct::Animations titleAnimations;
    titleAnimations.logoAppear = RequireAnimation(director.mTitleLogoAnimations, "appear");
    titleAnimations.logoWait = RequireAnimation(director.mTitleLogoAnimations, "wait");
    titleAnimations.logoDecide = RequireAnimation(director.mTitleLogoAnimations, "decide");
    titleAnimations.logoReactionA = RequireAnimation(director.mTitleLogoAnimations, "reactiona");
    titleAnimations.logoReactionB = RequireAnimation(director.mTitleLogoAnimations, "reactionb");
    titleAnimations.pressAppear = RequireAnimation(director.mPressStartAnimations, "appear");
    titleAnimations.pressWait = RequireAnimation(director.mPressStartAnimations, "wait");
    titleAnimations.pressEnd = RequireAnimation(director.mPressStartAnimations, "end");
    titleAnimations.pressButtonReaction = RequireAnimation(director.mPressStartAnimations, "buttonreaction");

    director.mTitleSequence = std::make_unique<game::TitleSequenceProduct>(director.mTitleLogoLayout, director.mPressStartLayout, titleAnimations);

    director.EnterState(MenuState::Boot);
    return director;
}

void MenuDirector::SetButtonState(bool aPressed, bool bPressed) {
    mInputA = aPressed;
    mInputB = bPressed;
}

void MenuDirector::RequestAdvance() {
    mAdvanceRequested = true;
}

void MenuDirector::EnterState(MenuState state) {
    mState = state;
    mStateTimeMs = 0.0F;
    mStateFrame = 0.0F;

    Log(LogLevel::Info, LogCategory::Menu, "Menu state -> " + std::string(ToString(state)));

    switch (mState) {
    case MenuState::Boot:
        if (mTitleSequence != nullptr) {
            mTitleSequence->kill();
        }
        break;
    case MenuState::TitleAppear:
        if (mTitleSequence != nullptr) {
            mTitleSequence->appear();
        }
        break;
    case MenuState::TitleLoop:
    case MenuState::TitleDecide:
    case MenuState::TitleDone:
        break;
    }
}

void MenuDirector::Update(float deltaMs) {
    if (deltaMs < 0.0F) {
        deltaMs = 0.0F;
    }

    const float deltaFrames = deltaMs * (60.0F / 1000.0F);
    mStateTimeMs += deltaMs;
    mStateFrame += deltaFrames;

    switch (mState) {
    case MenuState::Boot:
        if (mStateTimeMs >= static_cast<float>(mConfig.bootDurationMs)) {
            EnterState(MenuState::TitleAppear);
        }
        return;

    case MenuState::TitleAppear:
        if (mTitleSequence != nullptr) {
            mTitleSequence->update(deltaFrames, mInputA, mInputB);
            if (mTitleSequence->getState() == game::TitleSequenceProduct::State::LogoDisplay && mTitleSequence->getStateFrame() >= 1.0F) {
                EnterState(MenuState::TitleLoop);
            }
        }
        return;

    case MenuState::TitleLoop:
        if (mTitleSequence != nullptr) {
            bool forceDecide = false;
            if (mAdvanceRequested) {
                forceDecide = true;
            } else if (mConfig.autoAdvanceMs > 0U && mStateTimeMs >= static_cast<float>(mConfig.autoAdvanceMs)) {
                forceDecide = true;
            }

            const bool aPressed = mInputA || forceDecide;
            const bool bPressed = mInputB || forceDecide;
            mTitleSequence->update(deltaFrames, aPressed, bPressed);

            if (forceDecide) {
                mAdvanceRequested = false;
            }

            if (mTitleSequence->getState() == game::TitleSequenceProduct::State::Decide) {
                EnterState(MenuState::TitleDecide);
            }
        }
        return;

    case MenuState::TitleDecide:
        if (mTitleSequence != nullptr) {
            mTitleSequence->update(deltaFrames, mInputA, mInputB);
            if (!mTitleSequence->isActive()) {
                EnterState(MenuState::TitleDone);
            }
        }
        return;

    case MenuState::TitleDone:
        return;
    }
}

void MenuDirector::Render(BgfxRenderer& renderer) const {
    switch (mState) {
    case MenuState::Boot:
        return;
    case MenuState::TitleAppear:
    case MenuState::TitleLoop:
    case MenuState::TitleDecide:
    case MenuState::TitleDone:
        renderer.RenderLayout(mTitleLogoLayout);
        renderer.RenderLayout(mPressStartLayout);
        return;
    }
}

MenuState MenuDirector::GetState() const {
    return mState;
}

float MenuDirector::GetStateFrame() const {
    if ((mState == MenuState::TitleAppear || mState == MenuState::TitleLoop || mState == MenuState::TitleDecide) && mTitleSequence != nullptr) {
        return mTitleSequence->getStateFrame();
    }
    return mStateFrame;
}

const BrlytLayout& MenuDirector::GetPressStartLayout() const {
    return mPressStartLayout;
}

const BrlytLayout& MenuDirector::GetTitleLogoLayout() const {
    return mTitleLogoLayout;
}

}  // namespace pcport
