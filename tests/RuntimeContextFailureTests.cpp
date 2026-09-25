#include "runtime/RuntimeContext.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "Game/Screen/CaptureScreenDirector.hpp"
#include <aurora/system_config.hpp>
#include "JSystem/JUtility/JUTVideo.hpp"
#include <aurora/dvd.h>
#include <cstdlib>
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
            require(smgpc::runtime::try_active_scene_scheduler() == &runtime->scheduler(),
                    "startup clients must share the real runtime scheduler binding");
            require(smgpc::compat::has_name_obj_runtime_state(&runtime->capture_screen_director()),
                    "failure fixture must reach the actual capture director registration");
            registered = smgpc::compat::snapshot_name_obj_runtime_objects();
            throw InjectedLogFailure{};
        }
    };
}

int main() {
    Environment save_environment("SMGPC_SAVE_DIR");
    Environment nand_environment("SMGPC_NAND_DIR");
    smgpc::render::AuroraWindow window({.width = 640, .height = 456, .title = "Runtime constructor failure ownership"});
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
        require(smgpc::runtime::try_active_scene_scheduler() == nullptr, "failed/destroyed runtime retains scheduler binding");
        require(JUTVideo::getManager() == nullptr, "failed/destroyed runtime retains its JUTVideo owner");
        require(smgpc::runtime::RuntimeContext::try_instance() == nullptr, "failed/destroyed runtime retains its publication");
        require(smgpc::runtime::ScenarioCatalogOwnership::active() == nullptr, "failed/destroyed runtime retains scenario catalog publication");
        require(smgpc::runtime::ParticleResourceOwnership::active() == nullptr, "failed/destroyed runtime retains particle resources");
        require(aurora::SystemConfiguration::active() == nullptr, "failed/destroyed runtime retains console settings owner");
        require(smgpc::compat::name_obj_runtime_state_count() == objects, "failed/destroyed runtime retains NameObj callbacks");
        require(heap->available_bytes() == expected_capacity, "failed/destroyed runtime retains mapped texture storage");
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
    require(rejected && !logger.registered.empty(), "logger failure must follow actual capture director registration");
    require_retired(capacity);
    for (auto* object : logger.registered)
        require(!smgpc::compat::has_name_obj_runtime_state(object), "startup NameObj identity survived unwinding");
    logger.throw_after_registration = false;
    std::cout << "RuntimeContext: injected logger failure retires the actual capture director before global registration\n";
}
