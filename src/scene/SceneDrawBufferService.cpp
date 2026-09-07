#include <aurora/exception.hpp>
#include "SceneDrawBufferService.hpp"
#include "compat/ModelManagerOwner.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/System/DrawBufferHolder.hpp"
#include "Game/NameObj/NameObjListExecutor.hpp"
#include "Game/NameObj/NameObjCategoryList.hpp"
#include "Game/Util/Functor.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include <map>
#include <vector>
#include <stdexcept>
#include <exception>
#include <iterator>

namespace smgpc::scene {
namespace {
#include "DrawBufferInitialTable.inc"
#include "DrawCategoryInitialTable.inc"
constexpr auto draw_category_count = std::size(cDrawListInitTable) - 1;
void require_draw_category(int category) {
    if (category < 0 || static_cast<std::size_t>(category) >= draw_category_count)
        aurora::throw_host_exception<std::out_of_range>("Draw category is outside the original initial table");
}
constexpr auto category_count = std::size(cDrawBufferListInitTable) - 1;
void require_category(int category) {
    if (category < 0 || static_cast<std::size_t>(category) >= category_count)
        aurora::throw_host_exception<std::out_of_range>("Draw buffer category is outside the original initial table");
}
}
struct SceneDrawBufferService::State {
    struct Registration { int category; int executor = -1; };
    std::shared_ptr<compat::JkrAllocationDomain> domain;
    std::vector<std::vector<std::shared_ptr<compat::ModelManagerOwner>>> prototypes;
    NameObjListExecutor* executor = nullptr;
    DrawBufferHolder* holder = nullptr;
    struct Callback {
        std::shared_ptr<compat::JkrAllocationDomain> domain;
        std::unique_ptr<MR::FunctorBase> functor;
        std::size_t order;
    };
    std::array<std::vector<std::shared_ptr<Callback>>, draw_category_count> callbacks;
    std::array<unsigned, draw_category_count> executing{};
    std::map<LiveActor*, Registration> actors;
    bool allocated = false;
};
SceneDrawBufferService::SceneDrawBufferService() {
    compat::JkrHostAllocationScope host;
    _state = std::make_shared<State>();
}
void SceneDrawBufferService::bind_executor(NameObjListExecutor& executor, std::shared_ptr<compat::JkrAllocationDomain> domain) {
    compat::JkrHostAllocationScope host;
    if (!domain || !executor.mBufferHolder || !executor.mDrawList)
        aurora::throw_host_exception<std::invalid_argument>("Draw execution requires the actual initialized scene executor and Game domain");
    if (_state->executor)
        aurora::throw_host_exception<std::logic_error>("Retire the previous scene executor before binding another");
    _state->prototypes.resize(category_count);
    for (std::size_t i = 0; i < category_count; ++i) {
        const auto& row = cDrawBufferListInitTable[i];
        _state->prototypes.at(row.mDrawBufferType).resize(row.mCapacity);
    }
    _state->domain = std::move(domain);
    _state->executor = &executor;
    _state->holder = executor.mBufferHolder;
}
void SceneDrawBufferService::retire_draw_buffers() {
    compat::JkrHostAllocationScope host;
    if (!_state->actors.empty())
        aurora::throw_host_exception<std::logic_error>("Remove actor registrations before retiring original draw buffers");
    if (!_state->holder) return;
    const bool has_models = std::ranges::any_of(_state->prototypes, [](const auto& category) {
        return std::ranges::any_of(category, [](const auto& prototype) { return bool(prototype); });
    });
    if (has_models) GXDrawDone();
    delete _state->holder;
    _state->holder = nullptr;
    _state->executor->mBufferHolder = nullptr;
    _state->prototypes.clear();
    _state->allocated = false;
}
void SceneDrawBufferService::unbind_executor() {
    compat::JkrHostAllocationScope host;
    clear_draw_categories();
    retire_draw_buffers();
    _state->executor = nullptr;
    _state->domain.reset();
}
SceneDrawBufferService::~SceneDrawBufferService() {
    if (_state->executor) std::terminate();
}
void SceneDrawBufferService::validate_actor_registration(LiveActor& actor, int category,
                                                        const std::shared_ptr<compat::ModelManagerOwner>& owner) {
    compat::JkrHostAllocationScope host;
    if (!_state->holder) aurora::throw_host_exception<std::logic_error>("Construct original draw buffers before registering a model");
    require_category(category);
    if (_state->allocated) aurora::throw_host_exception<std::logic_error>("Original model registration must precede actor-list allocation");
    if (!owner || &owner->manager() != actor.mModelManager)
        aurora::throw_host_exception<std::invalid_argument>("Draw registration must retain the actor's actual ModelManager owner");
    auto& group = *_state->holder->getDrawBufferGroup(category);
    auto index = group.findExecuterIndex(MR::getModelResName(&actor));
    if (index < 0) {
        if (group.mExecutors.size() == group.mExecutors.capacity())
            aurora::throw_host_exception<std::length_error>("Original draw executor category capacity exceeded");
        index = group.mExecutors.size();
    }
    // Allocate native metadata and retain the model before the original
    // constructor publishes borrowed pointers into its draw executor.
    const auto [it, inserted] = _state->actors.try_emplace(&actor, State::Registration{category, index});
    if (!inserted) aurora::throw_host_exception<std::logic_error>("Actor already has an original draw-buffer registration");
    auto& prototype = _state->prototypes[category][index];
    if (!prototype) prototype = owner;
}
int SceneDrawBufferService::register_actor(LiveActor& actor, int category,
                                          std::shared_ptr<compat::ModelManagerOwner> owner) {
    const auto& registration = _state->actors.at(&actor);
    const auto index = _state->holder->getDrawBufferGroup(category)->findExecuterIndex(MR::getModelResName(&actor));
    if (index != registration.executor)
        aurora::throw_host_exception<std::logic_error>("Original draw registration differs from its retained model owner");
    return index;
}
void SceneDrawBufferService::allocate_actor_lists() {
    if (_state->allocated) aurora::throw_host_exception<std::logic_error>("Original draw actor lists have already been allocated");
    compat::JkrAllocationScope heap(_state->domain);
    _state->executor->allocateDrawBufferActorList();
    _state->allocated = true;
}
void SceneDrawBufferService::set_active(LiveActor& actor, bool active) {
    const auto& registration = _state->actors.at(&actor);
    auto* executor = _state->holder->getDrawBufferGroup(registration.category)->mExecutors[registration.executor];
    const bool present = executor->mNumActors && std::find(executor->mActors, executor->mActors + executor->mNumActors, &actor) != executor->mActors + executor->mNumActors;
    if (present != active)
        aurora::throw_host_exception<std::logic_error>("Original execution requirements must be the single draw-membership authority");
}
void SceneDrawBufferService::remove_actor(LiveActor& actor) {
    compat::JkrHostAllocationScope host;
    const auto found = _state->actors.find(&actor);
    if (found == _state->actors.end()) return;
    if (_state->allocated) set_active(actor, false);
    GXDrawDone();
    _state->actors.erase(found);
}
void SceneDrawBufferService::find_light_info(LiveActor& actor) {
    const auto& registration = _state->actors.at(&actor);
    _state->holder->findLightInfo(&actor, registration.category, registration.executor);
}
void SceneDrawBufferService::entry(int camera_type) {
    if (camera_type < 0 || camera_type >= 3) aurora::throw_host_exception<std::out_of_range>("Original draw camera category");
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
bool SceneDrawBufferService::has_draw_buffers() const noexcept { return _state->holder != nullptr; }
std::size_t SceneDrawBufferService::registration_count() const noexcept { return _state->actors.size(); }
DrawBufferHolder& SceneDrawBufferService::holder() noexcept { return *_state->holder; }

void SceneDrawBufferService::register_pre_draw_function(const MR::FunctorBase& functor, int category, std::size_t order) {
    compat::JkrHostAllocationScope host;
    require_draw_category(category);
    if (!_state->executor) aurora::throw_host_exception<std::logic_error>("Pre-draw functions require the active original scene executor");
    auto callback = std::make_shared<State::Callback>();
    callback->domain = compat::current_jkr_allocation_domain();
    if (!callback->domain) callback->domain = _state->domain;
    callback->order = order;
    auto& history = _state->callbacks[category];
    history.reserve(history.size() + 1);
    if (callback->domain) {
        compat::JkrAllocationScope heap(callback->domain);
        _state->executor->registerPreDrawFunction(functor, category);
        callback->functor.reset(_state->executor->mDrawList->mCategoryInfo[category]._C);
    } else {
        _state->executor->registerPreDrawFunction(functor, category);
        callback->functor.reset(_state->executor->mDrawList->mCategoryInfo[category]._C);
    }
    history.push_back(std::move(callback));
}

void SceneDrawBufferService::rollback_pre_draw_functions(std::size_t marker) {
    if (!_state->executor) return;
    compat::JkrHostAllocationScope host;
    for (std::size_t i = 0; i < draw_category_count; ++i) {
        auto& history = _state->callbacks[i];
        std::erase_if(history, [marker](const auto& callback) { return callback->order >= marker; });
        _state->executor->mDrawList->mCategoryInfo[i]._C = history.empty() ? nullptr : history.back()->functor.get();
    }
}

std::vector<NameObj*> SceneDrawBufferService::execute_draw_category(int category, std::span<NameObj* const> objects) {
    require_draw_category(category);
    auto state = _state;
    auto& array = state->executor->mDrawList->mCategoryInfo[category].mNameObjArr;
    if (state->executing[category]) aurora::throw_host_exception<std::logic_error>("A draw category cannot rebuild its active batch recursively");
    (void)objects; // Actual original category membership is authoritative.
    // Retain an executing callback even when it replaces itself or clears its
    // registration. Its borrowed caller's scene allocation domain stays alive.
    const auto callback = state->callbacks[category].empty() ? nullptr : state->callbacks[category].back();
    struct Guard { unsigned& value; explicit Guard(unsigned& n) : value(n) { ++value; } ~Guard() { --value; } } guard(state->executing[category]);
    // Original callbacks inherit the caller's current heap and routing.
    state->executor->executeDraw(category);
    compat::JkrHostAllocationScope host;
    if (array.size() == 0) return {};
    return {array.begin(), array.end()};
}

void SceneDrawBufferService::remove_draw_object(NameObj& object) {
    if (!_state->executor) return;
    for (std::size_t i = 0; i < draw_category_count; ++i) {
        auto& array = _state->executor->mDrawList->mCategoryInfo[i].mNameObjArr;
        if (array.size() && std::find(array.begin(), array.end(), &object) != array.end())
            _state->executor->removeToDraw(&object, i);
    }
}

void SceneDrawBufferService::clear_draw_categories() {
    if (!_state->executor) return;
    for (std::size_t i = 0; i < draw_category_count; ++i) {
        auto& info = _state->executor->mDrawList->mCategoryInfo[i];
        info.mNameObjArr.clear();
        info._C = nullptr;
        _state->callbacks[i].clear();
    }
}
}
