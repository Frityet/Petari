#include "compat/JkrAllocationDomain.hpp"
#include "compat/WPadOwnership.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadAcceleration.hpp"
#include "Game/System/WPadHolder.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <aurora/exception.hpp>
#include <aurora/wpad.hpp>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char* message) {
    if (!value) aurora::throw_host_exception<std::runtime_error>(message);
}
void near(float actual, float expected, const char* message) {
    require(std::isfinite(actual) && std::abs(actual - expected) < 0.0001F, message);
}
void vector_near(const TVec3f& value, float x, float y, float z, const char* message) {
    near(value.x, x, message);
    near(value.y, y, message);
    near(value.z, z, message);
}
KPADStatus& one_sample(WPad& pad) {
    pad.mReadInfo->mValidStatusCount = 1;
    auto& sample = pad.mReadInfo->mStatusArray[0];
    sample = KPADStatus{};
    sample.wpad_err = WPAD_ERR_NONE;
    sample.dev_type = WPAD_DEV_FREESTYLE;
    return sample;
}
void getters_and_axes(WPad& pad) {
    auto& core = *pad.mCorePadAccel;
    auto& sub = *pad.mSubPadAccel;
    TVec3f result(7, 8, 9);
    require(!core.getAcceleration(&result), "an empty original history has no current acceleration");
    vector_near(result, 0, 0, 0, "empty current getter zeros its output");
    result.set(7, 8, 9);
    require(!core.getPastAcceleration(&result, 0), "empty history rejects age zero");
    vector_near(result, 0, 0, 0, "empty past getter zeros its output");

    pad.mReadInfo->mValidStatusCount = 0;
    core.update();
    sub.update();
    require(core._624 == 0 && core._628 == 1 && core.isStationary() && core.isBalanced(),
            "the original no-sample path starts the history with one stable zero sample");
    auto& sample = one_sample(pad);
    sample.acc = {1, 2, 3};
    sample.ex_status.fs.acc = {4, 5, 6};
    core.update();
    sub.update();
    require(core.getAcceleration(&result), "core acceleration consumes the current KPAD record");
    vector_near(result, -1, 3, -2, "original core axes are minus X, plus Z, minus Y");
    require(sub.getAcceleration(&result), "Nunchuk acceleration consumes the extension record");
    vector_near(result, -4, 6, -5, "original Nunchuk axes use the separate extension acceleration");
    require(core._628 == 2 && sub._628 == 2, "a valid sample must not also duplicate the preceding sample");
    vector_near(core._62C, -.5F, 1.5F, -1, "original mean includes both available history records");

    sample.dev_type = WPAD_DEV_CORE;
    sample.acc = {7, 8, 9};
    const auto sub_slot = sub._624;
    const auto sub_count = sub._628;
    core.update();
    sub.update();
    require(sub._624 == sub_slot && sub._628 == sub_count,
            "a core-only record is skipped by the original Nunchuk history");
    sub.getAcceleration(&result);
    vector_near(result, -4, 6, -5, "unsupported extension input preserves the last processed acceleration");

    pad.mReadInfo->mValidStatusCount = 0;
    core.update();
    require(core.getAcceleration(&result), "no-sample update retains the last acceleration");
    vector_near(result, -7, 9, -8, "no-sample update duplicates the preceding record exactly");
    require(core.getPastAcceleration(&result, 1), "the duplicated sample has its prior record at age one");
    vector_near(result, -7, 9, -8, "history age indexing is relative to the current ring slot");

    core._1C = .01795F;
    require(core.isBalanced(), "the original balance cutoff is .018, not .0179");
    core._1C = .018F;
    require(!core.isBalanced(), "the original balance comparison is strict at .018");
}
void history_and_average(WPad& pad) {
    WPadAcceleration acceleration(&pad, WPAD_DEV_CORE);
    TVec3f result;
    // KPAD records are newest first. Feed a full SDK batch, then continue past
    // two ring wraps to exercise every retained slot and the original mean.
    pad.mReadInfo->mValidStatusCount = 120;
    for (int i = 0; i < 120; ++i) {
        auto& sample = pad.mReadInfo->mStatusArray[i];
        sample = KPADStatus{};
        sample.wpad_err = WPAD_ERR_NONE;
        sample.dev_type = WPAD_DEV_CORE;
        const float value = float(119 - i);
        sample.acc = {value, 2 * value, 3 * value};
    }
    acceleration.update();
    require(acceleration._624 == 119 && acceleration._628 == 120,
            "a full newest-first SDK batch is ingested oldest first without an extra sample");
    for (int value = 120; value < 300; ++value) {
        one_sample(pad).acc = {float(value), float(value * 2), float(value * 3)};
        acceleration.update();
    }
    require(acceleration._624 == 43 && acceleration._628 == 128,
            "the original ring wraps at 128 samples and saturates its available count");
    for (int age = 0; age < 128; ++age) {
        require(acceleration.getPastAcceleration(&result, age), "every retained original history age remains addressable");
        const float value = float(299 - age);
        vector_near(result, -value, 3 * value, -2 * value, "each ring age resolves the corresponding physical input record");
    }
    require(!acceleration.getPastAcceleration(&result, 128), "an overwritten history record is unavailable");
    vector_near(result, 0, 0, 0, "out-of-history access zeros its output");
    vector_near(acceleration._62C, -235.5F, 706.5F, -471, "the mean spans all 128 retained records");
    near(acceleration._1C, 14.F * 31.F / 32.F,
         "original average sums 31 squared adjacent differences and divides by 32 samples");
    require(acceleration.isStationary(), "retail stability deliberately observes physical history slot zero");
    vector_near(acceleration._10, -256, 768, -512, "the stability reference follows slot zero at the most recent ring wrap");
    pad.mReadInfo->mValidStatusCount = 0;
    acceleration.update();
    near(acceleration._1C, 14.F * 30.F / 32.F,
         "a duplicated no-input record contributes zero to the adjacent-difference average");
}
void rotation_and_stability(WPad& pad) {
    for (int plane = 0; plane < 2; ++plane) {
        for (int direction : {-1, 1}) {
            WPadAcceleration acceleration(&pad, WPAD_DEV_CORE);
            for (int i = 0; i < 32; ++i) {
                const float angle = float(direction * i) * 6.283185307179586F / 32;
                const float a = 2 * std::cos(angle);
                const float b = 2 * std::sin(angle);
                auto& sample = one_sample(pad);
                // Inverse of the independently asserted SDK-to-Game axes above.
                if (plane == 0) sample.acc = {-a, 0, b};
                else sample.acc = {0, -b, a};
                acceleration.update();
            }
            const float signed_area = -float(direction) * 128 * std::sin(6.283185307179586F / 32);
            near(plane == 0 ? acceleration._638.z : acceleration._638.x, signed_area,
                 "the recovered rotation sum agrees with the oriented area of the sampled regular polygon");
            require((plane == 0 ? acceleration._648 : acceleration._64C) == -direction,
                    "original horizontal and vertical rotation signs follow the sampled polygon orientation");
            require((plane == 0 ? acceleration._64C : acceleration._648) == 0,
                    "planar samples do not fabricate rotation on the other axis");
            acceleration._644 = 20;
            acceleration.updateRotate();
            require(acceleration._648 == 0 && acceleration._64C == 0,
                    "original rotation direction detection begins only at history index20");
            acceleration._648 = 1;
            acceleration._64C = -1;
            acceleration._644 = 2;
            acceleration.updateRotate();
            vector_near(acceleration._638, 0, 0, 0, "fewer than three rotation samples produce zero area");
            require(acceleration._648 == 1 && acceleration._64C == -1,
                    "the original short-history return preserves preceding direction flags");
        }
    }
    WPadAcceleration acceleration(&pad, WPAD_DEV_CORE);
    pad.mReadInfo->mValidStatusCount = 0;
    acceleration.update();
    acceleration.mHistory[0].x = .3F;
    acceleration.updateIsStable();
    require(!acceleration.isStationary(), "the original stability threshold includes equality");
    near(acceleration._10.x, .3F, "an unstable slot-zero sample becomes the next stability reference");
    acceleration.updateIsStable();
    require(acceleration.isStationary(), "the unchanged reference is stable on the next update");
}
} // namespace

int main() {
    try {
        auto heaps = smgpc::compat::JkrHeapRuntime::create(8U << 20);
        const auto free_before = heaps->root_heap().getFreeSize();
        for (int generation = 0; generation < 2; ++generation) {
            auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 512U << 10);
            aurora::wpad_service().clear();
            smgpc::compat::WPadOwnership owner(domain);
            auto& pad = owner.pad(0);
            require(JKRHeap::findFromRoot(pad.mCorePadAccel) == &domain->heap() &&
                    JKRHeap::findFromRoot(pad.mSubPadAccel) == &domain->heap(),
                    "the original WPad owns both actual acceleration children in its retained Game heap");
            getters_and_axes(pad);
            history_and_average(pad);
            rotation_and_stability(pad);
        }
        require(heaps->root_heap().getFreeSize() == free_before,
                "repeated actual input owner generations reclaim both acceleration histories");
        std::cout << "Original acceleration axes, history, averaging, rotation, stability and ownership passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
