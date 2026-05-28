#include "Application.hpp"
#include "Logger.hpp"

#include <aurora/main.h>

#include <cstdlib>
#include <exception>
#include <string>
#include <utility>
#include <vector>

namespace {

    [[nodiscard]] int read_positive_int_env(const char* name, int fallback) {
        const char* value = std::getenv(name);
        if (value == nullptr || value[0] == '\0') {
            return fallback;
        }

        const int parsed = std::atoi(value);
        return parsed > 0 ? parsed : fallback;
    }

    [[nodiscard]] std::vector<std::string> copy_arguments(int argc, char* argv[]) {
        auto arguments = std::vector<std::string>{};
        arguments.reserve(argc > 0 ? static_cast<std::size_t>(argc) : 0U);
        for (int i = 0; i < argc; ++i) {
            arguments.emplace_back(argv[i] != nullptr ? argv[i] : "");
        }
        if (arguments.empty()) {
            arguments.emplace_back("smg-pc");
        }
        return arguments;
    }

}  // namespace

int main(int argc, char* argv[]) try {
    const smgpc::app::BootstrapConfiguration configuration {
        .window_width = read_positive_int_env("SMGPC_WINDOW_WIDTH", smgpc::render::core::kWiiLogicalFramebufferWidth),
        .window_height = read_positive_int_env("SMGPC_WINDOW_HEIGHT", smgpc::render::core::kWiiLogicalFramebufferHeight),
        .window_title = "Super Mario Galaxy",
        .arguments = copy_arguments(argc, argv),
    };

    auto startup_logger = smgpc::logging::create_default_logger();
    smgpc::app::ensure_disc_image_open(configuration, *startup_logger);

    auto overrides = smgpc::app::ServiceGraphOverrides {};
    overrides.logger = std::move(startup_logger);
    auto services = smgpc::app::build_service_graph(configuration, std::move(overrides));
    auto& application = services.get< smgpc::app::IApplication >();
    return application.run();
} catch (const std::exception& e) {
    auto fallback_logger = smgpc::logging::create_default_logger();
    fallback_logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message {"Uncaught exception {}"}, e.what());
    return 1;
}
