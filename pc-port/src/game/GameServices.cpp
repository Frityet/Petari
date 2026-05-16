#include "GameServices.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cwchar>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Game/Screen/IconAButton.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/ProloguePictureBook.hpp"
#include "Game/Screen/SimpleLayout.hpp"
#include "Game/Screen/TitleSequenceProduct.hpp"
#include "AssetLoader.hpp"
#include "Logger.hpp"
#include "compat/FileSelectPreview.hpp"
#include "compat/GamePadCompat.hpp"
#include "compat/LayoutSceneCompat.hpp"
#include "compat/RuntimeContext.hpp"
#include "compat/TitleBackground.hpp"
#include "core/RenderCommandBuffer.hpp"
#include "layout/LayoutArchiveLoader.hpp"
#include "layout/LayoutDrawList.hpp"
#include "layout/LayoutRenderPass.hpp"

namespace smgpc::game {
    namespace {

        struct FrameCaptureConfiguration {
            bool enabled{};
            std::filesystem::path output_directory{};
            std::uint32_t capture_every_frames{1U};
            std::uint32_t maximum_captures{};
            std::uint32_t capture_start_frame{};
        };

        enum class DesktopFlowState {
            Title,
            FileSelect,
            Prologue,
        };

        enum class BootView {
            Title,
            FileSelect,
            Prologue,
        };

        constexpr float MENU_DESKTOP_WIDTH = 836.0F;
        constexpr float MENU_DESKTOP_HEIGHT = 456.0F;

        [[nodiscard]] std::string trim_ascii_space(std::string text) {
            const auto first = text.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                return {};
            }

            const auto last = text.find_last_not_of(" \t\r\n");
            return text.substr(first, last - first + 1U);
        }

        [[nodiscard]] std::vector< std::string > load_boot_background_layouts() {
            const char* value = std::getenv("SMGPC_BOOT_BACKGROUND_LAYOUTS");
            if (value == nullptr || value[0] == '\0') {
                return {};
            }

            std::vector< std::string > layouts{};
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

        [[nodiscard]] BootView read_boot_view() {
            const char* value = std::getenv("SMGPC_BOOT_VIEW");
            if (value == nullptr) {
                return BootView::Title;
            }

            const auto view = trim_ascii_space(value);
            if (view == "prologue") {
                return BootView::Prologue;
            }
            if (view == "fileselect" || view == "file_select") {
                return BootView::FileSelect;
            }
            return BootView::Title;
        }

        [[nodiscard]] std::optional< std::filesystem::path > get_path_from_environment(const char* name) {
            const char* value = std::getenv(name);
            if (value == nullptr or value[0] == '\0') {
                return std::nullopt;
            }
            return std::filesystem::path(value);
        }

        [[nodiscard]] std::optional< std::uint32_t > get_positive_number_from_environment(const char* name) {
            const char* value = std::getenv(name);
            if (value == nullptr or value[0] == '\0') {
                return std::nullopt;
            }

            errno = 0;
            char* end_pointer = nullptr;
            const unsigned long parsed_value = std::strtoul(value, &end_pointer, 10);
            if (errno != 0 || end_pointer == value || *end_pointer != '\0' || parsed_value == 0UL ||
                parsed_value > std::numeric_limits< std::uint32_t >::max()) {
                return std::nullopt;
            }

            return static_cast< std::uint32_t >(parsed_value);
        }

        [[nodiscard]] std::optional< double > get_positive_double_from_environment(const char* name) {
            const char* value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            errno = 0;
            char* end_pointer = nullptr;
            const double parsed_value = std::strtod(value, &end_pointer);
            if (errno != 0 || end_pointer == value || *end_pointer != '\0' || parsed_value <= 0.0) {
                return std::nullopt;
            }

            return parsed_value;
        }

        [[nodiscard]] bool get_bool_from_environment(const char* name) {
            const char* value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return false;
            }

            const auto text = trim_ascii_space(value);
            return text != "0" && text != "false" && text != "False" && text != "FALSE";
        }

        [[nodiscard]] std::uint32_t title_press_start_extra_draws() {
            return get_positive_number_from_environment("SMGPC_TITLE_PRESSSTART_EXTRA_DRAWS").value_or(2U);
        }

        [[nodiscard]] FrameCaptureConfiguration load_frame_capture_configuration(di::OptionalDependencyReference< logging::ILogger > logger) {
            const auto capture_directory = get_path_from_environment("SMGPC_CAPTURE_DIR");
            const auto capture_every_frames = get_positive_number_from_environment("SMGPC_CAPTURE_EVERY");
            const auto capture_maximum_frames = get_positive_number_from_environment("SMGPC_CAPTURE_MAX");
            const auto capture_start_frame = get_positive_number_from_environment("SMGPC_CAPTURE_START");
            const bool capture_enabled = capture_directory.has_value() || capture_every_frames.has_value() || capture_maximum_frames.has_value() ||
                                         capture_start_frame.has_value();

            FrameCaptureConfiguration configuration{};
            if (not capture_enabled) {
                return configuration;
            }

            configuration.enabled = true;
            configuration.output_directory = capture_directory.value_or(std::filesystem::current_path() / ".cache" / "captures");
            configuration.capture_every_frames = capture_every_frames.value_or(1U);
            configuration.maximum_captures = capture_maximum_frames.value_or(0U);
            configuration.capture_start_frame = capture_start_frame.value_or(0U);

            std::error_code filesystem_error{};
            std::filesystem::create_directories(configuration.output_directory, filesystem_error);
            if (filesystem_error) {
                configuration.enabled = false;
                if (logger) {
                    logger->warning(__FILE__, __LINE__, logging::Category::GAME, "Frame capture disabled because directory creation failed: {} ({})",
                                    configuration.output_directory.string(), filesystem_error.message());
                }
                return configuration;
            }

            if (logger) {
                logger->info(__FILE__, __LINE__, logging::Category::GAME, "Frame capture enabled: dir={}, every={}, max={}, start={}",
                             configuration.output_directory.string(), configuration.capture_every_frames, configuration.maximum_captures,
                             configuration.capture_start_frame);
            }

            return configuration;
        }

        [[nodiscard]] std::filesystem::path make_capture_path(const std::filesystem::path& capture_directory, std::uint64_t capture_index) {
            char file_name[64];
            std::snprintf(file_name, sizeof(file_name), "frame_%06llu.png", static_cast< unsigned long long >(capture_index));
            return capture_directory / file_name;
        }

        [[nodiscard]] std::pair< float, float > resolve_layout_size(const LayoutActor* logo_layout,
                                                                    di::OptionalDependencyReference< render::IRendererEngine > renderer_engine) {
            if (logo_layout != nullptr) {
                const auto* resource = logo_layout->getResource();
                if (resource != nullptr && resource->layout.size.x > 0.0F && resource->layout.size.y > 0.0F) {
                    return {resource->layout.size.x, resource->layout.size.y};
                }
            }

            if (renderer_engine) {
                const auto framebuffer = renderer_engine->framebuffer_size();
                return {static_cast< float >(framebuffer.width), static_cast< float >(framebuffer.height)};
            }

            return {1280.0F, 720.0F};
        }

        class DesktopGame final : public IGame {
        public:
            DesktopGame(di::DependencyReference< render::IWindowService > window_service,
                        di::DependencyReference< render::IInputService > input_service,
                        di::DependencyReference< render::IRendererEngine > renderer_engine,
                        di::DependencyReference< assets::AssetLoader > asset_loader,
                        di::DependencyReference< logging::ILogger > logger)
                : _window_service(std::move(window_service)), _input_service(std::move(input_service)), _renderer_engine(std::move(renderer_engine)),
                  _asset_loader(std::move(asset_loader)), _logger(std::move(logger)) {
            }

            [[nodiscard]] int run() override {
                _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Starting game loop");

                const auto [initial_framebuffer_width, initial_framebuffer_height] = _renderer_engine->framebuffer_size();
                const bool is_widescreen =
                    initial_framebuffer_height == 0U ?
                        true :
                        static_cast< float >(initial_framebuffer_width) / static_cast< float >(initial_framebuffer_height) > 1.34F;

                compat::set_runtime_context(compat::RuntimeContext{
                    .asset_loader = _asset_loader,
                    .renderer_engine = _renderer_engine,
                    .input_service = _input_service,
                    .logger = _logger,
                    .is_widescreen = is_widescreen,
                });

                std::vector< std::unique_ptr< SimpleLayout > > background_layouts{};
                const auto background_layout_names = load_boot_background_layouts();
                background_layouts.reserve(background_layout_names.size());
                for (const auto& layout_name : background_layout_names) {
                    auto layout = std::make_unique< SimpleLayout >("BootBackground", layout_name.c_str(), 1, -1);
                    layout->appear();
                    if (not layout->isDead()) {
                        layout->startAnim("Wait", 0U);
                        background_layouts.push_back(std::move(layout));
                    }
                }

                TitleSequenceProduct title_sequence{};
                compat::TitleBackground title_background{};
                FileSelectPreview file_select_preview{};
                ProloguePictureBook prologue_picture_book{};
                prologue_picture_book.initWithoutIter();

                const auto boot_view = read_boot_view();
                DesktopFlowState flow_state = DesktopFlowState::Title;
                if (boot_view == BootView::FileSelect) {
                    flow_state = DesktopFlowState::FileSelect;
                } else if (boot_view == BootView::Prologue) {
                    flow_state = DesktopFlowState::Prologue;
                }

                if (flow_state == DesktopFlowState::Title) {
                    title_sequence.appear();
                } else if (flow_state == DesktopFlowState::FileSelect) {
                    file_select_preview.appear();
                } else {
                    prologue_picture_book.appear();
                }

                std::unique_ptr< render::layout::LayoutRenderPass > layout_renderer{};
                render::layout::LayoutDrawList draw_list{};
                draw_list.reserve(512U);
                render::core::RenderCommandBuffer j3d_pass_commands{};
                render::core::RenderCommandBuffer layout_pass_commands{};

                const FrameCaptureConfiguration capture_configuration = load_frame_capture_configuration(_logger);
                const auto fixed_delta_seconds = get_positive_double_from_environment("SMGPC_FIXED_FRAME_DELTA");
                std::uint64_t rendered_frame_count = 0U;
                std::uint64_t requested_capture_count = 0U;
                bool capture_request_pending = false;
                constexpr double FIXED_STEP_SECONDS = 1.0 / 60.0;
                constexpr double MAX_DELTA_SECONDS = 0.25;
                double accumulator = FIXED_STEP_SECONDS * 2.0;

                while (_window_service->poll_events()) {
                    const auto frame_context = _renderer_engine->begin_frame();
                    compat::begin_input_frame(rendered_frame_count);

                    auto delta_seconds = fixed_delta_seconds.value_or(frame_context.frame_delta_seconds);
                    if (delta_seconds < 0.0) {
                        delta_seconds = 0.0;
                    }
                    if (delta_seconds > MAX_DELTA_SECONDS) {
                        delta_seconds = MAX_DELTA_SECONDS;
                    }
                    accumulator += delta_seconds;

                    while (accumulator >= FIXED_STEP_SECONDS) {
                        for (auto& layout : background_layouts) {
                            layout->movement();
                        }
                        if (flow_state == DesktopFlowState::Title) {
                            title_sequence.update();
                        } else if (flow_state == DesktopFlowState::FileSelect) {
                            file_select_preview.movement();
                        } else {
                            prologue_picture_book.movement();
                            compat::movement_layout_scene_layer(compat::LayoutSceneLayer::TalkLayout);
                        }
                        accumulator -= FIXED_STEP_SECONDS;
                    }

                    draw_list.clear();
                    for (const auto& layout : background_layouts) {
                        layout->appendDrawCommands(&draw_list);
	                    }
	                    const LayoutActor* layout_for_size = nullptr;
	                    if (flow_state == DesktopFlowState::Title) {
	                        title_background.appendDrawCommands(&draw_list, rendered_frame_count);
	                        if (const auto* logo_layout = title_sequence.getLogoLayout(); logo_layout != nullptr) {
	                            logo_layout->appendDrawCommands(&draw_list);
	                            title_background.appendLogoOverlayDrawCommands(&draw_list, logo_layout, rendered_frame_count);
	                            layout_for_size = logo_layout;
	                        }
                        if (const auto* press_start_layout = title_sequence.getPressStartLayout(); press_start_layout != nullptr) {
                            press_start_layout->appendDrawCommands(&draw_list);
                            const auto extra_draws = title_press_start_extra_draws();
                            for (std::uint32_t draw = 0U; draw < extra_draws; ++draw) {
                                press_start_layout->appendDrawCommands(&draw_list);
                            }
                        }
                    } else if (flow_state == DesktopFlowState::FileSelect) {
                        file_select_preview.appendDrawCommands(&draw_list, rendered_frame_count);
                        layout_for_size = file_select_preview.layoutForSize();
                    } else {
                        prologue_picture_book.appendDrawCommands(&draw_list);
                        layout_for_size = &prologue_picture_book;
                        const auto book_quad_count = draw_list.quads().size();
                        compat::append_layout_scene_layer_draw_commands(compat::LayoutSceneLayer::TalkLayout, &draw_list);
                        if (get_bool_from_environment("SMGPC_DEBUG_PROLOGUE_PROMPT") && rendered_frame_count % 60U == 0U) {
                            _logger->info(__FILE__, __LINE__, logging::Category::GAME,
                                          "Prologue prompt draw debug frame={} book_quads={} total_quads={}", rendered_frame_count, book_quad_count,
                                          draw_list.quads().size());
                            for (std::size_t quad_index = book_quad_count; quad_index < draw_list.quads().size(); ++quad_index) {
                                const auto& quad = draw_list.quads()[quad_index];
                                _logger->info(__FILE__, __LINE__, logging::Category::GAME,
                                              "Prologue prompt quad {} pos=({}, {})-({}, {}) color={:#010x} tex={}x{} id={}",
                                              quad_index - book_quad_count, quad.x0, quad.y0, quad.x1, quad.y1, quad.color_tl, quad.texture.width,
                                              quad.texture.height, quad.texture.id);
                            }
                        }
                    }

                    if (layout_renderer == nullptr) {
                        layout_renderer = std::make_unique< render::layout::LayoutRenderPass >();
                    }

                    j3d_pass_commands.clear();
                    layout_pass_commands.clear();
                    const auto framebuffer = _renderer_engine->framebuffer_size();
                    if (flow_state == DesktopFlowState::Title) {
                        title_background.appendJ3dDrawCommands(&j3d_pass_commands, rendered_frame_count, framebuffer.width, framebuffer.height);
                    }
                    const auto resolved_layout_size = resolve_layout_size(layout_for_size, _renderer_engine);
                    const bool uses_menu_reference_size = flow_state == DesktopFlowState::Title || flow_state == DesktopFlowState::FileSelect;
                    const float layout_width = uses_menu_reference_size ? MENU_DESKTOP_WIDTH : resolved_layout_size.first;
                    const float layout_height = uses_menu_reference_size ? MENU_DESKTOP_HEIGHT : resolved_layout_size.second;
                    const bool has_j3d_pass = !j3d_pass_commands.commands().empty();
                    const std::uint8_t layout_view_id = has_j3d_pass ? 1U : 0U;
                    layout_renderer->record(layout_pass_commands, draw_list, framebuffer.width, framebuffer.height, layout_width, layout_height, !has_j3d_pass,
                                            layout_view_id);
                    if (has_j3d_pass) {
                        _renderer_engine->submit(j3d_pass_commands);
                    }
                    _renderer_engine->submit(layout_pass_commands);

                    if (capture_configuration.enabled) {
                        const bool under_capture_limit =
                            capture_configuration.maximum_captures == 0U || requested_capture_count < capture_configuration.maximum_captures;
                        const bool after_capture_start = rendered_frame_count >= capture_configuration.capture_start_frame;
                        const std::uint64_t capture_frame_index =
                            after_capture_start ? (rendered_frame_count - capture_configuration.capture_start_frame) : 0U;
                        const bool capture_due = after_capture_start && (capture_frame_index % capture_configuration.capture_every_frames == 0U);
                        if (under_capture_limit && capture_due && not capture_request_pending) {
                            _renderer_engine->request_capture(
                                render::RenderCaptureRequest{make_capture_path(capture_configuration.output_directory, requested_capture_count)});
                            ++requested_capture_count;
                            capture_request_pending = true;
                        }
                    }

                    _renderer_engine->end_frame();

                    while (auto completed_capture = _renderer_engine->poll_completed_capture()) {
                        capture_request_pending = false;
                        _logger->debug(__FILE__, __LINE__, logging::Category::GAME, "Frame capture available at {}", completed_capture->string());
                        if (get_bool_from_environment("SMGPC_EXIT_AFTER_CAPTURE_MAX") && capture_configuration.maximum_captures != 0U &&
                            requested_capture_count >= capture_configuration.maximum_captures) {
                            _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Capture limit reached; exiting game loop");
                            return 0;
                        }
                    }

                    ++rendered_frame_count;
                    if (flow_state == DesktopFlowState::Title && not title_sequence.isActive()) {
                        _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Title sequence reached Dead state; starting file select");
                        file_select_preview.appear();
                        flow_state = DesktopFlowState::FileSelect;
                    } else if (flow_state == DesktopFlowState::FileSelect && file_select_preview.isEnd()) {
                        if (file_select_preview.completion() == FileSelectPreviewCompletion::CreatedNewFile) {
                            _logger->info(__FILE__, __LINE__, logging::Category::GAME, "New file creation ended; starting prologue picture book");
                            prologue_picture_book.appear();
                            flow_state = DesktopFlowState::Prologue;
                        } else {
                            _logger->info(__FILE__, __LINE__, logging::Category::GAME, "File select ended before gameplay stage load");
                            break;
                        }
                    } else if (flow_state == DesktopFlowState::Prologue && prologue_picture_book.isEnd()) {
                        _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Prologue picture book ended before gameplay stage load");
                        break;
                    }
                }

                _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Exiting game loop");
                return 0;
            }

        private:
            di::DependencyReference< render::IWindowService > _window_service;
            di::DependencyReference< render::IInputService > _input_service;
            di::DependencyReference< render::IRendererEngine > _renderer_engine;
            di::DependencyReference< assets::AssetLoader > _asset_loader;
            di::DependencyReference< logging::ILogger > _logger;
        };

    }  // namespace

    std::unique_ptr< IGame > create_default_game_service(di::DependencyReference< render::IWindowService > window_service,
                                                         di::DependencyReference< render::IInputService > input_service,
                                                         di::DependencyReference< render::IRendererEngine > renderer_engine,
                                                         di::DependencyReference< assets::AssetLoader > asset_loader,
                                                         di::DependencyReference< logging::ILogger > logger) {
        return std::make_unique< DesktopGame >(std::move(window_service), std::move(input_service), std::move(renderer_engine),
                                               std::move(asset_loader), std::move(logger));
    }

}  // namespace smgpc::game
