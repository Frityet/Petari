#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemFontHolder.hpp"
#include "Game/System/ResourceHolderManager.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/SystemUtil.hpp"
#include <nw4r/ut/ResFont.h>
#include <aurora/exception.hpp>
#include <stdexcept>

namespace {
    GameSystemFontHolder& fonts() {
        auto* system = SingletonHolder<GameSystem>::get();
        if (system == nullptr || system->mFontHolder == nullptr)
            aurora::throw_host_exception<std::logic_error>("Layout font access requires the original process font owner");
        return *system->mFontHolder;
    }

    ResourceHolderManager& resources() {
        auto* manager = SingletonHolder<ResourceHolderManager>::get();
        if (manager == nullptr)
            aurora::throw_host_exception<std::logic_error>("Layout resource access requires the original resource manager");
        return *manager;
    }
}

namespace MR {
    LayoutHolder* createAndAddLayoutHolder(const char* name) {
        return resources().createAndAddLayoutHolder(name, nullptr);
    }
    LayoutHolder* createAndAddLayoutHolderRawData(const char* name) {
        return resources().createAndAddLayoutHolderRawData(name);
    }
    nw4r::ut::Font* getFontOnCurrentLanguage() { return fonts().getMessageFont(); }
    nw4r::ut::Font* getMenuFontNW4R() { return fonts().mMenuFont; }
    nw4r::ut::Font* getNumberFontNW4R() { return fonts().mNumberFont; }
    nw4r::ut::Font* getPictureFontNW4R() { return fonts().mPictureFont; }
    nw4r::ut::Font* getCinemaFontNW4R() { return fonts().mCinemaFont; }
}
