#include "ReferenceCurveHelpers.hpp"
#include <array>
#include <cstdio>
#include <vector>

using aurora::nw4r::lyt::BrlanAnimation;

int main() {
    BrlanAnimation step;
    BrlanAnimation::Target step_target;
    step_target.curve_type = 1;
    step_target.step_keys = {{0.0F, 0}, {10.0F, 1}, {20.0F, 0}};
    step.contents.push_back({"Pane", {{"RLVI", {step_target}}}});
    BrlanAnimation hermite;
    BrlanAnimation::Target hermite_target;
    hermite_target.curve_type = 2;
    hermite_target.hermite_keys = {{0.0F, 0.0F, 0.0F}, {10.0F, 10.0F, 0.0F},
                                  {10.0F, 20.0F, 0.0F}, {20.0F, 30.0F, 0.0F}};
    hermite.contents.push_back({"Pane", {{"RLPA", {hermite_target}}}});
    unsigned gaps = 0;
    unsigned checks = 0;
    for (const float frame : std::array{0.0F, 5.0F, 9.998F, 9.9995F, 10.0F, 10.0005F, 15.0F, 20.0F}) {
        const auto expected_step = reference_curve::GetStepCurveValue(frame, step_target.step_keys.data(),
                                                                      step_target.step_keys.size());
        const bool actual_step = step.pane_frame("Pane", frame).visible.value();
        const auto expected_hermite = reference_curve::GetHermiteCurveValue(frame, hermite_target.hermite_keys.data(),
                                                                          hermite_target.hermite_keys.size());
        const float actual_hermite = hermite.pane_frame("Pane", frame).translate_x.value();
        const bool step_gap = actual_step != (expected_step != 0);
        const bool hermite_gap = actual_hermite != expected_hermite;
        gaps += step_gap + hermite_gap;
        checks += 2;
        std::printf("frame=%.7g step(reference=%u native=%u) hermite(reference=%.9g native=%.9g) gaps=%u\n",
                    frame, expected_step, actual_step, expected_hermite, actual_hermite, step_gap + hermite_gap);
        if (frame == 9.9995F && (expected_step != 1 || actual_step || expected_hermite != 20.0F || actual_hermite > 10.001F)) {
            return 1;
        }
    }
    std::printf("%u curve comparisons completed; %u currently differing outputs (audit, not compatibility pass)\n", checks, gaps);
    return gaps >= 2 ? 0 : 1;
}
