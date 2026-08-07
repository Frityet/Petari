#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    [[nodiscard]] bool near(float lhs, float rhs) {
        return std::fabs(lhs - rhs) < 0.0001F;
    }
}  // namespace

int main() {
    auto passed = 0;

    auto ctrl = J3DFrameCtrl{10};
    require(ctrl.getAttribute() == J3DFrameCtrl::EMode_LOOP && ctrl.getStart() == 0 && ctrl.getEnd() == 10 &&
                ctrl.getLoop() == 0 && near(ctrl.getRate(), 1.0F) && near(ctrl.getFrame(), 0.0F),
            "frame-controller initialization must match the retail loop controller");
    ++passed;

    ctrl.setAttribute(J3DFrameCtrl::EMode_NONE);
    ctrl.setFrame(8.0F);
    ctrl.setRate(3.0F);
    require(ctrl.checkPass(9.0F), "one-shot passage must use the next real frame interval");
    ctrl.update();
    require(near(ctrl.getFrame(), 9.999F) && near(ctrl.getRate(), 0.0F) && ctrl.checkState(1U),
            "one-shot playback must clamp and stop at the retail end frame");
    ++passed;

    ctrl.init(10);
    ctrl.setLoop(2);
    ctrl.setFrame(9.0F);
    ctrl.setRate(2.0F);
    require(ctrl.checkPass(9.5F) && ctrl.checkPass(2.5F),
            "loop passage must cover both sides of the retail wrap interval");
    ctrl.update();
    require(near(ctrl.getFrame(), 3.0F) && ctrl.checkState(2U),
            "loop playback must wrap through the configured loop frame");
    ++passed;

    std::cout << "J3DFrameCtrl tests passed: " << passed << "/3\n";
    return 0;
}
