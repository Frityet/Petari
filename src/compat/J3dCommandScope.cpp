#include "J3dCommandScope.hpp"
#include "Game/Util/MutexHolder.hpp"

#include <exception>

namespace smgpc::compat {
    namespace {
        BOOL interrupt_state() {
            const auto enabled = OSDisableInterrupts();
            OSRestoreInterrupts(enabled);
            return enabled;
        }
    }

    J3dCommandScope::MutexScope::MutexScope(OSMutex& mutex)
        : _mutex(mutex), _thread(OSGetCurrentThread()), _exceptions(std::uncaught_exceptions()) {
        OSLockMutex(&_mutex);
        _count = _mutex.count;
    }

    J3dCommandScope::MutexScope::~MutexScope() {
        // Some original routines manually balance recursive acquisitions.
        // Recover only acquisitions made after this scope when a host
        // exception crosses their unlock; pre-existing ownership is retained.
        if (std::uncaught_exceptions() > _exceptions)
            while (_mutex.thread == _thread && _mutex.count > _count) OSUnlockMutex(&_mutex);
        OSUnlockMutex(&_mutex);
    }

    J3dCommandScope::J3dCommandScope()
        : _interrupts(interrupt_state()), _heap(MR::MutexHolder<1>::sMutex),
          _commands(MR::MutexHolder<0>::sMutex), _previous(__GDCurrentDL) {
    }

    J3dCommandScope::~J3dCommandScope() {
        GDSetCurrent(_previous);
        OSRestoreInterrupts(_interrupts);
    }
}
