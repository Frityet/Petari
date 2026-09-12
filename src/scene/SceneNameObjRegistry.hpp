#pragma once

#include <memory>
#include <vector>

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
        SceneNameObjRegistry(NameObjHolder& holder, std::shared_ptr<compat::JkrAllocationDomain> domain);
        ~SceneNameObjRegistry();
        SceneNameObjRegistry(const SceneNameObjRegistry &) = delete;
        SceneNameObjRegistry &operator=(const SceneNameObjRegistry &) = delete;

        [[nodiscard]] NameObjHolder &holder() const noexcept;
        [[nodiscard]] std::vector<NameObj *> snapshot() const;
        void add(NameObj &object);
        void remove(NameObj &object) noexcept;
        static std::vector<NameObj*> snapshot_holder(const NameObjHolder&);
        static void add_to_holder(NameObjHolder&, NameObj&);
        static void remove_from_holder(NameObjHolder&, NameObj&) noexcept;

    private:
        friend SceneNameObjRegistry *current_scene_name_obj_registry() noexcept;
        friend void unregister_scene_name_obj(NameObj &object) noexcept;
        std::shared_ptr<compat::JkrAllocationDomain> _domain;
        std::unique_ptr<NameObjHolder> _owned_holder;
        NameObjHolder* _holder = nullptr;
        SceneNameObjRegistry *_next = nullptr;
        static SceneNameObjRegistry *sBindings;
    };

    [[nodiscard]] SceneNameObjRegistry *current_scene_name_obj_registry() noexcept;
    void register_scene_name_obj(NameObj &object);
    void unregister_scene_name_obj(NameObj &object) noexcept;
}  // namespace smgpc::scene
