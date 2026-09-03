// Direct calls into the production Aurora/JMath providers, not copied formulas.
#include "JSystem/JMath/JMath.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"

namespace JMath {
TSinCosTable<14, f32> sSinCosTable;
}

extern "C" {
void oracle_native_mtx(const float* q, float* out) {
    PSMTXQuat(reinterpret_cast<MtxPtr>(out), reinterpret_cast<const Quaternion*>(q));
}
void oracle_native_lerp(const float* p, const float* q, float t, float* out) {
    JMAQuatLerp(reinterpret_cast<const Quaternion*>(p), reinterpret_cast<const Quaternion*>(q), t,
               reinterpret_cast<Quaternion*>(out));
}
void oracle_native_euler(int x, int y, int z, float* out) {
    JMAEulerToQuat(static_cast<s16>(x), static_cast<s16>(y), static_cast<s16>(z), reinterpret_cast<Quaternion*>(out));
}
const float* oracle_native_table() { return reinterpret_cast<const float*>(JMath::sSinCosTable.table); }
}
