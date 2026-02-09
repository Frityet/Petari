#include "platform/WiiStubs.hpp"

#include "core/Logger.hpp"

namespace pcport {
namespace {

const WiiStubStatus kStatus = {
    .miiCount = 0,
    .homeButtonEnabled = false,
    .nwc24Available = false,
    .fixedTicks = 0x0123456789ABCDEFULL,
};

}  // namespace

const WiiStubStatus& WiiStubs::GetStatus() {
    return kStatus;
}

int WiiStubs::GetMiiCount() {
    Log(LogLevel::Debug, LogCategory::Stub, "WiiStubs::GetMiiCount -> 0");
    return kStatus.miiCount;
}

bool WiiStubs::IsHomeButtonEnabled() {
    Log(LogLevel::Debug, LogCategory::Stub, "WiiStubs::IsHomeButtonEnabled -> false");
    return kStatus.homeButtonEnabled;
}

bool WiiStubs::IsNwc24Available() {
    Log(LogLevel::Debug, LogCategory::Stub, "WiiStubs::IsNwc24Available -> false");
    return kStatus.nwc24Available;
}

std::uint64_t WiiStubs::GetSystemTicks() {
    return kStatus.fixedTicks;
}

}  // namespace pcport
