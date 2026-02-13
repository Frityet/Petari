#include "GameServices.hpp"

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <stdexcept>
#include <utility>
#include <vector>

#include "GameAssetService.hpp"
#include "Logger.hpp"
#include "RenderWindow.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/SimpleLayout.hpp"
#include "Game/Screen/TitleSequenceProduct.hpp"
#include "compat/RuntimeContext.hpp"
#include "layout/LayoutArchiveLoader.hpp"
#include "layout/LayoutBgfxRenderer.hpp"
#include "layout/LayoutDrawList.hpp"

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
    if (errno != 0 or end_pointer == value or *end_pointer != '\0' or parsed_value == 0UL or parsed_value > std::numeric_limits<std::uint32_t>::max()) {
        return std::nullopt;
    }

    return static_cast<std::uint32_t>(parsed_value);
}

[[nodiscard]] FrameCaptureConfiguration load_frame_capture_configuration(logging::ILogger *logger) {
    const auto capture_directory = get_path_from_environment("SMGPC_CAPTURE_DIR");
    const auto capture_every_frames = get_positive_number_from_environment("SMGPC_CAPTURE_EVERY");
    const auto capture_maximum_frames = get_positive_number_from_environment("SMGPC_CAPTURE_MAX");
    const auto capture_start_frame = get_positive_number_from_environment("SMGPC_CAPTURE_START");
    const bool capture_enabled = capture_directory.has_value() or capture_every_frames.has_value() or capture_maximum_frames.has_value() or capture_start_frame.has_value();

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
        if (logger != nullptr) {
            logger->warning(__FILE__, __LINE__, logging::Category::GAME, "Frame capture disabled because directory creation failed: {} ({})", configuration.output_directory.string(), filesystem_error.message());
        }
        return configuration;
    }

    if (logger != nullptr) {
        logger->info(__FILE__, __LINE__, logging::Category::GAME, "Frame capture enabled: dir={}, every={}, max={}, start={}", configuration.output_directory.string(), configuration.capture_every_frames, configuration.maximum_captures, configuration.capture_start_frame);
    }

    return configuration;
}

[[nodiscard]] std::filesystem::path make_capture_path(const std::filesystem::path &capture_directory, std::uint64_t capture_index) {
    char file_name[64];
    std::snprintf(file_name, sizeof(file_name), "frame_%06llu.png", static_cast<unsigned long long>(capture_index));
    return capture_directory / file_name;
}

[[nodiscard]] std::pair<float, float> resolve_layout_size(const LayoutActor *logo_layout, render::IRendererService *renderer_service) {
    if (logo_layout != nullptr) {
        const auto *resource = logo_layout->getResource();
        if (resource != nullptr && resource->layout.size.x > 0.0F && resource->layout.size.y > 0.0F) {
            return {resource->layout.size.x, resource->layout.size.y};
        }
    }

    if (renderer_service != nullptr) {
        const auto [framebuffer_width, framebuffer_height] = renderer_service->framebuffer_size();
        return {static_cast<float>(framebuffer_width), static_cast<float>(framebuffer_height)};
    }

    return {1280.0F, 720.0F};
}

class DesktopGame final : public IGame {
public:
    DesktopGame(
        di::DependencyReference<render::IRendererService> renderer_service,
        di::DependencyReference<assets::IGameAssetService> asset_service,
        di::DependencyReference<logging::ILogger> logger)
        : _renderer_service(std::move(renderer_service)), _asset_service(std::move(asset_service)), _logger(std::move(logger)) {
    }

    [[nodiscard]] int run() override {
        _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Starting game loop");

        const auto [initial_framebuffer_width, initial_framebuffer_height] = _renderer_service->framebuffer_size();
        const bool is_widescreen = initial_framebuffer_height == 0U
            ? true
            : static_cast<float>(initial_framebuffer_width) / static_cast<float>(initial_framebuffer_height) > 1.34F;

        compat::set_runtime_context(compat::RuntimeContext {
            .asset_service = &_asset_service.get(),
            .renderer_service = &_renderer_service.get(),
            .logger = &_logger.get(),
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
            } else {
                _logger->warning(
                    __FILE__,
                    __LINE__,
                    logging::Category::GAME,
                    "Background layout {} could not be loaded; skipping",
                    layout_name);
            }
        }

        TitleSequenceProduct title_sequence {};
        title_sequence.appear();

        std::unique_ptr<render::layout::LayoutBgfxRenderer> layout_renderer {};
        render::layout::LayoutDrawList draw_list {};
        draw_list.reserve(512U);

        const FrameCaptureConfiguration capture_configuration = load_frame_capture_configuration(&_logger.get());
        std::uint64_t rendered_frame_count = 0U;
        std::uint64_t requested_capture_count = 0U;
        bool capture_request_pending = false;

        using Clock = std::chrono::steady_clock;
        constexpr double FIXED_STEP_SECONDS = 1.0 / 60.0;
        constexpr double MAX_DELTA_SECONDS = 0.25;

        auto previous_time = Clock::now();
        double accumulator = FIXED_STEP_SECONDS * 2.0;

        while (_renderer_service->poll_events()) {
            const auto now = Clock::now();
            auto delta_seconds = std::chrono::duration<double>(now - previous_time).count();
            previous_time = now;

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
                layout_renderer = std::make_unique<render::layout::LayoutBgfxRenderer>(di::DependencyReference<logging::ILogger>{_logger.get()});
            }

            _renderer_service->render_frame();
            const auto [framebuffer_width, framebuffer_height] = _renderer_service->framebuffer_size();
            const auto [layout_width, layout_height] = resolve_layout_size(title_sequence.getLogoLayout(), &_renderer_service.get());
            layout_renderer->draw(
                draw_list,
                framebuffer_width,
                framebuffer_height,
                layout_width,
                layout_height);
            if (capture_configuration.enabled) {
                const bool under_capture_limit = capture_configuration.maximum_captures == 0U || requested_capture_count < capture_configuration.maximum_captures;
                const bool after_capture_start = rendered_frame_count >= capture_configuration.capture_start_frame;
                const std::uint64_t capture_frame_index = after_capture_start ? (rendered_frame_count - capture_configuration.capture_start_frame) : 0U;
                const bool capture_due = after_capture_start and (capture_frame_index % capture_configuration.capture_every_frames == 0U);
                if (under_capture_limit and capture_due and not capture_request_pending) {
                    _renderer_service->capture_next_frame(make_capture_path(capture_configuration.output_directory, requested_capture_count));
                    ++requested_capture_count;
                    capture_request_pending = true;
                }
            }

            auto &renderer = _renderer_service->renderer();
            renderer.on_frame_enter();
            renderer.draw();
            renderer.on_frame_exit();
            while (true) {
                auto completed_capture = _renderer_service->poll_completed_capture();
                if (not completed_capture.has_value()) {
                    break;
                }
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
    di::DependencyReference<render::IRendererService> _renderer_service;
    di::DependencyReference<assets::IGameAssetService> _asset_service;
    di::DependencyReference<logging::ILogger> _logger;
};

}  // namespace

std::unique_ptr<IGame> create_default_game_service(
    di::DependencyReference<render::IRendererService> renderer_service,
    di::DependencyReference<assets::IGameAssetService> asset_service,
    di::DependencyReference<logging::ILogger> logger) {
    return std::make_unique<DesktopGame>(std::move(renderer_service), std::move(asset_service), std::move(logger));
}

}  // namespace smgpc::game
