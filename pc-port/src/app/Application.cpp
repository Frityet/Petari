#include "Application.hpp"

#include "Game/Map/FileSelector.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/compat/RuntimeContext.hpp"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace smgpc::app {
    namespace {

        struct OneShotScreenshotRequest {
            std::filesystem::path path;
            std::uint64_t frame = 1U;
            bool exit_after_capture = false;
        };

        [[nodiscard]] std::optional< std::uint64_t > parse_frame_index(std::string_view text) {
            auto value = std::uint64_t{};
            const auto* begin = text.data();
            const auto* end = begin + text.size();
            const auto result = std::from_chars(begin, end, value);
            if (result.ec != std::errc{} || result.ptr != end) {
                return std::nullopt;
            }

            return value;
        }

        [[nodiscard]] std::optional< OneShotScreenshotRequest > screenshot_request_from_environment() {
            const auto* path_value = std::getenv("SMGPC_SCREENSHOT_PATH");
            if (path_value == nullptr || path_value[0] == '\0') {
                return std::nullopt;
            }

            auto request = OneShotScreenshotRequest{
                .path = std::filesystem::path(path_value),
                .frame = 1U,
            };

            const auto* frame_value = std::getenv("SMGPC_SCREENSHOT_FRAME");
            if (frame_value != nullptr && frame_value[0] != '\0') {
                if (const auto parsed = parse_frame_index(frame_value); parsed.has_value()) {
                    request.frame = *parsed;
                }
            }

            const auto* exit_after_capture_value = std::getenv("SMGPC_EXIT_AFTER_SCREENSHOT");
            request.exit_after_capture = exit_after_capture_value != nullptr && std::string_view(exit_after_capture_value) == "1";

            return request;
        }

        class DesktopApplication final : public IApplication {
        public:
            DesktopApplication(di::DependencyReference< render::IWindowService > window_service,
                               di::DependencyReference< render::IRendererEngine > renderer_engine, di::DependencyReference< logging::ILogger > logger)
                : _window_service(std::move(window_service)), _renderer_engine(std::move(renderer_engine)), _logger(std::move(logger)) {
            }

            [[nodiscard]] int run() override {
                _logger->info(logging::Category::APP, logging::Message{"Running original FileSelector title compatibility slice"});
                _logger->info(logging::Category::APP, logging::Message{"Hold keyboard A+B or Enter+Backspace to satisfy Wii A+B title input"});

                const auto screenshot_request = screenshot_request_from_environment();
                auto screenshot_queued = false;
                if (screenshot_request.has_value()) {
                    _logger->info(logging::Category::APP, logging::Message{"Will write a renderer PNG screenshot to {} on frame {}"},
                                  screenshot_request->path.string(), screenshot_request->frame);
                }

                auto runtime = game::RuntimeContext(_logger.get(), _window_service.get());
                auto file_selector = FileSelector();
                auto sent_auto_rush_begin = false;

                while (_window_service->poll_events()) {
                    const auto frame_context = _renderer_engine->begin_frame();
                    runtime.begin_frame(frame_context);
                    if (!sent_auto_rush_begin) {
                        file_selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN);
                        sent_auto_rush_begin = true;
                    }
                    file_selector.update();
                    file_selector.draw(_renderer_engine.get());
                    runtime.draw_layouts(_renderer_engine.get());
                    if (screenshot_request.has_value() && !screenshot_queued && frame_context.frame_index >= screenshot_request->frame) {
                        _renderer_engine->request_screenshot_png(screenshot_request->path);
                        screenshot_queued = true;
                    }
                    if (screenshot_request.has_value() && screenshot_request->exit_after_capture && screenshot_queued &&
                        frame_context.frame_index >= screenshot_request->frame + 4U) {
                        _window_service->close();
                    }
                    _renderer_engine->end_frame();
                }

                _renderer_engine->shutdown();
                return 0;
            }

        private:
            di::DependencyReference< render::IWindowService > _window_service;
            di::DependencyReference< render::IRendererEngine > _renderer_engine;
            di::DependencyReference< logging::ILogger > _logger;
        };

    }  // namespace

    ServiceGraph build_service_graph(const BootstrapConfiguration& configuration) {
        return build_service_graph(configuration, ServiceGraphOverrides{});
    }

    ServiceGraph build_service_graph(const BootstrapConfiguration& configuration, ServiceGraphOverrides&& overrides) {
        ServiceGraph graph{};

        if (overrides.logger) {
            graph.register_service< di::SingletonService< logging::ILogger > >(std::move(overrides.logger));
        } else {
            graph.register_service< di::SingletonService< logging::ILogger > >(logging::create_default_logger());
        }

        if (overrides.window_factory) {
            graph.register_service< di::SingletonService< render::IWindowFactory > >(std::move(overrides.window_factory));
        } else {
            graph.register_service< di::SingletonService< render::IWindowFactory >, logging::ILogger >(
                [](di::DependencyReference< logging::ILogger > logger) { return render::create_default_window_factory(std::move(logger)); });
        }

        if (overrides.window_service) {
            graph.register_service< di::SingletonService< render::IWindowService > >(std::move(overrides.window_service));
        } else {
            graph.register_service< di::SingletonService< render::IWindowService >, render::IWindowFactory >(
                [configuration](di::DependencyReference< render::IWindowFactory > window_factory) {
                    return window_factory->create(render::WindowConfiguration{
                        .width = configuration.window_width,
                        .height = configuration.window_height,
                        .title = configuration.window_title,
                    });
                });
        }

        if (overrides.renderer_engine) {
            graph.register_service< di::SingletonService< render::IRendererEngine > >(std::move(overrides.renderer_engine));
        } else {
            graph.register_service< di::SingletonService< render::IRendererEngine >, render::IWindowService, logging::ILogger >(
                [](di::DependencyReference< render::IWindowService > window_service, di::DependencyReference< logging::ILogger > logger) {
                    return render::create_default_renderer_engine(std::move(window_service), std::move(logger));
                });
        }

        if (overrides.application) {
            graph.register_service< di::SingletonService< IApplication > >(std::move(overrides.application));
        } else {
            graph.register_service< di::SingletonService< IApplication >, render::IWindowService, render::IRendererEngine, logging::ILogger >(
                [](di::DependencyReference< render::IWindowService > window_service,
                   di::DependencyReference< render::IRendererEngine > renderer_engine, di::DependencyReference< logging::ILogger > logger) {
                    return std::make_unique< DesktopApplication >(std::move(window_service), std::move(renderer_engine), std::move(logger));
                });
        }

        return graph;
    }

}  // namespace smgpc::app
