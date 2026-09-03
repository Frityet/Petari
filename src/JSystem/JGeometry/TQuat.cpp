#include "JSystem/JGeometry/TQuat.hpp"

namespace JGeometry {
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
