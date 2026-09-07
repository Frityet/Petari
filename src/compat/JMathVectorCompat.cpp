#include "JSystem/JMath/JMath.hpp"

#include <cmath>
#include <dolphin/ppc_math.h>

// Native translation of JMAVECScaleAdd in src/JSystem/JMath/JMath.cpp. Its
// original compiler schedules every input load before the first retail store.
void JMAVECScaleAdd(const Vec* vec1, const Vec* vec2, Vec* dst, f32 scale) {
    const f32 x = std::fma(vec1->x, scale, vec2->x);
    const f32 y = std::fma(vec1->y, scale, vec2->y);
    const f32 z = std::fma(vec1->z, scale, vec2->z);
    dst->x = ppc_psq_store_f32(x);
    dst->y = ppc_psq_store_f32(y);
    dst->z = ppc_psq_store_f32(z);
}
