#pragma once

#include "scene/NameObjChildOwner.hpp"

#include <revolution/types.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

class FileSelectSky;
class TitleSequenceProduct;

namespace smgpc::runtime {
    class RuntimeContext;
}

namespace smgpc::scene {

    class FileSelectFarVisual;
    class TitleFileSelectVisual;

    enum class TitleFileSelectRouteState : std::uint8_t {
        Title,
        MoveToFar,
        SelectBlank,
        LaunchRequested,
    };

    struct TitleFileSelectRouteSelection final {
        std::size_t visual_index = 0U;
        s32 file_number = 0;

        bool operator==(const TitleFileSelectRouteSelection &) const = default;
    };

    // Owns the bounded title-to-blank-file-select route. The retail title
    // component remains alive for the route's entire lifetime, as it does
    // under FileSelector, while the same FileSelectSky actor transfers to the
    // far composition exactly once.
    //
    // This route intentionally stops at a blank-slot launch request. Save
    // creation, RFL/Mii selection and the destination scene remain owned by
    // their eventual sequence host.
    class TitleFileSelectRoute final {
    public:
        explicit TitleFileSelectRoute(
            smgpc::runtime::RuntimeContext &runtime);
        ~TitleFileSelectRoute();

        TitleFileSelectRoute(const TitleFileSelectRoute &) = delete;
        TitleFileSelectRoute &operator=(const TitleFileSelectRoute &) = delete;
        TitleFileSelectRoute(TitleFileSelectRoute &&) = delete;
        TitleFileSelectRoute &operator=(TitleFileSelectRoute &&) = delete;

        // Call once after RuntimeContext::begin_frame(). This preserves the
        // scheduler-owned camera/number projection order and consumes only
        // the already-sampled WPAD edge state.
        void update();

        [[nodiscard]] TitleFileSelectRouteState state() const;
        [[nodiscard]] std::optional<TitleFileSelectRouteSelection>
        selected_slot() const;
        [[nodiscard]] std::optional<TitleFileSelectRouteSelection>
        launch_request() const;

        [[nodiscard]] FileSelectSky *sky();
        [[nodiscard]] const FileSelectSky *sky() const;
        [[nodiscard]] FileSelectFarVisual *far_visual();
        [[nodiscard]] const FileSelectFarVisual *far_visual() const;

    private:
        void begin_move_to_far();
        void begin_blank_selection();
        void move_selection(std::ptrdiff_t direction);

        smgpc::runtime::RuntimeContext *_runtime = nullptr;
        // This owner precedes the product so reverse member destruction first
        // retires the parent and then its captured raw-new NameObj children.
        NameObjChildOwner _title_sequence_children{};
        std::unique_ptr<TitleSequenceProduct> _title_sequence{};
        std::unique_ptr<TitleFileSelectVisual> _title_visual{};
        std::unique_ptr<FileSelectFarVisual> _far_visual{};
        TitleFileSelectRouteState _state =
            TitleFileSelectRouteState::Title;
        std::optional<TitleFileSelectRouteSelection> _selection{};
        std::optional<TitleFileSelectRouteSelection> _launch_request{};
    };

}  // namespace smgpc::scene
