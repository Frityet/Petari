#include "compat/MetrowerksStdCompat.hpp"
#include "Game/System/DrawSyncManager.hpp"
#include "Game/System/FileRipper.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemException.hpp"
#include "Game/System/GameSystemFontHolder.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/System/HeapMemoryWatcher.hpp"
#include "Game/System/Language.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "compat/NandSdkBinding.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "runtime/ConsoleNandImport.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SystemConfigService.hpp"
#include <JSystem/JKernel/JKRExpHeap.hpp>
#include <aurora/allocation.hpp>
#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <aurora/guest_thread.hpp>
#include <nw4r/lyt/init.h>
#include <revolution.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <unistd.h>

namespace {
void checkpoint(const char* operation) {
    std::fprintf(stderr, "[original-startup] %s\n", operation);
    std::fflush(stderr);
}
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

// This first executable is a process-isolated startup diagnostic. Keep service
// storage alive through failures: original workers may already borrow it, and
// complete process teardown is a separate owner boundary still being recovered.
// The runner terminates the process after reporting its precise frontier.
struct Bootstrap {
    smgpc::resource::GameResourceRuntime resources;
    smgpc::runtime::DvdFileSystemService dvd{"/"};
    smgpc::runtime::ArchiveMountService archives{dvd};
    smgpc::runtime::SaveDataService save;
    std::unique_ptr<smgpc::runtime::SystemConfigService> settings;
    std::unique_ptr<smgpc::compat::NandSdkBinding> nand;
    std::shared_ptr<smgpc::compat::JkrAllocationDomain> root;
    std::shared_ptr<smgpc::compat::JkrAllocationDomain> stationed;
    std::unique_ptr<smgpc::compat::ResourceHolderService> holders;

    void configure_nand() {
        auto pattern = (std::filesystem::temp_directory_path() / "petari-original-startup-nand-XXXXXX").string();
        const auto* created = mkdtemp(pattern.data());
        require(created != nullptr, "could not create an isolated startup NAND directory");
        const std::filesystem::path directory(created);
        save.set_host_directory(directory);
        if (const auto* source = std::getenv("SMGPC_NAND_DIR")) {
            const auto imported = smgpc::runtime::import_console_nand_directory(save.nand(), source,
                smgpc::runtime::NandImportExisting::Preserve);
            std::fprintf(stderr, "[original-startup] imported NAND files: %zu (%zu bytes)\n",
                imported.imported_files, imported.imported_bytes);
        }
        settings = std::make_unique<smgpc::runtime::SystemConfigService>(save.nand());
        nand = std::make_unique<smgpc::compat::NandSdkBinding>(save);
        std::fprintf(stderr, "[original-startup] private NAND: %s\n", directory.c_str());
    }
};

void original_main_initialization(Bootstrap& host) {
    const aurora::os::GuestThreadExecutionScope execution;
    host.resources.host_heaps()->prepare_mem2_arena(64U * 1024U * 1024U);
    // Retain the already explicit native JKR root. This does not create a child
    // cohort before HeapMemoryWatcher consumes the original remaining arenas.
    host.root = smgpc::compat::JkrAllocationDomain::retain_heap(
        host.resources.host_heaps(), host.resources.host_heaps()->root_heap(), host.resources.host_heaps());
    const aurora::allocation::ClientAllocationScope game({true, true});
    checkpoint("OSInitFastCast / DVDInit / VIInit");
    OSInitFastCast();
    DVDInit();
    VIInit();
    checkpoint("HeapMemoryWatcher::createRootHeap");
    HeapMemoryWatcher::createRootHeap();
    OSInitMutex(&MR::MutexHolder<0>::sMutex);
    OSInitMutex(&MR::MutexHolder<1>::sMutex);
    OSInitMutex(&MR::MutexHolder<2>::sMutex);
    checkpoint("LytInit / original layout allocator");
    nw4r::lyt::LytInit();
    MR::setLayoutDefaultAllocator();
    checkpoint("SingletonHolder<HeapMemoryWatcher>::init");
    SingletonHolder<HeapMemoryWatcher>::init();
    auto* watcher = SingletonHolder<HeapMemoryWatcher>::get();
    watcher->setCurrentHeapToStationedHeap();
    {
        const aurora::allocation::HostAllocationScope allocations;
        host.stationed = smgpc::compat::JkrAllocationDomain::retain_heap(host.root, *watcher->mStationedHeapNapa);
        host.holders = std::make_unique<smgpc::compat::ResourceHolderService>(host.dvd, host.stationed, host.resources.mem1_heap());
    }
    checkpoint("FileRipper::setup");
    FileRipper::setup(0x20000, MR::getStationedHeapNapa());
    checkpoint("GameSystemException::init / initAcosTable");
    GameSystemException::init();
    MR::initAcosTable();
    checkpoint("SingletonHolder<GameSystem>::init");
    SingletonHolder<GameSystem>::init();
    auto* system = SingletonHolder<GameSystem>::get();
    checkpoint("GameSystem::init entering (all original children)");
    system->init();
    require(system->mObjHolder && system->mFontHolder && system->mFontHolder->mEmbeddedMessageFont &&
                system->mSequenceDirector && system->mSceneController && system->mErrorWatcher &&
                system->mFrameControl && system->mStationedArchiveLoader &&
                system->mHomeButtonStateNotifier && system->mDimmingWatcher && DrawSyncManager::sInstance,
            "GameSystem::init returned without its required original child graph");
    require(MR::getLanguage() == system->mObjHolder->mLanguage,
            "the current language must be the actual GameSystem object-holder field");
    checkpoint("GameSystem::init returned with original child identities");
}
}

int main() {
    const auto* disc = std::getenv("SMGPC_REAL_DISC");
    if (!disc || !*disc) {
        checkpoint("SMGPC_REAL_DISC is required; no startup executed");
        return 77;
    }
    try {
        AuroraConfig config{};
        config.appName = "Original GameSystem startup diagnostic";
#if defined(__APPLE__)
        config.desiredBackend = BACKEND_METAL;
#else
        config.desiredBackend = BACKEND_VULKAN;
#endif
        config.allowCpuAdapter = true;
        config.windowWidth = 640;
        config.windowHeight = 480;
        config.vsync = false;
        config.pauseOnFocusLost = false;
        config.logLevel = LOG_WARNING;
        config.cachePath = std::getenv("AURORA_TEST_CACHE_PATH");
        const auto initialized = aurora_initialize(0, nullptr, &config);
        require(initialized.backend == config.desiredBackend, "the original startup requires the actual GPU backend");
        require(aurora_dvd_open(disc), "the original startup requires a readable game disc");
        auto* host = new Bootstrap;
        host->configure_nand();
        original_main_initialization(*host);
        checkpoint("stationed-loader completion and full typed retirement remain pending; this is not a full startup pass");
        std::_Exit(77);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "[original-startup] stopped: %s\n", error.what());
        std::fflush(stderr);
        std::_Exit(1);
    }
}
