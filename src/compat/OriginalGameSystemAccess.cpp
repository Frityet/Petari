#include "Game/System/GameSystem.hpp"
#include "Game/System/AudSystemWrapper.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/System/MessageHolder.hpp"
#include "Game/Util/MemoryUtil.hpp"
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
}
