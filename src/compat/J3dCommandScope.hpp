#pragma once

#include <dolphin/gd.h>
#include <dolphin/os.h>
#include <aurora/guest_thread.hpp>

namespace smgpc::compat {
    // Own the original current-heap and J3D mutexes in that order while
    // retaining the caller's GD/interrupt state. Device waits can yield the
    // guest CPU without exposing those shared contexts to another resource
    // owner. Original one-time interrupt snapshots remain unchanged.
    class J3dCommandScope final {
    public:
        J3dCommandScope();
        ~J3dCommandScope();
        J3dCommandScope(const J3dCommandScope&) = delete;
        J3dCommandScope& operator=(const J3dCommandScope&) = delete;

    private:
        class MutexScope final {
        public:
            explicit MutexScope(OSMutex& mutex);
            ~MutexScope();
            MutexScope(const MutexScope&) = delete;
            MutexScope& operator=(const MutexScope&) = delete;
        private:
            OSMutex& _mutex;
            OSThread* _thread;
            s32 _count;
            int _exceptions;
        };

        aurora::os::GuestThreadExecutionScope _execution;
        BOOL _interrupts;
        MutexScope _heap;
        MutexScope _commands;
        GDLObj* _previous;
    };
}
