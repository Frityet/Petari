#include "Game/Scene/MultiSceneEffectKeeper.hpp"
#include "Game/Effect/EffectSystemUtil.hpp"
#include "Game/Effect/MultiEmitter.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include <algorithm>

MultiSceneEffectKeeper::MultiSceneEffectKeeper(const char* pName, ModelManager* pModelManager, int emitterNum, const char* pResName)
    : mEmitter(), mResName(pResName) {
    if (pResName == nullptr) {
        mResName = pModelManager->getModelResourceHolder()->mModelResTable->getResName(static_cast< u32 >(0));
    }

    mEmitter.init(emitterNum + MR::getParticleResourceHolder()->getAutoEffectNum(mResName));
}

void MultiSceneEffectKeeper::init(const MultiSceneActor* pActor, const EffectSystem* pSystem) {
    if (mResName != nullptr) {
        MR::Effect::registerAutoEffectInfoGroup(this, pSystem, pActor, mResName);
    }
}

void MultiSceneEffectKeeper::add(const char* pName, const TVec3f* pTrans, const TVec3f* pRot, const TVec3f* pScale, const char* pEmitterName) {
    MultiEmitter* pEmitter = new MultiEmitter(pName, pTrans, pRot, pScale, TVec3f(0.0f, 0.0f, 0.0f));
    registerEmitter(pEmitter, pEmitterName);
}

void MultiSceneEffectKeeper::add(const char* pName, MtxPtr pMtx, const char* pEmitterName) {
    MultiEmitter* pEmitter = new MultiEmitter(pName, pMtx, TVec3f(0.0f, 0.0f, 0.0f));
    registerEmitter(pEmitter, pEmitterName);
}

MultiEmitter* MultiSceneEffectKeeper::create(const char* pName, EffectSystem* pSystem) {
    MultiEmitter* pEmitter = find(pName);
    pEmitter->create(pSystem);
    return pEmitter;
}

void MultiSceneEffectKeeper::deleteAll() {
    std::for_each(mEmitter.begin(), mEmitter.end(), std::mem_func(&MultiEmitter::deleteEmitter));
}

void MultiSceneEffectKeeper::forceDeleteAll(EffectSystem* pSystem) {
    std::for_each(mEmitter.begin(), mEmitter.end(), std::bind2nd(std::mem_func(&MultiEmitter::forceDelete), pSystem));
}

void MultiSceneEffectKeeper::clear() {
    std::for_each(mEmitter.begin(), mEmitter.end(), std::mem_func(&MultiEmitter::playCalcAndDeleteForeverEmitter));
}

MultiEmitter* MultiSceneEffectKeeper::get(const char* pName) const {
    return find(pName);
}

MultiEmitter* MultiSceneEffectKeeper::find(const char* pName) const {
    if (mEmitter.size() == 0) {
        return nullptr;
    }

    u16 hash = MR::getHashCode(pName);
    MultiEmitter* const* pEmitter = std::find_if(mEmitter.begin(), mEmitter.end(), std::bind2nd(std::mem_func(&MultiEmitter::isEqualName), hash));
    if (pEmitter != mEmitter.end()) {
        return *pEmitter;
    }
    return nullptr;
}

void MultiSceneEffectKeeper::registerEmitter(MultiEmitter* pEmitter, const char* pName) {
    if (pName != nullptr) {
        pEmitter->setName(pName);
    }
    mEmitter.push_back(pEmitter);
}
