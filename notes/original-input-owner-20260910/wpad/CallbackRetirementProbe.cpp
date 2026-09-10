#include <aurora/wpad.hpp>
#include <memory>
#include <cstdio>
extern "C" void PADControlMotor(u32, u32) {}
extern "C" BOOL PADSupportsRumble(u32) { return FALSE; }
std::unique_ptr<aurora::WpadClientScope> client;
std::unique_ptr<WPADInfo> request;
void retire_client(s32 channel, s32 result) {
    if (channel == 0 && result == WPAD_ERR_NONE) {
        client.reset();
        request.reset();
    }
}
int main() {
    auto& service = aurora::wpad_service();
    service.clear();
    client = std::make_unique<aurora::WpadClientScope>();
    request = std::make_unique<WPADInfo>();
    service.set_connected(0, true);
    WPADSetConnectCallback(0, retire_client);
    if (WPADGetInfoAsync(0, request.get(), nullptr) != WPAD_ERR_NONE) return 2;
    service.dispatch_callbacks();
    std::puts("Callback retirement safely cancelled the old client information write");
}
