#pragma once

#include <memory>
#include <vector>

class NameObj;
class SceneObjHolder;

namespace smgpc::scene {

    class SceneObjHolderBinding final {
    public:
        explicit SceneObjHolderBinding(SceneObjHolder &holder);
        ~SceneObjHolderBinding();

        SceneObjHolderBinding(const SceneObjHolderBinding &) = delete;
        SceneObjHolderBinding &operator=(const SceneObjHolderBinding &) = delete;
        SceneObjHolderBinding(SceneObjHolderBinding &&) = delete;
        SceneObjHolderBinding &operator=(SceneObjHolderBinding &&) = delete;

    private:
        friend class ::SceneObjHolder;

        SceneObjHolder *_holder;
        std::vector<std::unique_ptr<NameObj>> _owned_objects;
    };

    [[nodiscard]] SceneObjHolder *current_scene_obj_holder() noexcept;

}  // namespace smgpc::scene
