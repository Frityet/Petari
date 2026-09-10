#include "../../original-save-owner-20260910/upstream/ReferenceCurveHelpers.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

using aurora::nw4r::lyt::BrlanAnimation;

int main() {
    std::uint32_t seed = 0x20260910U;
    const auto next = [&] { seed = seed * 1664525U + 1013904223U; return seed; };
    unsigned comparisons = 0;
    unsigned mismatches = 0;
    for (unsigned curve = 0; curve < 64; ++curve) {
        BrlanAnimation::Target step;
        step.curve_type = 1;
        BrlanAnimation::Target hermite;
        hermite.curve_type = 2;
        hermite.target = 1;
        const unsigned count = 1 + next() % 16;
        float key_frame = 0.0F;
        for (unsigned key = 0; key < count; ++key) {
            if (key) key_frame += static_cast<float>(next() % 4);
            step.step_keys.push_back({key_frame, static_cast<std::uint16_t>(next() >> 16)});
            const auto value = static_cast<float>(static_cast<int>(next() % 257) - 128) * 0.25F;
            const auto slope = static_cast<float>(static_cast<int>(next() % 129) - 64) * 0.125F;
            hermite.hermite_keys.push_back({key_frame, value, slope});
        }
        BrlanAnimation animation;
        animation.contents = {{"Pane", {{"RLPA", {step, hermite}}}}};
        std::vector<float> frames;
        for (unsigned i = 0; i < 128; ++i) frames.push_back(static_cast<float>(i) * 0.1875F - 2.0F);
        for (const auto& key : hermite.hermite_keys) {
            const auto edge = key.frame - 0.001F;
            frames.push_back(std::nextafter(edge, -std::numeric_limits<float>::infinity()));
            frames.push_back(edge);
            frames.push_back(std::nextafter(edge, std::numeric_limits<float>::infinity()));
            frames.push_back(key.frame - 0.0005F);
            frames.push_back(key.frame);
            frames.push_back(key.frame + 0.0005F);
        }
        for (const auto frame : frames) {
            const auto original_step = reference_curve::GetStepCurveValue(frame, step.step_keys.data(), count);
            const auto original_hermite = reference_curve::GetHermiteCurveValue(frame, hermite.hermite_keys.data(), count);
            const auto native = animation.pane_frame("Pane", frame);
            const bool step_differs = native.translate_x.value() != original_step;
            const bool hermite_differs = std::bit_cast<std::uint32_t>(native.translate_y.value()) !=
                                         std::bit_cast<std::uint32_t>(original_hermite);
            comparisons += 2;
            mismatches += step_differs + hermite_differs;
            if ((step_differs || hermite_differs) && mismatches < 10) {
                std::printf("curve=%u keys=%u frame=%.9g step=%u/%g hermite=%a/%a\n", curve, count, frame,
                            original_step, native.translate_x.value(), original_hermite, native.translate_y.value());
            }
        }
    }
    std::printf("%u original/public-native curve comparisons across 64 deterministic curves; mismatches=%u\n",
                comparisons, mismatches);
    return mismatches ? 1 : 0;
}
