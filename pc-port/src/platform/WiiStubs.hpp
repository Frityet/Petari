#pragma once

#include <cstdint>

namespace pcport {

struct WiiStubStatus {
    int miiCount = 0;
    bool homeButtonEnabled = false;
    bool nwc24Available = false;
    std::uint64_t fixedTicks = 0x0123456789ABCDEFULL;
};

class WiiStubs {
public:
    static const WiiStubStatus& GetStatus();

    static int GetMiiCount();
    static bool IsHomeButtonEnabled();
    static bool IsNwc24Available();
    static std::uint64_t GetSystemTicks();
};

}  // namespace pcport
