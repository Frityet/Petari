#include "Game/NameObj/NameObj.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/GameSystemSceneControllerService.hpp"
#include "scene/NameObjLifecycleService.hpp"
#include "scene/SceneExecutionService.hpp"
#include "scene/SceneLifecycleService.hpp"
#include "scene/StageHostService.hpp"

#include <aurora/dvd.h>

#include <charconv>
#include <cstdlib>
#include <exception>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    struct ProbeOptions {
        std::string disc_image;
        std::string scene_name = "Game";
        std::string stage_name;
        s32 scenario_no = 1;
        s32 start_id = 0;
        s32 start_zone_id = 0;
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
                                                                std::string_view option) {
        const auto prefix = std::string(option) + "=";
        for (auto index = std::size_t{1U}; index < arguments.size(); ++index) {
            const auto& argument = arguments[index];
            if (argument == option) {
                if (index + 1U >= arguments.size() || arguments[index + 1U].empty()) {
                    throw std::runtime_error(std::string(option) + " requires a value");
                }
                return arguments[index + 1U];
            }
            if (argument.starts_with(prefix)) {
                const auto value = std::string_view(argument).substr(prefix.size());
                if (value.empty()) {
                    throw std::runtime_error(std::string(option) + " requires a value");
                }
                return value;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] s32 parse_s32(std::string_view value, std::string_view option) {
        auto parsed = s32{};
        const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
        if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) {
            throw std::runtime_error(std::string(option) + " requires a signed 32-bit integer");
        }
        return parsed;
    }

    [[nodiscard]] ProbeOptions parse_options(std::span<const std::string> arguments) {
        auto options = ProbeOptions{};
        const auto disc = option_value(arguments, "--disc");
        const auto stage = option_value(arguments, "--stage");
        if (!disc.has_value() || !stage.has_value()) {
            throw std::runtime_error(
                "usage: smg-pc-stage-construction-probe --disc PATH --stage NAME "
                "[--scene NAME] [--scenario N] [--start-id N] [--start-zone-id N]");
        }

        options.disc_image = *disc;
        options.stage_name = *stage;
        if (const auto scene = option_value(arguments, "--scene")) {
            options.scene_name = *scene;
        }
        if (const auto scenario = option_value(arguments, "--scenario")) {
            options.scenario_no = parse_s32(*scenario, "--scenario");
        }
        if (const auto start_id = option_value(arguments, "--start-id")) {
            options.start_id = parse_s32(*start_id, "--start-id");
        }
        if (const auto start_zone_id = option_value(arguments, "--start-zone-id")) {
            options.start_zone_id = parse_s32(*start_zone_id, "--start-zone-id");
        }
        if (options.scenario_no < 1) {
            throw std::runtime_error("--scenario must be positive");
        }
        return options;
    }

    [[nodiscard]] int run_probe(const ProbeOptions& options) {
        if (!aurora_dvd_open(options.disc_image.c_str())) {
            throw std::runtime_error("Aurora could not open disc image " + options.disc_image);
        }
        const auto dvd_guard = DvdCloseGuard{};

        auto logger = smgpc::logging::create_default_logger();
        auto window = smgpc::render::AuroraWindow({
            .width = 640,
            .height = 480,
            .title = "SMG PC strict stage construction probe",
        });

        auto resource_runtime = smgpc::resource::GameResourceRuntime{};

        auto runtime = smgpc::runtime::RuntimeContext(
            *logger, window, resource_runtime, smgpc::runtime::RuntimeContextSceneServiceMode::External);
        auto name_obj_lifecycle = smgpc::scene::NameObjLifecycleService(runtime);
        auto scene_execution = smgpc::scene::SceneExecutionService(runtime);
        auto scene_lifecycle = smgpc::scene::SceneLifecycleService(runtime);
        runtime.attach_name_obj_lifecycle(name_obj_lifecycle);
        runtime.attach_scene_execution(scene_execution);
        runtime.attach_scene_lifecycle(scene_lifecycle);

        auto scene_controller = smgpc::scene::GameSystemSceneControllerService(runtime, scene_lifecycle);
        auto stage_host = smgpc::scene::StageHostService(scene_controller);
        stage_host.request_stage({
            .scene_name = options.scene_name,
            .stage_name = options.stage_name,
            .scenario_no = options.scenario_no,
            .start_id = options.start_id,
            .start_zone_id = options.start_zone_id,
        });
        stage_host.update_scene_requests();

        if (!stage_host.has_active_stage(options.stage_name)) {
            throw std::runtime_error("strict stage construction returned without an active stage");
        }
        logger->info(smgpc::logging::Category::APP,
                     smgpc::logging::Message{"Strictly constructed retail stage {} scenario {}"},
                     options.stage_name, options.scenario_no);
        return 0;
    }

}  // namespace

int main(int argc, char* argv[]) try {
    return run_probe(parse_options(copy_arguments(argc, argv)));
} catch (const std::exception& error) {
    auto logger = smgpc::logging::create_default_logger();
    logger->fatal(smgpc::logging::Category::APP,
                  smgpc::logging::Message{"Strict stage construction failed: {}"}, error.what());
    return 1;
}
