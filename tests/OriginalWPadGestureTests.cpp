#include "compat/JkrAllocationDomain.hpp"
#include "compat/WPadOwnership.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadAcceleration.hpp"
#include "Game/System/WPadHVSwing.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <aurora/exception.hpp>
#include <aurora/wpad_motion.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char* message) {
    if (!value) aurora::throw_host_exception<std::runtime_error>(message);
}
void require_rest(const aurora::WpadVec3State& sample, const char* message) {
    require(sample.x == 0 && sample.y == 0 && sample.z == 1, message);
}
void adapter_contract() {
    aurora::WpadShakeGesture gesture;
    require_rest(gesture.sample(false), "an idle virtual controller publishes one gravity unit");
    float lateral_sum = 0;
    float peak = 0;
    for (int frame = 0; frame < 12; ++frame) {
        const auto sample = gesture.sample(true);
        require(std::isfinite(sample.x) && sample.y == 0 && sample.z == 1,
                "the bounded lateral shake preserves finite upright gravity");
        lateral_sum += sample.x;
        peak = std::max(peak, std::abs(sample.x));
    }
    require(lateral_sum == 0 && peak == 2, "the 200 ms shake has balanced lateral impulse and a two-g peak");
    for (int frame = 0; frame < 120; ++frame)
        require_rest(gesture.sample(true), "a held key cannot restart a completed physical gesture");
    require_rest(gesture.sample(false), "releasing a completed gesture leaves gravity at rest");
    require(gesture.sample(true).x == 1, "a new key edge begins a fresh physical gesture");
    require(gesture.sample(false).x == 1.5F, "key release does not truncate an already started physical gesture");
}

void original_detector(const std::shared_ptr<smgpc::compat::JkrHeapRuntime>& heaps) {
    auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 512U << 10);
    auto& input = aurora::wpad_service();
    input.clear();
    smgpc::compat::WPadOwnership owner(domain);
    auto& pad = owner.pad(0);
    input.set_device_type(0, aurora::WpadDeviceType::Freestyle);
    input.set_connected(0, true);
    require(JKRHeap::findFromRoot(pad.mCorePadSwing) == &domain->heap() &&
                JKRHeap::findFromRoot(pad.mSubPadSwing) == &domain->heap(),
            "both original gesture detectors belong to the retained input Game heap");
    aurora::WpadShakeGesture core;
    aurora::WpadShakeGesture sub;
    auto frame = [&](bool core_pressed, bool sub_pressed) {
        input.begin_frame();
        const auto core_sample = core.sample(core_pressed);
        const auto sub_sample = sub.sample(sub_pressed);
        input.set_core_acceleration(0, core_sample.x, core_sample.y, core_sample.z);
        input.set_sub_acceleration(0, sub_sample.x, sub_sample.y, sub_sample.z);
        owner.update_samples();
        const auto* record = pad.getKPadStatus(0);
        require(record && record->acc.x == core_sample.x && record->acc.z == 1 &&
                    record->ex_status.fs.acc.x == sub_sample.x && record->ex_status.fs.acc.z == 1,
                "the real SDK sample carries independent core and FreeStyle physical acceleration");
    };

    TVec3f past;
    for (int sample = 1; sample <= 20; ++sample) {
        frame(false, false);
        require(pad.mCorePadAccel->_628 == sample && pad.mSubPadAccel->_628 == sample,
                "original acceleration histories consume exactly one native record per frame");
        require(!MR::isCorePadSwing(0) && !MR::isCorePadSwingTrigger(0) && !MR::isSubPadSwing(0),
                "gravity-only warmup cannot invent a swing before twenty-frame history exists");
    }
    require(!pad.getPastAcceleration(&past, 20, WPAD_DEV_CORE),
            "twenty samples cannot yet supply original history age twenty");
    frame(false, false);
    require(pad.getPastAcceleration(&past, 20, WPAD_DEV_CORE) && past.x == 0 && past.y == 1 && past.z == 0,
            "the twenty-first sample exposes the original axis-converted gravity history");

    frame(true, false);
    require(MR::isCorePadSwing(0) && MR::isCorePadSwingTrigger(0) && !MR::isSubPadSwing(0) &&
                !pad.mSubPadSwing->mIsTriggerSwing,
            "a physical core pulse reaches the actual original swing and trigger without extension motion");
    int triggers = 1;
    for (int sample = 1; sample <= 64; ++sample) {
        frame(true, false);
        triggers += MR::isCorePadSwingTrigger(0) ? 1 : 0;
        require(!MR::isSubPadSwing(0), "core motion never writes the extension detector");
        if (sample >= 12)
            require_rest(input.core_acceleration(0), "the held key's physical pulse completes independently of history");
        if (sample >= 40)
            require(!MR::isCorePadSwing(0) && !MR::isCorePadSwingTrigger(0) && !pad.mCorePadSwing->_D,
                    "after the original comparison history and quiet timer settle, a held key stays idle");
    }
    // The original lagged comparison can observe both the pulse and its later
    // history. Do not impose an artificial one-trigger-per-key Game policy.
    std::cout << "Original core detector triggers during one physical gesture: " << triggers << '\n';
    frame(false, false);
    frame(true, false);
    require(MR::isCorePadSwing(0) && MR::isCorePadSwingTrigger(0),
            "a released and pressed key produces a fresh original swing after settling");
    for (int sample = 0; sample < 64; ++sample) frame(true, false);

    require(pad.mSubPadSwing->_8 == 2,
            "the original WPad constructor gives the extension its own two-g swing threshold");
    for (int sample = 0; sample < 2; ++sample) {
        frame(true, true);
        require(!MR::isCorePadSwing(0) && !MR::isCorePadSwingTrigger(0) && !MR::isSubPadSwing(0) &&
                    !pad.mSubPadSwing->mIsTriggerSwing,
                "the extension's one-g and one-and-a-half-g samples remain below its original threshold");
    }
    frame(true, true);
    require(!MR::isCorePadSwing(0) && !MR::isCorePadSwingTrigger(0) && MR::isSubPadSwing(0) &&
                pad.mSubPadSwing->mIsTriggerSwing,
            "the third extension sample reaches its original inclusive two-g threshold independently");
    for (int sample = 0; sample < 64; ++sample) frame(true, true);
    require(!MR::isSubPadSwing(0) && !pad.mSubPadSwing->mIsTriggerSwing && !pad.mSubPadSwing->_D,
            "a held extension gesture also completes through the original history");
}
} // namespace

int main() {
    try {
        adapter_contract();
        auto heaps = smgpc::compat::JkrHeapRuntime::create(8U << 20);
        const auto free_before = heaps->root_heap().getFreeSize();
        for (int generation = 0; generation < 2; ++generation) original_detector(heaps);
        require(heaps->root_heap().getFreeSize() == free_before,
                "repeated complete original input owners reclaim all gesture and history children");
        std::cout << "Original WPad gesture physics, warmup, swing, completion, retrigger, independence and ownership passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
