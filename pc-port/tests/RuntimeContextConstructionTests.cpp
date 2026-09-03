#include "runtime/RuntimeContext.hpp"
#include "runtime/ScreenAlphaCaptureService.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "Game/Screen/ScreenAlphaCapture.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include "Game/Camera/CameraContext.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "runtime/SystemConfigService.hpp"
#include "JSystem/JUtility/JUTVideo.hpp"
#include <aurora/dvd.h>
#include <aurora/sysconf.hpp>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

    struct Environment {
        const char* name;
        std::optional<std::string> previous;
        explicit Environment(const char* key) : name(key) {
            if (const char* value = std::getenv(key)) previous = value;
            unsetenv(name);
        }
        ~Environment() {
            if (previous) setenv(name, previous->c_str(), 1);
            else unsetenv(name);
        }
    };
    struct ConsoleDirectory {
        std::filesystem::path path;
        ConsoleDirectory() {
            const auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
            for (unsigned attempt = 0; attempt < 1000; ++attempt) {
                auto candidate = std::filesystem::temp_directory_path() /
                    ("smg-runtime-config-" + std::to_string(seed) + "-" + std::to_string(attempt));
                if (std::filesystem::create_directory(candidate)) { path = std::move(candidate); return; }
            }
            throw std::runtime_error("Cannot create temporary console directory");
        }
        ~ConsoleDirectory() { std::error_code error; std::filesystem::remove_all(path, error); }
        void aspect(u8 value) const {
            aurora::SysConf document;
            document.replace_integer("IPL.AR", aurora::SysConf::Type::Byte, value);
            document.replace_integer("IPL.PGS", aurora::SysConf::Type::Byte, value);
            auto bytes = document.encode();
            auto file = path / "shared2/sys/SYSCONF";
            std::filesystem::create_directories(file.parent_path());
            std::ofstream output(file, std::ios::binary);
            output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            require(bool(output), "write complete real console configuration fixture");
        }
    };

    struct InjectedLogFailure {};
    class FixtureLogger final : public smgpc::logging::ILogger {
    public:
        bool throw_after_registration = false;
        std::vector<NameObj*> registered;
        void write(std::FILE*, std::source_location, smgpc::logging::Level,
                   smgpc::logging::Category, std::string_view message) override {
            if (!throw_after_registration || message != "Using SMG disc image through Aurora DVD") return;
            auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
            require(runtime && JUTVideo::getManager(), "registration must remain available while startup clients run");
            const auto entries = runtime->scheduler().snapshot();
            require(entries.size() == 2, "failure fixture must reach both original CaptureScreenActor registrations");
            registered = smgpc::compat::snapshot_name_obj_runtime_objects();
            throw InjectedLogFailure{};
        }
    };
}

int main() {
    Environment save_environment("SMGPC_SAVE_DIR");
    Environment nand_environment("SMGPC_NAND_DIR");
    smgpc::render::AuroraWindow window({.width = 640, .height = 456, .title = "Runtime construction ownership"});
    smgpc::render::AuroraRenderer renderer(window);
    const auto* disc = std::getenv("SMGPC_REAL_DISC");
    if (disc) require(aurora_dvd_open(disc), "cannot open requested real disc");
    struct Disc { ~Disc() { aurora_dvd_close(); } } close_disc;
    DVDInit();
    smgpc::resource::GameResourceRuntime process({96U * 1024U * 1024U, 32U * 1024U * 1024U, 4U * 1024U * 1024U});
    auto heap = process.mem1_heap();
    const auto capacity = heap->available_bytes();
    const auto objects = smgpc::compat::name_obj_runtime_state_count();
    FixtureLogger logger;
    auto require_retired = [&](std::size_t expected_capacity) {
        require(smgpc::runtime::RuntimeContext::try_instance() == nullptr, "failed/destroyed runtime remains published");
        require(JUTVideo::getManager() == nullptr, "failed/destroyed runtime retains its JUTVideo owner");
        require(smgpc::compat::ResourceHolderService::active() == nullptr, "failed/destroyed runtime retains archive service");
        require(smgpc::runtime::ScenarioCatalogOwnership::active() == nullptr, "failed/destroyed runtime retains scenario catalog publication");
        require(smgpc::runtime::SystemConfigService::active() == nullptr, "failed/destroyed runtime retains console settings owner");
        require(smgpc::compat::name_obj_runtime_state_count() == objects, "failed/destroyed runtime retains NameObj callbacks");
        require(heap->available_bytes() == expected_capacity, "failed/destroyed runtime retains mapped texture storage");
        bool absent = false;
        try { (void)smgpc::runtime::ScreenAlphaCaptureService::active(); }
        catch (const std::logic_error&) { absent = true; }
        require(absent, "failed/destroyed runtime retains screen-alpha ownership");
    };

    {
        auto reservation = heap->allocate(capacity - 128);
        const auto held_capacity = heap->available_bytes();
        bool rejected = false;
        try { smgpc::runtime::RuntimeContext runtime(logger, window, process); }
        catch (const std::bad_alloc&) { rejected = true; }
        require(rejected, "mapped texture exhaustion must fail the actual runtime constructor");
        require_retired(held_capacity);
    }
    require_retired(capacity);
    std::cout << "RuntimeContext: real mapped capture allocation failure restores video/context/scene/heap owners\n";

    logger.throw_after_registration = true;
    bool rejected = false;
    try { smgpc::runtime::RuntimeContext runtime(logger, window, process); }
    catch (const InjectedLogFailure&) { rejected = true; }
    require(rejected && !logger.registered.empty(), "logger failure must follow actual scene callback registration");
    require_retired(capacity);
    for (auto* object : logger.registered)
        require(!smgpc::compat::has_name_obj_runtime_state(object), "startup callback identity survived unwinding");
    logger.throw_after_registration = false;
    std::cout << "RuntimeContext: injected logger failure retires both original capture callbacks before global registration\n";

    (void)renderer.begin_frame();
    ConsoleDirectory console;
    require(setenv("SMGPC_NAND_DIR", console.path.c_str(), 1) == 0, "set test console root");
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        console.aspect(cycle);
        {
            smgpc::runtime::RuntimeContext runtime(logger, window, process);
            require(smgpc::runtime::RuntimeContext::try_instance() == &runtime && JUTVideo::getManager(), "reconstructed runtime is not registered");
            require(runtime.scheduler().snapshot().size() == 2, "reconstruction did not install the exact capture callbacks");
            require(MR::getScreenAlphaTexture(0) != nullptr, "reconstruction lost its mapped screen-alpha texture");
            require(smgpc::runtime::SystemConfigService::active() && SCGetAspectRatio() == cycle,
                    "actual startup did not publish imported console settings");
            require(MR::getScreenWidth() == (cycle ? 832 : 608) &&
                        runtime.wii_video().render_mode().viWidth == (cycle ? 686 : 670) &&
                        runtime.wii_video().scan_mode() == (cycle ? VI_PROGRESSIVE : VI_INTERLACE),
                    "original render-mode selection and screen width must use the imported SC setting");
            {
                CameraContext camera;
                require(camera.getAspect() == (cycle ? 16.0f / 9.0f : 4.0f / 3.0f),
                        "original CameraContext must use the same console setting after startup");
            }
            require(smgpc::runtime::ScenarioCatalogOwnership::active() == nullptr, "platform construction eagerly created the Game catalog");
            if (disc) {
                runtime.initialize_scenario_catalog(process);
                auto catalog = runtime.retain_scenario_catalog();
                require(ScenarioDataFunction::getScenarioDataParser() == &catalog->parser(), "resource-ready startup published a different parser");
                require(catalog->parser().mScenarioData.size() == 48, "resource-ready startup did not load the actual disc catalog");
            }
        }
        require_retired(capacity);
    }
    std::cout << "RuntimeContext: imported 4:3/16:9 settings drive original render mode and CameraContext; both reconstructions restore all owners\n";
    renderer.end_frame();
}
