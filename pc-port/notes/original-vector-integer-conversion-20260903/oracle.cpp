#include "JSystem/JGeometry/TVec.hpp"
void convertShort(s16* out, f32 x, f32 y, f32 z) {
    TVec3s value(x, y, z);
    out[0] = value.x; out[1] = value.y; out[2] = value.z;
}
void convertUnsigned(u16* out, f32 x, f32 y) {
    JGeometry::TVec2<u16> value(x, y);
    out[0] = value.x; out[1] = value.y;
}
void convertWord(s32* out, f32 x, f32 y, f32 z) {
    JGeometry::TVec3<s32> value(x, y, z);
    out[0] = value.x; out[1] = value.y; out[2] = value.z;
}
