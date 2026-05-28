#include "Application.hpp"
#include "Logger.hpp"

#include <cstdlib>
#include <exception>

namespace {

    [[nodiscard]] int read_positive_int_env(const char* name, int fallback) {
        const char* value = std::getenv(name);
        if (value == nullptr || value[0] == '\0') {
            return fallback;
        }

        const int parsed = std::atoi(value);
        return parsed > 0 ? parsed : fallback;
    }

}  // namespace

int main() try {
    const smgpc::app::BootstrapConfiguration configuration {
        .window_width = read_positive_int_env("SMGPC_WINDOW_WIDTH", smgpc::render::core::kWiiLogicalFramebufferWidth),
        .window_height = read_positive_int_env("SMGPC_WINDOW_HEIGHT", smgpc::render::core::kWiiLogicalFramebufferHeight),
        .window_title = "Super Mario Galaxy",
    };

    auto services = smgpc::app::build_service_graph(configuration);
    auto& application = services.get< smgpc::app::IApplication >();
    return application.run();
} catch (const std::exception& e) {
    auto fallback_logger = smgpc::logging::create_default_logger();
    fallback_logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message {"Uncaught exception {}"}, e.what());
    return 1;
}
