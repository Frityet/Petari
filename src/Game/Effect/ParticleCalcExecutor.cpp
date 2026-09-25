#include "Game/LiveActor/LiveActor.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include <utility>
#include "resource/TextEncoding.hpp"
#include "Game/Effect/ParticleCalcExecutor.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "Game/Effect/ParticleEmitterHolder.hpp"
#include "Game/NameObj/NameObjAdaptor.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "JSystem/JParticle/JPAEmitterManager.hpp"

ParticleCalcExecutor::ParticleCalcExecutor(const EffectSystem* pHost, bool createAdaptor)
    : mHost(pHost), _4(nullptr), _8(nullptr), _C(nullptr), _10(nullptr), _14(true), _15(false) {
    try {
        if (createAdaptor) {
            initMovementAdaptor();
        }
    } catch (...) {
        delete std::exchange(_10, nullptr);
        delete std::exchange(_C, nullptr);
        delete std::exchange(_8, nullptr);
        delete std::exchange(_4, nullptr);
        throw;
    }
}

ParticleCalcExecutor::~ParticleCalcExecutor() {
    delete _10;
    delete _C;
    delete _8;
    delete _4;
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
    _4 = new NameObjAdaptor(CP932("パーティクル"));
    smgpc::compat::claim_name_obj_runtime_ownership(_4, this);
    {
        const MR::FunctorBase& functor = MR::Functor(this, &ParticleCalcExecutor::movementNormal);
        NameObjAdaptor* pAdaptor = _4;
        pAdaptor->connectToCalcAnim(functor);
        MR::connectToScene(pAdaptor, -1, 19, -1, -1);
    }
    _8 = new NameObjAdaptor(CP932("ポーズ無効3Dパーティクル"));
    smgpc::compat::claim_name_obj_runtime_ownership(_8, this);
    {
        const MR::FunctorBase& functor = MR::Functor(this, &ParticleCalcExecutor::movementIgnorePause3D);
        NameObjAdaptor* pAdaptor = _8;
        pAdaptor->connectToCalcAnim(functor);
        MR::connectToScene(pAdaptor, -1, 20, -1, -1);
    }
    _C = new NameObjAdaptor(CP932("ポーズ無効2Dパーティクル"));
    smgpc::compat::claim_name_obj_runtime_ownership(_C, this);
    {
        const MR::FunctorBase& functor = MR::Functor(this, &ParticleCalcExecutor::movementIgnorePause2D);
        NameObjAdaptor* pAdaptor = _C;
        pAdaptor->connectToCalcAnim(functor);
        MR::connectToScene(pAdaptor, -1, 20, -1, -1);
    }
    _10 = new NameObjAdaptor(CP932("更新チェック"));
    smgpc::compat::claim_name_obj_runtime_ownership(_10, this);
    {
        const MR::FunctorBase& functor = MR::Functor(this, &ParticleCalcExecutor::movementCheckUpdate);
        NameObjAdaptor* pAdaptor = _10;
        pAdaptor->connectToMovement(functor);
        MR::connectToScene(pAdaptor, 20, -1, -1, -1);
    }
}
