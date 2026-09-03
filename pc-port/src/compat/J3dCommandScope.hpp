#pragma once

#include <dolphin/gd.h>
#include <dolphin/os.h>

namespace smgpc::compat {
    // Retain cooperative SDK execution and the caller's GD/interrupt state
    // around original resource construction and post-load command generation.
    // Original routines with one-time interrupt snapshots remain unchanged.
    // When using a Game allocation domain, construct this after its
    // JkrAllocationScope so all callers acquire the shared locks in that order.
    class J3dCommandScope final {
    public:
        J3dCommandScope();
        ~J3dCommandScope();
        J3dCommandScope(const J3dCommandScope&) = delete;
        J3dCommandScope& operator=(const J3dCommandScope&) = delete;

    private:
        BOOL _interrupts;
        GDLObj* _previous;
    };
}
