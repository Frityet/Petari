#include "compat/JkrAllocationDomain.hpp"
#include "compat/WPadOwnership.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "Game/System/GameSystemFunction.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadRumble.hpp"
#include "Game/System/WPadRumbleData.hpp"
#include <cassert>
#include <cstdio>

namespace {
void seed(WPadRumble& rumble, const RumblePattern& pattern) {
    rumble._8 = true;
    for (auto& channel : rumble.mChannel) {
        channel._0 = &pattern;
        channel._4 = true;
        channel._8 = 17;
        channel._C = 4;
        channel._E = true;
        channel._10 = &rumble;
    }
}
void cleared(const WPadRumble& rumble) {
    assert(!rumble._8);
    for (const auto& channel : rumble.mChannel) {
        assert(channel._0 == nullptr && !channel._4 && channel._8 == 0 &&
               channel._C == 0 && !channel._E && channel._10 == nullptr);
    }
}
}
int main() {
    auto heaps = smgpc::compat::JkrHeapRuntime::create(8U << 20);
    const auto free_before = heaps->root_heap().getFreeSize();
    RumblePattern pattern{};
    for (int generation = 0; generation < 16; ++generation) {
        auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 1U << 20);
        smgpc::compat::WPadOwnership input(domain);
        for (int channel = 0; channel < 2; ++channel) {
            auto& pad = input.pad(channel);
            assert(pad.getRumbleInstance() == pad._18);
            seed(*pad._18, pattern);
            seed(*pad._1C, pattern);
            pad._18->pause();
            assert(pad._18->_8 && pad._18->mChannel[0]._0 == &pattern);
        }
        GameSystemFunction::onPauseBeginAllRumble();
        for (int channel = 0; channel < 2; ++channel) {
            auto& pad = input.pad(channel);
            assert(pad.getRumbleInstance() == pad._1C);
            cleared(*pad._18);
            cleared(*pad._1C);
            seed(*pad._1C, pattern);
        }
        GameSystemFunction::onPauseEndAllRumble();
        for (int channel = 0; channel < 2; ++channel) {
            auto& pad = input.pad(channel);
            assert(pad.getRumbleInstance() == pad._18);
            cleared(*pad._18);
            cleared(*pad._1C);
        }
    }
    assert(heaps->root_heap().getFreeSize() == free_before);
    std::puts("[pass] original two-controller gameplay/menu rumble selection, pause preservation, stop channel clearing and16 reclaimed Game owner generations");
}
