#include "compat/JkrAllocationDomain.hpp"
#include "compat/WPadOwnership.hpp"
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
void next_frame(smgpc::compat::WPadOwnership& owner) {
    aurora::wpad_service().begin_frame();
    owner.update_samples();
}
void original_holder_flow(const std::shared_ptr<smgpc::compat::JkrHeapRuntime>& heaps) {
    auto& sdk = aurora::wpad_service();
    sdk.clear();
    sdk.set_sensor_bar_position(WPAD_SENSOR_BAR_POS_BOTTOM);
    auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 512U << 10);
    smgpc::compat::WPadOwnership owner(domain);
    auto& holder = owner.holder();
    require(MR::getWPad(0) == &owner.pad(0) && MR::getWPad(1) == &owner.pad(1),
            "native publication exposes the two actual original WPadHolder children");
    require(holder.getWPad(-1) == nullptr && holder.getWPad(2) == nullptr,
            "the original holder rejects channels outside its two Game slots");
    for (int channel = 0; channel < WPAD_MAX_CONTROLLERS; ++channel) {
        auto& read = holder.mReadDataInfoArray[channel];
        require(read.mStatusArray && read.mValidStatusCount == 0 && !read.getKPadStatus(0),
                "the actual holder constructs all four empty SDK read records");
        require(JKRHeap::findFromRoot(read.mStatusArray) == &domain->heap() &&
                JKRHeap::findFromRoot(&read.mStatusArray[119]) == &domain->heap(),
                "all120 samples in each of four read records belong to the retained original heap");
    }
    require(std::abs(sdk.channel_state(0)->sensor_height + .15F) < .00001F &&
            std::abs(sdk.channel_state(1)->sensor_height + .15F) < .00001F,
            "original sensor-bar setup configures both real pointer children");
    auto* work = MR::allocFromWPadHeap(64);
    require(JKRHeap::findFromRoot(work) == &domain->heap(), "the SDK allocation callback reaches its actual retained WPad heap");
    require(MR::freeFromWPadHeap(work) == 1, "the original WPad heap release callback accepts its allocation");

    // Empty initial reads establish the original zero acceleration histories.
    next_frame(owner);
    require(!owner.pad(0).mIsConnected && owner.pad(0).getBattery() == -1,
            "an absent device leaves original connection and battery state unavailable");
    sdk.set_device_type(0, aurora::WpadDeviceType::Freestyle);
    sdk.set_connected(0, true);
    require(!owner.pad(0).mIsConnected, "platform connection is deferred until the published owner's callback pump");
    next_frame(owner);
    require(owner.pad(0).mIsConnected && owner.pad(0).mIsSubPadConnected &&
            owner.pad(0).getBattery() == -1 && holder.mReadDataInfoArray[0].mValidStatusCount == 1,
            "original connection and extension callbacks precede projection; newly requested information stays pending");
    next_frame(owner);
    require(owner.pad(0).getBattery() == 4,
            "the next SDK completion writes the actual WPadInfoChecker buffer and original battery callback consumes it");
    sdk.set_device_type(0, aurora::WpadDeviceType::Core);
    next_frame(owner);
    require(owner.pad(0).mIsConnected && !owner.pad(0).mIsSubPadConnected,
            "the original extension callback removes the Nunchuk independently of the remote connection");

    MR::setWPadHolderModeHomeButton();
    sdk.set_connected(2, true);
    sdk.set_connected(3, true);
    next_frame(owner);
    require(sdk.is_connected(2) && sdk.is_connected(3) &&
            holder.mReadDataInfoArray[2].mValidStatusCount == 1 && holder.mReadDataInfoArray[3].mValidStatusCount == 1,
            "the original Home Button mode keeps channels2 and3 in their actual SDK read records");
    HBMKPadData home{};
    MR::getHBMKPadData(&home, 2);
    require(home.kpad == holder.mReadDataInfoArray[2].mStatusArray && home.use_devtype == WPAD_DEV_CORE,
            "original Home Button data borrows the real third-channel record");
    MR::setWPadHolderModeGame();
    next_frame(owner);
    require(!sdk.is_connected(2) && !sdk.is_connected(3), "original in-game mode disconnects unused SDK channels2 and3");
    next_frame(owner);
    require(holder.mReadDataInfoArray[2].mValidStatusCount == 0 && holder.mReadDataInfoArray[3].mValidStatusCount == 0,
            "disconnected extra controllers produce no fabricated SDK records");
    MR::getHBMKPadData(&home, 2);
    require(home.kpad == nullptr, "original Home Button lookup reports an absent SDK controller");

    const auto history = owner.pad(0).mCorePadAccel->_628;
    sdk.set_pointer(0, 200, 100, true);
    require(sdk.pointer_history_count(0) != 0, "the native sampling service contains a pointer record before reset");
    MR::resetWPad();
    require(sdk.pointer_history_count(0) == 0 && owner.pad(0).mCorePadAccel->_628 == history,
            "original reset clears KPAD filtering while its intentionally empty WPad reset preserves Game acceleration history");
    MR::setAutoSleepTimeWiiRemote(true);
    require(sdk.auto_sleep_time() == 15, "the original long auto-sleep request reaches the SDK setting");
    MR::setAutoSleepTimeWiiRemote(false);
    require(sdk.auto_sleep_time() == 5, "the original normal auto-sleep request reaches the SDK setting");
    sdk.set_connected(0, false);
    next_frame(owner);
    require(!owner.pad(0).mIsConnected && !owner.pad(0).mIsSubPadConnected && owner.pad(0).getBattery() == -1,
            "original disconnect resets both connection flags and battery availability");
    sdk.set_sensor_bar_position(WPAD_SENSOR_BAR_POS_TOP);
}
void nested_owners(const std::shared_ptr<smgpc::compat::JkrHeapRuntime>& heaps) {
    auto& sdk = aurora::wpad_service();
    sdk.clear();
    auto outer_domain = smgpc::compat::JkrAllocationDomain::create(heaps, 512U << 10);
    smgpc::compat::WPadOwnership outer(outer_domain);
    next_frame(outer);
    sdk.set_device_type(0, aurora::WpadDeviceType::Freestyle);
    sdk.set_connected(0, true);
    next_frame(outer);
    require(outer.pad(0).getBattery() == -1, "the outer original owner has a pending information request before suspension");
    std::array<WPadRumble*, 2> menu{};
    for (int channel = 0; channel < 2; ++channel) {
        menu[channel] = outer.pad(channel)._1C;
        menu[channel]->registInstance();
    }
    {
        auto inner_domain = smgpc::compat::JkrAllocationDomain::create(heaps, 512U << 10);
        smgpc::compat::WPadOwnership inner(inner_domain);
        require(MR::getWPad(0) == &inner.pad(0) && inner.pad(0).getRumbleInstance() == inner.pad(0)._18,
                "a nested owner publishes its own actual WPad and gameplay rumble objects");
        next_frame(inner);
        require(inner.pad(0).mIsConnected && inner.pad(0).getBattery() == -1 && outer.pad(0).getBattery() == -1,
                "nested callbacks queue their own real information buffer without completing the suspended outer request");
        // Retire with the inner request still pending. It must not survive the
        // retained Game heap, while the outer request must remain deliverable.
    }
    require(MR::getWPad(0) == &outer.pad(0), "retiring the nested owner republishes the actual outer holder");
    for (int channel = 0; channel < 2; ++channel)
        require(outer.pad(channel).getRumbleInstance() == menu[channel],
                "nested teardown restores the exact previously active menu rumble object");
    next_frame(outer);
    require(outer.pad(0).getBattery() == 4, "the resumed SDK client completes the original suspended outer information request");
    const auto free_before = heaps->root_heap().getFreeSize();
    bool rejected = false;
    try {
        auto insufficient = smgpc::compat::JkrAllocationDomain::create(heaps, 4096);
        smgpc::compat::WPadOwnership failed(insufficient);
    } catch (const std::bad_alloc&) {
        rejected = true;
    }
    require(rejected, "the real four-buffer holder rejects an insufficient Game heap");
    require(heaps->root_heap().getFreeSize() == free_before && MR::getWPad(0) == &outer.pad(0),
            "failed nested construction releases its domain and preserves the actual outer publication");
    for (int channel = 0; channel < 2; ++channel)
        require(outer.pad(channel).getRumbleInstance() == menu[channel],
                "construction rollback preserves the exact outer menu rumble registrations");
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
        auto heaps = smgpc::compat::JkrHeapRuntime::create(8U << 20);
        const auto free_before = heaps->root_heap().getFreeSize();
        for (int generation = 0; generation < 2; ++generation) {
            original_holder_flow(heaps);
            nested_owners(heaps);
        }
        callback_retirement();
        require(heaps->root_heap().getFreeSize() == free_before,
                "actual holder, read records, children and failed-construction domains are reclaimed across generations");
        bool absent = false;
        try { (void)MR::getWPad(0); }
        catch (const std::logic_error&) { absent = true; }
        require(absent, "retiring the final original owner leaves no published Game controller");
        std::cout << "Original WPad holder, callbacks, four-channel reads, battery, modes, nested ownership and retirement passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
