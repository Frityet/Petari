#include "scene/TitleFileSelectRoute.hpp"

#include "Game/Screen/TitleSequenceProduct.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/FileSelectFarVisual.hpp"
#include "scene/TitleFileSelectVisual.hpp"

#include <revolution/wpad.h>

#include <stdexcept>
#include <utility>

namespace smgpc::scene {

    TitleFileSelectRoute::TitleFileSelectRoute(
        smgpc::runtime::RuntimeContext &runtime)
        : _runtime(&runtime) {
        if (smgpc::runtime::RuntimeContext::try_instance() != &runtime) {
            throw std::logic_error(
                "The Title/File Select route requires its active RuntimeContext.");
        }

        // FileSelector is itself the FileSelect stage owner during both the
        // title and far phases. Install that ordinary data context before any
        // visual child resolves stage-authored resources.
        runtime.set_current_stage_name("FileSelect");
        _title_visual = std::make_unique<TitleFileSelectVisual>(runtime, false);
        // FileSelector::init constructs its camera/items before title starts;
        // SceneNameObjListExecutor allocates every draw list only afterward.
        _far_visual = std::make_unique<FileSelectFarVisual>(runtime);
        _title_sequence =
            _title_sequence_children.capture_construction_children([] {
                return std::make_unique<TitleSequenceProduct>();
            });
        runtime.scheduler().allocate_draw_buffers();
        _title_sequence->appear();
    }

    TitleFileSelectRoute::~TitleFileSelectRoute() = default;

    void TitleFileSelectRoute::update() {
        if (smgpc::runtime::RuntimeContext::try_instance() != _runtime) {
            throw std::logic_error(
                "The Title/File Select route outlived its RuntimeContext.");
        }

        switch (_state) {
        case TitleFileSelectRouteState::Title:
            _title_sequence->updateNerve();
            if (!_title_sequence->isActive()) {
                begin_move_to_far();
            }
            return;

        case TitleFileSelectRouteState::MoveToFar:
            if (_far_visual->is_at_far_point() &&
                _far_visual->numbers_visible()) {
                begin_blank_selection();
            }
            return;

        case TitleFileSelectRouteState::SelectBlank: {
            const auto previous =
                MR::testCorePadTriggerLeft(WPAD_CHAN0) ||
                MR::testCorePadTriggerUp(WPAD_CHAN0);
            const auto next =
                MR::testCorePadTriggerRight(WPAD_CHAN0) ||
                MR::testCorePadTriggerDown(WPAD_CHAN0);

            // Opposed directions in the same sampled frame are neutral.
            if (previous != next) {
                move_selection(previous ? -1 : 1);
            }

            if (MR::testSystemTriggerA()) {
                _launch_request = _selection;
                _state = TitleFileSelectRouteState::LaunchRequested;
            }
            return;
        }

        case TitleFileSelectRouteState::LaunchRequested:
            return;
        }

        throw std::logic_error("Unknown Title/File Select route state.");
    }

    TitleFileSelectRouteState TitleFileSelectRoute::state() const {
        return _state;
    }

    std::optional<TitleFileSelectRouteSelection>
    TitleFileSelectRoute::selected_slot() const {
        return _selection;
    }

    std::optional<TitleFileSelectRouteSelection>
    TitleFileSelectRoute::launch_request() const {
        return _launch_request;
    }

    FileSelectSky *TitleFileSelectRoute::sky() {
        if (_far_visual != nullptr && _far_visual->sky() != nullptr) {
            return _far_visual->sky();
        }
        return _title_visual != nullptr ? _title_visual->sky() : nullptr;
    }

    const FileSelectSky *TitleFileSelectRoute::sky() const {
        if (_far_visual != nullptr && _far_visual->sky() != nullptr) {
            return _far_visual->sky();
        }
        return _title_visual != nullptr ? _title_visual->sky() : nullptr;
    }

    FileSelectFarVisual *TitleFileSelectRoute::far_visual() {
        return _far_visual.get();
    }

    const FileSelectFarVisual *TitleFileSelectRoute::far_visual() const {
        return _far_visual.get();
    }

    void TitleFileSelectRoute::begin_move_to_far() {
        if (_title_visual == nullptr || _far_visual == nullptr || _far_visual->sky() != nullptr) {
            throw std::logic_error(
                "The Title/File Select sky can only enter the far phase once.");
        }

        auto handoff = _title_visual->release_sky_for_file_select();
        _far_visual->begin_far(std::move(handoff));
        _title_visual.reset();
        _state = TitleFileSelectRouteState::MoveToFar;
    }

    void TitleFileSelectRoute::begin_blank_selection() {
        const auto slots = _far_visual->slots();
        if (slots.empty()) {
            throw std::logic_error(
                "The File Select far composition exposed no selectable blank slot.");
        }

        _selection = TitleFileSelectRouteSelection{
            .visual_index = 0U,
            .file_number = slots.front().file_number,
        };
        _far_visual->set_highlighted_slot(0U);
        _state = TitleFileSelectRouteState::SelectBlank;
    }

    void TitleFileSelectRoute::move_selection(std::ptrdiff_t direction) {
        if (!_selection.has_value() || direction == 0) {
            return;
        }

        const auto slots = _far_visual->slots();
        if (slots.empty() || _selection->visual_index >= slots.size()) {
            throw std::logic_error(
                "The selected blank File Select slot is no longer available.");
        }

        const auto previous_index = _selection->visual_index;
        const auto slot_count = static_cast<std::ptrdiff_t>(slots.size());
        auto next_index =
            static_cast<std::ptrdiff_t>(previous_index) + direction;
        next_index = (next_index % slot_count + slot_count) % slot_count;
        const auto next = static_cast<std::size_t>(next_index);

        _far_visual->set_highlighted_slot(next);
        _selection = TitleFileSelectRouteSelection{
            .visual_index = next,
            .file_number = slots[next].file_number,
        };
    }

}  // namespace smgpc::scene
