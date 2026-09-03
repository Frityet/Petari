#pragma once

#include "JSystem/JGeometry/TVec.hpp"

namespace JGeometry {
    template <typename T>
    struct TBox {
        T i{};
        T f{};
    };

    template <>
    struct TBox<TVec3<f32>> {
        void extend(const TVec3f &position) {
            if (i.x >= position.x) {
                i.x = position.x;
            }
            if (i.y >= position.y) {
                i.y = position.y;
            }
            if (i.z >= position.z) {
                i.z = position.z;
            }
            if (f.x <= position.x) {
                f.x = position.x;
            }
            if (f.y <= position.y) {
                f.y = position.y;
            }
            if (f.z <= position.z) {
                f.z = position.z;
            }
        }

        [[nodiscard]] bool intersectsPoint(const TVec3f &position) const {
            return position.x >= i.x && position.y >= i.y && position.z >= i.z && position.x < f.x && position.y < f.y && position.z < f.z;
        }

        void extend(const TVec3f &minimum, const TVec3f &maximum) {
            if (i.x >= minimum.x) i.x = minimum.x;
            if (i.y >= minimum.y) i.y = minimum.y;
            if (i.z >= minimum.z) i.z = minimum.z;
            if (f.x <= maximum.x) f.x = maximum.x;
            if (f.y <= maximum.y) f.y = maximum.y;
            if (f.z <= maximum.z) f.z = maximum.z;
        }

        void zero() {
            i.zero();
            f.zero();
        }

        void pad(f32 padding) {
            const TVec3f amount(padding);
            i.sub(amount);
            f.add(amount);
        }

        void getCenter(TVec3f *center) {
            center->lerp(f, i, 0.5F);
        }

        void set(const TVec3f &min, const TVec3f &max) {
            i.set(min);
            f.set(max);
        }

        TVec3f i{};
        TVec3f f{};
    };

    template <typename T>
    struct TBox3 : public TBox<TVec3<T>> {
    };

    template <typename T>
    struct TDirBox3 {
        TVec3<T> _0{};
        TVec3<T> _C{};
        TVec3<T> _18{};
        TVec3<T> _24{};
        TVec3<T> _30{};
    };
}  // namespace JGeometry

using TBox3f = JGeometry::TBox3<f32>;
using TDirBox3f = JGeometry::TDirBox3<f32>;
