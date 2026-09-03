#include "runtime/RuntimeContext.hpp"
#include "runtime/ScreenAlphaCaptureService.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "Game/Screen/ScreenAlphaCapture.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include "JSystem/JUtility/JUTVideo.hpp"
#include <aurora/dvd.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

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
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        {
            smgpc::runtime::RuntimeContext runtime(logger, window, process);
            require(smgpc::runtime::RuntimeContext::try_instance() == &runtime && JUTVideo::getManager(), "reconstructed runtime is not registered");
            require(runtime.scheduler().snapshot().size() == 2, "reconstruction did not install the exact capture callbacks");
            require(MR::getScreenAlphaTexture(0) != nullptr, "reconstruction lost its mapped screen-alpha texture");
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
    std::cout << "RuntimeContext: two complete reconstructions restore full MEM1 capacity and every owner identity\n";
    renderer.end_frame();
}
