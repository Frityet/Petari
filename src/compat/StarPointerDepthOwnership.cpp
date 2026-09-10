#include "compat/StarPointerDepthOwnership.hpp"

#include "Game/Screen/StarPointerController.hpp"
#include "Game/Screen/StarPointerDirector.hpp"
#include "Game/Screen/StarPointerGuidance.hpp"
#include "Game/Screen/StarPointerLayout.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/WPadOwnership.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "Game/System/StarPointerOnOffController.hpp"
#include "Game/Screen/StarPointerBlur.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include "layout/LayoutHost.hpp"

#include <aurora/depth_snapshot.hpp>
#include <aurora/exception.hpp>
#include <dolphin/gx/GXAurora.h>
#include <algorithm>
#include <array>
#include <deque>
#include <stdexcept>

namespace smgpc::compat {
namespace {
thread_local StarPointerDepthOwnership* current_owner = nullptr;
thread_local StarPointerSceneBinding* current_scene_binding = nullptr;
}

struct StarPointerDepthOwnership::State {
    struct Capture {
        AuroraDepthSnapshotId id = 0;
        std::array<DpdInfo, 2> infos;
        TPos3f view;
        std::array<f32, 7> projection;
        std::array<f32, 6> viewport;
    };

    explicit State(const void* owner, std::shared_ptr<JkrHeapRuntime> heaps)
        : native_owner(owner), domain(JkrAllocationDomain::create(std::move(heaps), 1024U * 1024U)) {
        input = std::make_unique<WPadOwnership>(domain);
        JkrAllocationScope game(domain);
        director = new StarPointerDirector;
        controllers = director->mControllers;
        transform = director->mTransHolder;
        peek = director->mPeekZ;
        modes = new StarPointerOnOffController;
    }

    static void retire_layout_object(NameObj* object) noexcept {
        if (auto* blur = dynamic_cast<StarPointerBlur*>(object)) {
            delete blur->mTexture;
            blur->mTexture = nullptr;
        }
        smgpc::layout::release_layout_actor_if_registered(object);
        delete object;
    }

    void rollback_layouts(NameObjRuntimeRegistrationMarker marker) noexcept {
        layouts = {director->mStarPointerLayouts, director->mStarPointerLayouts ? director->mStarPointerLayouts + 1 : nullptr};
        const auto separate_child = [](const NameObj* object, const void* context) noexcept {
            const auto& state = *static_cast<const State*>(context);
            return object != state.layouts[0] && object != state.layouts[1] &&
                   (!name_obj_runtime_ownership_is_claimed(object) || name_obj_runtime_owner(object) == state.native_owner);
        };
        while (auto* object = newest_name_obj_runtime_object_since_if(marker, separate_child, this)) retire_layout_object(object);
        layout_objects.clear();
        for (auto* layout : layouts) smgpc::layout::release_layout_actor_if_registered(layout);
        delete[] director->mStarPointerLayouts;
        director->mStarPointerLayouts = nullptr;
        director->mGuidance = nullptr;
        guidance = nullptr;
        layouts = {};
    }

    void release_layouts() noexcept {
        // Each original array element has a NameObj identity but must be
        // destroyed by delete[], after its separately allocated children.
        for (auto it = layout_objects.rbegin(); it != layout_objects.rend(); ++it) {
            auto* object = *it;
            if (object == layouts[0] || object == layouts[1]) continue;
            retire_layout_object(object);
        }
        layout_objects.clear();
        for (auto* layout : layouts) smgpc::layout::release_layout_actor_if_registered(layout);
        delete[] director->mStarPointerLayouts;
        director->mStarPointerLayouts = nullptr;
        director->mGuidance = nullptr;
        guidance = nullptr;
        layouts = {};
    }

    ~State() {
        for (const auto& capture : pending) GXAuroraReleaseDepthSnapshot(capture.id);
        release_layouts();
        delete modes;
        delete[] controllers;
        delete transform;
        delete peek;
        delete director;
        input.reset();
    }

    const void* native_owner;
    std::shared_ptr<JkrAllocationDomain> domain;
    std::unique_ptr<WPadOwnership> input;
    StarPointerDirector* director = nullptr;
    StarPointerOnOffController* modes = nullptr;
    std::vector<NameObj*> layout_objects;
    StarPointerController* controllers = nullptr;
    StarPointerTransformHolder* transform = nullptr;
    StarPointerPeekZ* peek = nullptr;
    StarPointerGuidance* guidance = nullptr;
    std::array<StarPointerLayout*, 2> layouts{};
    bool camera_ready = false;
    std::deque<Capture> pending;
};

StarPointerDepthOwnership::StarPointerDepthOwnership(std::shared_ptr<JkrHeapRuntime> heaps) {
    JkrHostAllocationScope host;
    _state = std::make_unique<State>(this, std::move(heaps));
    _previous = current_owner;
    current_owner = this;
}

StarPointerDepthOwnership::~StarPointerDepthOwnership() {
    // Snapshot IDs own no Game callbacks, and are retired before arena release.
    _state.reset();
    current_owner = _previous;
}

void StarPointerDepthOwnership::set_camera(const TPos3f& view, const TProj3f& projection, f32 fovy) {
    _state->transform->mViewMtx.setInline(view);
    _state->transform->mProjMtx.setInline(projection);
    _state->transform->mFovy = fovy;
    _state->camera_ready = true;
}

void StarPointerDepthOwnership::capture() {
    JkrHostAllocationScope host;
    if (!_state->camera_ready || !AuroraIsFrameActive()) return;
    if (!std::any_of(_state->layouts.begin(), _state->layouts.end(),
                     [](const StarPointerLayout* layout) { return layout && layout->mIsPointerValid; })) return;
    State::Capture capture;
    GXGetProjectionv(capture.projection.data());
    GXGetViewportv(capture.viewport.data());
    capture.view.setInline(_state->transform->mViewMtx);
    for (s32 channel = 0; channel < 2; ++channel) {
        capture.infos[channel] = _state->controllers[channel].mInfo;
        capture.infos[channel].mDrawReady = false;
    }
    _state->pending.push_back(std::move(capture));
    _state->pending.back().id = GXAuroraRequestDepthSnapshot();
    if (_state->pending.back().id == 0) _state->pending.pop_back();
}

void StarPointerDepthOwnership::update() {
    JkrHostAllocationScope host;
    _state->input->update_samples();
    // A pending image cannot borrow a later image's camera or pointer position.
    // Consume at most the newest completed prefix, once per original movement.
    State::Capture completed;
    bool has_completed = false;
    while (!_state->pending.empty()) {
        auto& next = _state->pending.front();
        AuroraDepthSnapshotInfo info{};
        const auto status = GXAuroraGetDepthSnapshotInfo(next.id, &info);
        if (status == AURORA_DEPTH_SNAPSHOT_PENDING) break;
        if (status == AURORA_DEPTH_SNAPSHOT_READY && info.id == next.id) {
            if (has_completed) GXAuroraReleaseDepthSnapshot(completed.id);
            completed = next;
            has_completed = true;
        } else {
            GXAuroraReleaseDepthSnapshot(next.id);
        }
        _state->pending.pop_front();
    }

    struct CompletionScope {
        State& state;
        TPos3f saved_view;
        AuroraDepthSnapshotId id;
        ~CompletionScope() {
            state.transform->mViewMtx.setInline(saved_view);
            if (id != 0) GXAuroraReleaseDepthSnapshot(id);
        }
    } completion{*_state, _state->transform->mViewMtx, has_completed ? completed.id : 0};
    if (has_completed) {
        _state->transform->mViewMtx.setInline(completed.view);
        std::copy(completed.projection.begin(), completed.projection.end(), _state->peek->mProjectionParameters);
        std::copy(completed.viewport.begin(), completed.viewport.end(), _state->peek->mViewportParameters);
        for (s32 channel = 0; channel < 2; ++channel) _state->controllers[channel].mInfo = completed.infos[channel];
        {
            aurora::ScopedDepthSnapshotRead snapshot(completed.id);
            JkrAllocationScope game(_state->domain);
            _state->peek->drawSyncCallback(_state->peek->mToken);
        }
    }

    {
        JkrAllocationScope game(_state->domain);
        if (_state->camera_ready) _state->director->update();
        _state->modes->update();
    }
}

void StarPointerDepthOwnership::initialize_layouts() {
    if (_state->guidance) return;
    JkrHostAllocationScope host;
    const auto marker = mark_name_obj_runtime_registrations();
    try {
        {
            JkrAllocationScope game(_state->domain);
            _state->director->createLayout();
        }
        _state->layouts = {_state->director->mStarPointerLayouts, _state->director->mStarPointerLayouts + 1};
        _state->guidance = _state->director->mGuidance;
        _state->layout_objects = snapshot_name_obj_runtime_objects_since(marker);
        std::erase_if(_state->layout_objects, [](const NameObj* object) { return name_obj_runtime_ownership_is_claimed(object); });
        for (auto* object : _state->layout_objects) claim_name_obj_runtime_ownership(object, this);
    } catch (...) {
        _state->rollback_layouts(marker);
        throw;
    }
}

void StarPointerDepthOwnership::draw() {
    if (_state->guidance) {
        JkrAllocationScope game(_state->domain);
        _state->director->draw();
    }
}

StarPointerDirector& StarPointerDepthOwnership::director() { return *_state->director; }
StarPointerOnOffController& StarPointerDepthOwnership::modes() { return *_state->modes; }

StarPointerController& StarPointerDepthOwnership::controller(s32 channel) {
    if (channel < 0 || channel >= 2)
        aurora::throw_host_exception<std::out_of_range>("StarPointer controller channel is outside the original two slots.");
    return _state->controllers[channel];
}

StarPointerTransformHolder& StarPointerDepthOwnership::transform() { return *_state->transform; }
StarPointerGuidance* StarPointerDepthOwnership::guidance() const noexcept { return _state->guidance; }

TVec3f& StarPointerDepthOwnership::world_position(s32 channel) {
    auto& record = controller(channel);
    if (!_state->layouts[channel])
        aurora::throw_host_exception<std::logic_error>(
            "Pointer world-depth queries require the actual cursor layout owner and draw validity.");
    return record.mWorldPos;
}

void StarPointerDepthOwnership::discard_depth_samples() {
    JkrHostAllocationScope host;
    for (const auto& capture : _state->pending) GXAuroraReleaseDepthSnapshot(capture.id);
    _state->pending.clear();
    for (s32 port = 0; port < 2; ++port) _state->controllers[port].mInfo.mDrawReady = false;
}

StarPointerSceneBinding::StarPointerSceneBinding() : _owner(current_owner), _previous(current_scene_binding) {
    if (!_owner) return;
    _owner->initialize_layouts();
    _previous_transform_update = _owner->director().mIsUpdateTransHolder;
    if (!_previous || _previous->_owner != _owner) _owner->modes().setStateToBase(this);
    _owner->modes().incModeCounter(this, StarPointerMode_Game);
    _owner->director().init();
    current_scene_binding = this;
}
StarPointerSceneBinding::~StarPointerSceneBinding() {
    if (!_owner) return;
    _owner->modes().popState(this);
    _owner->discard_depth_samples();
    _owner->director().mIsUpdateTransHolder = _previous_transform_update;
    current_scene_binding = _previous;
}

StarPointerDepthOwnership* try_star_pointer_depth() noexcept { return current_owner; }

StarPointerDepthOwnership& require_star_pointer_depth() {
    if (!current_owner)
        aurora::throw_host_exception<std::logic_error>("Original pointer controllers require their runtime owner.");
    return *current_owner;
}
} // namespace smgpc::compat
