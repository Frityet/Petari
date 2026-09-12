#include "compat/FileLoaderOwnership.hpp"
#include "Game/System/FileLoader.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include <aurora/exception.hpp>
#include <aurora/guest_thread.hpp>
#include <stdexcept>

namespace smgpc::compat {

void destroy_file_loader(FileLoader* loader) {
    if (loader == nullptr) return;
    const aurora::os::GuestThreadExecutionScope execution;
    if (loader->mLoaderThread && loader->mLoaderThread->mThread == OSGetCurrentThread())
        aurora::throw_host_exception<std::logic_error>("A file loader cannot retire its own executing worker");

    // Request indices can already have been cleared by the original process.
    // The actual file entries retain their completion queues independently.
    if (loader->mFileHolder) {
        for (auto* entry : loader->mFileHolder->mEntries) entry->waitReadDone();
    }
    delete loader->mLoaderThread;
    loader->mLoaderThread = nullptr;
    if (SingletonHolder<FileLoader>::get() == loader) SingletonHolder<FileLoader>::release();

    // Preserve original FileLoader removal order. Archive native backing must
    // therefore release metadata without dereferencing these retired bytes.
    if (loader->mFileHolder) {
        for (auto* entry : loader->mFileHolder->mEntries) delete entry;
        loader->mFileHolder->mEntries.clear();
        delete loader->mFileHolder;
        loader->mFileHolder = nullptr;
    }
    if (loader->mArchiveHolder) {
        for (auto* entry : loader->mArchiveHolder->mEntries) delete entry;
        loader->mArchiveHolder->mEntries.clear();
        delete loader->mArchiveHolder;
        loader->mArchiveHolder = nullptr;
    }
    delete[] loader->mRequestFileInfos;
    loader->mRequestFileInfos = nullptr;
    loader->mRequestedFileCount = 0;
    delete loader;
}

}
