#include "compat/MetrowerksStdCompat.hpp"
#include "compat/FileLoaderOwnership.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/System/FileLoader.hpp"
#include "Game/System/FileRipper.hpp"
#include "Game/System/HeapMemoryWatcher.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "JSystem/JKernel/JKRExpHeap.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/Yaz0.hpp"
#include "runtime/RuntimeServices.hpp"
#include <aurora/allocation.hpp>
#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <aurora/exception.hpp>
#include <aurora/guest_thread.hpp>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) aurora::throw_host_exception<std::runtime_error>(message);
}

void destroy_children(JKRHeap& parent) {
    while (auto* child = parent.mChildTree.getFirstChild()) {
        auto* heap = child->getObject();
        destroy_children(*heap);
        heap->destroy();
    }
}

// Complete original HeapMemoryWatcher and FileLoader objects, with no partial
// GameSystem or replacement file/thread implementation. The outer owner keeps
// every original heap alive until all file workers and archives have retired.
struct OriginalFileProcess {
    std::shared_ptr<smgpc::compat::JkrHeapRuntime> heaps;
    JKRErrorHandler old_error = JKRHeap::mErrorHandler;

    OriginalFileProcess() {
        heaps = smgpc::compat::JkrHeapRuntime::create(64U * 1024U * 1024U);
        heaps->prepare_mem2_arena(64U * 1024U * 1024U);
        const aurora::allocation::ClientAllocationScope allocations({true, true});
        HeapMemoryWatcher::createRootHeap();
        OSInitMutex(&MR::MutexHolder<0>::sMutex);
        OSInitMutex(&MR::MutexHolder<1>::sMutex);
        OSInitMutex(&MR::MutexHolder<2>::sMutex);
        SingletonHolder<HeapMemoryWatcher>::init();
        SingletonHolder<HeapMemoryWatcher>::get()->setCurrentHeapToStationedHeap();
        // A small original read buffer forces real streaming refills on retail
        // compressed resources. No decompressor state is replaced or skipped.
        FileRipper::setup(0x40, &heap());
    }

    ~OriginalFileProcess() {
        const aurora::allocation::ClientAllocationScope allocations({true, true});
        smgpc::compat::destroy_file_loader(SingletonHolder<FileLoader>::get());
        delete SingletonHolder<HeapMemoryWatcher>::release();
        heaps->root_heap().becomeCurrentHeap();
        heaps->root_heap().becomeSystemHeap();
        destroy_children(heaps->root_heap());
        HeapMemoryWatcher::sRootHeapGDDR3 = nullptr;
        JKRHeap::mErrorHandler = old_error;
    }

    JKRHeap& heap() const { return *SingletonHolder<HeapMemoryWatcher>::get()->mStationedHeapNapa; }
};

void cycle(OriginalFileProcess& process, smgpc::runtime::DvdFileSystemService& dvd) {
    const aurora::allocation::HostAllocationScope host;
    constexpr auto file_name = "/ObjectData/InvisibleWall10x10.arc";
    constexpr auto archive_name = "/StageData/HeavensDoorGalaxy/HeavensDoorGalaxyScenario.arc";
    const auto encoded_file = dvd.read_file(file_name);
    const auto file = smgpc::resource::decompress_yaz0(encoded_file);
    const auto write_extent = smgpc::resource::is_yaz0(encoded_file) ? file.size() : (file.size() + 31) & ~std::size_t(31);
    const auto archive_bytes = smgpc::resource::decompress_yaz0(dvd.read_file(archive_name));
    const auto reference = smgpc::resource::RarcArchive::from_borrowed(archive_bytes);
    std::vector<u8> destination(write_extent + 0x80, 0xA5);
    auto* buffer = static_cast<u8*>(ROUND_UP_PTR(destination.data() + 0x20, 0x40));
    const auto prefix = static_cast<std::size_t>(buffer - destination.data());
    const auto free_before = process.heap().getTotalFreeSize();
    const auto volumes_before = JKRFileLoader::sVolumeList.getNumLinks();
    {
        const aurora::allocation::ClientAllocationScope allocations({true, true});
        SingletonHolder<FileLoader>::init();
        auto* loader = SingletonHolder<FileLoader>::get();
        require(loader && loader->mLoaderThread, "original constructor must create its actual file worker");
        require(OSSetThreadPriority(loader->mLoaderThread->mThread, 20), "lower actual worker priority while queuing requests");
        loader->requestLoadToMainRAM(file_name, buffer, &process.heap(), JKRDvdRipper::ALLOC_DIRECTION_FORWARD, false);
        loader->requestMountArchive(archive_name, &process.heap(), true);
        loader->requestLoadToMainRAM(file_name, buffer, &process.heap(), JKRDvdRipper::ALLOC_DIRECTION_FORWARD, false);
        const auto& queue = loader->mLoaderThread->mQueue;
        require(loader->mRequestedFileCount == 2 && queue.usedCount == 2 &&
                    queue.msgArray[queue.firstIndex] == &loader->mRequestFileInfos[1] &&
                    queue.msgArray[(queue.firstIndex + 1) % queue.msgCount] == &loader->mRequestFileInfos[0],
                "original jam/send order and duplicate request suppression must preserve actual request identities");
        require(OSSetThreadPriority(loader->mLoaderThread->mThread, 13), "restore original file worker priority");
        auto* archive = loader->receiveArchive(archive_name);
        require(loader->receiveFile(file_name) == buffer && std::memcmp(buffer, file.data(), file.size()) == 0,
                "original queued read must decode every retail byte into the caller's destination");
        require(archive && archive == loader->mArchiveHolder->getArchive(archive_name) &&
                    archive->mHeader == loader->mFileHolder->getContext(archive_name) &&
                    process.heap().find(archive->mHeader),
                "original asynchronous mount must publish the exact file allocation and archive owner");
        require(archive->countResource() == reference.entries().size(), "original mount retains the complete retail resource catalog");
        for (const auto& entry : reference.entries()) {
            const auto bytes = reference.file_data(entry);
            const auto* resource = archive->getResource(entry.path.c_str());
            require((bytes.empty() && resource == nullptr) ||
                        (resource && archive->getResSize(resource) == bytes.size() &&
                         std::memcmp(resource, bytes.data(), bytes.size()) == 0),
                    "every original mounted resource must match the independently parsed retail archive");
        }
        loader->receiveAllRequestedFile();
        require(loader->mRequestFileInfos[0]._88 == 2 && loader->mRequestFileInfos[1]._88 == 2 &&
                    loader->mFileHolder->findEntry(file_name)->mState == 2 &&
                    loader->mFileHolder->findEntry(archive_name)->mState == 2,
                "original worker and receive methods must reach their actual completion states");
        loader->clearRequestFileInfo(false);
        smgpc::compat::destroy_file_loader(loader);
    }
    const auto free_after = process.heap().getTotalFreeSize();
    const auto volumes_after = JKRFileLoader::sVolumeList.getNumLinks();
    if (SingletonHolder<FileLoader>::get() != nullptr || volumes_after != volumes_before || free_after != free_before) {
        std::cerr << "File retirement: singleton=" << SingletonHolder<FileLoader>::get()
                  << " volumes=" << volumes_before << "->" << volumes_after
                  << " free=" << free_before << "->" << free_after << '\n';
        process.heap().dump();
    }
    require(SingletonHolder<FileLoader>::get() == nullptr && volumes_after == volumes_before && free_after == free_before,
            "typed retirement must join workers, clear the real singleton and reclaim files, mounts and original heap storage");
    require(std::all_of(destination.begin(), destination.begin() + prefix, [](u8 b) { return b == 0xA5; }) &&
                std::memcmp(buffer, file.data(), file.size()) == 0 &&
                std::all_of(destination.begin() + prefix + write_extent, destination.end(), [](u8 b) { return b == 0xA5; }),
            "reads and retirement must preserve caller-owned destination bytes and guards");
}
}

int main() {
    const auto* disc = std::getenv("SMGPC_REAL_DISC");
    if (!disc || !*disc) return 77;
    try {
        const aurora::os::GuestThreadExecutionScope execution;
        OSInit();
        DVDInit();
        require(aurora_dvd_open(disc), "real DVD fixture must open");
        smgpc::runtime::DvdFileSystemService dvd("/");
        {
            OriginalFileProcess process;
            for (unsigned i = 0; i < 3; ++i) cycle(process, dvd);
        }
        require(SingletonHolder<HeapMemoryWatcher>::get() == nullptr && JKRHeap::sRootHeap == nullptr,
                "complete original heap graph must retire after the file worker cohort");
        std::cout << "Original file queues, streamed retail bytes, archive identities and repeated retirement passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
