#pragma once

#include <math_types.hpp>
#include <revolution.h>

#include <algorithm>
#include <cmath>

namespace JMath {

    template <typename A, typename B>
    struct TSinCosPair {
        A a1{};
        B b1{};
    };

    template <typename T>
    struct TAngleConstant_;

    template <>
    struct TAngleConstant_<f32> {
        [[nodiscard]] static constexpr f32 RADIAN_DEG090() {
            return 1.57079632679489661923F;
        }

        [[nodiscard]] static constexpr f32 RADIAN_DEG180() {
            return PI;
        }

        [[nodiscard]] static constexpr f32 RADIAN_DEG360() {
            return TWO_PI;
        }

        [[nodiscard]] static constexpr f32 RADIAN_TO_DEGREE_FACTOR() {
            return 180.0F / RADIAN_DEG180();
        }
    };

    template <int Bits, typename T>
    class TSinCosTable {
    public:
        static constexpr u32 LEN = 1U << Bits;

        TSinCosTable() {
            for (auto index = u32{}; index < LEN; ++index) {
                const auto angle = static_cast<T>(index) *
                                   (TAngleConstant_<T>::RADIAN_DEG360() / static_cast<T>(LEN));
                table[index].a1 = static_cast<T>(std::sin(angle));
                table[index].b1 = static_cast<T>(std::cos(angle));
            }
        }

        [[nodiscard]] T sinShort(s16 value) const {
            return table[static_cast<u16>(value) >> (16U - Bits)].a1;
        }

        [[nodiscard]] T cosShort(s16 value) const {
            return table[static_cast<u16>(value) >> (16U - Bits)].b1;
        }

        [[nodiscard]] T sinRadian(T value) const {
            const auto scaled = value * (static_cast<T>(LEN) / TAngleConstant_<T>::RADIAN_DEG360());
            const auto index = static_cast<u16>(scaled < static_cast<T>(0) ? -scaled : scaled) & (LEN - 1U);
            const auto sine = table[index].a1;
            return scaled < static_cast<T>(0) ? -sine : sine;
        }

        [[nodiscard]] T cosRadian(T value) const {
            const auto scaled = std::abs(value) *
                                (static_cast<T>(LEN) / TAngleConstant_<T>::RADIAN_DEG360());
            return table[static_cast<u16>(scaled) & (LEN - 1U)].b1;
        }

        [[nodiscard]] T sinDegree(T value) const {
            return sinRadian(value / TAngleConstant_<T>::RADIAN_TO_DEGREE_FACTOR());
        }

        [[nodiscard]] T cosDegree(T value) const {
            return cosRadian(value / TAngleConstant_<T>::RADIAN_TO_DEGREE_FACTOR());
        }

        [[nodiscard]] T sinLap(T value) const {
            return sinRadian(value * TAngleConstant_<T>::RADIAN_DEG360());
        }

        [[nodiscard]] T cosLap(T value) const {
            return cosRadian(value * TAngleConstant_<T>::RADIAN_DEG360());
        }

        [[nodiscard]] T get(T value) const {
            return table[static_cast<u16>(value) & (LEN - 1U)].b1;
        }

        TSinCosPair<T, T> table[LEN]{};
    };

    template <s32 Len, typename T>
    class TAtanTable {
    public:
        TAtanTable() {
            for (auto index = s32{}; index < Len; ++index) {
                mTable[index] = static_cast<T>(std::atan(static_cast<T>(index) / static_cast<T>(Len)));
            }
            _1000 = static_cast<T>(std::atan(static_cast<T>(1)));
        }

        [[nodiscard]] T atan2_(T y, T x) const {
            return static_cast<T>(std::atan2(y, x));
        }

        [[nodiscard]] T get_(T y, T x) const {
            return atan2_(y, x);
        }

        T mTable[Len]{};
        T _1000{};
    };

    template <s32 Len, typename T>
    class TAsinAcosTable {
    public:
        TAsinAcosTable() {
            for (auto index = s32{}; index < Len; ++index) {
                const auto ratio = static_cast<T>(index) / static_cast<T>(Len - 1);
                mTable[index] = static_cast<T>(std::asin(ratio));
            }
            _1000 = TAngleConstant_<T>::RADIAN_DEG090();
        }

        [[nodiscard]] T get_(T value, T) const {
            return static_cast<T>(std::asin(std::clamp(value, static_cast<T>(-1), static_cast<T>(1))));
        }

        [[nodiscard]] T acos_(T value) const {
            return static_cast<T>(std::acos(std::clamp(value, static_cast<T>(-1), static_cast<T>(1))));
        }

        [[nodiscard]] T acosDegree(T value) const {
            return acos_(value) * TAngleConstant_<T>::RADIAN_TO_DEGREE_FACTOR();
        }

        T mTable[Len]{};
        T _1000{};
    };

    extern TSinCosTable<14, f32> sSinCosTable;
    extern TAtanTable<1024, f32> sAtanTable;
    extern TAsinAcosTable<1024, f32> sAsinAcosTable;

    [[nodiscard]] inline f32 acosDegree(f32 value) {
        return sAsinAcosTable.acosDegree(value);
    }

}  // namespace JMath

inline f32 JMACosRadian(f32 angle) {
    return JMath::sSinCosTable.cosRadian(angle);
}

inline f32 JMASinRadian(f32 angle) {
    return JMath::sSinCosTable.sinRadian(angle);
}

inline f32 JMACosShort(s16 angle) {
    return JMath::sSinCosTable.cosShort(angle);
}

inline f32 JMASinShort(s16 angle) {
    return JMath::sSinCosTable.sinShort(angle);
}

inline f32 JMASCos(s16 angle) {
    return JMACosShort(angle);
}

inline f32 JMASSin(s16 angle) {
    return JMASinShort(angle);
}

inline f32 JMACosDegree(f32 angle) {
    return JMath::sSinCosTable.cosDegree(angle);
}

inline f32 JMASinDegree(f32 angle) {
    return JMath::sSinCosTable.sinDegree(angle);
}

inline f32 JMACosLap(f32 angle) {
    return JMath::sSinCosTable.cosLap(angle);
}

inline f32 JMASinLap(f32 angle) {
    return JMath::sSinCosTable.sinLap(angle);
}

inline f32 JMAAcosRadian(f32 ratio) {
    return JMath::sAsinAcosTable.acos_(ratio);
}

inline f32 JMAAsinRadian(f32 ratio) {
    return std::asin(std::clamp(ratio, -1.0F, 1.0F));
}

inline f32 JMAATan2(f32 y, f32 x) {
    return JMath::sAtanTable.atan2_(y, x);
}
