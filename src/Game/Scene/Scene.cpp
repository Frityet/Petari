#include "Game/Scene/Scene.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObjHolder.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/System/DrawSyncManager.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "runtime/SceneScheduler.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <aurora/exception.hpp>
#include <algorithm>
#include <memory>
#include <stdexcept>

Scene::Scene(const char* pName) : NerveExecutor(pName) {
    mListExecutor = nullptr;
    _C = 0;
    mSceneObjHolder = nullptr;
}

Scene::~Scene() {
    prepareNativeRetirement();
    retireNativeExecution();
    if (mSceneObjHolder != nullptr) {
        delete mSceneObjHolder;
    }

    if (mListExecutor != nullptr) {
        delete mListExecutor;
    }
}

void Scene::init() {
}

void Scene::start() {
}

void Scene::update() {
}

void Scene::draw() const {
}

void Scene::calcAnim() {
}

void Scene::initNameObjListExecutor() {
    auto exec = std::make_unique<SceneNameObjListExecutor>();
    exec->init();
    mListExecutor = exec.release();
}

void Scene::initSceneObjHolder() {
    mSceneObjHolder = new SceneObjHolder();
    initializeNativeExecution();
}

void Scene::initializeNativeExecution() {
    auto* system = SingletonHolder< GameSystem >::get();
    if (!system || !system->mSceneController || system->mSceneController->mScene != this) {
        return;
    }

    const smgpc::compat::JkrHostAllocationScope host;
    if (mNativeDomain || !mListExecutor || !mSceneObjHolder || !system->mSceneController->mObjHolder) {
        aurora::throw_host_exception< std::logic_error >("Scene execution requires the controller's initialized scene and actual holders");
    }
    auto* heap = JKRHeap::findFromRoot(mSceneObjHolder);
    if (!heap) {
        aurora::throw_host_exception< std::logic_error >("Scene execution requires its actual Game heap");
    }

    mNativeDomain = smgpc::compat::JkrAllocationDomain::retain_heap(*heap);
    mNativeNameObjHolder = system->mSceneController->mObjHolder;
    try {
        mNativeScheduler = std::make_unique< smgpc::runtime::SceneScheduler >();
        mNativeSchedulerBinding = std::make_unique< smgpc::runtime::SceneSchedulerBinding >(*mNativeScheduler);
        mNativeAllocationBinding =
            std::make_unique< smgpc::runtime::SceneSchedulerAllocationBinding >(*mNativeScheduler, mNativeDomain);
        mSceneObjHolder->initializeNative(mNativeDomain);
        mListExecutor->bindNativeExecution(*mNativeScheduler, mNativeDomain);
        if (auto* game = dynamic_cast< GameScene* >(this)) {
            game->initNativeSceneChildren();
        }
    } catch (...) {
        prepareNativeRetirement();
        retireNativeExecution();
        throw;
    }
}

void Scene::prepareNativeRetirement() noexcept {
    if (!mNativeDomain || mNativeRetirementPrepared) {
        return;
    }
    mNativeRetirementPrepared = true;
    DrawSyncManager::retireNativeCallbacks(mNativeDomain->heap());
    if (mSceneObjHolder) {
        mSceneObjHolder->prepareNativeRetirement();
    }
}

void Scene::retireNativeExecution() noexcept {
    if (!mNativeDomain) {
        return;
    }
    const smgpc::compat::JkrHostAllocationScope host;
    if (mListExecutor) {
        mListExecutor->prepareNativeRetirement();
    }
    if (mNativeNameObjHolder) {
        const auto holderObjects = mNativeNameObjHolder->snapshotNativeObjects();
        const auto objects = smgpc::compat::snapshot_name_obj_runtime_objects();
        for (auto it = objects.rbegin(); it != objects.rend(); ++it) {
            auto* object = *it;
            if (!smgpc::compat::has_name_obj_runtime_state(object) || mSceneObjHolder->ownsNativeObject(object)) {
                continue;
            }
            bool belongsToHeap = false;
            for (auto* heap = JKRHeap::findFromRoot(object); heap; heap = heap->getParent()) {
                if (heap == &mNativeDomain->heap()) {
                    belongsToHeap = true;
                    break;
                }
            }
            // Startup can register an object with the process holder before
            // switching NameObjRegister to the scene holder. Its real heap
            // still identifies the scene responsible for native retirement.
            if (!belongsToHeap && std::find(holderObjects.begin(), holderObjects.end(), object) == holderObjects.end()) {
                continue;
            }
            if (auto* layout = dynamic_cast< LayoutActor* >(object)) {
                layout->releaseNativeResources();
            }
            if (auto* actor = dynamic_cast< LiveActor* >(object)) {
                smgpc::compat::release_actor_runtime_state(actor);
            }
            object->detachNativeHolder();
            smgpc::compat::release_name_obj_runtime_state(object);
        }
        mNativeNameObjHolder = nullptr;
    }
    if (mSceneObjHolder) {
        mSceneObjHolder->retireNativeResources();
    }
    if (mListExecutor) {
        mListExecutor->unbindNativeExecution();
    }
    mNativeAllocationBinding.reset();
    mNativeSchedulerBinding.reset();
    mNativeScheduler.reset();
}

void Scene::beginNativeFrame() {
    if (mNativeScheduler && mListExecutor->nativeInitialized()) {
        mNativeScheduler->begin_frame();
    }
}

void Scene::initializeNativeEffects(u32 particles, u32 emitters) {
    if (!mNativeDomain || mNativeRetirementPrepared || !mSceneObjHolder) {
        aurora::throw_host_exception< std::logic_error >("Scene effects require the actual controller's initialized scene");
    }
    if (mSceneObjHolder->isExist(SceneObj_EffectSystem)) {
        aurora::throw_host_exception< std::logic_error >("Scene effect system already initialized");
    }
    const smgpc::compat::JkrAllocationScope game(mNativeDomain);
    auto* effects = static_cast< EffectSystem* >(mSceneObjHolder->create(SceneObj_EffectSystem));
    effects->entry(MR::getParticleResourceHolder(), particles, emitters);
}
