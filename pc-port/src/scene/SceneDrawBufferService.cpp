#include "SceneDrawBufferService.hpp"
#include "compat/ModelManagerOwner.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/System/DrawBufferHolder.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include <map>
#include <vector>
#include <stdexcept>
#include <iterator>

namespace MR {
    enum CameraType {
        CameraType_3D = 0,
        CameraType_2D = 1,
        CameraType_Mirror = 2,
    };

}
namespace smgpc::scene {
namespace {
#include "DrawBufferInitialTable.inc"
constexpr auto category_count = std::size(cDrawBufferListInitTable) - 1;
void require_category(int category) {
    if (category < 0 || static_cast<std::size_t>(category) >= category_count)
        throw std::out_of_range("Draw buffer category is outside the original initial table");
}
}
struct SceneDrawBufferService::State {
    struct Registration { int category; int executor = -1; bool active = false; };
    std::shared_ptr<compat::JkrAllocationDomain> domain;
    std::vector<std::vector<std::shared_ptr<compat::ModelManagerOwner>>> prototypes;
    std::unique_ptr<DrawBufferHolder> holder;
    std::map<LiveActor*, Registration> actors;
    bool allocated = false;
};
SceneDrawBufferService::SceneDrawBufferService(std::shared_ptr<compat::JkrAllocationDomain> domain) {
    compat::JkrHostAllocationScope host;
    if (!domain) throw std::invalid_argument("Original draw holder requires a retained allocation domain");
    _state = std::make_unique<State>();
    _state->domain = std::move(domain);
    _state->prototypes.resize(category_count);
    for (std::size_t i = 0; i < category_count; ++i) {
        const auto& row = cDrawBufferListInitTable[i];
        _state->prototypes.at(row.mDrawBufferType).resize(row.mCapacity);
    }
    compat::JkrAllocationScope heap(_state->domain);
    _state->holder = std::make_unique<DrawBufferHolder>();
    _state->holder->initTable(cDrawBufferListInitTable, category_count);
}
SceneDrawBufferService::~SceneDrawBufferService() {
    compat::JkrHostAllocationScope host;
    // Callers disconnect actor registrations before actor destruction. Original
    // packets/materials and their prototype owners stay alive until this holder
    // has finished every view/draw call and its arrays are retired.
    for (auto& [actor, registration] : _state->actors)
        if (registration.active)
            _state->holder->deactive(actor, registration.category, registration.executor);
    GXDrawDone();
    _state.reset();
}
int SceneDrawBufferService::register_actor(LiveActor& actor, int category,
                                          std::shared_ptr<compat::ModelManagerOwner> owner) {
    compat::JkrHostAllocationScope host;
    require_category(category);
    if (_state->allocated) throw std::logic_error("Draw registrations must precede original actor-list allocation");
    if (!owner || &owner->manager() != actor.mModelManager)
        throw std::invalid_argument("Draw registration must retain the actor's actual ModelManager owner");
    auto& group = *_state->holder->getDrawBufferGroup(category);
    const auto previous = group.findExecuterIndex(MR::getModelResName(&actor));
    if (previous < 0 && group.mExecutors.mCount >= group.mExecutors.capacity())
        throw std::length_error("Original draw buffer executor capacity exhausted");
    const auto [it, inserted] = _state->actors.try_emplace(&actor, State::Registration{category});
    if (!inserted) throw std::logic_error("Actor already has an original draw-buffer registration");
    try {
        compat::JkrAllocationScope heap(_state->domain);
        it->second.executor = _state->holder->registerDrawBuffer(&actor, category);
    } catch (...) { _state->actors.erase(it); throw; }
    auto& prototype = _state->prototypes[category][it->second.executor];
    if (!prototype) prototype = std::move(owner);
    return it->second.executor;
}
void SceneDrawBufferService::allocate_actor_lists() {
    if (_state->allocated) throw std::logic_error("Original draw actor lists have already been allocated");
    compat::JkrAllocationScope heap(_state->domain);
    _state->holder->allocateActorListBuffer();
    _state->allocated = true;
}
void SceneDrawBufferService::set_active(LiveActor& actor, bool active) {
    auto& registration = _state->actors.at(&actor);
    if (!_state->allocated) throw std::logic_error("Allocate original actor lists before activating draw packets");
    if (registration.active == active) return;
    if (active) _state->holder->active(&actor, registration.category, registration.executor);
    else _state->holder->deactive(&actor, registration.category, registration.executor);
    registration.active = active;
}
void SceneDrawBufferService::remove_actor(LiveActor& actor) {
    compat::JkrHostAllocationScope host;
    const auto found = _state->actors.find(&actor);
    if (found == _state->actors.end()) return;
    if (found->second.active) set_active(actor, false);
    // The actor may release its final model/animation owner immediately after
    // this unregister boundary. Retire queued GX array/DL references first.
    GXDrawDone();
    _state->actors.erase(found);
}
void SceneDrawBufferService::find_light_info(LiveActor& actor) {
    const auto& registration = _state->actors.at(&actor);
    _state->holder->findLightInfo(&actor, registration.category, registration.executor);
}
void SceneDrawBufferService::entry(int camera_type) {
    if (camera_type < 0 || camera_type >= 3) throw std::out_of_range("Original draw camera category");
    _state->holder->entry(camera_type);
}
void SceneDrawBufferService::draw_opaque(int category) {
    require_category(category);
    _state->holder->drawOpa(category);
}
void SceneDrawBufferService::draw_translucent(int category) {
    require_category(category);
    _state->holder->drawXlu(category);
}
bool SceneDrawBufferService::is_allocated() const noexcept { return _state->allocated; }
std::size_t SceneDrawBufferService::registration_count() const noexcept { return _state->actors.size(); }
DrawBufferHolder& SceneDrawBufferService::holder() noexcept { return *_state->holder; }
}
