#include "compat/MetrowerksStdCompat.hpp"
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
#include "Game/System/WPad.hpp"
#include "Game/System/WPadHolder.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/DrawSyncManagerLifetime.hpp"
#include "compat/FileLoaderOwnership.hpp"
#include "compat/FunctionAsyncExecutorOwnership.hpp"
#include "compat/NandSdkBinding.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "compat/StarPointerDepthOwnership.hpp"
#include "layout/LayoutHost.hpp"
#include "scene/OriginalSceneSupport.hpp"
#include "scene/SceneNameObjRegistry.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "runtime/MessageHolderOwnership.hpp"
#include "runtime/ConsoleNandImport.hpp"
#include "runtime/SystemConfigService.hpp"
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
#include <charconv>
#include <cstdint>
#include <string_view>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace smgpc::compat {
void destroy_wpad_children(WPad&) noexcept;
}

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
    if (scenario && !stage) throw std::invalid_argument("--scenario requires --stage");
    if (stage) {
        if (stage->size() >= sizeof(SceneControlInfo::mStage))
            throw std::invalid_argument("--stage exceeds the original scene controller's name capacity");
        const auto number = scenario ? option_integer<s32>(*scenario, "--scenario") : 1;
        if (number < 1) throw std::invalid_argument("--scenario must be positive");
        options.selection = StageSelection{std::string(*stage), number};
    }
    return options;
}

void startup_phase(const char* phase) {
    std::fprintf(stderr, "[original-process] %s\n", phase);
    std::fflush(stderr);
}

void initialize_console_language(runtime::SystemConfigService& settings) {
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
        : resources(configuration.resource_budget), dvd("/"),
          archives(std::make_unique<runtime::ArchiveMountService>(dvd)), stage_selection(std::move(selection)) {
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
        settings = std::make_unique<runtime::SystemConfigService>(save.nand());
        nand = std::make_unique<compat::NandSdkBinding>(save);
    }

    ~OriginalProcess() { retire(); }

    void initialize() {
        const aurora::os::GuestThreadExecutionScope execution;
        if (SingletonHolder<GameSystem>::get() || SingletonHolder<HeapMemoryWatcher>::get())
            throw std::logic_error("The original process already has an owner");
        startup_phase("Preparing original heap arenas");
        resources.host_heaps()->prepare_mem2_arena(64U * 1024U * 1024U);
        root = compat::JkrAllocationDomain::retain_heap(resources.host_heaps(), resources.host_heaps()->root_heap(), resources.host_heaps());
        marker = compat::mark_name_obj_runtime_registrations();
        started = true;
        const aurora::allocation::ClientAllocationScope game({true, true});
        startup_phase("Initializing SDK, layout allocator and original heap watcher");
        OSInitFastCast();
        DVDInit();
        initialize_console_language(*settings);
        VIInit();
        HeapMemoryWatcher::createRootHeap();
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
            stationed = compat::JkrAllocationDomain::retain_heap(root, *heaps->mStationedHeapNapa);
            holders = std::make_unique<compat::ResourceHolderService>(dvd, stationed, resources.mem1_heap());
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

    void frame(render::AuroraWindow& window) {
        const aurora::os::GuestThreadExecutionScope execution;
        const aurora::allocation::ClientAllocationScope game({true, true});
        publish_input(window);
        scene::begin_original_scene_frame();
        auto& system = *SingletonHolder<GameSystem>::get();
        system.frameLoop();
        request_selected_stage(system);
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
        std::fprintf(stderr, "[original-process] Requesting authored stage %s scenario %d through GameSequence\n",
                     scenario->mGalaxyName, stage_selection->scenario);
        std::fflush(stderr);
        // The original sequence owns story decisions, entry (0,0), wipes,
        // scene loading and start. This frontend supplies only the selection.
        MR::requestChangeStageInGameMoving(scenario->mGalaxyName, stage_selection->scenario);
        stage_selection->requested = true;
    }

    void publish_input(const render::AuroraWindow& window) {
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
        input.set_button_mask(WPAD_CHAN0, mask);
        const auto pointer = window.input_pointer_state();
        input.set_pointer_resolution(WPAD_CHAN0, MR::getFrameBufferWidth(), MR::getFrameBufferHeight());
        input.set_pointer(WPAD_CHAN0, pointer.x, pointer.y, pointer.valid);
        input.set_distance_to_display(WPAD_CHAN0, pointer.valid ? 1.0f : 0.0f);
        const float x = float(window.is_input_pressed(Button::SUB_STICK_RIGHT)) - float(window.is_input_pressed(Button::SUB_STICK_LEFT));
        const float y = float(window.is_input_pressed(Button::SUB_STICK_UP)) - float(window.is_input_pressed(Button::SUB_STICK_DOWN));
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
            compat::destroy_function_async_executor(objects->mFunctionAsyncExecutor);
            objects->mFunctionAsyncExecutor = nullptr;
        }
        delete SingletonHolder<NANDManager>::release();
        compat::quiesce_draw_sync();
        if (system && system->mSceneController) {
            auto* controller = system->mSceneController;
            delete std::exchange(controller->mScene, nullptr);
            delete std::exchange(controller->mScenarioSelectScene, nullptr);
            delete std::exchange(controller->mPlayTimerScene, nullptr);
            delete std::exchange(controller->mIntermissionScene, nullptr);
        }
        if (objects) compat::destroy_star_pointer_director(objects->mStarPointerDirector);
        if (DrawSyncManager::sInstance) DrawSyncManager::end();
        // Original heap retirement releases raw Game arrays as arrays. Remove
        // native sidecars while all borrowed original records are still alive;
        // never individually delete an array element through NameObj*.
        while (auto* object = compat::newest_name_obj_runtime_object_since_if(marker, nullptr, nullptr)) {
            layout::release_layout_actor_if_registered(object);
            if (auto* actor = dynamic_cast<LiveActor*>(object)) compat::release_actor_runtime_state(actor);
            scene::unregister_scene_name_obj(*object);
            compat::release_name_obj_runtime_state(object);
        }
        if (objects && objects->mWPadHolder) {
            for (auto* pad : objects->mWPadHolder->mPad) {
                compat::destroy_wpad_children(*pad);
                delete pad;
            }
            for (s32 channel = 0; channel < WPAD_MAX_CONTROLLERS; ++channel)
                delete[] objects->mWPadHolder->mReadDataInfoArray[channel].mStatusArray;
            delete[] objects->mWPadHolder->mReadDataInfoArray;
            delete std::exchange(objects->mWPadHolder, nullptr);
        }
        if (objects) runtime::destroy_message_holder(objects->mMessageHolder);
        holders.reset();
        compat::destroy_file_loader(SingletonHolder<FileLoader>::get());
        archives.reset();
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
        destroy_child_heaps(resources.host_heaps()->root_heap());
        HeapMemoryWatcher::sRootHeapGDDR3 = nullptr;
        started = false;
    }

    resource::GameResourceRuntime resources;
    runtime::DvdFileSystemService dvd;
    std::unique_ptr<runtime::ArchiveMountService> archives;
    runtime::SaveDataService save;
    std::unique_ptr<runtime::SystemConfigService> settings;
    std::unique_ptr<compat::NandSdkBinding> nand;
    std::shared_ptr<compat::JkrAllocationDomain> root;
    std::shared_ptr<compat::JkrAllocationDomain> stationed;
    std::unique_ptr<compat::ResourceHolderService> holders;
    std::optional<StageSelection> stage_selection;
    compat::NameObjRuntimeRegistrationMarker marker;
    aurora::WpadShakeGesture shake;
    JUTDirectPrint* direct_print = nullptr;
    bool started = false;
};
}

int run_original_game(const BootstrapConfiguration& configuration, logging::ILogger& logger) {
    auto options = launch_options(configuration);
    render::AuroraWindow window({configuration.window_width, configuration.window_height, configuration.window_title});
    OriginalProcess process(configuration, std::move(options.selection));
    logger.info(logging::Category::APP, logging::Message{"Booting the original GameSystem"});
    process.initialize();
    logger.info(logging::Category::APP, logging::Message{"Running the original GameSystem frame loop"});
    std::uint64_t completed_frames = 0;
    const char* screenshot_path = std::getenv("SMGPC_SCREENSHOT_PATH");
    const char* screenshot_frame_text = std::getenv("SMGPC_SCREENSHOT_FRAME");
    const auto screenshot_frame = screenshot_frame_text && *screenshot_frame_text
        ? option_integer<std::uint64_t>(screenshot_frame_text, "SMGPC_SCREENSHOT_FRAME") : 1U;
    bool screenshot_written = false;
    while ((options.max_frames == 0 || completed_frames < options.max_frames) && window.poll_events()) {
        if (!aurora_begin_frame()) continue;
        try {
            process.frame(window);
        } catch (...) {
            aurora_end_frame();
            throw;
        }
        aurora_end_frame();
        ++completed_frames;
        if (screenshot_path && *screenshot_path && !screenshot_written && completed_frames >= screenshot_frame) {
            window.request_screenshot_png(screenshot_path);
            screenshot_written = true;
        }
    }
    logger.info(logging::Category::APP, logging::Message{"Original GameSystem stopped after {} completed frames"}, completed_frames);
    return 0;
}
}
