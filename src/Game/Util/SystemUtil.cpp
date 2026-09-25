#include "Game/Util/SystemUtil.hpp"
#include "Game/NameObj/NameObjHolder.hpp"
#include "Game/System/AudSystemWrapper.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemFontHolder.hpp"
#include "Game/System/GameSystemFunction.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/System/MessageHolder.hpp"
#include "Game/System/RenderMode.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include <nw4r/lyt/layout.h>
#include <nw4r/ut/ResFont.h>

namespace MR {
    GameSystemObjHolder* getGameSystemObjHolder() {
        return SingletonHolder< GameSystem >::get()->mObjHolder;
    }
};  // namespace MR

namespace {
    NameObjHolder* getSceneNameObjHolder() NO_INLINE {
        return SingletonHolder< GameSystem >::get()->mSceneController->mObjHolder;
    }
};  // namespace

namespace MR {
    nw4r::ut::Font* getFontOnCurrentLanguage() {
        return SingletonHolder< GameSystem >::get()->mFontHolder->getMessageFont();
    }

    nw4r::ut::Font* getPictureFontNW4R() {
        return SingletonHolder< GameSystem >::get()->mFontHolder->mPictureFont;
    }

    nw4r::ut::Font* getMenuFontNW4R() {
        return SingletonHolder< GameSystem >::get()->mFontHolder->mMenuFont;
    }

    nw4r::ut::Font* getNumberFontNW4R() {
        return SingletonHolder< GameSystem >::get()->mFontHolder->mNumberFont;
    }

    nw4r::ut::Font* getCinemaFontNW4R() {
        return SingletonHolder< GameSystem >::get()->mFontHolder->mCinemaFont;
    }

    void requestChangeArchivePlayer(bool isPlayerMario) {
        GameSystemFunction::requestChangeArchivePlayer(isPlayerMario);
    }

    void waitEndChangeArchivePlayer() {
        while (!GameSystemFunction::isEndChangeArchivePlayer()) {
        }
    }

    void callMethodAllSceneNameObj(NameObjMethod pMethod) {
        ::getSceneNameObjHolder()->callMethodAllObj(pMethod);
    }

    void suspendAllSceneNameObj() {
        ::getSceneNameObjHolder()->suspendAllObj();
    }

    void resumeAllSceneNameObj() {
        ::getSceneNameObjHolder()->resumeAllObj();
    }

    void syncWithFlagsAllSceneNameObj() {
        ::getSceneNameObjHolder()->syncWithFlags();
    }

    void setRandomSeedFromStageName() {
        u32 seed = getHashCode(getCurrentStageName());

        getGameSystemObjHolder()->mRandom.mSeed = seed;
    }

    void clearFileLoaderRequestFileInfo(bool param1) {
        getGameSystemObjHolder()->clearRequestFileInfo(param1);
    }

    bool isScreen16Per9() {
        return isAspectRatioFlag16Per9();
    }

    void initSceneMessage() {
        getGameSystemObjHolder()->mMessageHolder->initSceneData();
    }

    void destroySceneMessage() {
        getGameSystemObjHolder()->mMessageHolder->destroySceneData();
    }

    void resetSystemAndGameStatus() {
        GameDataFunction::resetAllGameData();
        GameSystemFunction::resetAllControllerRumble();
        resetGlobalTimer();
    }

    void stopAllSound(u32 param1) {
        getGameSystemObjHolder()->mAudioSystem->stopAllSound(param1);
    }

    void setLayoutDefaultAllocator() {
        nw4r::lyt::Layout::mspAllocator = &MR::NewDeleteAllocator::sAllocator;
    }

};  // namespace MR
