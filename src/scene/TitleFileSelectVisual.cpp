#include <aurora/exception.hpp>
#include "scene/TitleFileSelectVisual.hpp"

#include "Game/Map/FileSelectSky.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/SceneNameObjListExecutor.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/SceneExecutionBinding.hpp"
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

    TitleFileSelectSceneOwnership::TitleFileSelectSceneOwnership(smgpc::runtime::RuntimeContext& runtime)
        : _runtime(&runtime), _domain(smgpc::compat::JkrAllocationDomain::create(runtime.host_heaps(), 8U << 20)) {
        const smgpc::compat::JkrHostAllocationScope host;
        try {
            {
                const smgpc::compat::JkrAllocationScope game(_domain);
                _holder = std::make_unique<SceneObjHolder>();
                _executor = std::make_unique<SceneNameObjListExecutor>();
                _executor->init();
            }
            _binding = std::make_unique<SceneObjHolderBinding>(*_holder, nullptr, nullptr, _domain);
            _execution = std::make_unique<SceneExecutionBinding>(runtime.scheduler(), *_executor, _domain);
            _binding->initialize_effect_system(3072, 256);
        } catch (...) {
            retire();
            throw;
        }
    }

    TitleFileSelectSceneOwnership::~TitleFileSelectSceneOwnership() { retire(); }
    void TitleFileSelectSceneOwnership::prepare_retirement() {
        if (_execution) _execution->prepare_retirement();
    }
    void TitleFileSelectSceneOwnership::retire() {
        prepare_retirement();
        _binding.reset();
        _execution.reset();
        _executor.reset();
        _holder.reset();
        _domain.reset();
        if (smgpc::runtime::RuntimeContext::try_instance() == _runtime)
            _runtime->game_layout().deactivate_game_scene_draw_3d();
    }
    void TitleFileSelectSceneOwnership::complete_initialization() {
        _execution->complete_initialization();
        _binding->complete_initialization();
    }
    const std::shared_ptr<smgpc::compat::JkrAllocationDomain>& TitleFileSelectSceneOwnership::domain() const { return _domain; }

    TitleFileSelectVisualHandoff::TitleFileSelectVisualHandoff(
        smgpc::runtime::RuntimeContext &runtime,
        std::shared_ptr<TitleFileSelectSceneOwnership> ownership,
        std::unique_ptr<FileSelectSky> sky)
        : _runtime(&runtime), _scene_ownership(std::move(ownership)), _sky(std::move(sky)) {
        if (_sky == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>(
                "Title/File Select handoff requires the retained sky actor.");
        }
    }

    TitleFileSelectVisualHandoff::TitleFileSelectVisualHandoff(
        TitleFileSelectVisualHandoff &&other) noexcept
        : _runtime(std::exchange(other._runtime, nullptr)),
          _scene_ownership(std::move(other._scene_ownership)), _sky(std::move(other._sky)) {
    }

    TitleFileSelectVisualHandoff::~TitleFileSelectVisualHandoff() {
        if (_scene_ownership && _scene_ownership.use_count() == 1) _scene_ownership->prepare_retirement();
        _sky.reset();
        _scene_ownership.reset();
    }

    FileSelectSky *TitleFileSelectVisualHandoff::sky() {
        return _sky.get();
    }

    const FileSelectSky *TitleFileSelectVisualHandoff::sky() const {
        return _sky.get();
    }

    TitleFileSelectVisual::TitleFileSelectVisual(
        smgpc::runtime::RuntimeContext &runtime, bool complete_draw_registration)
        : _runtime(&runtime), _title_camera(cTitleCamera) {
        const smgpc::compat::JkrHostAllocationScope host;
        if (smgpc::runtime::RuntimeContext::try_instance() != &runtime) {
            aurora::throw_host_exception<std::logic_error>(
                "Title/File Select visuals require their active RuntimeContext.");
        }

        _scene_ownership = std::make_shared<TitleFileSelectSceneOwnership>(runtime);
        {
            const smgpc::compat::JkrAllocationScope game(_scene_ownership->domain());
            _sky = std::make_unique<FileSelectSky>("ファイルセレクト画面の空");
            _sky->initWithoutIter();
            _sky->appear();
        }
        if (complete_draw_registration) complete_initialization();
        _runtime->camera_system().set_game_camera_pose(_title_camera);
        _runtime->set_scene_camera_pose(_title_camera);
        _runtime->game_layout().activate_game_scene_draw_3d();
    }

    TitleFileSelectVisual::~TitleFileSelectVisual() {
        if (_scene_ownership && _scene_ownership.use_count() == 1) _scene_ownership->prepare_retirement();
        _sky.reset();
        _scene_ownership.reset();
    }

    void TitleFileSelectVisual::complete_initialization() { _scene_ownership->complete_initialization(); }
    std::shared_ptr<TitleFileSelectSceneOwnership> TitleFileSelectVisual::scene_ownership() const { return _scene_ownership; }

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
            aurora::throw_host_exception<std::logic_error>(
                "Title/File Select sky ownership was already transferred.");
        }
        _transferred = true;
        return TitleFileSelectVisualHandoff(*_runtime, std::move(_scene_ownership), std::move(_sky));
    }

}  // namespace smgpc::scene
