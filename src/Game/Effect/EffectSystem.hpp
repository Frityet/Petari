#pragma once

#include "Game/NameObj/NameObj.hpp"
#include <memory>

namespace smgpc::compat { class JkrAllocationDomain; }
class EffectKeeper;
class PaneEffectKeeper;
class MultiEmitter;

class AutoEffectGroupHolder;
class JPAEmitterManager;
class MultiEmitterCallBackBase;
class MultiEmitterParticleCallBack;
class ParticleCalcExecutor;
class ParticleDrawExecutor;
class ParticleEmitter;
class ParticleEmitterHolder;
class ParticleResourceHolder;
class SingleEmitter;

class EffectSystem : public NameObj {
public:
    EffectSystem(const char*, bool);
    ~EffectSystem() override;

    void retireNativeResources() noexcept;
    void registerNativeKeeper(EffectKeeper*);
    void registerNativeKeeper(PaneEffectKeeper*);
    void unregisterNativeKeeper(EffectKeeper*) noexcept;
    void unregisterNativeKeeper(PaneEffectKeeper*) noexcept;
    void retireNativeEmitter(const MultiEmitter&) const noexcept;
    std::shared_ptr<smgpc::compat::JkrAllocationDomain> nativeAllocationDomain() const noexcept;

    virtual void init(const JMapInfoIter& rIter);

    ParticleEmitter* createEmitter(u16, u8, u8);
    void forceDeleteEmitter(ParticleEmitter*) const;
    void forceDeleteSingleEmitter(SingleEmitter*) const;
    void createSingleEmitter(SingleEmitter*, MultiEmitterCallBackBase*, MultiEmitterParticleCallBack*);
    void entry(ParticleResourceHolder*, u32, u32);

    /* 0x0C */ JPAEmitterManager* mEmitterManager;
    /* 0x10 */ ParticleEmitterHolder* mEmitterHolder;
    /* 0x14 */ ParticleDrawExecutor* mDrawExec;
    /* 0x18 */ ParticleCalcExecutor* mCalcExec;
    /* 0x1C */ AutoEffectGroupHolder* mGroupHolder;
    /* 0x20 */ bool _20;

private:
    struct NativeState;
    std::unique_ptr<NativeState> mNativeState;
};

namespace MR {
    EffectSystem* getEffectSystem();
};  // namespace MR
