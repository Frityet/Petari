#pragma once

#include "camera/CameraPose.hpp"

#include <memory>

class FileSelectSky;

namespace smgpc::runtime {
    class RuntimeContext;
}

namespace smgpc::scene {

    class TitleFileSelectVisualHandoff final {
    public:
        TitleFileSelectVisualHandoff(TitleFileSelectVisualHandoff &&other) noexcept;
        ~TitleFileSelectVisualHandoff();

        TitleFileSelectVisualHandoff(const TitleFileSelectVisualHandoff &) = delete;
        TitleFileSelectVisualHandoff &operator=(
            const TitleFileSelectVisualHandoff &) = delete;
        TitleFileSelectVisualHandoff &operator=(
            TitleFileSelectVisualHandoff &&) = delete;

        [[nodiscard]] FileSelectSky *sky();
        [[nodiscard]] const FileSelectSky *sky() const;

    private:
        friend class TitleFileSelectVisual;
        TitleFileSelectVisualHandoff(
            smgpc::runtime::RuntimeContext &runtime,
            std::unique_ptr<FileSelectSky> sky);

        smgpc::runtime::RuntimeContext *_runtime = nullptr;
        std::unique_ptr<FileSelectSky> _sky{};
    };

    // Scene-owned visual composition shared by the retail title and the later
    // file-select phase. Rendering remains owned by FileSelectSky's ordinary
    // LiveActor scheduler registration.
    class TitleFileSelectVisual final {
    public:
        explicit TitleFileSelectVisual(smgpc::runtime::RuntimeContext &runtime, bool complete_draw_registration = true);
        ~TitleFileSelectVisual();

        TitleFileSelectVisual(const TitleFileSelectVisual &) = delete;
        TitleFileSelectVisual &operator=(const TitleFileSelectVisual &) = delete;

        [[nodiscard]] const smgpc::camera::CameraPose &title_camera() const;
        [[nodiscard]] FileSelectSky *sky();
        [[nodiscard]] const FileSelectSky *sky() const;

        // Transfers the same retail sky actor to the future File Select owner.
        // The actor remains registered with the scheduler until that new sole
        // owner destroys it.
        [[nodiscard]] TitleFileSelectVisualHandoff release_sky_for_file_select();

    private:
        smgpc::runtime::RuntimeContext *_runtime = nullptr;
        smgpc::camera::CameraPose _title_camera{};
        std::unique_ptr<FileSelectSky> _sky{};
        bool _transferred = false;
    };

}  // namespace smgpc::scene
