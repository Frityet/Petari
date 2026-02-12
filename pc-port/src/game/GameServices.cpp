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

#include "AssetServices.hpp"
#include "Logger.hpp"
#include "RenderWindow.hpp"
#include "layout/LayoutBgfxRenderer.hpp"
#include "layout/LayoutDrawList.hpp"
#include "title/TitleAssets.hpp"
#include "title/TitleLayoutActor.hpp"
#include "title/TitleRuntimeMR.hpp"
#include "title/TitleSequenceProduct.hpp"

namespace smgpc::game {
namespace {

[[nodiscard]] const char *asset_error_to_string(assets::AssetErrorCode code) {
    switch (code) {
    case assets::AssetErrorCode::NotFound:
        return "not-found";
    case assets::AssetErrorCode::IoFailure:
        return "io-failure";
    case assets::AssetErrorCode::InvalidFormat:
        return "invalid-format";
    }

    return "unknown";
}

struct FrameCaptureConfiguration {
    bool enabled {};
    std::filesystem::path output_directory {};
    std::uint32_t capture_every_frames {1U};
    std::uint32_t maximum_captures {};
    std::uint32_t capture_start_frame {};
};

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

[[nodiscard]] const assets::layout::tpl::DecodedImage *find_title_space_texture(const title::TitleAssets &title_assets) {
    for (const auto key : {std::string_view("mytitlespacekor"), std::string_view("mytitlespace")}) {
        const auto found = title_assets.title_logo.textures_by_name.find(std::string(key));
        if (found != title_assets.title_logo.textures_by_name.end()) {
            return &found->second;
        }
    }

    for (const auto &[name, texture] : title_assets.title_logo.textures_by_name) {
        if (name.find("mytitlespace") != std::string::npos) {
            return &texture;
        }
    }

    return nullptr;
}

[[nodiscard]] render::layout::TextureRef make_texture_ref(const assets::layout::tpl::DecodedImage *texture) {
    if (texture == nullptr || texture->rgba8.empty() || texture->width == 0U || texture->height == 0U) {
        return render::layout::TextureRef {
            .id = 0U,
            .rgba8 = nullptr,
            .width = 0U,
            .height = 0U,
        };
    }

    return render::layout::TextureRef {
        .id = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(texture)),
        .rgba8 = texture->rgba8.data(),
        .width = texture->width,
        .height = texture->height,
    };
}

void append_title_backdrop(render::layout::LayoutDrawList *draw_list, float layout_width, float layout_height, const assets::layout::tpl::DecodedImage *space_texture) {
    if (draw_list == nullptr || layout_width <= 0.0F || layout_height <= 0.0F) {
        return;
    }

    const auto no_texture = make_texture_ref(nullptr);
    const auto space_texture_ref = make_texture_ref(space_texture);

    const float horizon_y = layout_height * 0.56F;

    draw_list->push_quad(render::layout::QuadCommand {
        .x0 = 0.0F,
        .y0 = 0.0F,
        .x1 = layout_width,
        .y1 = horizon_y,
        .u0 = 0.0F,
        .v0 = 0.0F,
        .u1 = 1.0F,
        .v1 = 1.0F,
        .color_tl = render::layout::pack_abgr(6U, 26U, 59U, 255U),
        .color_tr = render::layout::pack_abgr(16U, 45U, 108U, 255U),
        .color_bl = render::layout::pack_abgr(14U, 86U, 144U, 255U),
        .color_br = render::layout::pack_abgr(42U, 98U, 162U, 255U),
        .texture = no_texture,
    });

    draw_list->push_quad(render::layout::QuadCommand {
        .x0 = layout_width * 0.35F,
        .y0 = 0.0F,
        .x1 = layout_width,
        .y1 = layout_height * 0.50F,
        .u0 = 0.0F,
        .v0 = 0.0F,
        .u1 = 1.0F,
        .v1 = 1.0F,
        .color_tl = render::layout::pack_abgr(74U, 46U, 135U, 0U),
        .color_tr = render::layout::pack_abgr(92U, 62U, 178U, 120U),
        .color_bl = render::layout::pack_abgr(54U, 84U, 145U, 0U),
        .color_br = render::layout::pack_abgr(80U, 120U, 185U, 96U),
        .blend_mode = render::layout::BlendMode::Additive,
        .texture = no_texture,
    });

    if (space_texture_ref.id != 0U) {
        draw_list->push_quad(render::layout::QuadCommand {
            .x0 = 0.0F,
            .y0 = 0.0F,
            .x1 = layout_width,
            .y1 = layout_height * 0.52F,
            .u0 = 0.0F,
            .v0 = 0.0F,
            .u1 = 2.1F,
            .v1 = 2.0F,
            .color_tl = render::layout::pack_abgr(255U, 255U, 255U, 124U),
            .color_tr = render::layout::pack_abgr(255U, 255U, 255U, 124U),
            .color_bl = render::layout::pack_abgr(255U, 255U, 255U, 80U),
            .color_br = render::layout::pack_abgr(255U, 255U, 255U, 80U),
            .texture = space_texture_ref,
        });
    }

    draw_list->push_quad(render::layout::QuadCommand {
        .x0 = 0.0F,
        .y0 = horizon_y - 9.0F,
        .x1 = layout_width,
        .y1 = horizon_y + 4.0F,
        .u0 = 0.0F,
        .v0 = 0.0F,
        .u1 = 1.0F,
        .v1 = 1.0F,
        .color_tl = render::layout::pack_abgr(222U, 255U, 255U, 90U),
        .color_tr = render::layout::pack_abgr(222U, 255U, 255U, 90U),
        .color_bl = render::layout::pack_abgr(170U, 255U, 255U, 12U),
        .color_br = render::layout::pack_abgr(170U, 255U, 255U, 12U),
        .blend_mode = render::layout::BlendMode::Additive,
        .texture = no_texture,
    });

    draw_list->push_quad(render::layout::QuadCommand {
        .x0 = 0.0F,
        .y0 = horizon_y - 2.0F,
        .x1 = layout_width,
        .y1 = layout_height,
        .u0 = 0.0F,
        .v0 = 0.0F,
        .u1 = 1.0F,
        .v1 = 1.0F,
        .color_tl = render::layout::pack_abgr(84U, 237U, 243U, 255U),
        .color_tr = render::layout::pack_abgr(84U, 237U, 243U, 255U),
        .color_bl = render::layout::pack_abgr(10U, 33U, 78U, 255U),
        .color_br = render::layout::pack_abgr(10U, 33U, 78U, 255U),
        .texture = no_texture,
    });

    if (space_texture_ref.id != 0U) {
        draw_list->push_quad(render::layout::QuadCommand {
            .x0 = 0.0F,
            .y0 = horizon_y + 4.0F,
            .x1 = layout_width,
            .y1 = layout_height,
            .u0 = 0.0F,
            .v0 = 0.2F,
            .u1 = 2.1F,
            .v1 = 2.2F,
            .color_tl = render::layout::pack_abgr(255U, 255U, 255U, 52U),
            .color_tr = render::layout::pack_abgr(255U, 255U, 255U, 52U),
            .color_bl = render::layout::pack_abgr(255U, 255U, 255U, 22U),
            .color_br = render::layout::pack_abgr(255U, 255U, 255U, 22U),
            .texture = space_texture_ref,
        });
    }
}

class DesktopGame final : public IGame {
public:
    DesktopGame(
        std::shared_ptr<render::IRendererService> renderer_service,
        std::shared_ptr<assets::IAssetManager> asset_manager,
        std::shared_ptr<logging::ILogger> logger)
        : _renderer_service(std::move(renderer_service)), _asset_manager(std::move(asset_manager)), _logger(std::move(logger)) {
        if (not _renderer_service or not _asset_manager or not _logger) {
            throw std::invalid_argument("DesktopGame requires non-null injected services.");
        }
    }

    [[nodiscard]] int run() override {
        _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Starting game loop");

        auto loaded_title_assets = title::load_title_assets(*_asset_manager, *_logger);
        if (not loaded_title_assets) {
            _logger->error(
                __FILE__,
                __LINE__,
                logging::Category::GAME,
                "Failed to load title assets [{}]: {}",
                asset_error_to_string(loaded_title_assets.failure().code),
                loaded_title_assets.failure().message);
            return 1;
        }

        title::TitleLayoutActor logo_layout(&loaded_title_assets->title_logo);
        title::TitleLayoutActor press_start_layout(&loaded_title_assets->press_start);
        title::TitleSequenceProduct title_sequence(&logo_layout, &press_start_layout);
        render::layout::LayoutBgfxRenderer layout_renderer(_logger);
        render::layout::LayoutDrawList draw_list {};
        draw_list.reserve(512U);
        const auto *title_space_texture = find_title_space_texture(*loaded_title_assets);
        const float layout_width = loaded_title_assets->title_logo.layout.size.x;
        const float layout_height = loaded_title_assets->title_logo.layout.size.y;

        title_sequence.appear();
        title::MR::setInputSource(_renderer_service.get(), _logger.get());
        const FrameCaptureConfiguration capture_configuration = load_frame_capture_configuration(_logger.get());
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
                title::MR::beginFrame();
                title_sequence.update();
                accumulator -= FIXED_STEP_SECONDS;
            }

            draw_list.clear();
            append_title_backdrop(&draw_list, layout_width, layout_height, title_space_texture);
            logo_layout.appendDrawCommands(&draw_list, loaded_title_assets->fonts_by_name);
            press_start_layout.appendDrawCommands(&draw_list, loaded_title_assets->fonts_by_name);

            _renderer_service->render_frame();
            const auto [framebuffer_width, framebuffer_height] = _renderer_service->framebuffer_size();
            layout_renderer.draw(
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
    std::shared_ptr<render::IRendererService> _renderer_service {};
    std::shared_ptr<assets::IAssetManager> _asset_manager {};
    std::shared_ptr<logging::ILogger> _logger {};
};

}  // namespace

std::shared_ptr<IGame> create_default_game_service(
    std::shared_ptr<render::IRendererService> renderer_service,
    std::shared_ptr<assets::IAssetManager> asset_manager,
    std::shared_ptr<logging::ILogger> logger) {
    return std::make_shared<DesktopGame>(std::move(renderer_service), std::move(asset_manager), std::move(logger));
}

}  // namespace smgpc::game
