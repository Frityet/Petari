#include "scene/SceneNameObjRegistry.hpp"

#include "Game/NameObj/NameObjHolder.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <algorithm>
#include <aurora/exception.hpp>
#include <stdexcept>

namespace smgpc::scene {
    thread_local SceneNameObjRegistry *SceneNameObjRegistry::sBindings = nullptr;

    SceneNameObjRegistry::SceneNameObjRegistry(std::shared_ptr<compat::JkrAllocationDomain> domain)
        : _domain(std::move(domain)) {
        if (!_domain)
            aurora::throw_host_exception<std::invalid_argument>("Scene NameObjHolder requires its actual Game allocation domain");
        {
            const compat::JkrAllocationScope game(_domain);
            // Original GameSystemSceneController construction capacity.
            _holder = std::make_unique<NameObjHolder>(0x1300);
        }
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
        _holder.reset();
    }

    NameObjHolder &SceneNameObjRegistry::holder() const noexcept {
        return *_holder;
    }

    std::vector<NameObj *> SceneNameObjRegistry::snapshot() const {
        const compat::JkrHostAllocationScope host;
        return {_holder->mObjArray1.begin(), _holder->mObjArray1.end()};
    }

    void SceneNameObjRegistry::add(NameObj &object) {
        auto &objects = _holder->mObjArray1;
        if (objects.size() >= objects.capacity())
            aurora::throw_host_exception<std::length_error>("Original scene NameObjHolder capacity exceeded");
        if (std::find(objects.begin(), objects.end(), &object) != objects.end())
            aurora::throw_host_exception<std::logic_error>("NameObj already belongs to the scene holder");
        _holder->add(&object);
    }

    void SceneNameObjRegistry::remove(NameObj &object) noexcept {
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
        erase(_holder->mObjArray1);
        erase(_holder->mObjArray2);
    }

    SceneNameObjRegistry *current_scene_name_obj_registry() noexcept {
        return SceneNameObjRegistry::sBindings;
    }

    void register_scene_name_obj(NameObj &object) {
        if (auto *registry = current_scene_name_obj_registry())
            registry->add(object);
    }

    void unregister_scene_name_obj(NameObj &object) noexcept {
        for (auto *registry = SceneNameObjRegistry::sBindings; registry; registry = registry->_next)
            registry->remove(object);
    }
}  // namespace smgpc::scene
