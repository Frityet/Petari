#pragma once

#include <memory>

class NameObj;
class NameObjHolder;
namespace smgpc::compat {
    class JkrAllocationDomain;
}

namespace smgpc::scene {
    // The genuine scene NameObjHolder retains every constructed NameObj,
    // including objects which have no movement/draw execution registration.
    class SceneNameObjRegistry final {
    public:
        explicit SceneNameObjRegistry(std::shared_ptr<compat::JkrAllocationDomain> domain);
        ~SceneNameObjRegistry();
        SceneNameObjRegistry(const SceneNameObjRegistry &) = delete;
        SceneNameObjRegistry &operator=(const SceneNameObjRegistry &) = delete;

        [[nodiscard]] NameObjHolder &holder() const noexcept;
        void add(NameObj &object);
        void remove(NameObj &object) noexcept;

    private:
        friend SceneNameObjRegistry *current_scene_name_obj_registry() noexcept;
        friend void unregister_scene_name_obj(NameObj &object) noexcept;
        std::shared_ptr<compat::JkrAllocationDomain> _domain;
        std::unique_ptr<NameObjHolder> _holder;
        SceneNameObjRegistry *_next = nullptr;
        static thread_local SceneNameObjRegistry *sBindings;
    };

    [[nodiscard]] SceneNameObjRegistry *current_scene_name_obj_registry() noexcept;
    void register_scene_name_obj(NameObj &object);
    void unregister_scene_name_obj(NameObj &object) noexcept;
}  // namespace smgpc::scene
