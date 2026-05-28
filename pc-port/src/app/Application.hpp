#pragma once

#include <memory>
#include <string>

#include "Logger.hpp"
#include "Game/compat/GameSystemSceneControllerService.hpp"
#include "Game/compat/GameSystemService.hpp"
#include "Game/compat/RuntimeContext.hpp"
#include "Game/compat/RuntimeServices.hpp"
#include "Game/compat/SceneLifecycleService.hpp"
#include "Game/compat/SceneScheduler.hpp"
#include "Game/compat/SequenceBootService.hpp"
#include "Game/compat/StageHostService.hpp"
#include "Game/compat/StorySequenceService.hpp"
#include "RendererService.hpp"
#include "ServiceProvider.hpp"

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
        std::unique_ptr<game::RuntimeContext> runtime_context = {};
        std::unique_ptr<game::GameSystemSceneControllerService> scene_controller = {};
        std::unique_ptr<game::StorySequenceService> story_sequence = {};
        std::unique_ptr<game::StageHostService> stage_host = {};
        std::unique_ptr<game::SequenceBootService> sequence_boot = {};
        std::unique_ptr<game::GameSystemService> game_system = {};
        std::unique_ptr<IApplication> application = {};
    };

    using ServiceGraph = di::ServiceProvider<
        di::SingletonService<logging::ILogger>,
        di::SingletonService<render::IWindowFactory>, di::SingletonService<render::IWindowService>,
        di::SingletonService<render::IRendererEngine>,
        di::SingletonService<game::RuntimeContext>,
        di::SingletonService<game::DvdFileSystemService>,
        di::SingletonService<game::WpadService>,
        di::SingletonService<game::AudioEventService>,
        di::SingletonService<game::EffectService>,
        di::SingletonService<game::ImageEffectService>,
        di::SingletonService<game::StarPointerService>,
        di::SingletonService<game::CameraSystemService>,
        di::SingletonService<game::PlayerSystemService>,
        di::SingletonService<game::GameLayoutService>,
        di::SingletonService<game::RumbleService>,
        di::SingletonService<game::SequenceRequestService>,
        di::SingletonService<game::SaveDataService>,
        di::SingletonService<game::MessageService>,
        di::SingletonService<game::SceneLightService>,
        di::SingletonService<game::RflService>,
        di::SingletonService<game::SceneScheduler>,
        di::SingletonService<game::SceneLifecycleService>,
        di::SingletonService<game::GameSystemSceneControllerService>,
        di::SingletonService<game::StorySequenceService>,
        di::SingletonService<game::StageHostService>,
        di::SingletonService<game::SequenceBootService>,
        di::SingletonService<game::GameSystemService>,
        di::SingletonService<IApplication>
    >;

    [[nodiscard]] ServiceGraph build_service_graph(const BootstrapConfiguration &configuration);
    [[nodiscard]] ServiceGraph build_service_graph(const BootstrapConfiguration &configuration, ServiceGraphOverrides &&overrides);

}  // namespace smgpc::app
