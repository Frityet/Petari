#pragma once

#include "scene/TitleFileSelectVisual.hpp"

#include <JSystem/JGeometry/TMatrix.hpp>
#include <JSystem/JGeometry/TVec.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>

class FileSelectCameraController;
class FileSelectNumber;
class FileSelectSky;
class PartsModel;

namespace smgpc::runtime {
    class RuntimeContext;
}

namespace smgpc::scene {

    struct FileSelectFarSlotView final {
        s32 file_number = 0;
        TVec3f base_position{};
        PartsModel *planet = nullptr;
        FileSelectNumber *number = nullptr;
        const TPos3f *planet_matrix = nullptr;
    };

    // Scene-owned far File Select composition. This deliberately uses the
    // retail camera, planet PartsModel and number layout while leaving the
    // RFL-dependent FileSelectItem face branch to its eventual exact owner.
    class FileSelectFarVisual final {
    public:
        FileSelectFarVisual(smgpc::runtime::RuntimeContext &runtime,
                            TitleFileSelectVisualHandoff &&handoff);
        ~FileSelectFarVisual();

        FileSelectFarVisual(const FileSelectFarVisual &) = delete;
        FileSelectFarVisual &operator=(const FileSelectFarVisual &) = delete;
        FileSelectFarVisual(FileSelectFarVisual &&) = delete;
        FileSelectFarVisual &operator=(FileSelectFarVisual &&) = delete;

        [[nodiscard]] FileSelectSky *sky();
        [[nodiscard]] const FileSelectSky *sky() const;
        [[nodiscard]] FileSelectCameraController *camera_controller();
        [[nodiscard]] const FileSelectCameraController *camera_controller() const;
        [[nodiscard]] bool is_at_far_point() const;
        [[nodiscard]] bool numbers_visible() const;
        [[nodiscard]] std::span<const FileSelectFarSlotView> slots() const;

        // Centralizes the visual portion of FileSelectItem::onPointing and
        // offPointing. Input, sound, rumble and save/RFL policy remain with
        // the route that chooses the highlighted authored slot.
        void set_highlighted_slot(std::optional<std::size_t> slot_index);
        [[nodiscard]] std::optional<std::size_t> highlighted_slot() const;

    private:
        struct Impl;

        // Impl must retire its scheduler children and StageLight binding before
        // the handoff releases the exact sky and deactivates scene 3D drawing.
        smgpc::runtime::RuntimeContext *_runtime = nullptr;
        TitleFileSelectVisualHandoff _handoff;
        std::unique_ptr<Impl> _impl{};
    };

}  // namespace smgpc::scene
