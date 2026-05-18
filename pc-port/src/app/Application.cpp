#include "Application.hpp"

#include "Game/compat/RuntimeContext.hpp"
#include "Game/compat/SequenceBootService.hpp"

#ifndef NDEBUG
#include "Game/compat/ParityTrace.hpp"
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

        void emit_configured_semantic_anchor(game::RuntimeContext &runtime) {
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
                               di::DependencyReference<render::IRendererEngine> renderer_engine, di::DependencyReference<logging::ILogger> logger)
                : _window_service(std::move(window_service)), _renderer_engine(std::move(renderer_engine)), _logger(std::move(logger)) {
            }

            [[nodiscard]] int run() override {
                _logger->info(logging::Category::APP, logging::Message{"Running SMG sequence boot compatibility slice"});
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
                const auto exit_on_picturebook_reached = bool_environment_enabled("SMGPC_EXIT_ON_PICTUREBOOK_REACHED");
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
                if (exit_on_picturebook_reached) {
                    _logger->info(logging::Category::APP, logging::Message{"Will exit when picturebook_reached is emitted"});
                }
                if (debug_change_stage_request_frame.has_value()) {
                    _logger->info(logging::Category::APP, logging::Message{"Will debug-request generic stage change after loading game data on frame {}"},
                                  *debug_change_stage_request_frame);
                }
#else
                constexpr auto frame_pacing_enabled = true;
#endif

                auto runtime = game::RuntimeContext(_logger.get(), _window_service.get());
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
                auto sequence_boot = game::SequenceBootService(runtime);
                sequence_boot.request_boot_to_file_select();
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
                    runtime.begin_frame(frame_context);
#ifndef NDEBUG
                    if (debug_change_stage_request_frame.has_value() && !debug_change_stage_requested &&
                        frame_context.frame_index >= *debug_change_stage_request_frame) {
                        runtime.sequence_requests().request_change_stage_in_game_after_loading_game_data();
                        debug_change_stage_requested = true;
                    }
#endif
                    sequence_boot.update_after_runtime_frame();
#ifndef NDEBUG
                    if (exit_on_picturebook_reached && sequence_boot.has_picturebook_reached()) {
                        _window_service->close();
                    }
#endif
#ifndef NDEBUG
                    const auto after_runtime_update = FrameClock::now();
#endif
                    if (should_render_frame) {
                        runtime.draw_3d_normal(_renderer_engine.get());
                    }
#ifndef NDEBUG
                    const auto after_draw_3d = FrameClock::now();
#endif
                    if (should_render_frame) {
                        runtime.draw_2d_normal(_renderer_engine.get());
                    }
#ifndef NDEBUG
                    const auto after_draw_2d = FrameClock::now();
                    if (parity_trace_request.has_value() && !parity_trace_written && frame_context.frame_index >= parity_trace_request->frame) {
                        game::write_runtime_parity_trace(parity_trace_request->path, frame_context, runtime);
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

        if (overrides.application) {
            graph.register_service<di::SingletonService<IApplication>>(std::move(overrides.application));
        } else {
            graph.register_service<di::SingletonService<IApplication>, render::IWindowService, render::IRendererEngine, logging::ILogger>(
                [](di::DependencyReference<render::IWindowService> window_service,
                   di::DependencyReference<render::IRendererEngine> renderer_engine, di::DependencyReference<logging::ILogger> logger) {
                    return std::make_unique<DesktopApplication>(std::move(window_service), std::move(renderer_engine), std::move(logger));
                });
        }

        return graph;
    }

}  // namespace smgpc::app
