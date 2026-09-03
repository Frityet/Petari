#include "JSystem/JMath/JMath.hpp"
extern "C" void native_scale_add(const Vec* first, const Vec* base, Vec* output, float scale) {
    JMAVECScaleAdd(first, base, output, scale);
}
