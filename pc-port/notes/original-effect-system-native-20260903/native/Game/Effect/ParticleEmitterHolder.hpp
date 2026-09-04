#pragma once

#include "Game/Effect/ParticleEmitter.hpp"
#include "Game/Util/Array.hpp"

class EffectSystem;

class ParticleEmitterHolder {
public:
    ParticleEmitterHolder(EffectSystem const*, int);

    void update(bool);
    void forceDeleteAllOneTimeEmitters();
    void forceDeleteAllEmitters();
    void requestMovementOnAllEmitters();
    ParticleEmitter* findAvailableParticleEmitter();
    void requestMovementOffAllLoopEmitters();

    EffectSystem const* mEffectSystem;                 // 0x0
    MR::AssignableArray< ParticleEmitter > mEmitters;  // 0x4
};
