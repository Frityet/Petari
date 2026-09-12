#include "Game/Util/ScreenUtil.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameSystemFunction.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/AudSystemWrapper.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/System/MessageHolder.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/SystemUtil.hpp"
#include <nw4r/lyt/layout.h>

namespace MR {
    GameSystemObjHolder* getGameSystemObjHolder() {
        return SingletonHolder< GameSystem >::get()->mObjHolder;
    }

    void initSceneMessage() {
        getGameSystemObjHolder()->mMessageHolder->initSceneData();
    }

    void destroySceneMessage() {
        getGameSystemObjHolder()->mMessageHolder->destroySceneData();
    }

    void stopAllSound(u32 frames) {
        getGameSystemObjHolder()->mAudioSystem->stopAllSound(frames);
    }

    void setLayoutDefaultAllocator() {
        nw4r::lyt::Layout::mspAllocator = &MR::NewDeleteAllocator::sAllocator;
    }

    void setRandomSeedFromStageName() {
        u32 seed = getHashCode(getCurrentStageName());
        getGameSystemObjHolder()->mRandom.mSeed = seed;
    }

    void clearFileLoaderRequestFileInfo(bool reset) {
        getGameSystemObjHolder()->clearRequestFileInfo(reset);
    }
}

namespace MR {
    void resetSystemAndGameStatus() {
        GameDataFunction::resetAllGameData();
        GameSystemFunction::resetAllControllerRumble();
        resetGlobalTimer();
    }
}
