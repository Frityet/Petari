#pragma once

#include <memory>
#include <vector>

class NameObj;
class SceneObjHolder;

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

        SceneObjHolder *_holder;
        std::vector<std::unique_ptr<NameObj>> _owned_objects;
        std::unique_ptr<AreaObjRuntime> _area_obj_runtime;
    };

    [[nodiscard]] SceneObjHolder *current_scene_obj_holder() noexcept;

}  // namespace smgpc::scene
