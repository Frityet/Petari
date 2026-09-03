#include "Game/Effect/ParticleCalcExecutor.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "Game/Effect/ParticleEmitterHolder.hpp"
#include "Game/NameObj/NameObjAdaptor.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "JSystem/JParticle/JPAEmitterManager.hpp"

ParticleCalcExecutor::ParticleCalcExecutor(const EffectSystem* pHost, bool createAdaptor)
    : mHost(pHost), _4(nullptr), _8(nullptr), _C(nullptr), _10(nullptr), _14(true), _15(false) {
    if (createAdaptor) {
        initMovementAdaptor();
    }
}

void ParticleCalcExecutor::movementNormal() {
    if (_14 && _15) {
        mHost->mEmitterHolder->update(false);
        mHost->mEmitterManager->calc(0);
        mHost->mEmitterManager->calc(1);
        mHost->mEmitterManager->calc(2);
        mHost->mEmitterManager->calc(3);
        mHost->mEmitterManager->calc(4);
        mHost->mEmitterManager->calc(5);
        mHost->mEmitterManager->calc(6);
        mHost->mEmitterManager->calc(7);
        mHost->mEmitterManager->calc(8);
    }
    _15 = false;
}

void ParticleCalcExecutor::movementIgnorePause3D() {
    if (_15) {
        mHost->mEmitterHolder->update(true);
        mHost->mEmitterManager->calc(1);
    }
}

void ParticleCalcExecutor::movementIgnorePause2D() {
    if (_15) {
        mHost->mEmitterHolder->update(true);
        mHost->mEmitterManager->calc(7);
    }
}

void ParticleCalcExecutor::movementCheckUpdate() {
    _15 = true;
}

void ParticleCalcExecutor::requestMovementOnPauseIgnore() {
    MR::requestMovementOn(_4);
    MR::requestMovementOn(_8);
    MR::requestMovementOn(_C);
}

void ParticleCalcExecutor::initMovementAdaptor() {
    _4 = new NameObjAdaptor("パーティクル");
    {
        const MR::FunctorBase& functor = MR::Functor(this, &ParticleCalcExecutor::movementNormal);
        NameObjAdaptor* pAdaptor = _4;
        pAdaptor->connectToCalcAnim(functor);
        MR::connectToScene(pAdaptor, -1, 19, -1, -1);
    }
    _8 = new NameObjAdaptor("ポーズ無効3Dパーティクル");
    {
        const MR::FunctorBase& functor = MR::Functor(this, &ParticleCalcExecutor::movementIgnorePause3D);
        NameObjAdaptor* pAdaptor = _8;
        pAdaptor->connectToCalcAnim(functor);
        MR::connectToScene(pAdaptor, -1, 20, -1, -1);
    }
    _C = new NameObjAdaptor("ポーズ無効2Dパーティクル");
    {
        const MR::FunctorBase& functor = MR::Functor(this, &ParticleCalcExecutor::movementIgnorePause2D);
        NameObjAdaptor* pAdaptor = _C;
        pAdaptor->connectToCalcAnim(functor);
        MR::connectToScene(pAdaptor, -1, 20, -1, -1);
    }
    _10 = new NameObjAdaptor("更新チェック");
    {
        const MR::FunctorBase& functor = MR::Functor(this, &ParticleCalcExecutor::movementCheckUpdate);
        NameObjAdaptor* pAdaptor = _10;
        pAdaptor->connectToMovement(functor);
        MR::connectToScene(pAdaptor, 20, -1, -1, -1);
    }
}
