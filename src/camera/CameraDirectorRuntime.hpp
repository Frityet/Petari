#pragma once

#include "camera/CameraPose.hpp"
#include "camera/CameraAnimation.hpp"

#include <memory>
#include <vector>

class CameraContext;
class CameraDirector;
class SceneObjHolder;

namespace smgpc::compat { class JkrAllocationDomain; }

namespace smgpc::camera {

    // The original objects and their raw graphs belong to SceneObjHolder's
    // retained Game heap. This adapter publishes their camera context to the
    // native renderer without selecting or calculating a camera itself.
    class CameraDirectorRuntime final {
    public:
        CameraDirectorRuntime(CameraContext &context, CameraDirector &director);
        ~CameraDirectorRuntime();
        CameraDirectorRuntime(const CameraDirectorRuntime &) = delete;
        CameraDirectorRuntime &operator=(const CameraDirectorRuntime &) = delete;

        void close_creating_chunks();
        void unpublish() noexcept;
        [[nodiscard]] void *retain_animation(std::span<const std::uint8_t> resource);
        [[nodiscard]] CameraPose pose() const;
        [[nodiscard]] CameraContext &context() const { return *_context; }
        [[nodiscard]] CameraDirector &director() const { return *_director; }
        [[nodiscard]] bool ready() const noexcept { return _ready; }

    private:
        CameraContext *_context = nullptr;
        CameraDirector *_director = nullptr;
        bool _ready = false;
        std::vector<NativeCameraAnimationData> _animations;
    };

    [[nodiscard]] CameraDirectorRuntime *current_camera_director_runtime() noexcept;
    [[nodiscard]] CameraContext *current_original_camera_context() noexcept;

}
