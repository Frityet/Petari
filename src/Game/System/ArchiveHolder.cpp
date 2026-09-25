#include "Game/System/ArchiveHolder.hpp"
#include "Game/Util.hpp"
#include "resource/JMapResource.hpp"
#include "resource/JpcResource.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <cstring>
#include <stdexcept>

struct ArchiveHolderArchiveEntry::NativeState {
    JKRHeap::Handle mDomain;
    std::vector< smgpc::resource::JMapSourceRegistration > mTables;
    std::vector< smgpc::resource::JpcSourceRegistration > mParticles;
};

ArchiveHolderArchiveEntry::ArchiveHolderArchiveEntry(void* pData, JKRHeap* pHeap, const char* pArchiveName)
    : mHeap(pHeap), mArchive(nullptr), mArchiveName(nullptr) {
    std::unique_ptr< JKRMemArchive > archive(new (pHeap, 0) JKRMemArchive());
    if (!archive->mountFixed(pData, JKR_MEM_BREAK_FLAG_0))
        aurora::throw_host_exception< std::logic_error >("ArchiveHolder requires a new valid fixed archive mount");

    {
        const aurora::allocation::HostAllocationScope host;
        mNativeState = std::make_unique< NativeState >();
        mNativeState->mDomain = pHeap->retainNativeLifetime();
        const auto source = archive->retainSource();
        const std::weak_ptr< const void > lifetime = archive->retainNativeResources();
        for (const auto& entry : source->entries()) {
            const auto bytes = source->file_data(entry);
            if (!bytes.empty())
                mNativeState->mTables.push_back(smgpc::resource::register_jmap_source(bytes, source, [lifetime] {
                    auto retained = lifetime.lock();
                    if (!retained)
                        aurora::throw_host_exception< std::logic_error >("JMap attachment requires a live original archive");
                    return retained;
                }));
            if (bytes.size() >= 4 && std::memcmp(bytes.data(), "JPAC", 4) == 0)
                mNativeState->mParticles.push_back(smgpc::resource::register_jpc_source(bytes, source));
        }
    }
    const s32 len = strlen(pArchiveName) + 1;
    mArchiveName = new (pHeap, 0) char[len];
    MR::copyString(mArchiveName, pArchiveName, len);
    mArchive = archive.release();
}

ArchiveHolderArchiveEntry::~ArchiveHolderArchiveEntry() {
    validateNativeRetirement();
    {
        // FileLoader follows the original file-before-archive removal order.
        // Registration retirement only removes identities; it never reads the
        // borrowed fixed buffer, which may already have been freed.
        const aurora::allocation::HostAllocationScope host;
        mNativeState.reset();
    }
    mArchive->unmount();
    delete[] mArchiveName;
}

std::shared_ptr< const void > ArchiveHolderArchiveEntry::retainNativeResources() const {
    return mArchive->retainNativeResources();
}

void ArchiveHolderArchiveEntry::validateNativeRetirement(std::size_t releasingBorrows) const {
    mArchive->validateNativeRetirement(releasingBorrows);
}

ArchiveHolder::ArchiveHolder() {
    mEntries.init(0x180);
    OSInitMutex(&mMutex);
}

ArchiveHolderArchiveEntry* ArchiveHolder::createAndAdd(void* pData, JKRHeap* pHeap, const char* pArchiveName) {
    ArchiveHolderArchiveEntry* entry = new (pHeap, 0) ArchiveHolderArchiveEntry(pData, pHeap, pArchiveName);
    OSMutex* mutex = &mMutex;
    OSLockMutex(mutex);

    mEntries.push_back(entry);

    OSUnlockMutex(mutex);
    return entry;
}

JKRMemArchive* ArchiveHolder::getArchive(const char* pArchiveName) const {
    ArchiveHolderArchiveEntry* entry = findEntry(pArchiveName);
    return (entry != nullptr) ? entry->mArchive : nullptr;
}

void ArchiveHolder::getArchiveAndHeap(const char* pArchiveName, JKRArchive** pArchive, JKRHeap** pHeap) const {
    ArchiveHolderArchiveEntry* entry = findEntry(pArchiveName);

    if (entry != nullptr) {
        *pArchive = entry->mArchive;
        *pHeap = entry->mHeap;
    }
}

void ArchiveHolder::removeIfIsEqualHeap(JKRHeap* pHeap) {
    if (pHeap == nullptr) {
        return;
    }

    validateNativeRetirement(pHeap);

    for (ArchiveHolderArchiveEntry** i = mEntries.begin(); i != mEntries.end();) {
        if ((*i)->mHeap == pHeap || MR::getHeapNapa((*i)->mHeap) == pHeap || MR::getHeapGDDR3((*i)->mHeap) == pHeap) {
            const auto heap = (*i)->mHeap->retainNativeLifetime();
            delete *i;
            mEntries.erase(i);
        } else {
            i++;
        }
    }
}

void ArchiveHolder::validateNativeRetirement(JKRHeap* pHeap) const {
    for (const auto* entry : mEntries) {
        if (pHeap == nullptr || entry->mHeap == pHeap || MR::getHeapNapa(entry->mHeap) == pHeap ||
            MR::getHeapGDDR3(entry->mHeap) == pHeap)
            entry->validateNativeRetirement();
    }
}

ArchiveHolderArchiveEntry* ArchiveHolder::findEntry(const char* pArchiveName) const {
    for (ArchiveHolderArchiveEntry* const* i = mEntries.begin(); i != mEntries.end(); i++) {
        if (MR::isEqualStringCase((*i)->mArchiveName, pArchiveName)) {
            return *i;
        }
    }

    return nullptr;
}

ArchiveHolderArchiveEntry* ArchiveHolder::findEntry(const JKRArchive* pArchive) const {
    for (auto* entry : mEntries) {
        if (entry->mArchive == pArchive)
            return entry;
    }
    return nullptr;
}
