#include "compat/InformationMessageCompat.hpp"

#include "Game/NameObj/NameObj.hpp"
#include "Game/Screen/IconAButton.hpp"
#include "Game/Screen/InformationMessage.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace {

    InformationMessage* sCurrentInformationMessage = nullptr;

    [[nodiscard]] bool is_unowned_information_message_child(
        const NameObj* object, const void*) noexcept {
        return !smgpc::compat::
                   name_obj_runtime_ownership_is_claimed(object) &&
               !smgpc::scene::
                   current_scene_obj_holder_binding_owns(object);
    }

    void rollback_information_message_children(
        smgpc::compat::NameObjRuntimeRegistrationMarker marker) noexcept {
        while (auto* object =
                   smgpc::compat::newest_name_obj_runtime_object_since_if(
                       marker, is_unowned_information_message_child,
                       nullptr)) {
            delete object;
        }
    }

}  // namespace

namespace smgpc::compat {

    struct InformationMessageBinding::Impl {
        std::unique_ptr<InformationMessage> message;
        // Declared after message so reverse destruction disconnects and
        // destroys the child before its non-owning follow-actor target.
        std::vector<std::unique_ptr<NameObj>> children;
    };

    InformationMessageBinding::InformationMessageBinding()
        : _impl(std::make_unique<Impl>()) {
        if (sCurrentInformationMessage != nullptr) {
            throw std::logic_error(
                "an InformationMessage is already bound to the active scene");
        }
        if (smgpc::runtime::RuntimeContext::try_instance() == nullptr) {
            throw std::logic_error(
                "InformationMessage construction requires an active RuntimeContext");
        }
        if (smgpc::scene::current_scene_obj_holder() == nullptr) {
            throw std::logic_error(
                "InformationMessage construction requires an active SceneObjHolder");
        }

        _impl->message = std::make_unique<InformationMessage>();
        smgpc::compat::claim_name_obj_runtime_ownership(
            _impl->message.get(), _impl.get());
        // The exact init raw-news IconAButton. Keep every potentially
        // allocating operation before the ownership commit so an exception
        // can retire the still-unclaimed registration suffix without
        // replacing the original failure.
        const auto marker = mark_name_obj_runtime_registrations();
        try {
            _impl->message->initWithoutIter();
            const auto registered =
                snapshot_name_obj_runtime_objects_since(marker);
            if (registered.size() != 1U ||
                registered.front() == _impl->message.get() ||
                dynamic_cast<IconAButton*>(registered.front()) == nullptr ||
                name_obj_runtime_ownership_is_claimed(
                    registered.front()) ||
                smgpc::scene::current_scene_obj_holder_binding_owns(
                    registered.front())) {
                throw std::logic_error(
                    "the exact InformationMessage did not create its one unowned IconAButton child");
            }

            _impl->children.reserve(registered.size());
            for (auto* object : registered) {
                smgpc::compat::claim_name_obj_runtime_ownership(
                    object, _impl.get());
                _impl->children.emplace_back(object);
            }
        } catch (...) {
            rollback_information_message_children(marker);
            throw;
        }

        sCurrentInformationMessage = _impl->message.get();
    }

    InformationMessageBinding::~InformationMessageBinding() {
        if (sCurrentInformationMessage != _impl->message.get()) {
            std::terminate();
        }
        sCurrentInformationMessage = nullptr;
    }

    InformationMessage& InformationMessageBinding::message() {
        return *_impl->message;
    }

    const InformationMessage& InformationMessageBinding::message() const {
        return *_impl->message;
    }

    InformationMessage* current_information_message() noexcept {
        return sCurrentInformationMessage;
    }

    InformationMessage& require_information_message() {
        auto* message = current_information_message();
        if (message == nullptr) {
            throw std::logic_error(
                "InformationMessage is unavailable without its active scene binding");
        }
        return *message;
    }

}  // namespace smgpc::compat
