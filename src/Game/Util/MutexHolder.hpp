#pragma once

#include "revolution.h"
#include "JSystem/JKernel/JKRHeap.hpp"

class JKRHeap;

namespace MR {
    template < int T >
    class MutexHolder {
    public:
        static OSMutex sMutex;
    };

    template < int T >
    OSMutex MutexHolder< T >::sMutex;
    // Original Game/J3D callers keep the same SDK-owned current-heap lock.
    template <>
    class MutexHolder< 1 > {
    public:
        inline static OSMutex& sMutex = JKRHeap::sCurrentHeapMutex;
    };
};  // namespace MR
