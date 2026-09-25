#include "JSystem/JKernel/JKRFileLoader.hpp"
#include <aurora/allocation.hpp>
#include <mutex>

namespace {
    std::once_flag volumeListInitialized;
}

JSUList<JKRFileLoader> JKRFileLoader::sFileLoaderList;
JSUList<JKRFileLoader> &JKRFileLoader::sVolumeList = JKRFileLoader::sFileLoaderList;
JKRFileLoader *JKRFileLoader::gCurrentFileLoader = nullptr;
OSMutex JKRFileLoader::sVolumeListMutex;

JKRFileLoader::VolumeLock::VolumeLock() {
    initializeVolumeList();
    OSLockMutex(&sVolumeListMutex);
}

JKRFileLoader::VolumeLock::~VolumeLock() {
    OSUnlockMutex(&sVolumeListMutex);
}

JKRFileLoader::JKRFileLoader()
    : JKRDisposer(), mLoaderLink(this), mLoaderName(nullptr), mLoaderType(0), mIsMounted(false), _34(0) {
}

JKRFileLoader::~JKRFileLoader() {
    VolumeLock lock;
    if (mLoaderLink.getList() == &sFileLoaderList)
        sFileLoaderList.remove(&mLoaderLink);
    if (gCurrentFileLoader == this)
        gCurrentFileLoader = nullptr;
}

void JKRFileLoader::unmount() {
    bool destroy = false;
    {
        VolumeLock lock;
        if (_34 != 0)
            destroy = --_34 == 0;
    }
    if (destroy)
        delete this;
}

void *JKRFileLoader::getGlbResource(const char *name, JKRFileLoader *loader) {
    if (loader != nullptr)
        return loader->getResource(0, name);
    VolumeLock lock;
    for (auto *link = sFileLoaderList.getFirst(); link != nullptr; link = link->getNext()) {
        if (void *resource = link->getObject()->getResource(0, name))
            return resource;
    }
    return nullptr;
}

void JKRFileLoader::initializeVolumeList() {
    aurora::allocation::HostAllocationScope host;
    // The original initializes the list's mutex, not the list itself. Native
    // owners may already have mounted archives before Game process startup.
    std::call_once(volumeListInitialized, [] { OSInitMutex(&sVolumeListMutex); });
}

void JKRFileLoader::prependVolumeList(JSULink<JKRFileLoader> *loader) {
    VolumeLock lock;
    sFileLoaderList.prepend(loader);
}

void JKRFileLoader::removeVolumeList(JSULink<JKRFileLoader> *loader) {
    VolumeLock lock;
    sFileLoaderList.remove(loader);
}
