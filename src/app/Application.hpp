#pragma once

#include "Logger.hpp"
#include "RendererService.hpp"
#include "resource/GameResourceRuntime.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace smgpc::app {

    struct BootstrapConfiguration {
        int window_width = smgpc::render::core::kWiiLogicalFramebufferWidth;
        int window_height = smgpc::render::core::kWiiLogicalFramebufferHeight;
        std::string window_title = "SMG PC";
        std::vector<std::string> arguments = {};
        std::optional<std::filesystem::path> disc_image = {};
        resource::GameResourceBudget resource_budget = {};
    };

    [[nodiscard]] std::filesystem::path required_disc_image(const BootstrapConfiguration &configuration);
    void ensure_disc_image_open(const BootstrapConfiguration &configuration, logging::ILogger &logger);
    void close_disc_image();

}  // namespace smgpc::app
