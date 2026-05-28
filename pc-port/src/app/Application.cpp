#include "Application.hpp"

#ifndef NDEBUG
#include "runtime/ParityTrace.hpp"
#endif

#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>

#ifndef NDEBUG
#include <fstream>
#endif

namespace smgpc::app {
    namespace {

        using FrameClock = std::chrono::steady_clock;
        constexpr auto WII_FRAME_RATE_HZ = 60.0;
        constexpr auto WII_FRAME_DURATION_SECONDS = 1.0 / WII_FRAME_RATE_HZ;
        constexpr auto WII_FRAME_DURATION = std::chrono::duration_cast<FrameClock::duration>(
            std::chrono::duration<double>(WII_FRAME_DURATION_SECONDS));

        [[nodiscard]] double logical_frame_time_seconds(std::uint64_t logical_frame_index) {
            return logical_frame_index <= 1U ? 0.0 : static_cast<double>(logical_frame_index - 1U) * WII_FRAME_DURATION_SECONDS;
        }

        [[nodiscard]] double logical_frame_delta_seconds(std::uint64_t logical_frame_index) {
            return logical_frame_index <= 1U ? 0.0 : WII_FRAME_DURATION_SECONDS;
        }

        void apply_logical_frame_timing(render::FrameContext &frame_context, std::uint64_t logical_frame_index) {
            frame_context.frame_index = logical_frame_index;
            frame_context.frame_time_seconds = logical_frame_time_seconds(logical_frame_index);
            frame_context.frame_delta_seconds = logical_frame_delta_seconds(logical_frame_index);
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

            [[nodiscard]] FrameClock::duration wait_for_frame_end() {
                const auto now = FrameClock::now();
                if (!_enabled) {
                    _next_frame_deadline = now + WII_FRAME_DURATION;
                    return FrameClock::duration::zero();
                }

                auto after_wait = now;
                auto sleep_duration = FrameClock::duration::zero();
                if (now < _next_frame_deadline) {
                    const auto sleep_begin = now;
                    std::this_thread::sleep_until(_next_frame_deadline);
                    after_wait = FrameClock::now();
                    sleep_duration = after_wait - sleep_begin;
                }

                while (_next_frame_deadline <= after_wait) {
                    _next_frame_deadline += WII_FRAME_DURATION;
                }

                return sleep_duration;
            }

        private:
            bool _enabled = true;
            FrameClock::time_point _next_frame_deadline = {};
        };

#ifndef NDEBUG
        struct OneShotScreenshotRequest {
            std::filesystem::path path;
            std::uint64_t frame = 1U;
            bool exit_after_capture = false;
        };

        struct OneShotParityTraceRequest {
            std::filesystem::path path;
            std::uint64_t frame = 1U;
        };

        struct FrameTimingSummary {
            std::uint64_t frame_count = 0U;
            double poll_events_ms = 0.0;
            double begin_frame_ms = 0.0;
            double runtime_update_ms = 0.0;
            double draw_3d_ms = 0.0;
            double draw_2d_ms = 0.0;
            double parity_trace_ms = 0.0;
            double screenshot_request_ms = 0.0;
            double end_frame_ms = 0.0;
            double frame_pacing_ms = 0.0;
            double total_ms = 0.0;
        };
#endif

#ifndef NDEBUG
        [[nodiscard]] double elapsed_ms(FrameClock::duration duration) {
            return std::chrono::duration<double, std::milli>(duration).count();
        }

        [[nodiscard]] double elapsed_ms(FrameClock::time_point begin, FrameClock::time_point end) {
            return std::chrono::duration<double, std::milli>(end - begin).count();
        }

        [[nodiscard]] bool bool_environment_value(const char *name, bool fallback) {
            const auto *value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return fallback;
            }

            const auto text = std::string_view(value);
            if (text == "0" || text == "false" || text == "False" || text == "off" || text == "OFF" || text == "no" || text == "NO") {
                return false;
            }
            if (text == "1" || text == "true" || text == "True" || text == "on" || text == "ON" || text == "yes" || text == "YES") {
                return true;
            }

            return fallback;
        }

        [[nodiscard]] bool bool_environment_enabled(const char *name) {
            return bool_environment_value(name, false);
        }

        [[nodiscard]] std::optional<std::string> string_environment(const char *name) {
            const auto *value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            return std::string(value);
        }

        void log_timing_summary(logging::ILogger &logger, const FrameTimingSummary &timing) {
            if (timing.frame_count == 0U) {
                return;
            }

            const auto inv_frames = 1.0 / static_cast<double>(timing.frame_count);
            logger.info(logging::Category::APP,
                        logging::Message{
                            "Frame timing summary over {} frames: poll={:.3f}ms begin={:.3f}ms update={:.3f}ms draw3d={:.3f}ms draw2d={:.3f}ms "
                            "trace={:.3f}ms screenshot={:.3f}ms end={:.3f}ms pace={:.3f}ms total={:.3f}ms"},
                        timing.frame_count, timing.poll_events_ms * inv_frames, timing.begin_frame_ms * inv_frames,
                        timing.runtime_update_ms * inv_frames, timing.draw_3d_ms * inv_frames, timing.draw_2d_ms * inv_frames,
                        timing.parity_trace_ms * inv_frames, timing.screenshot_request_ms * inv_frames, timing.end_frame_ms * inv_frames,
                        timing.frame_pacing_ms * inv_frames, timing.total_ms * inv_frames);
        }

        [[nodiscard]] std::optional<std::uint64_t> parse_frame_index(std::string_view text) {
            auto value = 0ULL;
            const auto *begin = text.data();
            const auto *end = begin + text.size();
            const auto result = std::from_chars(begin, end, value);
            if (result.ec != std::errc{} || result.ptr != end) {
                return std::nullopt;
            }

            return value;
        }

        [[nodiscard]] std::optional<OneShotScreenshotRequest> screenshot_request_from_environment() {
            const auto *path_value = std::getenv("SMGPC_SCREENSHOT_PATH");
            if (path_value == nullptr || path_value[0] == '\0') {
                return std::nullopt;
            }

            auto request = OneShotScreenshotRequest{
                .path = std::filesystem::path(path_value),
                .frame = 1U,
            };

            const auto *frame_value = std::getenv("SMGPC_SCREENSHOT_FRAME");
            if (frame_value != nullptr && frame_value[0] != '\0') {
                if (const auto parsed = parse_frame_index(frame_value); parsed.has_value()) {
                    request.frame = *parsed;
                }
            }

            const auto *exit_after_capture_value = std::getenv("SMGPC_EXIT_AFTER_SCREENSHOT");
            request.exit_after_capture = exit_after_capture_value != nullptr && std::string_view(exit_after_capture_value) == "1";

            return request;
        }

        [[nodiscard]] std::optional<OneShotParityTraceRequest> parity_trace_request_from_environment() {
            const auto *path_value = std::getenv("SMGPC_PARITY_TRACE_PATH");
            if (path_value == nullptr || path_value[0] == '\0') {
                return std::nullopt;
            }

            auto request = OneShotParityTraceRequest{
                .path = std::filesystem::path(path_value),
                .frame = 1U,
            };

            const auto *frame_value = std::getenv("SMGPC_PARITY_TRACE_FRAME");
            if (frame_value != nullptr && frame_value[0] != '\0') {
                if (const auto parsed = parse_frame_index(frame_value); parsed.has_value()) {
                    request.frame = *parsed;
                }
            }

            return request;
        }

        [[nodiscard]] std::optional<std::uint64_t> exit_frame_from_environment() {
            const auto *frame_value = std::getenv("SMGPC_EXIT_AFTER_FRAME");
            if (frame_value == nullptr || frame_value[0] == '\0') {
                return std::nullopt;
            }

            return parse_frame_index(frame_value);
        }

        [[nodiscard]] std::uint64_t event_poll_interval_from_environment() {
            const auto *value = std::getenv("SMGPC_EVENT_POLL_INTERVAL");
            if (value == nullptr || value[0] == '\0') {
                return 1U;
            }

            const auto parsed = parse_frame_index(value);
            if (!parsed.has_value() || *parsed == 0U) {
                return 1U;
            }
            return *parsed;
        }

        [[nodiscard]] std::optional<std::uint64_t> skip_render_until_frame_from_environment() {
            const auto *value = std::getenv("SMGPC_SKIP_RENDER_UNTIL_FRAME");
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            const auto parsed = parse_frame_index(value);
            if (!parsed.has_value() || *parsed <= 1U) {
                return std::nullopt;
            }
            return parsed;
        }

        [[nodiscard]] std::uint64_t render_frame_interval_from_environment() {
            const auto *value = std::getenv("SMGPC_RENDER_FRAME_INTERVAL");
            if (value == nullptr || value[0] == '\0') {
                return 1U;
            }

            const auto parsed = parse_frame_index(value);
            if (!parsed.has_value() || *parsed == 0U) {
                return 1U;
            }
            return *parsed;
        }

        [[nodiscard]] std::optional<std::uint64_t> debug_change_stage_request_frame_from_environment() {
            const auto *value = std::getenv("SMGPC_REQUEST_CHANGE_STAGE_AFTER_LOADING_FRAME");
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            return parse_frame_index(value);
        }

        [[nodiscard]] bool has_active_layout(const smgpc::runtime::RuntimeContext &runtime, std::string_view layout_name) {
            for (const auto &layout : runtime.scheduler().debug_layout_runtime_snapshot()) {
                if (layout.layout_name == layout_name && !layout.dead && !layout.suspended) {
                    return true;
                }
            }

            return false;
        }

        void emit_configured_semantic_anchor(smgpc::runtime::RuntimeContext &runtime) {
            const auto name = string_environment("SMGPC_SEMANTIC_ANCHOR_NAME");
            if (!name.has_value()) {
                return;
            }

            const auto category = string_environment("SMGPC_SEMANTIC_ANCHOR_CATEGORY").value_or("capture");
            const auto detail = string_environment("SMGPC_SEMANTIC_ANCHOR_DETAIL").value_or("runtime parity capture");
            runtime.emit_semantic_trace_event(category, *name, detail);
        }

        [[nodiscard]] bool frame_pacing_enabled_from_environment() {
            return bool_environment_value("SMGPC_FRAME_PACING", true);
        }
#endif

        class DesktopApplication final : public IApplication {
        public:
            DesktopApplication(di::DependencyReference<render::IWindowService> window_service,
                               di::DependencyReference<render::IRendererEngine> renderer_engine, di::DependencyReference<logging::ILogger> logger,
                               di::DependencyReference<smgpc::runtime::RuntimeContext> runtime,
                               di::DependencyReference<smgpc::scene::GameSystemService> game_system)
                : _window_service(std::move(window_service)), _renderer_engine(std::move(renderer_engine)), _logger(std::move(logger)),
                  _runtime(std::move(runtime)), _game_system(std::move(game_system)) {
            }

            [[nodiscard]] int run() override {
                _logger->info(logging::Category::APP, logging::Message{"Running SMG sequence boot slice"});
                _logger->info(logging::Category::APP, logging::Message{"Hold keyboard A+B or Enter+Backspace to satisfy Wii A+B title input"});

#ifndef NDEBUG
                const auto screenshot_request = screenshot_request_from_environment();
                const auto parity_trace_request = parity_trace_request_from_environment();
                const auto exit_after_frame = exit_frame_from_environment();
                const auto timing_enabled = bool_environment_enabled("SMGPC_FRAME_TIMING_SUMMARY");
                const auto event_poll_interval = event_poll_interval_from_environment();
                const auto skip_render_until_frame = skip_render_until_frame_from_environment();
                const auto render_frame_interval = render_frame_interval_from_environment();
                const auto frame_pacing_enabled = frame_pacing_enabled_from_environment();
                const auto exit_on_layout_name = string_environment("SMGPC_EXIT_ON_LAYOUT_NAME");
                const auto debug_change_stage_request_frame = debug_change_stage_request_frame_from_environment();
                auto timing = FrameTimingSummary{};
                auto screenshot_queued = false;
                auto parity_trace_written = false;
                auto debug_change_stage_requested = false;
                if (screenshot_request.has_value()) {
                    _logger->info(logging::Category::APP, logging::Message{"Will write a renderer PNG screenshot to {} on frame {}"},
                                  screenshot_request->path.string(), screenshot_request->frame);
                }
                if (parity_trace_request.has_value()) {
                    _logger->info(logging::Category::APP, logging::Message{"Will write runtime parity trace to {} on frame {}"},
                                  parity_trace_request->path.string(), parity_trace_request->frame);
                }
                if (exit_after_frame.has_value()) {
                    _logger->info(logging::Category::APP, logging::Message{"Will exit after frame {}"}, *exit_after_frame);
                }
                if (event_poll_interval > 1U) {
                    _logger->info(logging::Category::APP, logging::Message{"Polling desktop events every {} frames"}, event_poll_interval);
                }
                if (skip_render_until_frame.has_value()) {
                    _logger->info(logging::Category::APP, logging::Message{"Skipping renderer submission until frame {}"},
                                  *skip_render_until_frame);
                }
                if (render_frame_interval > 1U) {
                    _logger->info(logging::Category::APP, logging::Message{"Submitting one renderer frame every {} simulation frames"},
                                  render_frame_interval);
                }
                if (frame_pacing_enabled) {
                    _logger->info(logging::Category::APP, logging::Message{"Pacing simulation frames at {:.3f} Hz"}, WII_FRAME_RATE_HZ);
                } else {
                    _logger->info(logging::Category::APP, logging::Message{"Debug frame pacing explicitly disabled by SMGPC_FRAME_PACING=0"});
                }
                if (exit_on_layout_name.has_value()) {
                    _logger->info(logging::Category::APP, logging::Message{"Will exit when layout {} is active"}, *exit_on_layout_name);
                }
                if (debug_change_stage_request_frame.has_value()) {
                    _logger->info(logging::Category::APP, logging::Message{"Will debug-request generic stage change after loading game data on frame {}"},
                                  *debug_change_stage_request_frame);
                }
#else
                constexpr auto frame_pacing_enabled = true;
#endif

                auto &runtime = _runtime.get();
                auto &game_system = _game_system.get();
                if (!runtime.save_data().host_directory().has_value()) {
                    const auto save_directory = default_save_directory();
                    runtime.save_data().set_host_directory(save_directory);
                    _logger->info(logging::Category::APP, logging::Message{"Using default SMG save files from {}"}, save_directory.string());
                }
#ifndef NDEBUG
                emit_configured_semantic_anchor(runtime);
                if (parity_trace_request.has_value()) {
                    runtime.set_j3d_packet_trace_frame(parity_trace_request->frame);
                }
#endif
                auto loop_frame_index = std::uint64_t{};
                auto frame_pacer = FramePacer(frame_pacing_enabled);

                while (true) {
#ifndef NDEBUG
                    const auto frame_start = FrameClock::now();
                    const auto should_poll_events = loop_frame_index % event_poll_interval == 0U;
                    const auto window_open = should_poll_events ? _window_service->poll_events() : !_window_service->should_close();
                    const auto after_poll_events = FrameClock::now();
#else
                    const auto window_open = _window_service->poll_events();
#endif
                    if (!window_open) {
                        break;
                    }
                    const auto logical_frame_index = loop_frame_index + 1U;
#ifndef NDEBUG
                    const auto is_interval_render_frame = render_frame_interval == 1U || logical_frame_index % render_frame_interval == 0U;
                    const auto needs_parity_trace_render = parity_trace_request.has_value() && !parity_trace_written &&
                                                           logical_frame_index >= parity_trace_request->frame;
                    const auto needs_screenshot_render = screenshot_request.has_value() && !screenshot_queued &&
                                                         logical_frame_index >= screenshot_request->frame;
                    const auto needs_screenshot_flush_render =
                        screenshot_request.has_value() && screenshot_queued && screenshot_request->exit_after_capture &&
                        logical_frame_index <= screenshot_request->frame + 4U;
                    const auto should_render_frame =
                        ((!skip_render_until_frame.has_value() || logical_frame_index >= *skip_render_until_frame) &&
                         is_interval_render_frame) ||
                        needs_parity_trace_render || needs_screenshot_render || needs_screenshot_flush_render;
#else
                    constexpr auto should_render_frame = true;
#endif
                    auto frame_context = render::FrameContext{
                        .frame_index = logical_frame_index,
                        .frame_time_seconds = logical_frame_time_seconds(logical_frame_index),
                        .frame_delta_seconds = logical_frame_delta_seconds(logical_frame_index),
                        .framebuffer = _renderer_engine->framebuffer_size(),
                        .has_focus = _window_service->is_focused(),
                        .is_minimized = _window_service->is_minimized(),
                    };
                    if (should_render_frame) {
                        frame_context = _renderer_engine->begin_frame();
                        apply_logical_frame_timing(frame_context, logical_frame_index);
                    }
#ifndef NDEBUG
                    const auto after_begin_frame = FrameClock::now();
#endif
                    game_system.begin_frame(frame_context);
#ifndef NDEBUG
                    if (debug_change_stage_request_frame.has_value() && !debug_change_stage_requested &&
                        frame_context.frame_index >= *debug_change_stage_request_frame) {
                        runtime.sequence_requests().request_change_stage_in_game_after_loading_game_data();
                        debug_change_stage_requested = true;
                    }
#endif
                    game_system.update();
#ifndef NDEBUG
                    if (exit_on_layout_name.has_value() && has_active_layout(runtime, *exit_on_layout_name)) {
                        _window_service->close();
                    }
#endif
#ifndef NDEBUG
                    const auto after_runtime_update = FrameClock::now();
#endif
                    if (should_render_frame) {
                        game_system.draw_3d_normal(_renderer_engine.get());
                    }
#ifndef NDEBUG
                    const auto after_draw_3d = FrameClock::now();
#endif
                    if (should_render_frame) {
                        game_system.draw_2d_normal(_renderer_engine.get());
                    }
#ifndef NDEBUG
                    const auto after_draw_2d = FrameClock::now();
                    if (parity_trace_request.has_value() && !parity_trace_written && frame_context.frame_index >= parity_trace_request->frame) {
                        smgpc::runtime::write_runtime_parity_trace(parity_trace_request->path, frame_context, runtime);
                        parity_trace_written = true;
                    }
                    const auto after_parity_trace = FrameClock::now();
                    if (screenshot_request.has_value() && !screenshot_queued && frame_context.frame_index >= screenshot_request->frame) {
                        _renderer_engine->request_screenshot_png(screenshot_request->path);
                        screenshot_queued = true;
                    }
                    const auto after_screenshot_request = FrameClock::now();
                    if (screenshot_request.has_value() && screenshot_request->exit_after_capture && screenshot_queued &&
                        frame_context.frame_index >= screenshot_request->frame + 4U) {
                        _window_service->close();
                    }
                    if (exit_after_frame.has_value() && frame_context.frame_index >= *exit_after_frame) {
                        _window_service->close();
                    }
#endif
                    if (should_render_frame) {
                        _renderer_engine->end_frame();
                    }
#ifndef NDEBUG
                    const auto after_end_frame = FrameClock::now();
                    const auto frame_pacing_duration = frame_pacer.wait_for_frame_end();
                    const auto after_frame_pacing = FrameClock::now();
                    if (timing_enabled) {
                        ++timing.frame_count;
                        timing.poll_events_ms += elapsed_ms(frame_start, after_poll_events);
                        timing.begin_frame_ms += elapsed_ms(after_poll_events, after_begin_frame);
                        timing.runtime_update_ms += elapsed_ms(after_begin_frame, after_runtime_update);
                        timing.draw_3d_ms += elapsed_ms(after_runtime_update, after_draw_3d);
                        timing.draw_2d_ms += elapsed_ms(after_draw_3d, after_draw_2d);
                        timing.parity_trace_ms += elapsed_ms(after_draw_2d, after_parity_trace);
                        timing.screenshot_request_ms += elapsed_ms(after_parity_trace, after_screenshot_request);
                        timing.end_frame_ms += elapsed_ms(after_screenshot_request, after_end_frame);
                        timing.frame_pacing_ms += elapsed_ms(frame_pacing_duration);
                        timing.total_ms += elapsed_ms(frame_start, after_frame_pacing);
                    }
#else
                    static_cast<void>(frame_pacer.wait_for_frame_end());
#endif
                    ++loop_frame_index;
                }

#ifndef NDEBUG
                if (timing_enabled) {
                    log_timing_summary(_logger.get(), timing);
                }
#endif
                _renderer_engine->shutdown();
                return 0;
            }

        private:
            di::DependencyReference<render::IWindowService> _window_service;
            di::DependencyReference<render::IRendererEngine> _renderer_engine;
            di::DependencyReference<logging::ILogger> _logger;
            di::DependencyReference<smgpc::runtime::RuntimeContext> _runtime;
            di::DependencyReference<smgpc::scene::GameSystemService> _game_system;
        };

    }  // namespace

    ServiceGraph build_service_graph(const BootstrapConfiguration &configuration) {
        return build_service_graph(configuration, ServiceGraphOverrides{});
    }

    ServiceGraph build_service_graph(const BootstrapConfiguration &configuration, ServiceGraphOverrides &&overrides) {
        ServiceGraph graph = {};

        if (overrides.logger) {
            graph.register_service<di::SingletonService<logging::ILogger>>(std::move(overrides.logger));
        } else {
            graph.register_service<di::SingletonService<logging::ILogger>>(logging::create_default_logger());
        }

        if (overrides.window_factory) {
            graph.register_service<di::SingletonService<render::IWindowFactory>>(std::move(overrides.window_factory));
        } else {
            graph.register_service<di::SingletonService<render::IWindowFactory>, logging::ILogger>(
                [](di::DependencyReference<logging::ILogger> logger) { return render::create_default_window_factory(std::move(logger)); });
        }

        if (overrides.window_service) {
            graph.register_service<di::SingletonService<render::IWindowService>>(std::move(overrides.window_service));
        } else {
            graph.register_service<di::SingletonService<render::IWindowService>, render::IWindowFactory>(
                [configuration](di::DependencyReference<render::IWindowFactory> window_factory) {
                    return window_factory->create(render::WindowConfiguration{
                        .width = configuration.window_width,
                        .height = configuration.window_height,
                        .title = configuration.window_title,
                    });
                });
        }

        if (overrides.renderer_engine) {
            graph.register_service<di::SingletonService<render::IRendererEngine>>(std::move(overrides.renderer_engine));
        } else {
            graph.register_service<di::SingletonService<render::IRendererEngine>, render::IWindowService, logging::ILogger>(
                [](di::DependencyReference<render::IWindowService> window_service, di::DependencyReference<logging::ILogger> logger) {
                    return render::create_default_renderer_engine(std::move(window_service), std::move(logger));
                });
        }

        if (overrides.runtime_context) {
            graph.register_service<di::SingletonService<smgpc::runtime::RuntimeContext>>(std::move(overrides.runtime_context));
        } else {
            graph.register_service<di::SingletonService<smgpc::runtime::RuntimeContext>, logging::ILogger, render::IWindowService>(
                [](di::DependencyReference<logging::ILogger> logger, di::DependencyReference<render::IWindowService> window_service) {
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
            graph.register_service<di::SingletonService<IApplication>, render::IWindowService, render::IRendererEngine, logging::ILogger,
                                   smgpc::runtime::RuntimeContext, smgpc::scene::GameSystemService>(
                [](di::DependencyReference<render::IWindowService> window_service,
                   di::DependencyReference<render::IRendererEngine> renderer_engine, di::DependencyReference<logging::ILogger> logger,
                   di::DependencyReference<smgpc::runtime::RuntimeContext> runtime,
                   di::DependencyReference<smgpc::scene::GameSystemService> game_system) {
                    return std::make_unique<DesktopApplication>(std::move(window_service), std::move(renderer_engine), std::move(logger),
                                                                std::move(runtime), std::move(game_system));
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
