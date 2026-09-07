#include "Game/Effect/EffectSystem.hpp"
#include "Game/Effect/AutoEffectGroupHolder.hpp"
#include "Game/Effect/ParticleCalcExecutor.hpp"
#include "Game/Effect/ParticleDrawExecutor.hpp"
#include "Game/Effect/ParticleEmitterHolder.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Effect/EffectSystemUtil.hpp"
#include "Game/Effect/SingleEmitter.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "JSystem/JParticle/JPAParticle.hpp"
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JParticle/JPAEmitterManager.hpp>

EffectSystem::EffectSystem(const char* pName, bool createAdaptor)
    : NameObj(pName), mEmitterManager(nullptr), mDrawExec(nullptr), mCalcExec(nullptr), mGroupHolder(new AutoEffectGroupHolder()), _20(true) {
    mDrawExec = new ParticleDrawExecutor(this, createAdaptor);
    mCalcExec = new ParticleCalcExecutor(this, createAdaptor);
}

ParticleEmitter* EffectSystem::createEmitter(u16 id, u8 groupId, u8 resourceId) {
    if (!_20) {
        return nullptr;
    }
    ParticleEmitter* pEmitter = mEmitterHolder->findAvailableParticleEmitter();
    if (pEmitter == nullptr) {
        return nullptr;
    }
    MR::Effect::createParticleEmitter(pEmitter, mEmitterManager, TVec3f(0.0f, 0.0f, 0.0f), id, groupId, resourceId);
    return pEmitter->mEmitter != nullptr ? pEmitter : nullptr;
}

void EffectSystem::entry(ParticleResourceHolder* pResource, u32 particleNum, u32 emitterNum) {
    mEmitterManager = new JPAEmitterManager(particleNum, emitterNum, MR::getCurrentHeap(), 9, 1);
    mEmitterManager->entryResourceManager(pResource->mResourceMgr, 0);
    mEmitterHolder = new ParticleEmitterHolder(this, emitterNum);
    pResource->swapTexture(MR::getScreenResTIMG(), "IndDummy");
}

void EffectSystem::init(const JMapInfoIter& rIter) {
}

void EffectSystem::forceDeleteEmitter(ParticleEmitter* pEmitter) const {
    if (pEmitter->mEmitter != nullptr) {
        SingleEmitter* pSingleEmitter = MR::Effect::getLinkSingleEmitter(pEmitter->mEmitter);

        if (pSingleEmitter != nullptr) {
            pSingleEmitter->mEmitter = nullptr;
        }

        mEmitterManager->forceDeleteEmitter(pEmitter->mEmitter);
        pEmitter->invalidate();
    }
}

void EffectSystem::forceDeleteSingleEmitter(SingleEmitter* pSingleEmitter) const {
    ParticleEmitter* pEmitter = pSingleEmitter->mEmitter;

    if (pSingleEmitter->mEmitter != nullptr) {
        pSingleEmitter->mEmitter = nullptr;

        mEmitterManager->forceDeleteEmitter(pEmitter->mEmitter);
        pEmitter->invalidate();
    }
}

void EffectSystem::createSingleEmitter(SingleEmitter* pSingleEmitter, MultiEmitterCallBackBase* pCallBack,
                                       MultiEmitterParticleCallBack* pParticleCallBack) {
    if (pSingleEmitter->isValid()) {
        if (!pSingleEmitter->isOneTime()) {
            return;
        }

        pSingleEmitter->unlink();
    }

    ParticleEmitter* pEmitter = createEmitter(pSingleEmitter->_4, pSingleEmitter->mGroupId, 0);

    if (pEmitter != nullptr) {
        pSingleEmitter->link(pEmitter);

        // todo -- probably some internal stuff that happens here...look into this
        if (pCallBack != nullptr) {
            pEmitter->mEmitter->mpEmtrCallBack = (JPAEmitterCallBack*)pCallBack;
        }

        if (pParticleCallBack != nullptr) {
            pEmitter->mEmitter->mpPtclCallBack = (JPAParticleCallBack*)pParticleCallBack;
        }
    }
}

namespace MR {
    EffectSystem* getEffectSystem() {
        return getSceneObj< EffectSystem >(SceneObj_EffectSystem);
    }
};  // namespace MR
