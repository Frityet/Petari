#include "compat/MetrowerksStdCompat.hpp"
#include "scene/OriginalSceneSupport.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObjHolder.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/System/DrawSyncManager.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/GameSceneBinding.hpp"
#include "Game/NameObj/NameObjListExecutor.hpp"
#include "scene/SceneLifetimeBinding.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "scene/SceneInitializationState.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <memory>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>

namespace smgpc::scene {
namespace {
class OriginalSceneSupport;
extern std::unique_ptr<OriginalSceneSupport> active_support;

class OriginalSceneSupport final {
public:
    OriginalSceneSupport(Scene& scene, NameObjHolder& names, JKRHeap& heap)
        : _scene(&scene), _names(&names), _domain(compat::JkrAllocationDomain::retain_heap(heap)), _objects(scene.mSceneObjHolder), _executor(scene.mListExecutor) {
        auto* game = dynamic_cast<GameScene*>(&scene);
        _scheduler_binding = std::make_unique<runtime::SceneSchedulerBinding>(_scheduler);
        _allocation_binding = std::make_unique<runtime::SceneSchedulerAllocationBinding>(_scheduler, _domain);
        _objects->initializeNative(_domain);
        try {
        _executor->bindNativeExecution(_scheduler, _domain);
        _lifetime = std::make_unique<SceneLifetimeBinding>(scene, [](void* context) noexcept {
            if (active_support.get() != context) std::terminate();
            active_support.reset();
        }, this);
        if (game) _game = std::make_unique<GameSceneBinding>(*game);
        } catch (...) {
            _executor->prepareNativeRetirement();
            _objects->retireNativeResources();
            _executor->unbindNativeExecution();
            throw;
        }
    }

    ~OriginalSceneSupport() {
        prepare_retirement();
        // GameScene's child binding runs first at the original Scene base
        // boundary. Other scenes still leave Game arrays to their real heap;
        // release their native sidecars before that storage can be recycled.
        _game.reset();
        _executor->prepareNativeRetirement();
        const auto holder_objects = _names->snapshotNativeObjects();
        auto objects = compat::snapshot_name_obj_runtime_objects();
        for (auto it = objects.rbegin(); it != objects.rend(); ++it) {
            auto* object = *it;
            if (!compat::has_name_obj_runtime_state(object) || _objects->ownsNativeObject(object)) continue;
            bool belongs_to_heap = false;
            for (auto* heap = JKRHeap::findFromRoot(object); heap; heap = heap->getParent()) {
                if (heap == &_domain->heap()) {
                    belongs_to_heap = true;
                    break;
                }
            }
            // The original startup changes NameObjRegister's holder only after
            // stationed initialization. Earlier scene objects can therefore be
            // listed in the process holder while allocated in this scene heap.
            if (!belongs_to_heap && std::find(holder_objects.begin(), holder_objects.end(), object) == holder_objects.end()) continue;
            if (auto* layout = dynamic_cast<LayoutActor*>(object)) layout->releaseNativeResources();
            if (auto* actor = dynamic_cast<LiveActor*>(object)) compat::release_actor_runtime_state(actor);
            object->detachNativeHolder();
            compat::release_name_obj_runtime_state(object);
        }
        _objects->retireNativeResources();
        _executor->unbindNativeExecution();
        _allocation_binding.reset();
        _scheduler_binding.reset();
    }

    void prepare_retirement() noexcept {
        DrawSyncManager::retireNativeCallbacks(_domain->heap());
        _objects->prepareNativeRetirement();
    }
    Scene& scene() const noexcept { return *_scene; }
    void initialize_effects(unsigned particles, unsigned emitters) {
        if (_objects->isExist(SceneObj_EffectSystem))
            throw std::logic_error("Scene effect system already initialized");
        const compat::JkrAllocationScope game(_domain);
        auto* effects = static_cast<EffectSystem*>(_objects->create(SceneObj_EffectSystem));
        effects->entry(MR::getParticleResourceHolder(), particles, emitters);
    }
    void begin_frame() { if (_executor->nativeInitialized()) _scheduler.begin_frame(); }

private:
    Scene* _scene;
    NameObjHolder* _names;
    std::shared_ptr<compat::JkrAllocationDomain> _domain;
    runtime::SceneScheduler _scheduler;
    std::unique_ptr<runtime::SceneSchedulerBinding> _scheduler_binding;
    SceneInitializationBinding _initialization_state;
    std::unique_ptr<runtime::SceneSchedulerAllocationBinding> _allocation_binding;
    SceneObjHolder* _objects;
    NameObjListExecutor* _executor;
    std::unique_ptr<SceneLifetimeBinding> _lifetime;
    std::unique_ptr<GameSceneBinding> _game;
};
std::unique_ptr<OriginalSceneSupport> active_support;
}

void bind_original_scene_support(Scene& scene) {
    auto* system = SingletonHolder<GameSystem>::get();
    if (!system || !system->mSceneController || system->mSceneController->mScene != &scene) return;
    const compat::JkrHostAllocationScope host;
    if (active_support || !scene.mListExecutor || !scene.mSceneObjHolder || !system->mSceneController->mObjHolder)
        throw std::logic_error("Original scene support requires the controller's initialized scene and actual holders");
    auto* heap = JKRHeap::findFromRoot(scene.mSceneObjHolder);
    if (!heap) throw std::logic_error("Original scene support requires its actual Game heap");
    active_support = std::make_unique<OriginalSceneSupport>(scene, *system->mSceneController->mObjHolder, *heap);
}

void prepare_original_scene_support_retirement(Scene& scene) noexcept {
    if (active_support && &active_support->scene() == &scene) active_support->prepare_retirement();
}

void initialize_original_scene_effects(unsigned particles, unsigned emitters) {
    if (!active_support) throw std::logic_error("Original scene effects require the actual controller's scene support");
    active_support->initialize_effects(particles, emitters);
}

void begin_original_scene_frame() {
    if (active_support) active_support->begin_frame();
}
}
