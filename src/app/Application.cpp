#include "Application.hpp"

#include <aurora/dvd.h>
#include <aurora/exception.hpp>
#include <cstdlib>
#include <span>
#include <stdexcept>
#include <string_view>

namespace smgpc::app {
    namespace {
        bool g_disc_open = false;

        [[nodiscard]] std::optional<std::filesystem::path> disc_image_from_arguments(std::span<const std::string> arguments) {
            for (std::size_t i = 1U; i < arguments.size(); ++i) {
                const auto &argument = arguments[i];
                if (argument == "--disc") {
                    if (i + 1U >= arguments.size() || arguments[i + 1U].empty()) {
                        aurora::throw_host_exception<std::runtime_error>("--disc requires a disc image path");
                    }
                    return std::filesystem::path(arguments[i + 1U]);
                }
                constexpr auto prefix = std::string_view("--disc=");
                if (argument.starts_with(prefix)) {
                    const auto value = argument.substr(prefix.size());
                    if (value.empty()) {
                        aurora::throw_host_exception<std::runtime_error>("--disc requires a disc image path");
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
            aurora::throw_host_exception<std::runtime_error>("Missing disc image. Launch with `smg-pc --disc <path>` or set SMGPC_DISC_IMAGE.");
        }

        void ensure_disc_image_open_impl(const BootstrapConfiguration &configuration, logging::ILogger &logger) {
            if (g_disc_open) {
                return;
            }

            const auto disc_image = required_disc_image_impl(configuration);
            const auto disc_path = disc_image.string();
            if (!aurora_dvd_open(disc_path.c_str())) {
                aurora::throw_host_exception<std::runtime_error>("Aurora could not open disc image " + disc_path);
            }
            g_disc_open = true;
            logger.info(logging::Category::APP, logging::Message {"Opened Aurora disc image {}"}, disc_path);
        }

    }  // namespace

    std::filesystem::path required_disc_image(const BootstrapConfiguration &configuration) {
        return required_disc_image_impl(configuration);
    }

    void ensure_disc_image_open(const BootstrapConfiguration &configuration, logging::ILogger &logger) {
        ensure_disc_image_open_impl(configuration, logger);
    }

    void close_disc_image() {
        if (!g_disc_open) return;
        aurora_dvd_close();
        g_disc_open = false;
    }

}  // namespace smgpc::app
