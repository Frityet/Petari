#include <MSL_C/stdio.h>
#include "Game/System/AudSystemWrapper.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include "app/OriginalGameApplication.hpp"
#include "app/Application.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObjRegister.hpp"
#include "Game/Scene/IntermissionScene.hpp"
#include "Game/Scene/PlayTimerScene.hpp"
#include "Game/Scene/ScenarioSelectScene.hpp"
#include "Game/System/DrawSyncManager.hpp"
#include "Game/System/FileLoader.hpp"
#include "Game/System/FileRipper.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemFunction.hpp"
#include "Game/System/GameSequenceDirector.hpp"
#include "Game/System/GameSequenceFunction.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include "Game/System/GameSystemException.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/System/GameSystemResetAndPowerProcess.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/System/HeapMemoryWatcher.hpp"
#include "Game/System/MainLoopFramework.hpp"
#include "Game/System/NANDManager.hpp"
#include "Game/System/ResourceHolderManager.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadHolder.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/System/FunctionAsyncExecutor.hpp"
#include "Game/Screen/StarPointerDirector.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Scene/Scene.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/MessageHolderOwnership.hpp"
#include "runtime/ConsoleNandImport.hpp"
#include <aurora/system_config.hpp>
#include "runtime/DebugWpadInputScript.hpp"
#include "runtime/DebugWpadInputFile.hpp"
#include "runtime/OriginalProcessTrace.hpp"
#include <JSystem/JKernel/JKRExpHeap.hpp>
#include <JSystem/JKernel/JKRThread.hpp>
#include <JSystem/JUtility/JUTDirectPrint.hpp>
#include <JSystem/JUtility/JUTVideo.hpp>
#include <aurora/aurora.h>
#include <aurora/guest_thread.hpp>
#include <aurora/exception.hpp>
#include <aurora/wpad_motion.hpp>
#include <dolphin/gx/GXAurora.h>
#include <dolphin/ar.h>
#include <nw4r/lyt/init.h>
#include <SDL3/SDL_filesystem.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <string_view>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace smgpc::app {
namespace {

std::optional<std::string_view> option_value(const BootstrapConfiguration& configuration, std::string_view name) {
    const auto prefix = std::string(name) + "=";
    for (std::size_t i = 1; i < configuration.arguments.size(); ++i) {
        std::string_view value = configuration.arguments[i];
        if (value == name) {
            if (++i == configuration.arguments.size() || configuration.arguments[i].empty())
                throw std::invalid_argument(std::string(name) + " requires a value");
            return configuration.arguments[i];
        }
        if (value.starts_with(prefix)) {
            value.remove_prefix(prefix.size());
            if (value.empty()) throw std::invalid_argument(std::string(name) + " requires a value");
            return value;
        }
    }
    return std::nullopt;
}

template<class Integer>
Integer option_integer(std::string_view value, std::string_view name) {
    Integer result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
        throw std::invalid_argument(std::string(name) + " requires an integer in range");
    return result;
}

struct StageSelection {
    std::string stage;
    s32 scenario = 1;
    std::optional<s32> save_slot;
    bool save_preload_requested = false;
    bool requested = false;
};

struct LaunchOptions {
    std::uint64_t max_frames = 0;
    std::optional<StageSelection> selection;
};

LaunchOptions launch_options(const BootstrapConfiguration& configuration) {
    LaunchOptions options;
    if (const auto frames = option_value(configuration, "--max-frames"))
        options.max_frames = option_integer<std::uint64_t>(*frames, "--max-frames");
    const auto stage = option_value(configuration, "--stage");
    const auto scenario = option_value(configuration, "--scenario");
    const auto save_slot = option_value(configuration, "--save-slot");
    if (scenario && !stage) throw std::invalid_argument("--scenario requires --stage");
    if (save_slot && !stage) throw std::invalid_argument("--save-slot requires --stage");
    if (stage) {
        if (stage->size() >= sizeof(SceneControlInfo::mStage))
            throw std::invalid_argument("--stage exceeds the original scene controller's name capacity");
        const auto number = scenario ? option_integer<s32>(*scenario, "--scenario") : 1;
        if (number < 1) throw std::invalid_argument("--scenario must be positive");
        options.selection = StageSelection{std::string(*stage), number};
        if (save_slot) {
            const auto slot = option_integer<s32>(*save_slot, "--save-slot");
            if (slot < 1 || slot > 6) throw std::invalid_argument("--save-slot must be between 1 and 6");
            options.selection->save_slot = slot;
        }
    }
    return options;
}

void startup_phase(const char* phase) {
    std::fprintf(stderr, "[original-process] %s\n", phase);
    std::fflush(stderr);
}

#ifndef NDEBUG
class DebugFrameTiming {
public:
    DebugFrameTiming() {
        const char* value = std::getenv("SMGPC_DEBUG_FRAME_TIMING");
        enabled = value && std::string_view(value) == "1";
        if (enabled) boundary = Clock::now();
    }

    void begin_complete() {
        if (!enabled) return;
        begin = Clock::now();
        begin_ms += milliseconds(begin - boundary);
    }

    void process_complete() {
        if (!enabled) return;
        process = Clock::now();
        process_ms += milliseconds(process - begin);
    }

    void complete() {
        if (!enabled) return;
        const auto end = Clock::now();
        end_ms += milliseconds(end - process);
        const double elapsed = milliseconds(end - boundary);
        total_ms += elapsed;
        const auto index = count % recent_ms.size();
        window_ms += elapsed - recent_ms[index];
        recent_ms[index] = elapsed;
        ++count;
        boundary = end;
    }

    void report() const {
        if (!enabled) return;
        const auto window_count = std::min<std::uint64_t>(count, recent_ms.size());
        const double divisor = count ? static_cast<double>(count) : 1.0;
        std::fprintf(stderr,
                     "[original-process] Frame timing: completed=%llu wall_ms=%.3f mean_ms=%.3f begin_poll_ms=%.3f process_ms=%.3f end_ms=%.3f last_window_frames=%llu last_window_wall_ms=%.3f last_window_fps=%.3f (wall time includes polling, retrace and device waits)\n",
                     static_cast<unsigned long long>(count), total_ms, total_ms / divisor,
                     begin_ms / divisor, process_ms / divisor, end_ms / divisor,
                     static_cast<unsigned long long>(window_count), window_ms,
                     window_ms > 0.0 ? 1000.0 * window_count / window_ms : 0.0);
    }

private:
    using Clock = std::chrono::steady_clock;
    static double milliseconds(Clock::duration elapsed) {
        return std::chrono::duration<double, std::milli>(elapsed).count();
    }
    bool enabled = false;
    std::uint64_t count = 0;
    Clock::time_point boundary{}, begin{}, process{};
    std::array<double, 300> recent_ms{};
    double total_ms = 0.0, window_ms = 0.0;
    double begin_ms = 0.0, process_ms = 0.0, end_ms = 0.0;
};
#endif

void initialize_console_language(aurora::SystemConfiguration& settings) {
    // A newly created native console has no IPL settings. Select its initial
    // language from the mounted disc's region before original Game boot; an
    // imported/configured console keeps its authored setting unchanged.
    if (settings.document().find("IPL.LNG")) return;
    const auto* disc = DVDGetCurrentDiskID();
    u8 language;
    switch (disc->gameName[3]) {
    case 'J':
    case 'W':
        language = SC_LANG_JAPANESE;
        break;
    case 'K':
    case 'Q':
    case 'T':
        language = SC_LANG_KOREAN;
        break;
    case 'E': case 'B': case 'N':
    case 'D': case 'F': case 'H': case 'I': case 'L': case 'M':
    case 'P': case 'R': case 'S': case 'U': case 'V':
    case 'X': case 'Y': case 'Z':
        language = SC_LANG_ENGLISH;
        break;
    default:
        aurora::throw_host_exception<std::runtime_error>("Cannot select an initial console language for this disc region");
    }
    if (!settings.replace_u8(language, SC_ITEM_ID_IPL_LANGUAGE))
        aurora::throw_host_exception<std::runtime_error>("Cannot initialize the native console language setting");
    std::fprintf(stderr, "[original-process] Initialized console language %u for disc region %c\n", language, disc->gameName[3]);
}

void destroy_child_heaps(JKRHeap& parent) {
    while (auto* node = parent.mChildTree.getFirstChild()) {
        auto* child = node->getObject();
        destroy_child_heaps(*child);
        child->destroy();
    }
}

class OriginalProcess final {
public:
    OriginalProcess(const BootstrapConfiguration& configuration, std::optional<StageSelection> selection)
        : resources(configuration.resource_budget), stage_selection(std::move(selection)) {
        const char* directory = std::getenv("SMGPC_SAVE_DIR");
        if (directory && *directory) {
            save.set_host_directory(directory);
        } else {
            const char* preferences = SDL_GetPrefPath("Petari", "SuperMarioGalaxy");
            if (!preferences) throw std::runtime_error("Cannot determine the native save directory");
            save.set_host_directory(std::filesystem::path(preferences) / "NAND");
        }
        if (const char* source = std::getenv("SMGPC_NAND_DIR"); source && *source)
            (void)runtime::import_console_nand_directory(save.nand(), source, runtime::NandImportExisting::Preserve);
        settings = std::make_unique<aurora::SystemConfiguration>(save.nand());
        save.activate_nand();
#ifndef NDEBUG
        if (input_script.button_span_count() || input_script.pointer_span_count() || input_script.stick_span_count()) {
            std::fprintf(stderr, "[original-process] Debug controller script configured: %zu button spans, %zu pointer spans, %zu stick spans; zero-based inclusive frame ranges\n",
                         input_script.button_span_count(), input_script.pointer_span_count(), input_script.stick_span_count());
            for (const char* name : {"SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT", "SMGPC_DEBUG_WPAD_STICK_SCRIPT"})
                if (const auto* value = std::getenv(name); value && *value)
                    std::fprintf(stderr, "[original-process] Scripted controller evidence: %s=%s\n", name, value);
            std::fflush(stderr);
        }
#endif
    }

    ~OriginalProcess() { retire(); }

#ifndef NDEBUG
    void set_debug_observer(OriginalGameDebugObserver observer) { debug_observer = observer; }
#endif

    void initialize() {
        const aurora::os::GuestThreadExecutionScope execution;
        if (SingletonHolder<GameSystem>::get() || SingletonHolder<HeapMemoryWatcher>::get())
            throw std::logic_error("The original process already has an owner");
        startup_phase("Preparing original heap arenas");
        resources.prepare_mem2_arena(64U * 1024U * 1024U);
        root = resources.root_heap();
        marker = NameObj::markNativeRegistrations();
        started = true;
        const aurora::allocation::ClientAllocationScope game({true, true});
        startup_phase("Initializing SDK, layout allocator and original heap watcher");
        OSInitFastCast();
        DVDInit();
        initialize_console_language(*settings);
        VIInit();
        HeapMemoryWatcher::createRootHeap();
        HeapMemoryWatcher::sRootHeapGDDR3->bindNativeBackingStorage(resources.mem2_storage());
        OSInitMutex(&MR::MutexHolder<0>::sMutex);
        OSInitMutex(&MR::MutexHolder<1>::sMutex);
        OSInitMutex(&MR::MutexHolder<2>::sMutex);
        nw4r::lyt::LytInit();
        MR::setLayoutDefaultAllocator();
        SingletonHolder<HeapMemoryWatcher>::init();
        auto* heaps = SingletonHolder<HeapMemoryWatcher>::get();
        heaps->setCurrentHeapToStationedHeap();
        {
            const aurora::allocation::HostAllocationScope host;
            stationed = heaps->mStationedHeapNapa->retainNativeLifetime();
        }
        startup_phase("Initializing original file and exception services");
        FileRipper::setup(0x20000, MR::getStationedHeapNapa());
        GameSystemException::init();
        // The original display uses the direct-print framebuffer even when
        // native exception reporting does not create the Wii exception viewer.
        if (!JUTDirectPrint::getManager()) direct_print = JUTDirectPrint::start();
        MR::initAcosTable();
        startup_phase("Constructing GameSystem");
        SingletonHolder<GameSystem>::init();
        startup_phase("Entering GameSystem::init");
        SingletonHolder<GameSystem>::get()->init();
        startup_phase("GameSystem::init completed");
    }

    void frame(render::AuroraWindow& window, std::uint64_t frame_index) {
        const aurora::os::GuestThreadExecutionScope execution;
        const aurora::allocation::ClientAllocationScope game({true, true});
        publish_input(window, frame_index);
        auto& system = *SingletonHolder<GameSystem>::get();
        if (system.mSceneController && system.mSceneController->mScene) {
            system.mSceneController->mScene->beginNativeFrame();
        }
        system.frameLoop();
#ifndef NDEBUG
        frame_trace.capture(system, frame_index);
#endif
        request_selected_stage(system);
#ifndef NDEBUG
        if (debug_observer.after_frame) debug_observer.after_frame(debug_observer.context, system, frame_index);
#endif
    }

private:
    void request_selected_stage(GameSystem& system) {
        if (!stage_selection || stage_selection->requested || !system.isDoneLoadSystemArchive()) return;
        auto* sequence = system.mSequenceDirector;
        auto* controller = system.mSceneController;
        if (!sequence || !sequence->isInitializedGameDataHolder() || !controller || !controller->mScenarioParser)
            aurora::throw_host_exception<std::logic_error>("Stage launch requires the original initialized game data and scenario owners");
        if (GameSystemFunction::isResetProcessing() || GameSystemFunction::isOccurredSystemWarning() ||
            GameSequenceFunction::isActiveSaveDataHandleSequence() || !controller->mScene ||
            controller->getCurrentSceneForExecute() != controller->mScene || controller->isExistRequest() ||
            controller->mNextSceneControlInfo.mScene[0] != '\0') return;
        const auto* scenario = controller->mScenarioParser->getScenarioData(stage_selection->stage.c_str());
        if (!scenario || !scenario->getScenarioDataIter(stage_selection->scenario).isValid())
            aurora::throw_host_exception<std::invalid_argument>("Requested stage/scenario is absent from the original scenario catalog");
        if (stage_selection->save_slot) {
            if (!stage_selection->save_preload_requested) {
                GameSequenceFunction::startPreLoadSaveDataSequence();
                stage_selection->save_preload_requested = true;
                return;
            }
            if (!GameSequenceFunction::isSuccessSaveDataHandleSequence())
                aurora::throw_host_exception<std::runtime_error>("The original save sequence could not load the selected save");
            auto* save_sequence = sequence->mSaveDataHandleSequence;
            auto* file = save_sequence->getCurrentUserFile();
            const auto slot = *stage_selection->save_slot;
            save_sequence->restoreUserFileConfigData(file, slot);
            if (file->mIsConfigDataCorrupted || !file->isCreated())
                aurora::throw_host_exception<std::runtime_error>("The selected save slot is empty or has corrupt configuration data");
            GameSequenceFunction::startGameDataLoadSequence(slot, file->isLastLoadedMario());
            if (file->mIsGameDataCorrupted)
                aurora::throw_host_exception<std::runtime_error>("The selected save slot has corrupt game data");
            std::fprintf(stderr, "[original-process] Loaded original save slot %d (%s): %d stars\n",
                         slot, file->getGameDataName(), file->getPowerStarNum());
        }
        std::fprintf(stderr, "[original-process] Requesting authored stage %s scenario %d through GameSequence\n",
                     scenario->mGalaxyName, stage_selection->scenario);
        std::fflush(stderr);
        // The original sequence owns story decisions, entry (0,0), wipes,
        // scene loading and start. This frontend supplies only the selection.
        MR::requestChangeStageInGameMoving(scenario->mGalaxyName, stage_selection->scenario);
        stage_selection->requested = true;
    }

    void publish_input(const render::AuroraWindow& window, std::uint64_t frame_index) {
        auto& input = aurora::wpad_service();
        input.begin_frame();
        input.set_device_type(WPAD_CHAN0, aurora::WpadDeviceType::Freestyle);
        input.set_connected(WPAD_CHAN0, true);
        using Button = render::InputButton;
        const std::pair<Button, u32> buttons[] = {
            {Button::CORE_PAD_A, WPAD_BUTTON_A}, {Button::CORE_PAD_B, WPAD_BUTTON_B},
            {Button::CORE_PAD_UP, WPAD_BUTTON_UP}, {Button::CORE_PAD_DOWN, WPAD_BUTTON_DOWN},
            {Button::CORE_PAD_LEFT, WPAD_BUTTON_LEFT}, {Button::CORE_PAD_RIGHT, WPAD_BUTTON_RIGHT},
            {Button::CORE_PAD_PLUS, WPAD_BUTTON_PLUS}, {Button::CORE_PAD_MINUS, WPAD_BUTTON_MINUS},
            {Button::CORE_PAD_C, WPAD_BUTTON_C}, {Button::CORE_PAD_Z, WPAD_BUTTON_Z},
        };
        u32 mask = 0;
        for (const auto& [button, bit] : buttons)
            if (window.is_input_pressed(button)) mask |= bit;
        auto pointer = window.input_pointer_state();
        float x = float(window.is_input_pressed(Button::SUB_STICK_RIGHT)) - float(window.is_input_pressed(Button::SUB_STICK_LEFT));
        float y = float(window.is_input_pressed(Button::SUB_STICK_UP)) - float(window.is_input_pressed(Button::SUB_STICK_DOWN));
#ifndef NDEBUG
        auto applied = input_script.apply(frame_index, mask, pointer, x, y);
        const auto file_applied = input_file.apply(frame_index, mask, pointer, x, y);
        applied.buttons |= file_applied.buttons;
        applied.pointer |= file_applied.pointer;
        applied.stick |= file_applied.stick;
        if (applied.buttons != script_applied.buttons || applied.pointer != script_applied.pointer || applied.stick != script_applied.stick) {
            std::fprintf(stderr, "[original-process] Debug controller script frame %llu: buttons=%s pointer=%s stick=%s hold=0x%08x axes=%g,%g\n",
                         static_cast<unsigned long long>(frame_index), applied.buttons ? "active" : "inactive",
                         applied.pointer ? "active" : "inactive", applied.stick ? "active" : "inactive", mask, x, y);
            std::fflush(stderr);
            script_applied = applied;
        }
#else
        (void)frame_index;
#endif
        input.set_button_mask(WPAD_CHAN0, mask);
        input.set_pointer_resolution(WPAD_CHAN0, MR::getFrameBufferWidth(), MR::getFrameBufferHeight());
        input.set_pointer(WPAD_CHAN0, pointer.x, pointer.y, pointer.valid);
        input.set_distance_to_display(WPAD_CHAN0, pointer.valid ? 1.0f : 0.0f);
        input.set_sub_stick(WPAD_CHAN0, x, y);
        const auto acceleration = shake.sample(window.is_input_pressed(Button::CORE_PAD_SWING));
        input.set_core_acceleration(WPAD_CHAN0, acceleration.x, acceleration.y, acceleration.z);
        input.set_sub_acceleration(WPAD_CHAN0, 0.0f, 0.0f, 1.0f);
        input.dispatch_callbacks();
    }

    void retire() {
        if (!started) return;
        const aurora::os::GuestThreadExecutionScope execution;
        const aurora::allocation::HostAllocationScope host;
        OSSetPowerCallback(nullptr);
        OSSetResetCallback(nullptr);
        aurora::wpad_service().exchange_client({});
        auto* system = SingletonHolder<GameSystem>::get();
        auto* objects = system ? system->mObjHolder : nullptr;
        // Stop producers before releasing the queues, files and resources that
        // original worker callbacks borrow, including failed partial startup.
        if (objects) {
            delete objects->mFunctionAsyncExecutor;
            objects->mFunctionAsyncExecutor = nullptr;
        }
        delete SingletonHolder<NANDManager>::release();
        DrawSyncManager::quiesceNativeCallbacks();
        if (system && system->mSceneController) {
            auto* controller = system->mSceneController;
            delete std::exchange(controller->mScene, nullptr);
            delete std::exchange(controller->mScenarioSelectScene, nullptr);
            delete std::exchange(controller->mPlayTimerScene, nullptr);
            delete std::exchange(controller->mIntermissionScene, nullptr);
        }
        if (objects) delete std::exchange(objects->mStarPointerDirector, nullptr);
        if (DrawSyncManager::sInstance) DrawSyncManager::end();
        // Original heap retirement releases raw Game arrays as arrays. Remove
        // native resources while all borrowed original records are still alive;
        // never individually delete an array element through NameObj*.
        while (auto* object = NameObj::newestNativeObjectSince(marker, nullptr, nullptr)) {
            if (auto* actor = dynamic_cast<LayoutActor*>(object)) actor->releaseNativeResources();
            if (auto* actor = dynamic_cast<LiveActor*>(object)) actor->releaseNativeResources();
            object->detachNativeHolder();
            object->retireNativeLifetime();
        }
        // Actor sound resources are retired. Destroy the actual audio owner
        // before its name resources, GameSystem publication and heaps disappear.
        if (objects) delete std::exchange(objects->mAudioSystem, nullptr);
        if (objects) delete std::exchange(objects->mWPadHolder, nullptr);
        if (objects) runtime::destroy_message_holder(objects->mMessageHolder);
        if (objects) delete std::exchange(objects->mParticleResHolder, nullptr);
        if (system && system->mSceneController)
            delete std::exchange(system->mSceneController->mScenarioParser, nullptr);
        resources.root_heap()->retireNativeResourceReferences();
        if (auto* manager = SingletonHolder<ResourceHolderManager>::get()) {
            manager->validateRetirement();
            delete SingletonHolder<ResourceHolderManager>::release();
        }
        FileLoader::destroy(SingletonHolder<FileLoader>::get());
        AuroraDrainGXCommands();
        GXSetDrawDoneCallback(nullptr);
        delete MainLoopFramework::sManager;
        MainLoopFramework::sManager = nullptr;
        if (JUTVideo::getManager()) {
            JUTVideo::destroyManager();
            VISetBlack(TRUE);
            VISetNextFrameBuffer(nullptr);
            VIFlush();
            VIWaitForRetrace();
        }
        if (direct_print) {
            direct_print->changeFrameBuffer(nullptr, 0, 0);
            delete direct_print;
            JUTDirectPrint::sDirectPrint = nullptr;
            direct_print = nullptr;
        }
        ARReset();
        // JKRThread owns cancellation/join of each actual SDK worker. Remaining
        // ARAM/decompression workers must precede their parent heap disposal.
        while (auto* link = JKRThread::sThreadList.getFirst()) delete link->getObject();
        SingletonHolder<ResourceHolderManager>::release();
        SingletonHolder<GameSystemResetAndPowerProcess>::release();
        SingletonHolder<NameObjRegister>::release();
        SingletonHolder<GameSystem>::release();
        SingletonHolder<HeapMemoryWatcher>::release();
        stationed.reset();
        root.reset();
        destroy_child_heaps(*resources.root_heap());
        HeapMemoryWatcher::sRootHeapGDDR3 = nullptr;
        started = false;
    }

    resource::GameResourceRuntime resources;
    runtime::SaveDataService save;
    std::unique_ptr<aurora::SystemConfiguration> settings;
    JKRHeap::Handle root;
    JKRHeap::Handle stationed;
    std::optional<StageSelection> stage_selection;
    NameObj::NativeRegistrationMarker marker;
    aurora::WpadShakeGesture shake;
#ifndef NDEBUG
    OriginalGameDebugObserver debug_observer;
    runtime::DebugWpadInputScript input_script = runtime::DebugWpadInputScript::from_environment();
    runtime::DebugWpadInputFile input_file = runtime::DebugWpadInputFile::from_environment();
    runtime::DebugWpadInputScript::Applied script_applied;
    runtime::OriginalProcessTrace frame_trace;
#endif
    JUTDirectPrint* direct_print = nullptr;
    bool started = false;
};
}

int run_original_game(const BootstrapConfiguration& configuration, logging::ILogger& logger
#ifndef NDEBUG
                      , OriginalGameDebugObserver observer
#endif
) {
    auto options = launch_options(configuration);
    render::AuroraWindow window({configuration.window_width, configuration.window_height, configuration.window_title});
    OriginalProcess process(configuration, std::move(options.selection));
#ifndef NDEBUG
    process.set_debug_observer(observer);
#endif
    logger.info(logging::Category::APP, logging::Message{"Booting the original GameSystem"});
    process.initialize();
    logger.info(logging::Category::APP, logging::Message{"Running the original GameSystem frame loop"});
    std::uint64_t completed_frames = 0;
    const char* screenshot_path = std::getenv("SMGPC_SCREENSHOT_PATH");
    const char* screenshot_frame_text = std::getenv("SMGPC_SCREENSHOT_FRAME");
    const auto screenshot_frame = screenshot_frame_text && *screenshot_frame_text
        ? option_integer<std::uint64_t>(screenshot_frame_text, "SMGPC_SCREENSHOT_FRAME") : 1U;
    bool screenshot_written = false;
#ifndef NDEBUG
    DebugFrameTiming timing;
#endif
    while ((options.max_frames == 0 || completed_frames < options.max_frames) && window.poll_events()) {
        if (!aurora_begin_frame()) continue;
#ifndef NDEBUG
        timing.begin_complete();
#endif
        try {
            process.frame(window, completed_frames);
        } catch (...) {
            aurora_end_frame();
            throw;
        }
#ifndef NDEBUG
        timing.process_complete();
#endif
        aurora_end_frame();
        ++completed_frames;
        if (screenshot_path && *screenshot_path && !screenshot_written && completed_frames >= screenshot_frame) {
            window.request_screenshot_png(screenshot_path);
            screenshot_written = true;
        }
#ifndef NDEBUG
        timing.complete();
#endif
    }
#ifndef NDEBUG
    timing.report();
#endif
    logger.info(logging::Category::APP, logging::Message{"Original GameSystem stopped after {} completed frames"}, completed_frames);
    return 0;
}
}
