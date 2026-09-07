#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"

// Literal original MemoryUtil.cpp heap selection/restoration methods.
namespace MR {
    CurrentHeapRestorer::CurrentHeapRestorer(JKRHeap* pHeap) {
        _0 = JKRHeap::sCurrentHeap;
        OSLockMutex(&MR::MutexHolder<1>::sMutex);
        MR::becomeCurrentHeap(pHeap);
    }

    CurrentHeapRestorer::~CurrentHeapRestorer() {
        MR::becomeCurrentHeap(_0);
        OSUnlockMutex(&MR::MutexHolder<1>::sMutex);
    }

    JKRHeap* getCurrentHeap() {
        return JKRHeap::sCurrentHeap;
    }

    void becomeCurrentHeap(JKRHeap* pHeap) {
        OSLockMutex(&MR::MutexHolder<1>::sMutex);
        pHeap->becomeCurrentHeap();
        OSUnlockMutex(&MR::MutexHolder<1>::sMutex);
    }

    bool isEqualCurrentHeap(JKRHeap* pHeap) {
        return JKRHeap::sCurrentHeap == pHeap;
    }
}
