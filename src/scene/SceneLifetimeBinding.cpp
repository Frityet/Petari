#include "scene/SceneLifetimeBinding.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>

namespace smgpc::scene {
    thread_local SceneLifetimeBinding *SceneLifetimeBinding::sBindings = nullptr;

    SceneLifetimeBinding::SceneLifetimeBinding(Scene &scene, Retirement retirement, void *context)
        : _scene(&scene), _retirement(retirement), _context(context), _next(nullptr) {
        if (!retirement || !context) {
            aurora::throw_host_exception<std::invalid_argument>("Scene lifetime binding requires a concrete retirement owner");
        }
        for (auto *binding = sBindings; binding; binding = binding->_next) {
            if (binding->_scene == &scene) {
                aurora::throw_host_exception<std::logic_error>("Scene already has a native lifetime owner");
            }
        }
        _next = sBindings;
        sBindings = this;
    }

    SceneLifetimeBinding::~SceneLifetimeBinding() {
        unlink();
    }

    void SceneLifetimeBinding::unlink() noexcept {
        if (!_scene)
            return;
        for (auto **link = &sBindings; *link; link = &(*link)->_next) {
            if (*link == this) {
                *link = _next;
                break;
            }
        }
        _scene = nullptr;
        _next = nullptr;
    }

    void retire_scene_services(Scene &scene) noexcept {
        for (auto *binding = SceneLifetimeBinding::sBindings; binding; binding = binding->_next) {
            if (binding->_scene != &scene)
                continue;
            const auto retirement = binding->_retirement;
            auto *context = binding->_context;
            // Retirement may destroy the native binding itself. Unpublish it
            // before invoking its owner and never touch it after the callback.
            binding->unlink();
            retirement(context);
            return;
        }
    }
}  // namespace smgpc::scene
