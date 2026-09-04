#include "Game/Effect/ParticleEmitterHolder.hpp"
#include "Game/Effect/EffectSystem.hpp"

#include "Game/Effect/EffectSystemUtil.hpp"
#include "Game/Effect/MultiEmitterCallBack.hpp"
#include <algorithm>
#include <functional.hpp>

ParticleEmitterHolder::ParticleEmitterHolder(const EffectSystem* pEffectSystem, int numEmitters) : mEffectSystem(pEffectSystem) {
    mEmitters.init(numEmitters);
}

void ParticleEmitterHolder::update(bool isCalcFor2D) {
    for (ParticleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        JPABaseEmitter* pBaseEmitter = pEmitter->mEmitter;
        if (pBaseEmitter == nullptr || isCalcFor2D != (pBaseEmitter->getGroupID() == 1)) {
            continue;
        }

        if (pBaseEmitter->isEnableDeleteEmitter()) {
            mEffectSystem->forceDeleteEmitter(pEmitter);
        } else if (!pEmitter->mStopped) {
            if (MR::Effect::getLinkSingleEmitter(pBaseEmitter) != nullptr) {
                static_cast< MultiEmitterCallBackBase* >(pBaseEmitter->mpEmtrCallBack)->init(pBaseEmitter);
            }
            pEmitter->mStopped = true;
        }
    }
}

void ParticleEmitterHolder::forceDeleteAllOneTimeEmitters() {
    for (ParticleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        if (pEmitter->isValid() && !pEmitter->isContinuousParticle()) {
            mEffectSystem->forceDeleteEmitter(pEmitter);
        }
    }
}

void ParticleEmitterHolder::forceDeleteAllEmitters() {
    for (ParticleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        mEffectSystem->forceDeleteEmitter(pEmitter);
    }
}

void ParticleEmitterHolder::requestMovementOnAllEmitters() {
    for (ParticleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        if (pEmitter->isValid() && pEmitter->mEmitter->getGroupID() != 1 && pEmitter->mEmitter->getGroupID() != 7) {
            pEmitter->pauseOff();
        }
    }
}

ParticleEmitter* ParticleEmitterHolder::findAvailableParticleEmitter() {
    ParticleEmitter* pEmitter = std::find_if(mEmitters.begin(), mEmitters.end(), std::not1(std::mem_fun_ref(&ParticleEmitter::isValid)));
    return pEmitter == mEmitters.end() ? nullptr : pEmitter;
}

void ParticleEmitterHolder::requestMovementOffAllLoopEmitters() {
    for (ParticleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        if (pEmitter->isValid() && pEmitter->isContinuousParticle()) {
            pEmitter->pauseOn();
        }
    }
}
