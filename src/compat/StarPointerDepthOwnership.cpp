#include "compat/StarPointerDepthOwnership.hpp"

#include "Game/Screen/StarPointerController.hpp"
#include "Game/Screen/StarPointerDirector.hpp"
#include "Game/Screen/StarPointerGuidance.hpp"
#include "Game/Screen/StarPointerLayout.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "compat/JkrAllocationDomain.hpp"
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
}

struct StarPointerDepthOwnership::State {
    struct Capture {
        AuroraDepthSnapshotId id = 0;
        std::array<DpdInfo, 2> infos;
        TPos3f view;
        std::array<f32, 7> projection;
        std::array<f32, 6> viewport;
    };

    explicit State(std::shared_ptr<JkrHeapRuntime> heaps)
        : domain(JkrAllocationDomain::create(std::move(heaps), 1024U * 1024U)) {
        JkrAllocationScope game(domain);
        controllers = new StarPointerController[2];
        transform = new StarPointerTransformHolder;
        peek = new StarPointerPeekZ;
        for (s32 channel = 0; channel < 2; ++channel) {
            controllers[channel].initAndSetPort(channel);
            peek->mInfos[channel] = &controllers[channel].mInfo;
        }
    }

    ~State() {
        for (const auto& capture : pending) GXAuroraReleaseDepthSnapshot(capture.id);
        if (guidance) {
            smgpc::layout::release_layout_actor_if_registered(guidance);
            delete guidance;
        }
    }

    std::shared_ptr<JkrAllocationDomain> domain;
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
    _state = std::make_unique<State>(std::move(heaps));
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
        if (_state->camera_ready) _state->transform->movement();
        for (s32 channel = 0; channel < 2; ++channel) {
            if (MR::isConnectedWPad(channel))
                _state->controllers[channel].movement(_state->peek->mProjectionParameters, _state->peek->mViewportParameters);
        }
        if (_state->guidance) {
            _state->guidance->movement();
            _state->guidance->calcAnim();
        }
    }
}

void StarPointerDepthOwnership::initialize_guidance() {
    if (_state->guidance) return;
    JkrAllocationScope game(_state->domain);
    auto* guidance = new StarPointerGuidance("スターポインタガイダンス");
    try {
        guidance->initWithoutIter();
    } catch (...) {
        smgpc::layout::release_layout_actor_if_registered(guidance);
        delete guidance;
        throw;
    }
    _state->guidance = guidance;
}

void StarPointerDepthOwnership::draw_guidance() {
    if (_state->guidance) {
        JkrAllocationScope game(_state->domain);
        _state->guidance->draw();
    }
}

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

StarPointerLayout* StarPointerDepthOwnership::bind_layout(s32 channel, StarPointerLayout* layout) {
    (void)controller(channel);
    JkrHostAllocationScope host;
    for (const auto& capture : _state->pending) GXAuroraReleaseDepthSnapshot(capture.id);
    _state->pending.clear();
    for (s32 port = 0; port < 2; ++port) _state->controllers[port].mInfo.mDrawReady = false;
    auto* previous = _state->layouts[channel];
    _state->layouts[channel] = layout;
    return previous;
}

StarPointerLayoutBinding::StarPointerLayoutBinding(StarPointerDepthOwnership& owner, s32 channel, StarPointerLayout& layout)
    : _owner(owner), _channel(channel), _previous(owner.bind_layout(channel, &layout)) {}
StarPointerLayoutBinding::~StarPointerLayoutBinding() { _owner.bind_layout(_channel, _previous); }

StarPointerDepthOwnership& require_star_pointer_depth() {
    if (!current_owner)
        aurora::throw_host_exception<std::logic_error>("Original pointer controllers require their runtime owner.");
    return *current_owner;
}
} // namespace smgpc::compat
