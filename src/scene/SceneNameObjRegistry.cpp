#include "scene/SceneNameObjRegistry.hpp"

#include "Game/NameObj/NameObjHolder.hpp"
#include "Game/NameObj/NameObjRegister.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <algorithm>
#include <aurora/exception.hpp>
#include <stdexcept>

namespace smgpc::scene {
    SceneNameObjRegistry *SceneNameObjRegistry::sBindings = nullptr;

    SceneNameObjRegistry::SceneNameObjRegistry(std::shared_ptr<compat::JkrAllocationDomain> domain)
        : _domain(std::move(domain)) {
        if (!_domain)
            aurora::throw_host_exception<std::invalid_argument>("Scene NameObjHolder requires its actual Game allocation domain");
        {
            const compat::JkrAllocationScope game(_domain);
            // Original GameSystemSceneController construction capacity.
            _owned_holder = std::make_unique<NameObjHolder>(0x1300);
            _holder = _owned_holder.get();
        }
        _next = sBindings;
        sBindings = this;
    }

    SceneNameObjRegistry::SceneNameObjRegistry(NameObjHolder& holder, std::shared_ptr<compat::JkrAllocationDomain> domain)
        : _domain(std::move(domain)), _holder(&holder) {
        if (!_domain)
            aurora::throw_host_exception<std::invalid_argument>("Borrowing the original NameObjHolder requires its actual heap owner");
        _next = sBindings;
        sBindings = this;
    }

    SceneNameObjRegistry::~SceneNameObjRegistry() {
        for (auto **link = &sBindings; *link; link = &(*link)->_next) {
            if (*link == this) {
                *link = _next;
                break;
            }
        }
        _owned_holder.reset();
    }

    NameObjHolder &SceneNameObjRegistry::holder() const noexcept {
        return *_holder;
    }

    std::vector<NameObj *> SceneNameObjRegistry::snapshot() const {
        return snapshot_holder(*_holder);
    }

    std::vector<NameObj*> SceneNameObjRegistry::snapshot_holder(const NameObjHolder& holder) {
        const compat::JkrHostAllocationScope host;
        return {holder.mObjArray1.begin(), holder.mObjArray1.end()};
    }

    void SceneNameObjRegistry::add(NameObj &object) { add_to_holder(*_holder, object); }

    void SceneNameObjRegistry::add_to_holder(NameObjHolder& holder, NameObj& object) {
        auto &objects = holder.mObjArray1;
        if (objects.size() >= objects.capacity())
            aurora::throw_host_exception<std::length_error>("Original scene NameObjHolder capacity exceeded");
        if (std::find(objects.begin(), objects.end(), &object) != objects.end())
            aurora::throw_host_exception<std::logic_error>("NameObj already belongs to the scene holder");
        holder.add(&object);
    }

    void SceneNameObjRegistry::remove(NameObj &object) noexcept { remove_from_holder(*_holder, object); }

    void SceneNameObjRegistry::remove_from_holder(NameObjHolder& holder, NameObj& object) noexcept {
        // Native rollback can destroy a single object before the scene heap
        // retires. Remove its identity from the original list and lookup cache
        // without changing the order or storage of surviving objects.
        auto erase = [&object](auto &objects) {
            for (auto *it = objects.begin(); it != objects.end();) {
                if (*it == &object) {
                    objects.erase(it);
                    *objects.end() = nullptr;
                } else {
                    ++it;
                }
            }
        };
        erase(holder.mObjArray1);
        erase(holder.mObjArray2);
    }

    SceneNameObjRegistry *current_scene_name_obj_registry() noexcept {
        return SceneNameObjRegistry::sBindings;
    }

    void register_scene_name_obj(NameObj &object) {
        if (auto* original = SingletonHolder<NameObjRegister>::get(); original && original->mHolder) {
            SceneNameObjRegistry::add_to_holder(*original->mHolder, object);
        } else if (auto *registry = current_scene_name_obj_registry()) {
            registry->add(object);
        }
    }

    void unregister_scene_name_obj(NameObj &object) noexcept {
        for (auto *registry = SceneNameObjRegistry::sBindings; registry; registry = registry->_next)
            registry->remove(object);
        // The original register switches between the process and scene holders.
        // Native partial destruction removes this identity from both real lists.
        auto remove = [&object](NameObjHolder* holder) {
            if (holder) SceneNameObjRegistry::remove_from_holder(*holder, object);
        };
        if (auto* original = SingletonHolder<NameObjRegister>::get()) remove(original->mHolder);
        if (auto* system = SingletonHolder<GameSystem>::get()) {
            if (system->mObjHolder) remove(system->mObjHolder->mObjHolder);
            if (system->mSceneController) remove(system->mSceneController->mObjHolder);
        }
    }
}  // namespace smgpc::scene
