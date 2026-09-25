#include "Game/Effect/MultiEmitter.hpp"
#include "Game/LiveActor/EffectKeeper.hpp"
#include "Game/Screen/PaneEffectKeeper.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <algorithm>
#include <stdexcept>
#include <vector>
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

struct EffectSystem::NativeState {
    JKRHeap::Handle domain;
    std::shared_ptr<const void> particleResources;
    std::vector<EffectKeeper*> actorKeepers;
    std::vector<PaneEffectKeeper*> layoutKeepers;
    bool retired = false;
};

EffectSystem::EffectSystem(const char* pName, bool createAdaptor)
    : NameObj(pName), mEmitterManager(nullptr), mEmitterHolder(nullptr), mDrawExec(nullptr), mCalcExec(nullptr), mGroupHolder(nullptr), _20(true) {
    {
        const aurora::allocation::HostAllocationScope host;
        mNativeState = std::make_unique<NativeState>();
        mNativeState->domain = JKRHeap::retainCurrentNativeLifetime();
        if (!mNativeState->domain)
            aurora::throw_host_exception<std::logic_error>("EffectSystem requires its actual Game heap");
    }
    try {
        mGroupHolder = new AutoEffectGroupHolder();
        mDrawExec = new ParticleDrawExecutor(this, createAdaptor);
        mCalcExec = new ParticleCalcExecutor(this, createAdaptor);
    } catch (...) {
        delete mCalcExec;
        delete mDrawExec;
        delete mGroupHolder;
        throw;
    }
}

EffectSystem::~EffectSystem() {
    const aurora::allocation::HostAllocationScope host;
    retireNativeResources();
    // Executors own their original NameObj adaptors, whose functors borrow
    // these objects. Their callbacks retire before the particle storage.
    delete mCalcExec;
    delete mDrawExec;
    delete mGroupHolder;
    delete mEmitterHolder;
    delete mEmitterManager;
    mNativeState.reset();
}

void EffectSystem::retireNativeResources() noexcept {
    const aurora::allocation::HostAllocationScope host;
    if (mNativeState->retired)
        return;
    mNativeState->retired = true;
    _20 = false;
    // Each keeper unregisters itself and clears its actor's borrowed pointer.
    // This also handles actor storage that survives its scene EffectSystem.
    while (!mNativeState->actorKeepers.empty())
        delete mNativeState->actorKeepers.back();
    while (!mNativeState->layoutKeepers.empty())
        delete mNativeState->layoutKeepers.back();
    if (mEmitterHolder)
        mEmitterHolder->forceDeleteAllEmitters();
}

void EffectSystem::registerNativeKeeper(EffectKeeper* keeper) {
    const aurora::allocation::HostAllocationScope host;
    if (mNativeState->retired || !mEmitterManager || !mEmitterHolder)
        aurora::throw_host_exception<std::logic_error>("Effect keeper requires an initialized EffectSystem");
    mNativeState->actorKeepers.push_back(keeper);
}

void EffectSystem::registerNativeKeeper(PaneEffectKeeper* keeper) {
    const aurora::allocation::HostAllocationScope host;
    if (mNativeState->retired || !mEmitterManager || !mEmitterHolder)
        aurora::throw_host_exception<std::logic_error>("Pane effect keeper requires an initialized EffectSystem");
    mNativeState->layoutKeepers.push_back(keeper);
}

void EffectSystem::unregisterNativeKeeper(EffectKeeper* keeper) noexcept {
    const aurora::allocation::HostAllocationScope host;
    std::erase(mNativeState->actorKeepers, keeper);
}

void EffectSystem::unregisterNativeKeeper(PaneEffectKeeper* keeper) noexcept {
    const aurora::allocation::HostAllocationScope host;
    std::erase(mNativeState->layoutKeepers, keeper);
}

void EffectSystem::retireNativeEmitter(const MultiEmitter& multi) const noexcept {
    if (!mEmitterHolder)
        return;
    for (auto& slot : mEmitterHolder->mEmitters) {
        if (!slot.mEmitter)
            continue;
        // Replacement one-shots unlink the current SingleEmitter but retain
        // callbacks into its keeper. Retire every such historical instance.
        const auto token = slot.mEmitter->mLastNonzeroUserWork;
        for (const auto& single : multi.mEmitters) {
            if (token == reinterpret_cast<uintptr_t>(&single)) {
                forceDeleteEmitter(&slot);
                break;
            }
        }
    }
}

JKRHeap::Handle EffectSystem::nativeAllocationHeap() const noexcept {
    return mNativeState->domain;
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
    if (!pResource || mEmitterManager || mNativeState->retired || particleNum == 0 || emitterNum == 0)
        aurora::throw_host_exception<std::logic_error>("EffectSystem entry requires actual resources and fresh nonzero pools");
    mNativeState->particleResources = pResource->retainNativeResources();
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
