#include "Application.hpp"

#ifndef NDEBUG
#include "runtime/ParityTrace.hpp"
#endif

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#if defined(__GNUC__) || defined(__clang__)
extern "C" bool aurora_dvd_open(const char *disc_path) __attribute__((weak));
extern "C" void aurora_dvd_close(void) __attribute__((weak));
#endif

namespace smgpc::app {
    namespace {

        using FrameClock = std::chrono::steady_clock;
        constexpr auto WII_FRAME_RATE_HZ = 60.0;
        constexpr auto WII_FRAME_DURATION_SECONDS = 1.0 / WII_FRAME_RATE_HZ;
        constexpr auto WII_FRAME_DURATION = std::chrono::duration_cast<FrameClock::duration>(
            std::chrono::duration<double>(WII_FRAME_DURATION_SECONDS));

        bool g_disc_open = false;

        [[nodiscard]] double logical_frame_time_seconds(std::uint64_t logical_frame_index) {
            return logical_frame_index <= 1U ? 0.0 : static_cast<double>(logical_frame_index - 1U) * WII_FRAME_DURATION_SECONDS;
        }

        [[nodiscard]] std::filesystem::path default_save_directory() {
            if (const auto *xdg_data_home = std::getenv("XDG_DATA_HOME"); xdg_data_home != nullptr && xdg_data_home[0] != '\0') {
                return std::filesystem::path(xdg_data_home) / "smgpc" / "save";
            }
            if (const auto *home = std::getenv("HOME"); home != nullptr && home[0] != '\0') {
                return std::filesystem::path(home) / ".local" / "share" / "smgpc" / "save";
            }
            return std::filesystem::path(".smgpc") / "save";
        }

        template <typename Service, Service &(smgpc::runtime::RuntimeContext::*Getter)()>
        void register_runtime_service_reference(ServiceGraph &graph) {
            graph.register_service_reference<di::SingletonService<Service>, smgpc::runtime::RuntimeContext>(
                [](di::DependencyReference<smgpc::runtime::RuntimeContext> runtime) -> Service & {
                    return (runtime.get().*Getter)();
                });
        }

        class FramePacer final {
        public:
            explicit FramePacer(bool enabled) : _enabled(enabled), _next_frame_deadline(FrameClock::now() + WII_FRAME_DURATION) {
            }

            void wait_for_frame_end() {
                const auto now = FrameClock::now();
                if (!_enabled) {
                    _next_frame_deadline = now + WII_FRAME_DURATION;
                    return;
                }

                if (now < _next_frame_deadline) {
                    std::this_thread::sleep_until(_next_frame_deadline);
                }

                const auto after_wait = FrameClock::now();
                while (_next_frame_deadline <= after_wait) {
                    _next_frame_deadline += WII_FRAME_DURATION;
                }
            }

        private:
            bool _enabled = true;
            FrameClock::time_point _next_frame_deadline = {};
        };

        [[nodiscard]] std::optional<std::uint64_t> parse_frame_index(std::string_view text) {
            auto value = 0ULL;
            const auto *begin = text.data();
            const auto *end = begin + text.size();
            const auto result = std::from_chars(begin, end, value);
            if (result.ec != std::errc {} || result.ptr != end) {
                return std::nullopt;
            }
            return value;
        }

        [[nodiscard]] std::optional<std::uint64_t> exit_frame_from_environment() {
            const auto *frame_value = std::getenv("SMGPC_EXIT_AFTER_FRAME");
            if (frame_value == nullptr || frame_value[0] == '\0') {
                return std::nullopt;
            }
            return parse_frame_index(frame_value);
        }

        [[nodiscard]] bool exit_after_screenshot_from_environment() {
            const auto *value = std::getenv("SMGPC_EXIT_AFTER_SCREENSHOT");
            return value != nullptr && value[0] != '\0' && std::string_view(value) != "0";
        }

#ifndef NDEBUG
        [[nodiscard]] std::chrono::milliseconds debug_hold_after_trace_duration() {
            const auto *value = std::getenv("SMGPC_DEBUG_HOLD_AFTER_TRACE_MS");
            if (value == nullptr || value[0] == '\0') {
                return std::chrono::milliseconds {0};
            }

            auto milliseconds = 0ULL;
            const auto *begin = value;
            const auto *end = value + std::string_view(value).size();
            const auto result = std::from_chars(begin, end, milliseconds);
            if (result.ec != std::errc {} || result.ptr != end) {
                return std::chrono::milliseconds {0};
            }

            constexpr auto kMaxDebugHoldMs = 30'000ULL;
            return std::chrono::milliseconds {static_cast<std::chrono::milliseconds::rep>(std::min(milliseconds, kMaxDebugHoldMs))};
        }
#endif

        [[nodiscard]] bool frame_pacing_enabled_from_environment() {
            const auto *value = std::getenv("SMGPC_FRAME_PACING");
            return value == nullptr || std::string_view(value) != "0";
        }

        [[nodiscard]] std::optional<std::string> string_environment(const char *name) {
            const auto *value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }
            return std::string(value);
        }

#ifndef NDEBUG
        template <typename Runtime>
        [[nodiscard]] bool has_active_layout(const Runtime &runtime, std::string_view layout_name) {
            if constexpr (requires(const Runtime &value) { value.scheduler().debug_layout_runtime_snapshot(); }) {
                for (const auto &layout : runtime.scheduler().debug_layout_runtime_snapshot()) {
                    if (layout.layout_name == layout_name && !layout.dead && !layout.suspended) {
                        return true;
                    }
                }
            }
            return false;
        }

        template <typename Runtime>
        void emit_configured_semantic_anchor(Runtime &runtime) {
            const auto name = string_environment("SMGPC_SEMANTIC_ANCHOR_NAME");
            if (!name.has_value()) {
                return;
            }

            const auto category = string_environment("SMGPC_SEMANTIC_ANCHOR_CATEGORY").value_or("capture");
            const auto detail = string_environment("SMGPC_SEMANTIC_ANCHOR_DETAIL").value_or("runtime parity capture");
            if constexpr (requires(Runtime &value, const std::string &event_category, const std::string &event_name,
                                   const std::string &event_detail) {
                              value.emit_semantic_trace_event(event_category, event_name, event_detail);
                          }) {
                runtime.emit_semantic_trace_event(category, *name, detail);
            }
        }
#endif

        [[nodiscard]] std::optional<std::filesystem::path> disc_image_from_arguments(std::span<const std::string> arguments) {
            for (std::size_t i = 1U; i < arguments.size(); ++i) {
                const auto &argument = arguments[i];
                if (argument == "--disc") {
                    if (i + 1U >= arguments.size() || arguments[i + 1U].empty()) {
                        throw std::runtime_error("--disc requires a disc image path");
                    }
                    return std::filesystem::path(arguments[i + 1U]);
                }
                constexpr auto prefix = std::string_view("--disc=");
                if (argument.starts_with(prefix)) {
                    const auto value = argument.substr(prefix.size());
                    if (value.empty()) {
                        throw std::runtime_error("--disc requires a disc image path");
                    }
                    return std::filesystem::path(value);
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] std::filesystem::path required_disc_image_impl(const BootstrapConfiguration &configuration) {
            if (const auto argument_disc = disc_image_from_arguments(configuration.arguments); argument_disc.has_value()) {
                return *argument_disc;
            }
            if (configuration.disc_image.has_value()) {
                return *configuration.disc_image;
            }
            if (const auto *env_disc = std::getenv("SMGPC_DISC_IMAGE"); env_disc != nullptr && env_disc[0] != '\0') {
                return std::filesystem::path(env_disc);
            }
            throw std::runtime_error("Missing disc image. Launch with `smg-pc --disc <path>` or set SMGPC_DISC_IMAGE.");
        }

        void ensure_disc_image_open_impl(const BootstrapConfiguration &configuration, logging::ILogger &logger) {
            if (g_disc_open) {
                return;
            }

            const auto disc_image = required_disc_image_impl(configuration);
            const auto disc_path = disc_image.string();
#if defined(__GNUC__) || defined(__clang__)
            if (aurora_dvd_open == nullptr) {
                logger.warning(logging::Category::APP,
                               logging::Message {"Aurora DVD image support is not linked; accepted disc image {}"}, disc_path);
                return;
            }
            if (!aurora_dvd_open(disc_path.c_str())) {
                throw std::runtime_error("Aurora could not open disc image " + disc_path);
            }
            g_disc_open = true;
            logger.info(logging::Category::APP, logging::Message {"Opened Aurora disc image {}"}, disc_path);
#else
            logger.warning(logging::Category::APP,
                           logging::Message {"Aurora DVD image support is not available on this toolchain; accepted disc image {}"}, disc_path);
#endif
        }

        class DesktopApplication final : public IApplication {
        public:
            DesktopApplication(BootstrapConfiguration configuration, di::DependencyReference<render::AuroraWindow> window_service,
                               di::DependencyReference<render::AuroraRenderer> renderer, di::DependencyReference<logging::ILogger> logger,
                               di::DependencyReference<smgpc::runtime::RuntimeContext> runtime,
                               di::DependencyReference<smgpc::scene::GameSystemService> game_system)
                : _configuration(std::move(configuration)), _window_service(std::move(window_service)), _renderer(std::move(renderer)),
                  _logger(std::move(logger)), _runtime(std::move(runtime)), _game_system(std::move(game_system)) {
            }

            [[nodiscard]] int run() override {
                _logger->info(logging::Category::APP, logging::Message {"Running SMG on Aurora"});
                _logger->info(logging::Category::APP, logging::Message {"Hold keyboard A+B or Enter+Backspace to satisfy Wii A+B title input"});
                _logger->info(logging::Category::APP, logging::Message {
                    "Freecam debug keys: toggle F9, teleport to HeavensDoor F10, move with WASD, up/down Space/LeftShift, mouse look"});

                auto &runtime = _runtime.get();
                auto &game_system = _game_system.get();
                if (!runtime.save_data().host_directory().has_value()) {
                    const auto save_directory = default_save_directory();
                    runtime.save_data().set_host_directory(save_directory);
                    _logger->info(logging::Category::APP, logging::Message {"Using SMG save files from {}"}, save_directory.string());
                }

                const auto skip_render_until_frame = parse_frame_index(string_environment("SMGPC_SKIP_RENDER_UNTIL_FRAME").value_or(""));
#ifndef NDEBUG
                const auto exit_after_frame = exit_frame_from_environment();
                const auto exit_on_layout_name = string_environment("SMGPC_EXIT_ON_LAYOUT_NAME");
                const auto parity_trace_path = string_environment("SMGPC_PARITY_TRACE_PATH");
                const auto parity_trace_frame = parse_frame_index(string_environment("SMGPC_PARITY_TRACE_FRAME").value_or("1")).value_or(1U);
                const auto screenshot_path = string_environment("SMGPC_SCREENSHOT_PATH");
                const auto screenshot_frame = parse_frame_index(string_environment("SMGPC_SCREENSHOT_FRAME").value_or("1")).value_or(1U);
                const auto exit_after_screenshot = exit_after_screenshot_from_environment();
                const auto debug_hold_after_trace = debug_hold_after_trace_duration();
                auto parity_trace_written = false;
                auto screenshot_written = false;
                emit_configured_semantic_anchor(runtime);
                if (parity_trace_path.has_value()) {
                    runtime.set_j3d_packet_trace_frame(parity_trace_frame);
                }
                if (exit_after_frame.has_value()) {
                    _logger->info(logging::Category::APP, logging::Message {"Will exit after frame {}"}, *exit_after_frame);
                }
#endif

                auto frame_pacer = FramePacer(frame_pacing_enabled_from_environment());
                auto loop_frame_index = std::uint64_t {};
                while (_window_service->poll_events()) {
                    const auto logical_frame_index = loop_frame_index + 1U;
                    auto frame_context = _renderer->begin_frame();
                    auto renderer_context = render::ScopedAuroraRendererContext(_renderer.get());
                    frame_context.frame_index = logical_frame_index;
                    frame_context.frame_time_seconds = logical_frame_time_seconds(logical_frame_index);
                    frame_context.frame_delta_seconds = WII_FRAME_DURATION_SECONDS;

                    game_system.begin_frame(frame_context);
                    game_system.update();

                    if (runtime.should_exit_application()) {
                        _logger->info(logging::Category::APP, logging::Message {"Closing application after runtime request: {}"},
                                      runtime.application_exit_reason());
                        _window_service->close();
                    }

#ifndef NDEBUG
                    if (exit_on_layout_name.has_value() && has_active_layout(runtime, *exit_on_layout_name)) {
                        _window_service->close();
                    }
#endif

                    const auto should_draw_frame = !skip_render_until_frame.has_value() || frame_context.frame_index >= *skip_render_until_frame;
                    if (should_draw_frame) {
                        game_system.draw_3d_normal();
                        game_system.draw_2d_normal();
                    }

#ifndef NDEBUG
                    auto parity_trace_written_this_frame = false;
                    if (parity_trace_path.has_value() && !parity_trace_written && frame_context.frame_index >= parity_trace_frame) {
                        smgpc::runtime::write_runtime_parity_trace(std::filesystem::path(*parity_trace_path), frame_context, runtime);
                        parity_trace_written = true;
                        parity_trace_written_this_frame = true;
                    }
                    if (exit_after_frame.has_value() && frame_context.frame_index >= *exit_after_frame) {
                        _window_service->close();
                    }
#endif

                    _renderer->end_frame();
#ifndef NDEBUG
                    if (screenshot_path.has_value() && !screenshot_written && frame_context.frame_index >= screenshot_frame) {
                        _renderer->request_screenshot_png(std::filesystem::path(*screenshot_path));
                        screenshot_written = true;
                        if (exit_after_screenshot) {
                            _window_service->close();
                        }
                    }
                    if (parity_trace_written_this_frame && debug_hold_after_trace.count() > 0) {
                        _logger->info(logging::Category::APP,
                                      logging::Message {"Holding presented Aurora frame for {} ms after trace capture"},
                                      debug_hold_after_trace.count());
                        std::this_thread::sleep_for(debug_hold_after_trace);
                    }
#endif
                    frame_pacer.wait_for_frame_end();
                    ++loop_frame_index;
                }

                _renderer->shutdown();
                if (g_disc_open) {
#if defined(__GNUC__) || defined(__clang__)
                    aurora_dvd_close();
#endif
                    g_disc_open = false;
                }
                static_cast<void>(_configuration);
                return 0;
            }

        private:
            BootstrapConfiguration _configuration;
            di::DependencyReference<render::AuroraWindow> _window_service;
            di::DependencyReference<render::AuroraRenderer> _renderer;
            di::DependencyReference<logging::ILogger> _logger;
            di::DependencyReference<smgpc::runtime::RuntimeContext> _runtime;
            di::DependencyReference<smgpc::scene::GameSystemService> _game_system;
        };

    }  // namespace

    std::filesystem::path required_disc_image(const BootstrapConfiguration &configuration) {
        return required_disc_image_impl(configuration);
    }

    void ensure_disc_image_open(const BootstrapConfiguration &configuration, logging::ILogger &logger) {
        ensure_disc_image_open_impl(configuration, logger);
    }

    ServiceGraph build_service_graph(const BootstrapConfiguration &configuration) {
        return build_service_graph(configuration, ServiceGraphOverrides {});
    }

    ServiceGraph build_service_graph(const BootstrapConfiguration &configuration, ServiceGraphOverrides &&overrides) {
        ServiceGraph graph = {};

        if (overrides.logger) {
            graph.register_service<di::SingletonService<logging::ILogger>>(std::move(overrides.logger));
        } else {
            graph.register_service<di::SingletonService<logging::ILogger>>(logging::create_default_logger());
        }

        if (overrides.window_service) {
            graph.register_service<di::SingletonService<render::AuroraWindow>>(std::move(overrides.window_service));
        } else {
            graph.register_service<di::SingletonService<render::AuroraWindow>>([configuration]() {
                return std::make_unique<render::AuroraWindow>(render::WindowConfiguration {
                    .width = configuration.window_width,
                    .height = configuration.window_height,
                    .title = configuration.window_title,
                });
            });
        }

        if (overrides.aurora_renderer) {
            graph.register_service<di::SingletonService<render::AuroraRenderer>>(std::move(overrides.aurora_renderer));
        } else {
            graph.register_service<di::SingletonService<render::AuroraRenderer>, render::AuroraWindow>(
                [](di::DependencyReference<render::AuroraWindow> window_service) {
                    return std::make_unique<render::AuroraRenderer>(window_service.get());
                });
        }

        if (overrides.runtime_context) {
            graph.register_service<di::SingletonService<smgpc::runtime::RuntimeContext>>(std::move(overrides.runtime_context));
        } else {
            graph.register_service<di::SingletonService<smgpc::runtime::RuntimeContext>, logging::ILogger, render::AuroraWindow>(
                [configuration](di::DependencyReference<logging::ILogger> logger,
                                di::DependencyReference<render::AuroraWindow> window_service) {
                    ensure_disc_image_open(configuration, logger.get());
                    return std::make_unique<smgpc::runtime::RuntimeContext>(logger.get(), window_service.get(),
                                                                            smgpc::runtime::RuntimeContextSceneServiceMode::External);
                });
        }

        register_runtime_service_reference<smgpc::runtime::DvdFileSystemService, &smgpc::runtime::RuntimeContext::dvd>(graph);
        register_runtime_service_reference<smgpc::runtime::WiiIosService, &smgpc::runtime::RuntimeContext::ios>(graph);
        register_runtime_service_reference<smgpc::runtime::WiiPlatformService, &smgpc::runtime::RuntimeContext::wii_platform>(graph);
        register_runtime_service_reference<smgpc::runtime::WiiVideoService, &smgpc::runtime::RuntimeContext::wii_video>(graph);
        register_runtime_service_reference<smgpc::runtime::WpadService, &smgpc::runtime::RuntimeContext::wpad>(graph);
        register_runtime_service_reference<smgpc::runtime::AudioEventService, &smgpc::runtime::RuntimeContext::audio>(graph);
        register_runtime_service_reference<smgpc::runtime::EffectService, &smgpc::runtime::RuntimeContext::effects>(graph);
        register_runtime_service_reference<smgpc::runtime::ImageEffectService, &smgpc::runtime::RuntimeContext::image_effects>(graph);
        register_runtime_service_reference<smgpc::runtime::StarPointerService, &smgpc::runtime::RuntimeContext::star_pointer>(graph);
        register_runtime_service_reference<smgpc::runtime::CameraSystemService, &smgpc::runtime::RuntimeContext::camera_system>(graph);
        register_runtime_service_reference<smgpc::runtime::PlayerSystemService, &smgpc::runtime::RuntimeContext::player_system>(graph);
        register_runtime_service_reference<smgpc::runtime::GameLayoutService, &smgpc::runtime::RuntimeContext::game_layout>(graph);
        register_runtime_service_reference<smgpc::runtime::RumbleService, &smgpc::runtime::RuntimeContext::rumble>(graph);
        register_runtime_service_reference<smgpc::runtime::SequenceRequestService, &smgpc::runtime::RuntimeContext::sequence_requests>(graph);
        register_runtime_service_reference<smgpc::runtime::SysConfigService, &smgpc::runtime::RuntimeContext::sys_config>(graph);
        register_runtime_service_reference<smgpc::runtime::SaveDataService, &smgpc::runtime::RuntimeContext::save_data>(graph);
        register_runtime_service_reference<smgpc::runtime::NandFileSystemService, &smgpc::runtime::RuntimeContext::nand>(graph);
        register_runtime_service_reference<smgpc::runtime::MessageService, &smgpc::runtime::RuntimeContext::messages>(graph);
        register_runtime_service_reference<smgpc::runtime::SceneLightService, &smgpc::runtime::RuntimeContext::scene_lights>(graph);
        graph.register_service_reference<di::SingletonService<smgpc::runtime::RflService>, smgpc::runtime::RuntimeContext, smgpc::runtime::NandFileSystemService>(
            [](di::DependencyReference<smgpc::runtime::RuntimeContext> runtime,
               di::DependencyReference<smgpc::runtime::NandFileSystemService> nand) -> smgpc::runtime::RflService & {
                (void)nand;
                return runtime->rfl();
            });
        graph.add_service_dependency<smgpc::runtime::RflService, smgpc::runtime::NandFileSystemService>();
        register_runtime_service_reference<smgpc::runtime::SceneScheduler, &smgpc::runtime::RuntimeContext::scheduler>(graph);

        graph.register_service<di::SingletonService<smgpc::scene::NameObjLifecycleService>, smgpc::runtime::RuntimeContext>(
            [](di::DependencyReference<smgpc::runtime::RuntimeContext> runtime) {
                auto service = std::make_unique<smgpc::scene::NameObjLifecycleService>(runtime.get());
                runtime->attach_name_obj_lifecycle(*service);
                return service;
            });
        graph.register_service<di::SingletonService<smgpc::scene::SceneExecutionService>, smgpc::runtime::RuntimeContext>(
            [](di::DependencyReference<smgpc::runtime::RuntimeContext> runtime) {
                auto service = std::make_unique<smgpc::scene::SceneExecutionService>(runtime.get());
                runtime->attach_scene_execution(*service);
                return service;
            });
        graph.register_service<di::SingletonService<smgpc::scene::SceneLifecycleService>, smgpc::runtime::RuntimeContext,
                               smgpc::scene::NameObjLifecycleService, smgpc::scene::SceneExecutionService>(
            [](di::DependencyReference<smgpc::runtime::RuntimeContext> runtime,
               di::DependencyReference<smgpc::scene::NameObjLifecycleService> name_obj_lifecycle,
               di::DependencyReference<smgpc::scene::SceneExecutionService> scene_execution) {
                (void)name_obj_lifecycle;
                (void)scene_execution;
                auto service = std::make_unique<smgpc::scene::SceneLifecycleService>(runtime.get());
                runtime->attach_scene_lifecycle(*service);
                return service;
            });

        if (overrides.scene_controller) {
            graph.register_service<di::SingletonService<smgpc::scene::GameSystemSceneControllerService>>(std::move(overrides.scene_controller));
        } else {
            graph.register_service<di::SingletonService<smgpc::scene::GameSystemSceneControllerService>, smgpc::runtime::RuntimeContext,
                                   smgpc::scene::SceneLifecycleService>(
                [](di::DependencyReference<smgpc::runtime::RuntimeContext> runtime,
                   di::DependencyReference<smgpc::scene::SceneLifecycleService> scene_lifecycle) {
                    return std::make_unique<smgpc::scene::GameSystemSceneControllerService>(runtime.get(), scene_lifecycle.get());
                });
        }

        if (overrides.story_sequence) {
            graph.register_service<di::SingletonService<smgpc::scene::StorySequenceService>>(std::move(overrides.story_sequence));
        } else {
            graph.register_service<di::SingletonService<smgpc::scene::StorySequenceService>, smgpc::runtime::RuntimeContext>(
                [](di::DependencyReference<smgpc::runtime::RuntimeContext> runtime) {
                    return std::make_unique<smgpc::scene::StorySequenceService>(runtime.get());
                });
        }

        if (overrides.stage_host) {
            graph.register_service<di::SingletonService<smgpc::scene::StageHostService>>(std::move(overrides.stage_host));
        } else {
            graph.register_service<di::SingletonService<smgpc::scene::StageHostService>, smgpc::scene::GameSystemSceneControllerService>(
                [](di::DependencyReference<smgpc::scene::GameSystemSceneControllerService> scene_controller) {
                    return std::make_unique<smgpc::scene::StageHostService>(scene_controller.get());
                });
        }

        if (overrides.sequence_boot) {
            graph.register_service<di::SingletonService<smgpc::scene::SequenceBootService>>(std::move(overrides.sequence_boot));
        } else {
            graph.register_service<di::SingletonService<smgpc::scene::SequenceBootService>, smgpc::runtime::RuntimeContext, smgpc::scene::StorySequenceService,
                                   smgpc::scene::StageHostService>(
                [](di::DependencyReference<smgpc::runtime::RuntimeContext> runtime,
                   di::DependencyReference<smgpc::scene::StorySequenceService> story_sequence,
                   di::DependencyReference<smgpc::scene::StageHostService> stage_host) {
                    return std::make_unique<smgpc::scene::SequenceBootService>(runtime.get(), story_sequence.get(), stage_host.get());
                });
        }

        if (overrides.game_system) {
            graph.register_service<di::SingletonService<smgpc::scene::GameSystemService>>(std::move(overrides.game_system));
        } else {
            graph.register_service<di::SingletonService<smgpc::scene::GameSystemService>, smgpc::runtime::RuntimeContext,
                                   smgpc::scene::GameSystemSceneControllerService, smgpc::scene::SequenceBootService,
                                   smgpc::scene::NameObjLifecycleService, smgpc::scene::SceneExecutionService>(
                [](di::DependencyReference<smgpc::runtime::RuntimeContext> runtime,
                   di::DependencyReference<smgpc::scene::GameSystemSceneControllerService> scene_controller,
                   di::DependencyReference<smgpc::scene::SequenceBootService> sequence_boot,
                   di::DependencyReference<smgpc::scene::NameObjLifecycleService> name_obj_lifecycle,
                   di::DependencyReference<smgpc::scene::SceneExecutionService> scene_execution) {
                    (void)name_obj_lifecycle;
                    (void)scene_execution;
                    return std::make_unique<smgpc::scene::GameSystemService>(runtime.get(), scene_controller.get(), sequence_boot.get());
                });
        }

        if (overrides.application) {
            graph.register_service<di::SingletonService<IApplication>>(std::move(overrides.application));
        } else {
            graph.register_service<di::SingletonService<IApplication>, render::AuroraWindow, render::AuroraRenderer, logging::ILogger,
                                   smgpc::runtime::RuntimeContext, smgpc::scene::GameSystemService>(
                [configuration](di::DependencyReference<render::AuroraWindow> window_service,
                                di::DependencyReference<render::AuroraRenderer> aurora_renderer,
                                di::DependencyReference<logging::ILogger> logger,
                                di::DependencyReference<smgpc::runtime::RuntimeContext> runtime,
                                di::DependencyReference<smgpc::scene::GameSystemService> game_system) {
                    return std::make_unique<DesktopApplication>(configuration, std::move(window_service), std::move(aurora_renderer),
                                                                std::move(logger), std::move(runtime), std::move(game_system));
                });
        }

#ifndef NDEBUG
        if (const auto *export_path = std::getenv("SMGPC_DI_EXPORT_PATH"); export_path != nullptr && export_path[0] != '\0') {
            std::ofstream(export_path) << graph.dependencies_to_mermaid();
        }
#endif

        return graph;
    }

}  // namespace smgpc::app
