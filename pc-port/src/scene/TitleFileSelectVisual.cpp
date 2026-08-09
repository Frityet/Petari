#include "scene/TitleFileSelectVisual.hpp"

#include "Game/Map/FileSelectSky.hpp"
#include "runtime/RuntimeContext.hpp"

#include <stdexcept>
#include <utility>

namespace smgpc::scene {

    namespace {
        // FileSelectCameraController::exeTitle authored values. Its title eye
        // and watch point share the 15,800-unit Y plane before the 60-frame
        // transition to the ordinary file-select far point.
        constexpr auto cTitleCamera = smgpc::camera::CameraPose{
            .eye = {0.0F, 15800.0F, 15000.0F},
            .watch = {0.0F, 15800.0F, 0.0F},
            .up = {0.0F, 1.0F, 0.0F},
            .fovy_degrees = 60.0F,
        };
    }  // namespace

    TitleFileSelectVisualHandoff::TitleFileSelectVisualHandoff(
        smgpc::runtime::RuntimeContext &runtime,
        std::unique_ptr<FileSelectSky> sky)
        : _runtime(&runtime), _sky(std::move(sky)) {
        if (_sky == nullptr) {
            throw std::invalid_argument(
                "Title/File Select handoff requires the retained sky actor.");
        }
    }

    TitleFileSelectVisualHandoff::TitleFileSelectVisualHandoff(
        TitleFileSelectVisualHandoff &&other) noexcept
        : _runtime(std::exchange(other._runtime, nullptr)),
          _sky(std::move(other._sky)) {
    }

    TitleFileSelectVisualHandoff::~TitleFileSelectVisualHandoff() {
        _sky.reset();
        if (_runtime != nullptr &&
            smgpc::runtime::RuntimeContext::try_instance() == _runtime) {
            _runtime->game_layout().deactivate_game_scene_draw_3d();
        }
    }

    FileSelectSky *TitleFileSelectVisualHandoff::sky() {
        return _sky.get();
    }

    const FileSelectSky *TitleFileSelectVisualHandoff::sky() const {
        return _sky.get();
    }

    TitleFileSelectVisual::TitleFileSelectVisual(
        smgpc::runtime::RuntimeContext &runtime)
        : _runtime(&runtime), _title_camera(cTitleCamera) {
        if (smgpc::runtime::RuntimeContext::try_instance() != &runtime) {
            throw std::logic_error(
                "Title/File Select visuals require their active RuntimeContext.");
        }

        auto sky = std::make_unique<FileSelectSky>("ファイルセレクト画面の空");
        sky->initWithoutIter();
        sky->appear();

        _sky = std::move(sky);
        _runtime->camera_system().set_game_camera_pose(_title_camera);
        _runtime->set_scene_camera_pose(_title_camera);
        _runtime->game_layout().activate_game_scene_draw_3d();
    }

    TitleFileSelectVisual::~TitleFileSelectVisual() {
        _sky.reset();
        if (!_transferred &&
            smgpc::runtime::RuntimeContext::try_instance() == _runtime) {
            _runtime->game_layout().deactivate_game_scene_draw_3d();
        }
    }

    const smgpc::camera::CameraPose &TitleFileSelectVisual::title_camera() const {
        return _title_camera;
    }

    FileSelectSky *TitleFileSelectVisual::sky() {
        return _sky.get();
    }

    const FileSelectSky *TitleFileSelectVisual::sky() const {
        return _sky.get();
    }

    TitleFileSelectVisualHandoff
    TitleFileSelectVisual::release_sky_for_file_select() {
        if (_sky == nullptr) {
            throw std::logic_error(
                "Title/File Select sky ownership was already transferred.");
        }
        _transferred = true;
        return TitleFileSelectVisualHandoff(*_runtime, std::move(_sky));
    }

}  // namespace smgpc::scene
