#include <aurora/exception.hpp>
#include "Game/Util/SystemUtil.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "runtime/ParticleResourceOwnership.hpp"
#include <stdexcept>

namespace MR {
    ParticleResourceHolder* getParticleResourceHolder() {
        if (auto* system = SingletonHolder<GameSystem>::get()) {
            if (!system->mObjHolder || !system->mObjHolder->mParticleResHolder)
                aurora::throw_host_exception<std::logic_error>("Original GameSystem particle resources are not initialized");
            return system->mObjHolder->mParticleResHolder;
        }
        auto* owner = smgpc::runtime::ParticleResourceOwnership::active();
        if (!owner)
            aurora::throw_host_exception<std::logic_error>("Particle resources require a constructed process owner");
        return &owner->holder();
    }
}
