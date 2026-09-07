#include "compat/ModelManagerOwner.hpp"

#include "Game/LiveActor/ModelManager.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "Game/Animation/XanimeCore.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/J3dCommandScope.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "Game/Util/MutexHolder.hpp"
#include <exception>
#include <vector>
#include <stdexcept>

namespace smgpc::compat {
    namespace {
        class LoadStateScope final {
        public:
            LoadStateScope() : _system(j3dSys), _thread(OSGetCurrentThread()), _exceptions(std::uncaught_exceptions()) {
                _count = mutex().thread == _thread ? mutex().count : 0;
            }
            ~LoadStateScope() {
                j3dSys = _system;
                if (std::uncaught_exceptions() > _exceptions)
                    while (mutex().thread == _thread && mutex().count > _count) OSUnlockMutex(&mutex());
            }
        private:
            static OSMutex& mutex() { return MR::MutexHolder<0>::sMutex; }
            J3DSys _system;
            OSThread* _thread;
            int _exceptions;
            int _count;
        };
    }
    struct ModelManagerOwner::Storage {
        std::shared_ptr<JkrAllocationDomain> domain;
        std::shared_ptr<const ResourceArchiveOwner> model_resources;
        std::shared_ptr<const ResourceArchiveOwner> animation_resources;
        std::unique_ptr<ModelManager> manager;
        std::vector<std::shared_ptr<void>> lifetime_dependencies;
        J3DModel* created_model = nullptr;
        XanimePlayer* created_player = nullptr;
        XanimeCore* created_core = nullptr;
        ~Storage() {
            // Original model/player destructors have no child-freeing work;
            // their raw allocations retire with the retained original scene
            // heap. The holder owns the shared material-animation buffer.
            // The manager may point to a later authored player. Restore its
            // original captured player before releasing attached animator graphs.
            if (manager) manager->mXanimePlayer = created_player;
            lifetime_dependencies.clear();
            delete created_core;
            delete created_player;
            delete created_model;
        }
    };
    ModelManagerOwner::ModelManagerOwner(ResourceHolderService& service, std::shared_ptr<JkrAllocationDomain> domain,
                                         const char* model, const char* animation, bool create_dl) {
        JkrHostAllocationScope host;
        if (!domain || domain != service.allocation_domain())
            throw std::invalid_argument("Models and their holder-owned material buffers require the same retained scene heap");
        if (ResourceHolderService::active() != &service)
            throw std::invalid_argument("ModelManager requires the active original resource service");
        _storage = std::make_unique<Storage>();
        auto& state = *_storage;
        state.domain = std::move(domain);
        JkrAllocationScope heap(state.domain);
        J3dCommandScope commands;
        LoadStateScope restore;
        state.manager = std::make_unique<ModelManager>();
        state.manager->init(model, animation, create_dl);
        state.created_model = state.manager->getJ3DModel();
        state.created_player = state.manager->mXanimePlayer;
        state.created_core = state.created_player ? state.created_player->mCore : nullptr;
        state.model_resources = service.retain(*state.manager->getModelResourceHolder());
        state.animation_resources = service.retain(*state.manager->getResourceHolder());
    }
    void ModelManagerOwner::retain_lifetime_dependency(std::shared_ptr<void> dependency) {
        JkrHostAllocationScope host;
        if (!dependency) throw std::invalid_argument("Model lifetime dependency is empty");
        _storage->lifetime_dependencies.push_back(std::move(dependency));
    }
    ModelManagerOwner::~ModelManagerOwner() {
        JkrHostAllocationScope host;
        _storage.reset();
    }
    ModelManager& ModelManagerOwner::manager() const noexcept { return *_storage->manager; }
    const std::shared_ptr<JkrAllocationDomain>& ModelManagerOwner::allocation_domain() const noexcept { return _storage->domain; }
}
