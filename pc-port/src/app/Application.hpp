#pragma once

#include <memory>
#include <string>

#include "Logger.hpp"
#include "RendererService.hpp"
#include "ServiceProvider.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/GameSystemSceneControllerService.hpp"
#include "scene/GameSystemService.hpp"
#include "scene/SceneLifecycleService.hpp"
#include "scene/SequenceBootService.hpp"
#include "scene/StageHostService.hpp"
#include "scene/StorySequenceService.hpp"

namespace smgpc::app {

    struct BootstrapConfiguration {
        int window_width = smgpc::render::core::kWiiLogicalFramebufferWidth;
        int window_height = smgpc::render::core::kWiiLogicalFramebufferHeight;
        std::string window_title = "SMG PC";
    };

    class IApplication {
    public:
        virtual ~IApplication() = default;
        [[nodiscard]] virtual int run() = 0;
    };

    struct ServiceGraphOverrides {
        std::unique_ptr<logging::ILogger> logger = {};
        std::unique_ptr<render::IWindowFactory> window_factory = {};
        std::unique_ptr<render::IWindowService> window_service = {};
        std::unique_ptr<render::IRendererEngine> renderer_engine = {};
        std::unique_ptr<compat::RuntimeContext> runtime_context = {};
        std::unique_ptr<compat::GameSystemSceneControllerService> scene_controller = {};
        std::unique_ptr<compat::StorySequenceService> story_sequence = {};
        std::unique_ptr<compat::StageHostService> stage_host = {};
        std::unique_ptr<compat::SequenceBootService> sequence_boot = {};
        std::unique_ptr<compat::GameSystemService> game_system = {};
        std::unique_ptr<IApplication> application = {};
    };

    using ServiceGraph = di::ServiceProvider<
        di::SingletonService<logging::ILogger>,
        di::SingletonService<render::IWindowFactory>, di::SingletonService<render::IWindowService>,
        di::SingletonService<render::IRendererEngine>,
        di::SingletonService<compat::RuntimeContext>,
        di::SingletonService<compat::DvdFileSystemService>,
        di::SingletonService<compat::WpadService>,
        di::SingletonService<compat::AudioEventService>,
        di::SingletonService<compat::EffectService>,
        di::SingletonService<compat::ImageEffectService>,
        di::SingletonService<compat::StarPointerService>,
        di::SingletonService<compat::CameraSystemService>,
        di::SingletonService<compat::PlayerSystemService>,
        di::SingletonService<compat::GameLayoutService>,
        di::SingletonService<compat::RumbleService>,
        di::SingletonService<compat::SequenceRequestService>,
        di::SingletonService<compat::SaveDataService>,
        di::SingletonService<compat::MessageService>,
        di::SingletonService<compat::SceneLightService>,
        di::SingletonService<compat::RflService>,
        di::SingletonService<compat::SceneScheduler>,
        di::SingletonService<compat::SceneLifecycleService>,
        di::SingletonService<compat::GameSystemSceneControllerService>,
        di::SingletonService<compat::StorySequenceService>,
        di::SingletonService<compat::StageHostService>,
        di::SingletonService<compat::SequenceBootService>,
        di::SingletonService<compat::GameSystemService>,
        di::SingletonService<IApplication>>;

    [[nodiscard]] ServiceGraph build_service_graph(const BootstrapConfiguration &configuration);
    [[nodiscard]] ServiceGraph build_service_graph(const BootstrapConfiguration &configuration, ServiceGraphOverrides &&overrides);

}  // namespace smgpc::app
