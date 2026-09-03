#pragma once

#include "JSystem/JKernel/JKRDisposer.hpp"

namespace smgpc::compat {

    // Native owners embedded in an original heap need their C++ members
    // destroyed by that heap's actual disposer list. Intrusive registration is
    // object identity: copying or moving a value must never copy its link.
    class NativeJkrDisposer : public JKRDisposer {
    protected:
        NativeJkrDisposer() = default;
        NativeJkrDisposer(const NativeJkrDisposer&) : JKRDisposer() {}
        NativeJkrDisposer(NativeJkrDisposer&&) : JKRDisposer() {}
        NativeJkrDisposer& operator=(const NativeJkrDisposer&) noexcept { return *this; }
        NativeJkrDisposer& operator=(NativeJkrDisposer&&) noexcept { return *this; }
        ~NativeJkrDisposer() override = default;
    };

} // namespace smgpc::compat
