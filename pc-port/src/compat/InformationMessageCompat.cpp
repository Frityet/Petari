#include "compat/InformationMessageCompat.hpp"

#include "Game/NameObj/NameObj.hpp"
#include "Game/Screen/IconAButton.hpp"
#include "Game/Screen/InformationMessage.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

    InformationMessage* sCurrentInformationMessage = nullptr;

    [[nodiscard]] bool contains_identity(const std::vector<NameObj*>& objects,
                                         const NameObj* wanted) {
        return std::ranges::find(objects, wanted) != objects.end();
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
        const auto existing = snapshot_name_obj_runtime_objects();
        try {
            _impl->message->initWithoutIter();
        } catch (...) {
            for (auto* object : snapshot_name_obj_runtime_objects()) {
                if (object != _impl->message.get() &&
                    !contains_identity(existing, object)) {
                    _impl->children.emplace_back(object);
                }
            }
            throw;
        }

        for (auto* object : snapshot_name_obj_runtime_objects()) {
            if (object != _impl->message.get() &&
                !contains_identity(existing, object)) {
                _impl->children.emplace_back(object);
            }
        }
        if (_impl->children.size() != 1U ||
            dynamic_cast<IconAButton*>(_impl->children.front().get()) == nullptr) {
            throw std::logic_error(
                "the exact InformationMessage did not create its one IconAButton child");
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
