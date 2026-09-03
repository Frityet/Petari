#include "JSystem/JMath/JMATrigonometric.hpp"

namespace JMath {
    template <>
    f32 TAtanTable< 1024, f32 >::atan2_(f32 y, f32 x) const {
        if (y >= 0.0f) {
            if (x >= 0.0f) {
                if (x >= y) {
                    return get_(y, x);
                }
                return 1.5707964f - get_(x, y);
            }

            x = -x;
            if (x < y) {
                return 1.5707964f + get_(x, y);
            }
            return 3.1415927f - get_(y, x);
        }

        y = -y;
        if (x < 0.0f) {
            x = -x;
            if (x >= y) {
                return -3.1415927f + get_(y, x);
            }
            return -1.5707964f - get_(x, y);
        }

        if (x < y) {
            return -1.5707964f + get_(x, y);
        }
        return -get_(y, x);
    }

    template <>
    f32 TAtanTable< 1024, f32 >::get_(f32 y, f32 x) const {
        if (x == 0.0f) {
            return 0.0f;
        }

        s32 index = 0.5f + (1024.0f * y) / x;
        if (index == 1024) {
            return _1000;
        }
        return mTable[index];
    }

    template <>
    TSinCosTable< 14, f32 >::TSinCosTable() {
        for (s32 i = 0; i < 16384; i++) {
            f64 angle = (static_cast< f64 >(i) * 6.2831854820251465) / 16384.0;
            table[i].a1 = sin(angle);
            table[i].b1 = cos(angle);
        }
    }

    template <>
    TAtanTable< 1024, f32 >::TAtanTable() {
        for (s32 i = 0; i < 1024; i++) {
            mTable[i] = atan(static_cast< f64 >(i) * (1.0 / 1024.0));
        }

        mTable[0] = 0.0f;
        _1000 = 0.7853982f;
    }

    template <>
    TAsinAcosTable< 1024, f32 >::TAsinAcosTable() {
        for (s32 i = 0; i < 1024; i++) {
            mTable[i] = asin(static_cast< f64 >(i) * (1.0 / 1024.0));
        }

        mTable[0] = 0.0f;
        _1000 = 0.7853982f;
    }
}  // namespace JMath
