#include "JSystem/JGeometry/TQuat.hpp"

namespace JGeometry {
    template <>
    void TQuat4< f32 >::setRotate(const TVec3< f32 >& rFrom, const TVec3< f32 >& rTo) {
        TVec3< f32 > axis = rFrom.cross(rTo);
        f32 crossLength = axis.length();
        if (crossLength <= TUtil< f32 >::epsilon()) {
            set< f32 >(0.0f, 0.0f, 0.0f, 1.0f);
            return;
        }

        f32 halfAngle = 0.5f * JMAATan2(crossLength, rFrom.dot(rTo));
        f32 scale = static_cast< f32 >(sin(static_cast< f64 >(halfAngle))) / crossLength;
        x = axis.x * scale;
        y = axis.y * scale;
        z = axis.z * scale;
        w = cos(static_cast< f64 >(halfAngle));
    }

    template <>
    void TQuat4< f32 >::slerp(const TQuat4< f32 >& rTarget, f32 rate) {
        TQuat4< f32 > from;
        from.normalize(*this);
        TQuat4< f32 > to;
        to.normalize(rTarget);

        f32 dot = from.dot(to);
        bool isOpposite = dot < 0.0f;
        if (isOpposite) {
            dot = -dot;
        }

        f32 fromWeight;
        f32 toWeight = rate;
        if (1.0f - dot <= TUtil< f32 >::epsilon()) {
            fromWeight = 1.0f - rate;
        } else {
            f32 angle = TUtil< f32 >::acos(dot);
            f32 sinAngle = sin(static_cast< f64 >(angle));
            fromWeight = static_cast< f32 >(sin(static_cast< f64 >((1.0f - rate) * angle))) / sinAngle;
            toWeight = static_cast< f32 >(sin(static_cast< f64 >(rate * angle))) / sinAngle;
        }

        if (isOpposite) {
            toWeight = -toWeight;
        }

        set< f32 >(fromWeight * from.x + toWeight * to.x,
                   fromWeight * from.y + toWeight * to.y,
                   fromWeight * from.z + toWeight * to.z,
                   fromWeight * from.w + toWeight * to.w);
    }
}  // namespace JGeometry
