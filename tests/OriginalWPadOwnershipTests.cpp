#include "Game/System/WPad.hpp"
#include "Game/System/WPadAcceleration.hpp"
#include "Game/System/WPadHolder.hpp"
#include "Game/System/WPadRumble.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <aurora/exception.hpp>
#include <aurora/wpad.hpp>
#include <revolution/hbm.h>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) aurora::throw_host_exception<std::runtime_error>(message);
}
std::unique_ptr<aurora::WpadClientScope> retiring_client;
int completion_count;
void retire_on_connect(s32 channel, s32 result) {
    if (channel == 0 && result == WPAD_ERR_NONE) retiring_client.reset();
}
void count_completion(s32, s32) { ++completion_count; }
void callback_retirement() {
    auto& sdk = aurora::wpad_service();
    sdk.clear();
    completion_count = 0;
    WPADInfo abandoned;
    std::memset(&abandoned, 0x5a, sizeof(abandoned));
    std::array<unsigned char, sizeof(WPADInfo)> before{};
    std::memcpy(before.data(), &abandoned, sizeof(abandoned));
    retiring_client = std::make_unique<aurora::WpadClientScope>();
    sdk.set_connected(0, true);
    WPADSetConnectCallback(0, retire_on_connect);
    require(WPADGetInfoAsync(0, &abandoned, count_completion) == WPAD_ERR_NONE,
            "the SDK accepts an information request owned by its current client");
    sdk.dispatch_callbacks();
    sdk.dispatch_callbacks();
    require(!retiring_client && completion_count == 0 && std::memcmp(&abandoned, before.data(), sizeof(abandoned)) == 0,
            "retiring the SDK client inside a callback cancels its pending buffer write and completion");
}
} // namespace

int main() {
    try {
        callback_retirement();
        std::cout << "WPAD client retirement cancels pending buffer writes and completions\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
