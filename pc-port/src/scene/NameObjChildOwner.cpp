#include "scene/NameObjChildOwner.hpp"

#include "scene/SceneObjHolderRuntime.hpp"

#include <algorithm>
#include <stdexcept>

namespace smgpc::scene {
    namespace {

        [[nodiscard]] bool is_unowned_construction_child(
            const NameObj *object, const void *) noexcept {
            return !current_scene_obj_holder_binding_owns(object) &&
                   !smgpc::compat::
                       name_obj_runtime_ownership_is_claimed(object);
        }

    }  // namespace

    NameObjChildOwner::~NameObjChildOwner() {
        clear();
    }

    void NameObjChildOwner::clear() noexcept {
        for (auto registration = _registrations.rbegin();
             registration != _registrations.rend(); ++registration) {
            if (registration->delegated_postpass) {
                smgpc::compat::
                    release_name_obj_runtime_postpass_delegation(
                        registration->object, this);
                registration->delegated_postpass = false;
            }
        }
        _registrations.clear();
        _next_registration_postpass_index = 0U;
        while (!_children.empty()) {
            _children.pop_back();
        }
    }

    bool NameObjChildOwner::empty() const noexcept {
        return _children.empty();
    }

    std::size_t NameObjChildOwner::size() const noexcept {
        return _children.size();
    }

    void NameObjChildOwner::adopt_root_registration_suffix(
        smgpc::compat::NameObjRuntimeRegistrationMarker marker,
        NameObj &root, const void *root_owner) {
        if (!_registrations.empty() || !_children.empty()) {
            throw std::logic_error(
                "A root registration graph cannot be adopted twice.");
        }
        if (root_owner == nullptr) {
            throw std::invalid_argument(
                "A root registration graph requires a storage owner.");
        }

        auto registrations =
            smgpc::compat::snapshot_name_obj_runtime_objects_since(marker);
        if (registrations.empty() || registrations.front() != &root ||
            std::ranges::count(registrations, &root) != 1 ||
            current_scene_obj_holder_binding_owns(&root) ||
            smgpc::compat::name_obj_runtime_ownership_is_claimed(&root)) {
            throw std::logic_error(
                "Root construction did not produce one leading unowned registration.");
        }

        const auto owned_descendant_count =
            static_cast<std::size_t>(std::ranges::count_if(
                registrations, [&root](const NameObj *object) {
                    return object != &root &&
                           !current_scene_obj_holder_binding_owns(object) &&
                           !smgpc::compat::
                               name_obj_runtime_ownership_is_claimed(object);
                }));
        _children.reserve(owned_descendant_count);
        _registrations.reserve(registrations.size());

        try {
            smgpc::compat::claim_name_obj_runtime_ownership(
                &root, root_owner);
            for (auto *object : registrations) {
                const auto independently_owned =
                    object == &root ||
                    current_scene_obj_holder_binding_owns(object) ||
                    smgpc::compat::
                        name_obj_runtime_ownership_is_claimed(object);
                if (independently_owned) {
                    smgpc::compat::delegate_name_obj_runtime_postpass(
                        object, this);
                }
            }
            for (auto *object : registrations) {
                const auto independently_owned =
                    object == &root ||
                    current_scene_obj_holder_binding_owns(object) ||
                    smgpc::compat::
                        name_obj_runtime_ownership_is_claimed(object);
                if (!independently_owned) {
                    smgpc::compat::claim_name_obj_runtime_ownership(
                        object, this);
                    _children.emplace_back(object);
                }
                _registrations.push_back(Registration{
                    .object = object,
                    .delegated_postpass = independently_owned,
                });
            }
        } catch (...) {
            for (auto *object : registrations) {
                smgpc::compat::
                    release_name_obj_runtime_postpass_delegation(
                        object, this);
            }
            rollback_unowned_registered_since(marker);
            throw;
        }
    }

    void NameObjChildOwner::init_registration_suffix_after_placement() {
        while (_next_registration_postpass_index <
               _registrations.size()) {
            auto &registration = _registrations[
                _next_registration_postpass_index];
            const auto release_delegation = [&] {
                if (!registration.delegated_postpass) {
                    return;
                }
                smgpc::compat::
                    release_name_obj_runtime_postpass_delegation(
                        registration.object, this);
                registration.delegated_postpass = false;
            };
            try {
                registration.object->initAfterPlacement();
                release_delegation();
                ++_next_registration_postpass_index;
            } catch (...) {
                release_delegation();
                throw;
            }
        }
    }

    void NameObjChildOwner::rollback_registration_suffix(
        smgpc::compat::NameObjRuntimeRegistrationMarker marker) noexcept {
        rollback_unowned_registered_since(marker);
    }

    void NameObjChildOwner::validate_candidate(const NameObj *child) const {
        if (child == nullptr) {
            throw std::invalid_argument("A scene cannot own a null NameObj child.");
        }
        if (std::ranges::any_of(_children, [child](const auto &owned) {
                return owned.get() == child;
            })) {
            throw std::logic_error("A scene cannot adopt the same NameObj child twice.");
        }
        if (current_scene_obj_holder_binding_owns(child) ||
            smgpc::compat::name_obj_runtime_ownership_is_claimed(child)) {
            throw std::logic_error(
                "A scene cannot adopt a NameObj child owned by another runtime boundary.");
        }
    }

    void NameObjChildOwner::adopt_registered_since(
        smgpc::compat::NameObjRuntimeRegistrationMarker marker) {
        auto children =
            smgpc::compat::snapshot_name_obj_runtime_objects_since(marker);
        std::erase_if(children, [](const NameObj *child) {
            return current_scene_obj_holder_binding_owns(child) ||
                   smgpc::compat::
                       name_obj_runtime_ownership_is_claimed(child);
        });
        for (const auto *child : children) {
            validate_candidate(child);
        }

        _children.reserve(_children.size() + children.size());
        for (auto *child : children) {
            smgpc::compat::claim_name_obj_runtime_ownership(
                child, this);
            _children.emplace_back(child);
        }
    }

    void NameObjChildOwner::rollback_unowned_registered_since(
        smgpc::compat::NameObjRuntimeRegistrationMarker marker) noexcept {
        while (auto *child =
                   smgpc::compat::newest_name_obj_runtime_object_since_if(
                       marker, is_unowned_construction_child, nullptr)) {
            delete child;
        }
    }

}  // namespace smgpc::scene
