#include <aurora/wpad_motion.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iostream>
void require(bool value,const char* message) {if (!value) throw std::runtime_error(message);}
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

int main(){adapter_contract();std::cout << "adapter physical contract passed\n";}
