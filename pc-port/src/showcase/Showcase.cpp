#include "Application.hpp"
#include "Game/Screen/TitleSequenceProduct.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "runtime/RuntimeContext.hpp"

#include <aurora/dvd.h>
#include <aurora/main.h>

#include <charconv>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    struct ShowcaseOptions {
        int window_width = 960;
        int window_height = 720;
        std::uint64_t max_frames = 0U;
        std::optional<std::filesystem::path> screenshot_path;
        std::optional<std::uint64_t> screenshot_frame;
        bool exit_after_screenshot = false;
    };

    class DvdCloseGuard final {
    public:
        DvdCloseGuard() = default;
        DvdCloseGuard(const DvdCloseGuard&) = delete;
        DvdCloseGuard& operator=(const DvdCloseGuard&) = delete;
        ~DvdCloseGuard() {
            aurora_dvd_close();
        }
    };

    [[nodiscard]] std::vector<std::string> copy_arguments(int argc, char* argv[]) {
        auto arguments = std::vector<std::string>{};
        arguments.reserve(argc > 0 ? static_cast<std::size_t>(argc) : 0U);
        for (auto index = 0; index < argc; ++index) {
            arguments.emplace_back(argv[index] != nullptr ? argv[index] : "");
        }
        return arguments;
    }

    [[nodiscard]] std::optional<std::string_view> option_value(std::span<const std::string> arguments,
                                                                std::string_view name) {
        const auto prefix = std::string(name) + "=";
        for (auto index = std::size_t{2U}; index < arguments.size(); ++index) {
            const auto& argument = arguments[index];
            if (argument == name) {
                if (index + 1U >= arguments.size() || arguments[index + 1U].empty()) {
                    throw std::runtime_error(std::string(name) + " requires a value");
                }
                return arguments[index + 1U];
            }
            if (argument.starts_with(prefix)) {
                const auto value = std::string_view(argument).substr(prefix.size());
                if (value.empty()) {
                    throw std::runtime_error(std::string(name) + " requires a value");
                }
                return value;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] bool has_option(std::span<const std::string> arguments, std::string_view name) {
        for (auto index = std::size_t{2U}; index < arguments.size(); ++index) {
            if (arguments[index] == name) {
                return true;
            }
        }
        return false;
    }

    template <typename Integer>
    [[nodiscard]] Integer parse_integer(std::string_view text, std::string_view option_name) {
        auto value = Integer{};
        const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
        if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
            throw std::runtime_error(std::string(option_name) + " requires an integer");
        }
        return value;
    }

    [[nodiscard]] ShowcaseOptions parse_options(std::span<const std::string> arguments) {
        if (arguments.size() < 2U || arguments[1] != "title") {
            throw std::runtime_error(
                "usage: smg-pc-showcase title --disc PATH "
                "[--screenshot PATH] [--screenshot-frame N] [--exit-after-screenshot]");
        }

        auto options = ShowcaseOptions{};
        if (const auto value = option_value(arguments, "--width")) {
            options.window_width = parse_integer<int>(*value, "--width");
        }
        if (const auto value = option_value(arguments, "--height")) {
            options.window_height = parse_integer<int>(*value, "--height");
        }
        if (const auto value = option_value(arguments, "--max-frames")) {
            options.max_frames = parse_integer<std::uint64_t>(*value, "--max-frames");
        }
        if (const auto value = option_value(arguments, "--screenshot")) {
            options.screenshot_path = std::filesystem::path(*value);
        }
        if (const auto value = option_value(arguments, "--screenshot-frame")) {
            options.screenshot_frame = parse_integer<std::uint64_t>(*value, "--screenshot-frame");
        }
        options.exit_after_screenshot = has_option(arguments, "--exit-after-screenshot");

        if (options.window_width <= 0 || options.window_height <= 0) {
            throw std::runtime_error("showcase window dimensions must be positive");
        }
        if (options.screenshot_path.has_value() && !options.screenshot_frame.has_value()) {
            options.screenshot_frame = 210U;
        }
        return options;
    }

    void prepare_screenshot_path(const ShowcaseOptions& options) {
        if (!options.screenshot_path.has_value() || options.screenshot_path->parent_path().empty()) {
            return;
        }
        auto error = std::error_code{};
        std::filesystem::create_directories(options.screenshot_path->parent_path(), error);
        if (error) {
            throw std::runtime_error("could not create screenshot directory: " + error.message());
        }
    }

    [[nodiscard]] bool capture_requested_frame(smgpc::render::AuroraRenderer& renderer,
                                                const ShowcaseOptions& options,
                                                std::uint64_t frame_index,
                                                bool& captured) {
        if (captured || !options.screenshot_path.has_value() || !options.screenshot_frame.has_value() ||
            frame_index < *options.screenshot_frame) {
            return false;
        }
        renderer.request_screenshot_png(*options.screenshot_path);
        captured = true;
        return options.exit_after_screenshot;
    }

    [[nodiscard]] smgpc::app::BootstrapConfiguration bootstrap_configuration(
        const ShowcaseOptions& options, const std::vector<std::string>& arguments) {
        return {
            .window_width = options.window_width,
            .window_height = options.window_height,
            .window_title = "Super Mario Galaxy PC - retail title showcase",
            .arguments = arguments,
        };
    }

    [[nodiscard]] int run_title_showcase(const ShowcaseOptions& options,
                                         const smgpc::app::BootstrapConfiguration& configuration) {
        auto logger = smgpc::logging::create_default_logger();
        const auto disc_image = smgpc::app::required_disc_image(configuration);
        if (!aurora_dvd_open(disc_image.string().c_str())) {
            throw std::runtime_error("Aurora could not open disc image " + disc_image.string());
        }
        const auto dvd_guard = DvdCloseGuard{};

        auto window = smgpc::render::AuroraWindow({
            .width = options.window_width,
            .height = options.window_height,
            .title = configuration.window_title,
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto captured = false;
        auto frame_index = std::uint64_t{};

        {
            auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
            auto title_sequence = TitleSequenceProduct{};
            title_sequence.appear();
            logger->info(smgpc::logging::Category::APP,
                         smgpc::logging::Message{"Showing the exact retail TitleSequenceProduct with original disc resources"});
            logger->info(smgpc::logging::Category::APP,
                         smgpc::logging::Message{"Hold keyboard A+B or Enter+Backspace when prompted"});

            while (window.poll_events()) {
                auto frame_context = renderer.begin_frame();
                frame_index = frame_context.frame_index;
                {
                    const auto renderer_context = smgpc::render::ScopedAuroraRendererContext(renderer);
                    runtime.begin_frame(frame_context);
                    title_sequence.updateNerve();
                    runtime.draw_2d_normal();
                }
                renderer.end_frame();

                if (capture_requested_frame(renderer, options, frame_index, captured)) {
                    window.close();
                }
                if (!title_sequence.isActive()) {
                    logger->info(smgpc::logging::Category::APP,
                                 smgpc::logging::Message{"Retail title component completed at frame {}"}, frame_index);
                    window.close();
                }
                if (options.max_frames != 0U && frame_index >= options.max_frames) {
                    window.close();
                }
            }
        }
        return 0;
    }

}  // namespace

int main(int argc, char* argv[]) try {
    const auto arguments = copy_arguments(argc, argv);
    const auto options = parse_options(arguments);
    prepare_screenshot_path(options);
    const auto configuration = bootstrap_configuration(options, arguments);
    return run_title_showcase(options, configuration);
} catch (const std::exception& error) {
    auto logger = smgpc::logging::create_default_logger();
    logger->fatal(smgpc::logging::Category::APP,
                  smgpc::logging::Message{"Showcase failed: {}"}, error.what());
    return 1;
}
