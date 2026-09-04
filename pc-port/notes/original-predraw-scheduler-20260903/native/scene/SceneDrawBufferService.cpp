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
#include <iterator>

namespace smgpc::scene {
namespace {
#include "DrawBufferInitialTable.inc"
#include "DrawCategoryInitialTable.inc"
constexpr auto draw_category_count = std::size(cDrawListInitTable) - 1;
void require_draw_category(int category) {
    if (category < 0 || static_cast<std::size_t>(category) >= draw_category_count)
        throw std::out_of_range("Draw category is outside the original initial table");
}
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
    std::unique_ptr<NameObjListExecutor> executor;
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
    _state->executor = std::make_unique<NameObjListExecutor>();
    _state->executor->mDrawList = new NameObjCategoryList(draw_category_count, cDrawListInitTable, &NameObj::draw, true, "");
}
SceneDrawBufferService::SceneDrawBufferService(std::shared_ptr<compat::JkrAllocationDomain> domain) : SceneDrawBufferService() {
    begin_draw_buffer_registration(std::move(domain));
}
void SceneDrawBufferService::begin_draw_buffer_registration(std::shared_ptr<compat::JkrAllocationDomain> domain) {
    compat::JkrHostAllocationScope host;
    if (!domain) throw std::invalid_argument("Original draw holder requires a retained allocation domain");
    retire_draw_buffers();
    _state->domain = std::move(domain);
    _state->prototypes.resize(category_count);
    for (std::size_t i = 0; i < category_count; ++i) {
        const auto& row = cDrawBufferListInitTable[i];
        _state->prototypes.at(row.mDrawBufferType).resize(row.mCapacity);
    }
    compat::JkrAllocationScope heap(_state->domain);
    _state->holder = new DrawBufferHolder();
    _state->executor->mBufferHolder = _state->holder;
    _state->holder->initTable(cDrawBufferListInitTable, category_count);
}
void SceneDrawBufferService::retire_draw_buffers() {
    compat::JkrHostAllocationScope host;
    if (!_state->actors.empty()) throw std::logic_error("Remove actor registrations before retiring original draw buffers");
    if (_state->holder) GXDrawDone();
    delete _state->holder;
    _state->holder = nullptr;
    _state->executor->mBufferHolder = nullptr;
    _state->prototypes.clear();
    _state->domain.reset();
    _state->allocated = false;
}
SceneDrawBufferService::~SceneDrawBufferService() {
    compat::JkrHostAllocationScope host;
    // Callers disconnect actor registrations before actor destruction. Original
    // packets/materials and their prototype owners stay alive until this holder
    // has finished every view/draw call and its arrays are retired.
    for (auto& [actor, registration] : _state->actors)
        if (registration.active)
            _state->holder->deactive(actor, registration.category, registration.executor);
    if (_state->holder) GXDrawDone();
    clear_draw_categories();
    _state.reset();
}
int SceneDrawBufferService::register_actor(LiveActor& actor, int category,
                                          std::shared_ptr<compat::ModelManagerOwner> owner) {
    compat::JkrHostAllocationScope host;
    if (!_state->holder) throw std::logic_error("Construct original draw buffers before registering a model");
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
bool SceneDrawBufferService::has_draw_buffers() const noexcept { return _state->holder != nullptr; }
std::size_t SceneDrawBufferService::registration_count() const noexcept { return _state->actors.size(); }
DrawBufferHolder& SceneDrawBufferService::holder() noexcept { return *_state->holder; }

void SceneDrawBufferService::register_pre_draw_function(const MR::FunctorBase& functor, int category, std::size_t order) {
    compat::JkrHostAllocationScope host;
    require_draw_category(category);
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
    if (state->executing[category]) throw std::logic_error("A draw category cannot rebuild its active batch recursively");
    {
        compat::JkrHostAllocationScope host;
        for (int i = 0; i < array.size();) {
            if (std::ranges::find(objects, array[i]) == objects.end()) state->executor->removeToDraw(array[i], category);
            else ++i;
        }
        if (objects.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("Original draw category capacity exceeds s32");
        if (objects.size() > static_cast<std::size_t>(array.capacity())) {
            std::vector<NameObj*> previous;
            if (array.size()) previous.assign(array.begin(), array.end());
            // The host scheduler discovers category membership dynamically. Grow the
            // actual typed array; the original register/execute algorithms stay intact.
            auto storage = std::make_unique<NameObj*[]>(objects.size());
            delete[] array.mArray.mArr;
            array.mArray.mArr = storage.release();
            array.mArray.mMaxSize = static_cast<s32>(objects.size());
            array.clear();
            for (auto* object : previous) state->executor->addToDraw(object, category);
        }
        for (auto* object : objects) {
            if (!object) throw std::invalid_argument("An original draw category requires actual NameObj objects");
            if (array.size() == 0 || std::find(array.begin(), array.end(), object) == array.end())
                state->executor->addToDraw(object, category);
        }
    }
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
    for (std::size_t i = 0; i < draw_category_count; ++i) {
        auto& array = _state->executor->mDrawList->mCategoryInfo[i].mNameObjArr;
        if (array.size() && std::find(array.begin(), array.end(), &object) != array.end())
            _state->executor->removeToDraw(&object, i);
    }
}

void SceneDrawBufferService::clear_draw_categories() {
    for (std::size_t i = 0; i < draw_category_count; ++i) {
        auto& info = _state->executor->mDrawList->mCategoryInfo[i];
        info.mNameObjArr.clear();
        info._C = nullptr;
        _state->callbacks[i].clear();
    }
}
}
