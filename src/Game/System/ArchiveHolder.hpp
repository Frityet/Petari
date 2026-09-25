#pragma once

#include "Game/Util/Array.hpp"
#include <JSystem/JKernel/JKRMemArchive.hpp>
#include <revolution/types.h>

class ArchiveHolderArchiveEntry {
public:
    /// @brief Creates a new `ArchiveHolderArchiveEntry`.
    ArchiveHolderArchiveEntry(void*, JKRHeap*, const char*);

    /// @brief Destroys the `ArchiveHolderArchiveEntry`.
    ~ArchiveHolderArchiveEntry();

    std::shared_ptr< const void > retainNativeResources() const;
    void validateNativeRetirement(std::size_t = 0) const;

    /* 0x0 */ JKRMemArchive* mArchive;
    /* 0x4 */ JKRHeap* mHeap;
    /* 0x8 */ char* mArchiveName;

private:
    struct NativeState;
    std::unique_ptr< NativeState > mNativeState;
};

class ArchiveHolder {
public:
    /// @brief Creates a new `ArchiveHolder`.
    ArchiveHolder();

    /// @brief Creates and adds a new archive entry.
    /// @param pData The data for the archive.
    /// @param pHeap The heap to allocate the archive in.
    /// @param pArchiveName The name of the archive.
    /// @return The pointer to the created entry.
    ArchiveHolderArchiveEntry* createAndAdd(void*, JKRHeap*, const char*);

    /// @brief Gets the archive for the given archive name.
    /// @param pArchiveName The name of the archive.
    /// @return The pointer to the found archive, or `nullptr` if not found.
    JKRMemArchive* getArchive(const char*) const;

    /// @brief Gets the archive and heap for the given archive name.
    /// @param pArchiveName The name of the archive.
    /// @param pArchive The output pointer for the archive.
    /// @param pHeap The output pointer for the heap.
    void getArchiveAndHeap(const char*, JKRArchive**, JKRHeap**) const;

    /// @brief Removes entries that match the given heap.
    /// @param pHeap The heap to compare.
    void removeIfIsEqualHeap(JKRHeap*);
    // A null heap validates every entry, as required before process teardown.
    void validateNativeRetirement(JKRHeap* = nullptr) const;

    /// @brief Finds the entry with the given archive name.
    /// @param pArchiveName The name of the archive.
    /// @return The pointer to the found entry, or `nullptr` if not found.
    ArchiveHolderArchiveEntry* findEntry(const char*) const;
    ArchiveHolderArchiveEntry* findEntry(const JKRArchive*) const;

    /* 0x0 */ MR::Vector< MR::AssignableArray< ArchiveHolderArchiveEntry* > > mEntries;
    /* 0xC */ OSMutex mMutex;
};
