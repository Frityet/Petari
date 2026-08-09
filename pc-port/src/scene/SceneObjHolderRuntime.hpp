#pragma once

#include <memory>
#include <vector>

class NameObj;
class SceneObjHolder;

namespace smgpc::compat {
    class CapturedFrameBlurService;
    class GlobalGravityOwnership;
}

namespace smgpc::scene {

    class AreaObjRuntime;

    class SceneObjHolderBinding final {
    public:
        explicit SceneObjHolderBinding(SceneObjHolder &holder);
        ~SceneObjHolderBinding();

        SceneObjHolderBinding(const SceneObjHolderBinding &) = delete;
        SceneObjHolderBinding &operator=(const SceneObjHolderBinding &) = delete;
        SceneObjHolderBinding(SceneObjHolderBinding &&) = delete;
        SceneObjHolderBinding &operator=(SceneObjHolderBinding &&) = delete;

        void init_after_placement();

    private:
        friend class ::SceneObjHolder;
        friend AreaObjRuntime *current_area_obj_runtime() noexcept;
        friend smgpc::compat::CapturedFrameBlurService *
        current_captured_frame_blur_service() noexcept;
        friend smgpc::compat::GlobalGravityOwnership *
        current_global_gravity_ownership() noexcept;

        SceneObjHolder *_holder;
        std::vector<std::unique_ptr<NameObj>> _owned_objects;
        std::unique_ptr<smgpc::compat::GlobalGravityOwnership>
            _global_gravity_ownership;
        std::unique_ptr<AreaObjRuntime> _area_obj_runtime;
        std::unique_ptr<smgpc::compat::CapturedFrameBlurService>
            _captured_frame_blur_service;
    };

    [[nodiscard]] SceneObjHolder *current_scene_obj_holder() noexcept;
    [[nodiscard]] smgpc::compat::CapturedFrameBlurService *
    current_captured_frame_blur_service() noexcept;
    [[nodiscard]] smgpc::compat::GlobalGravityOwnership *
    current_global_gravity_ownership() noexcept;

}  // namespace smgpc::scene
