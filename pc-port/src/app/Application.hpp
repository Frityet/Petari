#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Logger.hpp"
#include "RendererService.hpp"
#include "ServiceProvider.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/GameSystemSceneControllerService.hpp"
#include "scene/GameSystemService.hpp"
#include "scene/NameObjLifecycleService.hpp"
#include "scene/SceneExecutionService.hpp"
#include "scene/SceneLifecycleService.hpp"
#include "scene/SequenceBootService.hpp"
#include "scene/StageHostService.hpp"
#include "scene/StorySequenceService.hpp"

namespace smgpc::app {

    struct BootstrapConfiguration {
        int window_width = smgpc::render::core::kWiiLogicalFramebufferWidth;
        int window_height = smgpc::render::core::kWiiLogicalFramebufferHeight;
        std::string window_title = "SMG PC";
        std::vector<std::string> arguments = {};
        std::optional<std::filesystem::path> disc_image = {};
    };

    class IApplication {
    public:
        virtual ~IApplication() = default;
        [[nodiscard]] virtual int run() = 0;
    };

    struct ServiceGraphOverrides {
        std::unique_ptr<logging::ILogger> logger = {};
        std::unique_ptr<render::AuroraWindow> window_service = {};
        std::unique_ptr<render::AuroraRenderer> renderer_engine = {};
        std::unique_ptr<smgpc::runtime::RuntimeContext> runtime_context = {};
        std::unique_ptr<smgpc::scene::GameSystemSceneControllerService> scene_controller = {};
        std::unique_ptr<smgpc::scene::StorySequenceService> story_sequence = {};
        std::unique_ptr<smgpc::scene::StageHostService> stage_host = {};
        std::unique_ptr<smgpc::scene::SequenceBootService> sequence_boot = {};
        std::unique_ptr<smgpc::scene::GameSystemService> game_system = {};
        std::unique_ptr<IApplication> application = {};
    };

    using ServiceGraph = di::ServiceProvider<
        di::SingletonService<logging::ILogger>,
        di::SingletonService<render::AuroraWindow>,
        di::SingletonService<render::AuroraRenderer>,
        di::SingletonService<smgpc::runtime::RuntimeContext>,
        di::SingletonService<smgpc::runtime::DvdFileSystemService>,
        di::SingletonService<smgpc::runtime::WiiIosService>,
        di::SingletonService<smgpc::runtime::WiiPlatformService>,
        di::SingletonService<smgpc::runtime::WiiVideoService>,
        di::SingletonService<smgpc::runtime::WpadService>,
        di::SingletonService<smgpc::runtime::AudioEventService>,
        di::SingletonService<smgpc::runtime::EffectService>,
        di::SingletonService<smgpc::runtime::ImageEffectService>,
        di::SingletonService<smgpc::runtime::StarPointerService>,
        di::SingletonService<smgpc::runtime::CameraSystemService>,
        di::SingletonService<smgpc::runtime::PlayerSystemService>,
        di::SingletonService<smgpc::runtime::GameLayoutService>,
        di::SingletonService<smgpc::runtime::RumbleService>,
        di::SingletonService<smgpc::runtime::SequenceRequestService>,
        di::SingletonService<smgpc::runtime::SysConfigService>,
        di::SingletonService<smgpc::runtime::SaveDataService>,
        di::SingletonService<smgpc::runtime::NandFileSystemService>,
        di::SingletonService<smgpc::runtime::MessageService>,
        di::SingletonService<smgpc::runtime::SceneLightService>,
        di::SingletonService<smgpc::runtime::RflService>,
        di::SingletonService<smgpc::runtime::SceneScheduler>,
        di::SingletonService<smgpc::scene::NameObjLifecycleService>,
        di::SingletonService<smgpc::scene::SceneExecutionService>,
        di::SingletonService<smgpc::scene::SceneLifecycleService>,
        di::SingletonService<smgpc::scene::GameSystemSceneControllerService>,
        di::SingletonService<smgpc::scene::StorySequenceService>,
        di::SingletonService<smgpc::scene::StageHostService>,
        di::SingletonService<smgpc::scene::SequenceBootService>,
        di::SingletonService<smgpc::scene::GameSystemService>,
        di::SingletonService<IApplication>>;

    [[nodiscard]] ServiceGraph build_service_graph(const BootstrapConfiguration &configuration);
    [[nodiscard]] ServiceGraph build_service_graph(const BootstrapConfiguration &configuration, ServiceGraphOverrides &&overrides);
    [[nodiscard]] std::filesystem::path required_disc_image(const BootstrapConfiguration &configuration);
    void ensure_disc_image_open(const BootstrapConfiguration &configuration, logging::ILogger &logger);

}  // namespace smgpc::app
