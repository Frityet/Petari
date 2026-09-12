#include "scene/SceneLifetimeBinding.hpp"
#include <aurora/exception.hpp>
#include <mutex>
#include <stdexcept>

namespace smgpc::scene {
    namespace {
        // This protects registration metadata only. A retirement callback can
        // remove itself or other bindings and must run outside this mutex.
        std::mutex bindings_mutex;
    }
    SceneLifetimeBinding *SceneLifetimeBinding::sBindings = nullptr;

    SceneLifetimeBinding::SceneLifetimeBinding(Scene &scene, Retirement retirement, void *context)
        : _scene(&scene), _retirement(retirement), _context(context), _next(nullptr) {
        if (!retirement || !context) {
            aurora::throw_host_exception<std::invalid_argument>("Scene lifetime binding requires a concrete retirement owner");
        }
        const std::lock_guard lock(bindings_mutex);
        for (auto *binding = sBindings; binding; binding = binding->_next) {
            if (binding->_scene == &scene && binding->_context == context) {
                aurora::throw_host_exception<std::logic_error>("The same native lifetime owner is already bound to this Scene");
            }
        }
        _next = sBindings;
        sBindings = this;
    }

    SceneLifetimeBinding::~SceneLifetimeBinding() {
        const std::lock_guard lock(bindings_mutex);
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
        for (;;) {
            SceneLifetimeBinding::Retirement retirement;
            void *context;
            {
                const std::lock_guard lock(bindings_mutex);
                auto *binding = SceneLifetimeBinding::sBindings;
                while (binding && binding->_scene != &scene)
                    binding = binding->_next;
                if (!binding)
                    return;
                retirement = binding->_retirement;
                context = binding->_context;
                // Retirement may destroy the native binding itself. Unpublish
                // it before invoking its owner and never access it afterward.
                binding->unlink();
            }
            retirement(context);
            // The callback may also remove another service. Resolve the next
            // live binding again instead of keeping a pointer into that owner.
        }
    }
}  // namespace smgpc::scene
