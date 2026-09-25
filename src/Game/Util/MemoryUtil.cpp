#include "Game/Util/MemoryUtil.hpp"
#include "Game/System/HeapMemoryWatcher.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include <JSystem/JKernel/JKRExpHeap.hpp>
#include <JSystem/JKernel/JKRSolidHeap.hpp>
#include <cstring>
#include <stdexcept>
#include <aurora/exception.hpp>

namespace MR {
    CurrentHeapRestorer::CurrentHeapRestorer(JKRHeap* pHeap) : mCurrentHeap(*pHeap) {
    }

    CurrentHeapRestorer::~CurrentHeapRestorer() = default;

    void* NewDeleteAllocator::alloc(MEMAllocator* pAllocator, u32 size) {
        return new u8[size];
    }

    void NewDeleteAllocator::free(MEMAllocator* pAllocator, void* pPtr) {
        delete[] static_cast< u8* >(pPtr);
    }

    MEMAllocatorFunc NewDeleteAllocator::sAllocatorFunc = {
        NewDeleteAllocator::alloc,
        NewDeleteAllocator::free,
    };
    MEMAllocator NewDeleteAllocator::sAllocator = {&sAllocatorFunc};

    MEMAllocator* getHomeButtonLayoutAllocator() {
        // The native Home Button Menu has no recovered implementation yet.
        aurora::throw_host_exception<std::logic_error>("Native Home Button Menu allocator is unavailable");
    }

    JKRHeap* getCurrentHeap() {
        return JKRHeap::sCurrentHeap;
    }

    f32 getHeapFreeRatio(JKRHeap* pHeap) {
        u32 size = pHeap->mSize;
        return static_cast< f32 >(pHeap->getTotalFreeSize()) / size;
    }

    JKRHeap* getAproposHeapForSceneArchive(f32 maxFreeSizeRate) {
        JKRHeap* pFileCacheHeap = SingletonHolder< HeapMemoryWatcher >::get()->mFileCacheHeap;

        if (pFileCacheHeap != nullptr) {
            if (getHeapFreeRatio(pFileCacheHeap) < maxFreeSizeRate) {
                pFileCacheHeap = SingletonHolder< HeapMemoryWatcher >::get()->mSceneHeapGDDR;
            }
        }

        return pFileCacheHeap;
    }

    JKRExpHeap* getStationedHeapNapa() {
        return SingletonHolder< HeapMemoryWatcher >::get()->mStationedHeapNapa;
    }

    JKRExpHeap* getStationedHeapGDDR3() {
        return SingletonHolder< HeapMemoryWatcher >::get()->mStationedHeapGDDR;
    }

    JKRSolidHeap* getSceneHeapNapa() {
        return SingletonHolder< HeapMemoryWatcher >::get()->mSceneHeapNapa;
    }

    JKRSolidHeap* getSceneHeapGDDR3() {
        return SingletonHolder< HeapMemoryWatcher >::get()->mSceneHeapGDDR;
    }

    JKRHeap* getHeapNapa(const JKRHeap* pHeap) {
        return SingletonHolder< HeapMemoryWatcher >::get()->getHeapNapa(pHeap);
    }

    JKRHeap* getHeapGDDR3(const JKRHeap* pHeap) {
        return SingletonHolder< HeapMemoryWatcher >::get()->getHeapGDDR3(pHeap);
    }

    void becomeCurrentHeap(JKRHeap* pHeap) {
        OSLockMutex(&JKRHeap::sCurrentHeapMutex);
        pHeap->becomeCurrentHeap();
        OSUnlockMutex(&JKRHeap::sCurrentHeapMutex);
    }

    bool isEqualCurrentHeap(JKRHeap* pHeap) {
        return JKRHeap::sCurrentHeap == pHeap;
    }

    void adjustHeapSize(JKRExpHeap* pHeap, const char* pParam2) {
        pHeap->adjustSize();
    }

    void copyMemory(void* destination, const void* source, u32 size) {
        if (size == 0U) {
            return;
        }
        if (destination == nullptr || source == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Cannot copy unavailable memory.");
        }
        std::memcpy(destination, source, size);
    }

    void fillMemory(void* destination, u8 value, u32 size) {
        if (size == 0U) {
            return;
        }
        if (destination == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Cannot fill unavailable memory.");
        }
        std::memset(destination, value, size);
    }

    void zeroMemory(void* destination, u32 size) {
        fillMemory(destination, 0U, size);
    }

    u32 calcCheckSum(const void* pointer, u32 size) {
        const auto* bytes = static_cast<const u8*>(pointer);
        u16 sum = 0;
        u16 inverse_sum = 0;
        for (u32 offset = 0; offset < size / sizeof(u16); ++offset) {
            u16 value;
            std::memcpy(&value, bytes + offset * sizeof(u16), sizeof(value));
            sum += value;
            inverse_sum += static_cast<u16>(~value);
        }
        return (static_cast<u32>(sum) << 16) | inverse_sum;
    }

    void* allocFromWPadHeap(u32 size) {
        return SingletonHolder< HeapMemoryWatcher >::get()->mWPadHeap->alloc(size, 0);
    }

    u8 freeFromWPadHeap(void* pPtr) {
        SingletonHolder< HeapMemoryWatcher >::get()->mWPadHeap->free(pPtr);

        return 1;
    }
};  // namespace MR
