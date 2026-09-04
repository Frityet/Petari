#include "Game/Util/SystemUtil.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/Util/SingletonHolder.hpp"
namespace MR {
    GameSystemObjHolder* getGameSystemObjHolder() {
        return SingletonHolder< GameSystem >::get()->mObjHolder;
    }
    ParticleResourceHolder* getParticleResourceHolder() {
        return getGameSystemObjHolder()->mParticleResHolder;
    }
}
