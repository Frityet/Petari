#pragma once
#include <memory>
#include <cstddef>
class LiveActor;
class DrawBufferHolder;
namespace smgpc::compat { class JkrAllocationDomain; class ModelManagerOwner; }
namespace smgpc::scene {
// Native lifetime owner for the original scene draw holder. Registration and
// allocation are separate, matching NameObjListExecutor's construction phases.
class SceneDrawBufferService final {
public:
    explicit SceneDrawBufferService(std::shared_ptr<compat::JkrAllocationDomain>);
    ~SceneDrawBufferService();
    SceneDrawBufferService(const SceneDrawBufferService&) = delete;
    SceneDrawBufferService& operator=(const SceneDrawBufferService&) = delete;
    int register_actor(LiveActor&, int category, std::shared_ptr<compat::ModelManagerOwner>);
    void allocate_actor_lists();
    void set_active(LiveActor&, bool);
    void remove_actor(LiveActor&);
    void find_light_info(LiveActor&);
    void entry(int camera_type);
    void draw_opaque(int category);
    void draw_translucent(int category);
    bool is_allocated() const noexcept;
    std::size_t registration_count() const noexcept;
    DrawBufferHolder& holder() noexcept;
private:
    struct State;
    std::unique_ptr<State> _state;
};
}
