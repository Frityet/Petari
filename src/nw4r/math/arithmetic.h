#pragma once

#include <revolution/os.h>
#include <revolution/types.h>
#include <cmath>

namespace nw4r::math {
inline f32 U16ToF32(u16 value) {
    f32 result;
    OSu16tof32(&value, &result);
    return result;
}
inline u16 F32ToU16(f32 value) {
    u16 result;
    OSf32tou16(&value, &result);
    return result;
}
inline f32 FAbs(f32 value) { return std::fabs(value); }
inline f32 FSelect(f32 condition, f32 ifPositive, f32 ifNegative) {
    return condition >= 0.0f ? ifPositive : ifNegative;
}
} // namespace nw4r::math
