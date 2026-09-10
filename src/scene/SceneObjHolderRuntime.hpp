#pragma once

#include "scene/SceneInitializationState.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

class NameObj;
class SceneObjHolder;

namespace smgpc::runtime { class SceneScheduler; class SceneSchedulerAllocationBinding; class SceneMessageBinding; }
namespace smgpc::camera { class CameraDirectorRuntime; }

namespace smgpc::compat {
    class CollisionDirectorOwnership;
    class DemoDirectorOwnership;
    class EffectSystemOwnership;
    class ImageEffectOwnership;
    class JkrAllocationDomain;
    class CapturedFrameBlurService;
    class GlobalGravityOwnership;
}

namespace smgpc::scene {

    class AreaObjRuntime;

    using SceneObjFactoryOverride = NameObj *(*)(int id, void *context);

    class SceneObjHolderBinding final {
    public:
        explicit SceneObjHolderBinding(
            SceneObjHolder &holder,
            SceneObjFactoryOverride factory_override = nullptr,
            void *factory_context = nullptr,
            std::shared_ptr<smgpc::compat::JkrAllocationDomain> allocation_domain = {});
        ~SceneObjHolderBinding();

        SceneObjHolderBinding(const SceneObjHolderBinding &) = delete;
        SceneObjHolderBinding &operator=(const SceneObjHolderBinding &) = delete;
        SceneObjHolderBinding(SceneObjHolderBinding &&) = delete;
        SceneObjHolderBinding &operator=(SceneObjHolderBinding &&) = delete;

        void initialize_effect_system(unsigned particles = 3072, unsigned emitters = 256,
                                      std::size_t byte_budget = 8U * 1024U * 1024U);
        void initialize_camera_system();
        void init_after_placement();
        void acknowledge_scene_postpass(std::span<NameObj *const> objects);
        void complete_camera_parameters();
        void complete_initialization();

    private:
        friend class ::SceneObjHolder;
        friend std::shared_ptr<smgpc::compat::JkrAllocationDomain> current_scene_allocation_domain() noexcept;
        friend smgpc::compat::CollisionDirectorOwnership* current_collision_director_ownership() noexcept;
        friend smgpc::compat::EffectSystemOwnership* current_effect_system_ownership() noexcept;
        friend AreaObjRuntime *current_area_obj_runtime() noexcept;
        friend smgpc::compat::CapturedFrameBlurService *
        current_captured_frame_blur_service() noexcept;
        friend smgpc::compat::GlobalGravityOwnership *
        current_global_gravity_ownership() noexcept;
        friend bool current_scene_obj_holder_binding_owns(
            const NameObj *object) noexcept;
        friend void adopt_current_scene_obj_holder_descendant(
            NameObj *object);

        friend smgpc::compat::DemoDirectorOwnership* current_demo_director_ownership() noexcept;
        std::unique_ptr<smgpc::compat::DemoDirectorOwnership> _demo_director_ownership;
        SceneInitializationBinding _initialization_state;
        std::unique_ptr<smgpc::runtime::SceneMessageBinding> _scene_messages;
        std::shared_ptr<smgpc::compat::JkrAllocationDomain> _game_allocation_domain;
        std::unique_ptr<smgpc::runtime::SceneSchedulerAllocationBinding> _game_allocation_binding;
        std::unique_ptr<smgpc::camera::CameraDirectorRuntime> _camera_runtime;
        std::unique_ptr<smgpc::compat::EffectSystemOwnership> _effect_system_ownership;
        std::unique_ptr<smgpc::compat::ImageEffectOwnership> _image_effect_ownership;
        std::unique_ptr<smgpc::compat::CollisionDirectorOwnership> _collision_director_ownership;
        smgpc::runtime::SceneScheduler* _effect_scheduler = nullptr;
        std::size_t _effect_registration_marker = 0U;
        SceneObjHolder *_holder;
        std::vector<std::unique_ptr<NameObj>> _owned_objects;
        std::vector<NameObj *> _owned_registration_objects;
        std::size_t _next_registration_postpass_index = 0U;
        struct ProvisionalSlot final {
            int id;
            NameObj *object;
        };
        std::vector<ProvisionalSlot> _provisional_slots;
        std::size_t _construction_depth = 0U;
        SceneObjFactoryOverride _factory_override;
        void *_factory_context;
        std::unique_ptr<smgpc::compat::GlobalGravityOwnership>
            _global_gravity_ownership;
        std::unique_ptr<AreaObjRuntime> _area_obj_runtime;
        std::unique_ptr<smgpc::compat::CapturedFrameBlurService>
            _captured_frame_blur_service;
    };

    [[nodiscard]] smgpc::compat::CollisionDirectorOwnership* current_collision_director_ownership() noexcept;
    [[nodiscard]] smgpc::compat::EffectSystemOwnership* current_effect_system_ownership() noexcept;
    [[nodiscard]] SceneObjHolder *current_scene_obj_holder() noexcept;
    [[nodiscard]] smgpc::compat::DemoDirectorOwnership* current_demo_director_ownership() noexcept;
    [[nodiscard]] std::shared_ptr<smgpc::compat::JkrAllocationDomain> current_scene_allocation_domain() noexcept;
    [[nodiscard]] bool current_scene_obj_holder_binding_owns(
        const NameObj *object) noexcept;
    void adopt_current_scene_obj_holder_descendant(NameObj *object);
    [[nodiscard]] smgpc::compat::CapturedFrameBlurService *
    current_captured_frame_blur_service() noexcept;
    [[nodiscard]] smgpc::compat::GlobalGravityOwnership *
    current_global_gravity_ownership() noexcept;

}  // namespace smgpc::scene
