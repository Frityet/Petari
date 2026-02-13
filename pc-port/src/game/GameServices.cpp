#include "GameServices.hpp"

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/SimpleLayout.hpp"
#include "Game/Screen/TitleSequenceProduct.hpp"
#include "GameAssetService.hpp"
#include "Logger.hpp"
#include "compat/RuntimeContext.hpp"
#include "layout/LayoutArchiveLoader.hpp"
#include "layout/LayoutRenderPass.hpp"
#include "layout/LayoutDrawList.hpp"
#include "core/RenderCommandBuffer.hpp"

namespace smgpc::game {
namespace {

struct FrameCaptureConfiguration {
    bool enabled {};
    std::filesystem::path output_directory {};
    std::uint32_t capture_every_frames {1U};
    std::uint32_t maximum_captures {};
    std::uint32_t capture_start_frame {};
};

[[nodiscard]] std::string trim_ascii_space(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }

    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1U);
}

[[nodiscard]] std::vector<std::string> load_boot_background_layouts() {
    const char *value = std::getenv("SMGPC_BOOT_BACKGROUND_LAYOUTS");
    if (value == nullptr || value[0] == '\0') {
        return {};
    }

    std::vector<std::string> layouts {};
    std::string input(value);
    std::size_t begin = 0U;

    while (begin <= input.size()) {
        const auto comma = input.find(',', begin);
        const auto end = (comma == std::string::npos) ? input.size() : comma;
        auto token = trim_ascii_space(input.substr(begin, end - begin));
        if (not token.empty()) {
            layouts.push_back(std::move(token));
        }
        if (comma == std::string::npos) {
            break;
        }
        begin = comma + 1U;
    }

    return layouts;
}

[[nodiscard]] std::optional<std::filesystem::path> get_path_from_environment(const char *name) {
    const char *value = std::getenv(name);
    if (value == nullptr or value[0] == '\0') {
        return std::nullopt;
    }
    return std::filesystem::path(value);
}

[[nodiscard]] std::optional<std::uint32_t> get_positive_number_from_environment(const char *name) {
    const char *value = std::getenv(name);
    if (value == nullptr or value[0] == '\0') {
        return std::nullopt;
    }

    errno = 0;
    char *end_pointer = nullptr;
    const unsigned long parsed_value = std::strtoul(value, &end_pointer, 10);
    if (errno != 0 || end_pointer == value || *end_pointer != '\0' || parsed_value == 0UL || parsed_value > std::numeric_limits<std::uint32_t>::max()) {
        return std::nullopt;
    }

    return static_cast<std::uint32_t>(parsed_value);
}

[[nodiscard]] FrameCaptureConfiguration load_frame_capture_configuration(di::OptionalDependencyReference<logging::ILogger> logger) {
    const auto capture_directory = get_path_from_environment("SMGPC_CAPTURE_DIR");
    const auto capture_every_frames = get_positive_number_from_environment("SMGPC_CAPTURE_EVERY");
    const auto capture_maximum_frames = get_positive_number_from_environment("SMGPC_CAPTURE_MAX");
    const auto capture_start_frame = get_positive_number_from_environment("SMGPC_CAPTURE_START");
    const bool capture_enabled = capture_directory.has_value() || capture_every_frames.has_value() || capture_maximum_frames.has_value() || capture_start_frame.has_value();

    FrameCaptureConfiguration configuration {};
    if (not capture_enabled) {
        return configuration;
    }

    configuration.enabled = true;
    configuration.output_directory = capture_directory.value_or(std::filesystem::current_path() / ".cache" / "captures");
    configuration.capture_every_frames = capture_every_frames.value_or(1U);
    configuration.maximum_captures = capture_maximum_frames.value_or(0U);
    configuration.capture_start_frame = capture_start_frame.value_or(0U);

    std::error_code filesystem_error {};
    std::filesystem::create_directories(configuration.output_directory, filesystem_error);
    if (filesystem_error) {
        configuration.enabled = false;
        if (logger) {
            logger->warning(
                __FILE__,
                __LINE__,
                logging::Category::GAME,
                "Frame capture disabled because directory creation failed: {} ({})",
                configuration.output_directory.string(),
                filesystem_error.message());
        }
        return configuration;
    }

    if (logger) {
        logger->info(
            __FILE__,
            __LINE__,
            logging::Category::GAME,
            "Frame capture enabled: dir={}, every={}, max={}, start={}",
            configuration.output_directory.string(),
            configuration.capture_every_frames,
            configuration.maximum_captures,
            configuration.capture_start_frame);
    }

    return configuration;
}

[[nodiscard]] std::filesystem::path make_capture_path(const std::filesystem::path &capture_directory, std::uint64_t capture_index) {
    char file_name[64];
    std::snprintf(file_name, sizeof(file_name), "frame_%06llu.png", static_cast<unsigned long long>(capture_index));
    return capture_directory / file_name;
}

[[nodiscard]] std::pair<float, float> resolve_layout_size(const LayoutActor *logo_layout, di::OptionalDependencyReference<render::IRendererEngine> renderer_engine) {
    if (logo_layout != nullptr) {
        const auto *resource = logo_layout->getResource();
        if (resource != nullptr && resource->layout.size.x > 0.0F && resource->layout.size.y > 0.0F) {
            return {resource->layout.size.x, resource->layout.size.y};
        }
    }

    if (renderer_engine) {
        const auto framebuffer = renderer_engine->framebuffer_size();
        return {static_cast<float>(framebuffer.width), static_cast<float>(framebuffer.height)};
    }

    return {1280.0F, 720.0F};
}

class DesktopGame final : public IGame {
public:
    DesktopGame(
        di::DependencyReference<render::IWindowService> window_service,
        di::DependencyReference<render::IInputService> input_service,
        di::DependencyReference<render::IRendererEngine> renderer_engine,
        di::DependencyReference<assets::IGameAssetService> asset_service,
        di::DependencyReference<logging::ILogger> logger)
        : _window_service(std::move(window_service)),
          _input_service(std::move(input_service)),
          _renderer_engine(std::move(renderer_engine)),
          _asset_service(std::move(asset_service)),
          _logger(std::move(logger)) {
    }

    [[nodiscard]] int run() override {
        _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Starting game loop");

        const auto [initial_framebuffer_width, initial_framebuffer_height] = _renderer_engine->framebuffer_size();
        const bool is_widescreen = initial_framebuffer_height == 0U
            ? true
            : static_cast<float>(initial_framebuffer_width) / static_cast<float>(initial_framebuffer_height) > 1.34F;

        compat::set_runtime_context(compat::RuntimeContext {
            .asset_service = _asset_service,
            .renderer_engine = _renderer_engine,
            .input_service = _input_service,
            .logger = _logger,
            .is_widescreen = is_widescreen,
        });

        std::vector<std::unique_ptr<SimpleLayout>> background_layouts {};
        const auto background_layout_names = load_boot_background_layouts();
        background_layouts.reserve(background_layout_names.size());
        for (const auto &layout_name : background_layout_names) {
            auto layout = std::make_unique<SimpleLayout>("BootBackground", layout_name.c_str(), 1, -1);
            layout->appear();
            if (not layout->isDead()) {
                layout->startAnim("Wait", 0U);
                background_layouts.push_back(std::move(layout));
            }
        }

        TitleSequenceProduct title_sequence {};
        title_sequence.appear();

        std::unique_ptr<render::layout::LayoutRenderPass> layout_renderer {};
        render::layout::LayoutDrawList draw_list {};
        draw_list.reserve(512U);
        render::core::RenderCommandBuffer layout_pass_commands {};

        const FrameCaptureConfiguration capture_configuration = load_frame_capture_configuration(_logger);
        std::uint64_t rendered_frame_count = 0U;
        std::uint64_t requested_capture_count = 0U;
        bool capture_request_pending = false;
        constexpr double FIXED_STEP_SECONDS = 1.0 / 60.0;
        constexpr double MAX_DELTA_SECONDS = 0.25;
        double accumulator = FIXED_STEP_SECONDS * 2.0;

        while (_window_service->poll_events()) {
            const auto frame_context = _renderer_engine->begin_frame();

            auto delta_seconds = frame_context.frame_delta_seconds;
            if (delta_seconds < 0.0) {
                delta_seconds = 0.0;
            }
            if (delta_seconds > MAX_DELTA_SECONDS) {
                delta_seconds = MAX_DELTA_SECONDS;
            }
            accumulator += delta_seconds;

            while (accumulator >= FIXED_STEP_SECONDS) {
                for (auto &layout : background_layouts) {
                    layout->movement();
                }
                title_sequence.update();
                accumulator -= FIXED_STEP_SECONDS;
            }

            draw_list.clear();
            for (const auto &layout : background_layouts) {
                layout->appendDrawCommands(&draw_list);
            }
            if (const auto *logo_layout = title_sequence.getLogoLayout(); logo_layout != nullptr) {
                logo_layout->appendDrawCommands(&draw_list);
            }
            if (const auto *press_start_layout = title_sequence.getPressStartLayout(); press_start_layout != nullptr) {
                press_start_layout->appendDrawCommands(&draw_list);
            }

            if (layout_renderer == nullptr) {
                layout_renderer = std::make_unique<render::layout::LayoutRenderPass>();
            }

            layout_pass_commands.clear();
            const auto framebuffer = _renderer_engine->framebuffer_size();
            const auto [layout_width, layout_height] = resolve_layout_size(title_sequence.getLogoLayout(), _renderer_engine);
            layout_renderer->record(
                layout_pass_commands,
                draw_list,
                framebuffer.width,
                framebuffer.height,
                layout_width,
                layout_height);
            _renderer_engine->submit(layout_pass_commands);

            if (capture_configuration.enabled) {
                const bool under_capture_limit = capture_configuration.maximum_captures == 0U || requested_capture_count < capture_configuration.maximum_captures;
                const bool after_capture_start = rendered_frame_count >= capture_configuration.capture_start_frame;
                const std::uint64_t capture_frame_index = after_capture_start ? (rendered_frame_count - capture_configuration.capture_start_frame) : 0U;
                const bool capture_due = after_capture_start && (capture_frame_index % capture_configuration.capture_every_frames == 0U);
                if (under_capture_limit && capture_due && not capture_request_pending) {
                    _renderer_engine->request_capture(render::RenderCaptureRequest {make_capture_path(capture_configuration.output_directory, requested_capture_count)});
                    ++requested_capture_count;
                    capture_request_pending = true;
                }
            }

            _renderer_engine->end_frame();

            while (auto completed_capture = _renderer_engine->poll_completed_capture()) {
                capture_request_pending = false;
                _logger->debug(__FILE__, __LINE__, logging::Category::GAME, "Frame capture available at {}", completed_capture->string());
            }

            ++rendered_frame_count;
            if (not title_sequence.isActive()) {
                _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Title sequence reached Dead state");
                break;
            }
        }

        _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Exiting game loop");
        return 0;
    }

private:
    di::DependencyReference<render::IWindowService> _window_service;
    di::DependencyReference<render::IInputService> _input_service;
    di::DependencyReference<render::IRendererEngine> _renderer_engine;
    di::DependencyReference<assets::IGameAssetService> _asset_service;
    di::DependencyReference<logging::ILogger> _logger;
};

}  // namespace

std::unique_ptr<IGame> create_default_game_service(
    di::DependencyReference<render::IWindowService> window_service,
    di::DependencyReference<render::IInputService> input_service,
    di::DependencyReference<render::IRendererEngine> renderer_engine,
    di::DependencyReference<assets::IGameAssetService> asset_service,
    di::DependencyReference<logging::ILogger> logger) {
    return std::make_unique<DesktopGame>(
        std::move(window_service),
        std::move(input_service),
        std::move(renderer_engine),
        std::move(asset_service),
        std::move(logger));
}

}  // namespace smgpc::game
