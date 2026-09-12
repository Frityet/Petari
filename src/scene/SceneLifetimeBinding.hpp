#pragma once

class Scene;

namespace smgpc::scene {
    // Native services retire after an original derived Scene destructor has
    // run, but before its base destroys the actual holder and executor.
    // The external scene owner retains their allocation domain through delete.
    // Distinct services retire in reverse binding order, so children can be
    // released before the initialization and execution services they borrow.
    // Registration follows Scene identity across native worker threads. The
    // scene owner must still serialize access to each Scene and its services.
    class SceneLifetimeBinding final {
    public:
        using Retirement = void (*)(void *) noexcept;
        SceneLifetimeBinding(Scene &scene, Retirement retirement, void *context);
        ~SceneLifetimeBinding();
        SceneLifetimeBinding(const SceneLifetimeBinding &) = delete;
        SceneLifetimeBinding &operator=(const SceneLifetimeBinding &) = delete;

    private:
        friend void retire_scene_services(Scene &scene) noexcept;
        void unlink() noexcept;
        Scene *_scene;
        Retirement _retirement;
        void *_context;
        SceneLifetimeBinding *_next;
        static SceneLifetimeBinding *sBindings;
    };

    void retire_scene_services(Scene &scene) noexcept;
}  // namespace smgpc::scene
