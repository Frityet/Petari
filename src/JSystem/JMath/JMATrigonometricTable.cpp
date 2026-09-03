#include "JSystem/JMath/JMATrigonometric.hpp"

namespace JMath {
    template <>
    TAsinAcosTable< 1024, f32 >::TAsinAcosTable() {
        for (s32 i = 0; i < 1024; i++) {
            mTable[i] = asin(static_cast< f64 >(i) * (1.0 / 1024.0));
        }

        mTable[0] = 0.0f;
        _1000 = 0.7853982f;
    }
}  // namespace JMath
