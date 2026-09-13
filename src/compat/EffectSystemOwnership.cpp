#include <aurora/allocation.hpp>
#include "resource/TextEncoding.hpp"
#include <aurora/exception.hpp>
#include "compat/EffectSystemOwnership.hpp"

#include "Game/Effect/AutoEffectGroup.hpp"
#include "Game/Effect/AutoEffectGroupHolder.hpp"
#include "Game/Effect/AutoEffectInfo.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "Game/Effect/MultiEmitter.hpp"
#include "Game/Effect/MultiEmitterCallBack.hpp"
#include "Game/Effect/MultiEmitterParticleCallBack.hpp"
#include "Game/Effect/ParticleCalcExecutor.hpp"
#include "Game/Effect/ParticleDrawExecutor.hpp"
#include "Game/Effect/ParticleEmitterHolder.hpp"
#include "Game/Effect/SingleEmitter.hpp"
#include "Game/Effect/SyncBckEffectChecker.hpp"
#include "Game/Effect/SyncBckEffectInfo.hpp"
#include "Game/LiveActor/EffectKeeper.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/PaneEffectKeeper.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "JSystem/JParticle/JPAEmitterManager.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <map>
#include <stdexcept>

namespace {
    // These stable Game names outlive every owner that borrows their bytes.
    std::string encode_owner_name(std::string_view name) {
        aurora::allocation::HostAllocationScope host;
        return smgpc::resource::encode_cp932(name);
    }

    const std::string cEffectSystemName = encode_owner_name("エフェクトシステム");
}  // namespace

namespace smgpc::compat {
    namespace {
        void destroy_multi_emitter(MultiEmitter* multi) noexcept {
            if (auto* sync = multi->_24) {
                for (auto* resource : sync->mBckResources)
                    delete resource;
                delete sync;
            }
            delete multi->mParticleCallBack;
            delete multi->mCallBack;
            delete multi;
        }

        template <class EmitterTable>
        void retire_emitter_callbacks(const EffectSystem& system, const EmitterTable& emitters) noexcept {
            for (auto& slot : system.mEmitterHolder->mEmitters) {
                if (!slot.mEmitter)
                    continue;
                const auto token = slot.mEmitter->mLastNonzeroUserWork;
                bool belongs = false;
                for (auto* multi : emitters) {
                    for (const auto& single : multi->mEmitters) {
                        if (token == reinterpret_cast<uintptr_t>(&single))
                            belongs = true;
                    }
                }
                // One-shot replacement unlinks older instances whose callbacks
                // still borrow this keeper. Retire those instances as well.
                if (belongs)
                    system.forceDeleteEmitter(&slot);
            }
        }

        void destroy_keeper(EffectKeeper* keeper) noexcept {
            if (!keeper)
                return;
            // Child-emitter links are borrowed links to entries of this table.
            for (auto* multi : keeper->_C)
                destroy_multi_emitter(multi);
            delete keeper->_20;
            if (auto* table = keeper->_18) {
                delete[] table->mHashCodes;
                delete[] table->_8;
                delete[] table->_C;
                delete[] table->_10;
                delete table;
            }
            delete keeper;
        }

        struct ActorEffectOwner final {
            EffectSystemOwnership *scene;
            LiveActor *actor;
            EffectKeeper *keeper = nullptr;

            ~ActorEffectOwner() {
                if (!keeper)
                    return;
                retire_emitter_callbacks(scene->system(), keeper->_C);
                actor->mEffectKeeper = nullptr;
                destroy_keeper(keeper);
            }
        };

        auto &actor_owners() {
            static std::map<const LiveActor *, std::unique_ptr<ActorEffectOwner>> owners;
            return owners;
        }

        struct LayoutEffectOwner final {
            std::shared_ptr<JkrAllocationDomain> domain;
            const EffectSystem* system = nullptr;
            LayoutActor* actor = nullptr;
            PaneEffectKeeper* keeper = nullptr;

            ~LayoutEffectOwner() {
                if (!keeper)
                    return;
                retire_emitter_callbacks(*system, keeper->mEmitters);
                actor->mEffectKeeper = nullptr;
                for (auto* multi : keeper->mEmitters)
                    destroy_multi_emitter(multi);
                delete keeper;
            }
        };

        auto& layout_owners() {
            static std::map<const LayoutActor*, std::unique_ptr<LayoutEffectOwner>> owners;
            return owners;
        }
    }  // namespace

    struct EffectSystemOwnership::Storage final {
        std::shared_ptr<runtime::ParticleResourceOwnership> resources;
        ParticleResourceHolder* catalog = nullptr;
        std::shared_ptr<JkrAllocationDomain> domain;
        EffectSystem *system = nullptr;
        ParticleDrawExecutor *draw = nullptr;
        ParticleCalcExecutor *calc = nullptr;
        AutoEffectGroupHolder *groups = nullptr;
        JPAEmitterManager *manager = nullptr;
        ParticleEmitterHolder *emitters = nullptr;
        bool entered = false;
        bool retired = false;
    };

    EffectSystemOwnership::EffectSystemOwnership(std::size_t byte_budget) {
        JkrHostAllocationScope host;
        _storage = std::make_unique<Storage>();
        if (auto* system = SingletonHolder<GameSystem>::get()) {
            if (!system->mObjHolder || !system->mObjHolder->mParticleResHolder)
                aurora::throw_host_exception<std::logic_error>("Original process particle resources have not been constructed");
            _storage->catalog = system->mObjHolder->mParticleResHolder;
            _storage->domain = scene::current_scene_allocation_domain();
            if (!_storage->domain)
                aurora::throw_host_exception<std::logic_error>("Original effects require their actual scene heap");
        } else {
            auto *runtime = runtime::RuntimeContext::try_instance();
            if (!runtime)
                aurora::throw_host_exception<std::logic_error>("EffectSystem requires the active resource runtime");
            _storage->resources = runtime->retain_particle_resources();
            _storage->catalog = &_storage->resources->holder();
            _storage->domain = JkrAllocationDomain::create(runtime->host_heaps(), byte_budget);
        }
    }

    EffectSystem *EffectSystemOwnership::construct() {
        if (_storage->system)
            aurora::throw_host_exception<std::logic_error>("EffectSystem already constructed");
        JkrAllocationScope heap(_storage->domain);
        auto *system = new EffectSystem(cEffectSystemName.c_str(), true);
        _storage->system = system;
        _storage->draw = system->mDrawExec;
        _storage->calc = system->mCalcExec;
        _storage->groups = system->mGroupHolder;
        return system;
    }

    void EffectSystemOwnership::entry(std::uint32_t particles, std::uint32_t emitters) {
        if (!_storage->system || _storage->entered || particles == 0 || emitters == 0)
            aurora::throw_host_exception<std::logic_error>("EffectSystem entry requires a fresh system and nonzero pools");
        JkrAllocationScope heap(_storage->domain);
        _storage->system->entry(_storage->catalog, particles, emitters);
        _storage->manager = _storage->system->mEmitterManager;
        _storage->emitters = _storage->system->mEmitterHolder;
        _storage->entered = true;
    }

    EffectSystem &EffectSystemOwnership::system() const {
        if (!_storage->entered || _storage->retired)
            aurora::throw_host_exception<std::logic_error>("EffectSystem emitter pool is unavailable");
        return *_storage->system;
    }

    const std::shared_ptr<JkrAllocationDomain> &EffectSystemOwnership::allocation_domain() const noexcept {
        return _storage->domain;
    }

    void EffectSystemOwnership::retire() noexcept {
        if (_storage->retired)
            return;
        for (auto it = actor_owners().begin(); it != actor_owners().end();) {
            if (it->second->scene == this)
                it = actor_owners().erase(it);
            else
                ++it;
        }
        for (auto it = layout_owners().begin(); it != layout_owners().end();) {
            if (it->second->system == _storage->system)
                it = layout_owners().erase(it);
            else
                ++it;
        }
        if (_storage->entered)
            _storage->emitters->forceDeleteAllEmitters();
        _storage->retired = true;
    }

    EffectSystemOwnership::~EffectSystemOwnership() {
        retire();
        // The binding destroys every NameObj adaptor first, releasing functors
        // that borrow these executors. SDK raw arrays live in the retained heap.
        delete _storage->draw;
        delete _storage->calc;
        if (auto *groups = _storage->groups) {
            for (auto *group : groups->mGroups) {
                for (auto *info : group->mInfos)
                    delete info;
                delete group;
            }
            delete groups;
        }
        delete _storage->emitters;
        delete _storage->manager;
    }

    void initialize_actor_effect_keeper(LiveActor *actor, int capacity, const char *name, bool sort) {
        JkrHostAllocationScope host;
        auto *scene = scene::current_effect_system_ownership();
        if (!scene)
            aurora::throw_host_exception<std::logic_error>("LiveActor effect registration requires an initialized scene EffectSystem");
        (void)scene->system();
        if (!actor || actor->mEffectKeeper || actor_owners().contains(actor) || capacity < 0)
            aurora::throw_host_exception<std::logic_error>("LiveActor effect keeper requires a fresh actor and nonnegative capacity");
        auto owner = std::make_unique<ActorEffectOwner>();
        owner->scene = scene;
        owner->actor = actor;
        {
            JkrAllocationScope heap(scene->allocation_domain());
            owner->keeper = new EffectKeeper(actor->getName(), MR::getModelResourceHolder(actor), capacity, name);
            actor->mEffectKeeper = owner->keeper;
            if (sort)
                owner->keeper->enableSort();
            owner->keeper->init(actor);
            if (actor->mBinder)
                owner->keeper->setBinder(actor->mBinder);
        }
        actor_owners().emplace(actor, std::move(owner));
    }

    void release_actor_effect_keeper(const LiveActor *actor) noexcept {
        actor_owners().erase(actor);
    }

    void initialize_layout_effect_keeper(LayoutActor* actor, int capacity, const char* name, const EffectSystem* effect_system) {
        std::unique_ptr<LayoutEffectOwner> owner;
        {
            JkrHostAllocationScope host;
            if (!actor || !actor->mLayoutManager || actor->mEffectKeeper || layout_owners().contains(actor) || capacity < 0)
                aurora::throw_host_exception<std::logic_error>("Layout effects require a fresh keeper and an initialized layout");
            owner = std::make_unique<LayoutEffectOwner>();
            owner->domain = current_jkr_allocation_domain();
            if (!owner->domain)
                aurora::throw_host_exception<std::logic_error>("Layout effects require the original caller's Game heap");
            owner->actor = actor;
            owner->system = effect_system ? effect_system : MR::getEffectSystem();
        }
        // Preserve the actual current Game heap. The retained domain may be an
        // ancestor of that heap; selecting it here would change retail routing.
        actor->mEffectKeeper = new PaneEffectKeeper(actor, actor->mLayoutManager, capacity, name);
        owner->keeper = actor->mEffectKeeper;
        owner->keeper->init(actor, effect_system);
        JkrHostAllocationScope host;
        layout_owners().emplace(actor, std::move(owner));
    }

    void release_layout_effect_keeper(const LayoutActor* actor) noexcept {
        JkrHostAllocationScope host;
        layout_owners().erase(actor);
    }
}  // namespace smgpc::compat
