#include "Game/Util/SystemUtil.hpp"
#include "runtime/ParticleResourceOwnership.hpp"
#include <stdexcept>

namespace MR {
    ParticleResourceHolder* getParticleResourceHolder() {
        auto* owner = smgpc::runtime::ParticleResourceOwnership::active();
        if (!owner)
            throw std::logic_error("Particle resources require a constructed process owner");
        return &owner->holder();
    }
}
