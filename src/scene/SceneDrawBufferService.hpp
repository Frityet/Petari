#pragma once
#include <memory>
#include <cstddef>
#include <span>
#include <vector>
class LiveActor;
class DrawBufferHolder;
class NameObj;
class NameObjListExecutor;
namespace MR { class FunctorBase; }
namespace smgpc::compat { class JkrAllocationDomain; class ModelManagerOwner; }
namespace smgpc::scene {
// Native lifetime owner for the original scene draw holder. Registration and
// allocation are separate, matching NameObjListExecutor's construction phases.
class SceneDrawBufferService final {
public:
    SceneDrawBufferService();
    ~SceneDrawBufferService();
    SceneDrawBufferService(const SceneDrawBufferService&) = delete;
    SceneDrawBufferService& operator=(const SceneDrawBufferService&) = delete;
    void bind_executor(NameObjListExecutor&, std::shared_ptr<compat::JkrAllocationDomain>);
    void unbind_executor();
    void retire_draw_buffers();
    void validate_actor_registration(LiveActor&, int category, const std::shared_ptr<compat::ModelManagerOwner>&);
    int register_actor(LiveActor&, int category, std::shared_ptr<compat::ModelManagerOwner>);
    void allocate_actor_lists();
    void set_active(LiveActor&, bool);
    void remove_actor(LiveActor&);
    void find_light_info(LiveActor&);
    void entry(int camera_type);
    void draw_opaque(int category);
    void draw_translucent(int category);
    bool is_allocated() const noexcept;
    bool has_draw_buffers() const noexcept;
    std::size_t registration_count() const noexcept;
    DrawBufferHolder& holder() noexcept;
    void register_pre_draw_function(const MR::FunctorBase&, int category, std::size_t order);
    void rollback_pre_draw_functions(std::size_t marker);
    std::vector<NameObj*> execute_draw_category(int category, std::span<NameObj* const> objects);
    void remove_draw_object(NameObj&);
    void clear_draw_categories();
private:
    struct State;
    std::shared_ptr<State> _state;
};
}
