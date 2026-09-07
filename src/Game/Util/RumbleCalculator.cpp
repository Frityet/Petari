#include "Game/Util/RumbleCalculator.hpp"
#include <JSystem/JMath/JMATrigonometric.hpp>
#include <math_types.hpp>

RumbleCalculator::RumbleCalculator(f32 a2, f32 a3, f32 a4, u32 a5) {
    _4 = a5;
    _8 = a5;
    _C.x = 0.0f;
    _C.y = 0.0f;
    _C.z = 0.0f;
    _18 = a2;
    _1C = a3;
    _20 = a4;
}

void RumbleCalculator::start(u32 a1) {
    if (a1) {
        _8 = a1;
    }

    _4 = 0;
    _C.x = 0.0f;
    _C.y = 0.0f;
    _C.z = 0.0f;
}

void RumbleCalculator::calc() {
    if (_4 >= _8) {
        _C.zero();
        return;
    }

    f32 rate = static_cast< f32 >(_4) / static_cast< f32 >(_8);
    f32 attenuation = 1.0f + -rate;
    TVec3f phase;
    phase.x = _18 * (rate * TWO_PI);
    phase.y = phase.x + _1C;
    phase.z = phase.y + _1C;
    calcValues(&_C, phase);
    _C.scale(attenuation * _20);
    _4++;
}

void RumbleCalculator::reset() {
    _4 = _8;
    _C.zero();
}

RumbleCalculatorCosMultLinear::RumbleCalculatorCosMultLinear(f32 a2, f32 a3, f32 a4, u32 a5) : RumbleCalculator(a2, a3, a4, a5) {
}

void RumbleCalculatorCosMultLinear::calcValues(TVec3f* pValue, const TVec3f& rPhase) {
    f32 z = JMACosRadian(rPhase.z);
    f32 y = JMACosRadian(rPhase.y);
    f32 x = JMACosRadian(rPhase.x);
    pValue->set(x, y, z);
}
