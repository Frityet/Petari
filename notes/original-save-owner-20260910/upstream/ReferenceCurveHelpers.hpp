// Exact helper bodies extracted from the merged original lyt_animation.cpp.
// Only res key type aliases below adapt the container representation for this probe.
#include <aurora/nw4r/brlan.hpp>
#include <cstdint>
namespace reference_curve {
using f32 = float;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
namespace res {
using StepKey = aurora::nw4r::lyt::BrlanAnimation::StepKey;
using HermiteKey = aurora::nw4r::lyt::BrlanAnimation::HermiteKey;
}
const f32 R_SAME_TOLERANCE = 1.0e-5F;
const f32 R_FRAME_TOLERANCE = 0.001F;
            inline bool RIsSame(const f32 a, const f32 b, const f32 tolerance = R_SAME_TOLERANCE) {
                f32 c = a - b;
                return (-tolerance < c && c < tolerance);
            }

            u16 GetStepCurveValue(f32 frame, const res::StepKey* keyArray, u32 keySize) {
                if (keySize == 1 || frame <= keyArray[0].frame) {
                    return keyArray[0].value;
                } else if (frame >= keyArray[keySize - 1].frame) {
                    return keyArray[keySize - 1].value;
                }

                int ikeyL = 0;
                int ikeyR = (int)keySize - 1;
                while (ikeyL != ikeyR - 1 && ikeyL != ikeyR) {
                    int ikeyCenter = (ikeyL + ikeyR) / 2;
                    const res::StepKey& centerKey = keyArray[ikeyCenter];
                    if (frame < centerKey.frame) {
                        ikeyR = ikeyCenter;
                    } else {
                        ikeyL = ikeyCenter;
                    }
                }

                if (RIsSame(frame, keyArray[ikeyR].frame, R_FRAME_TOLERANCE)) {
                    return keyArray[ikeyR].value;
                } else {
                    return keyArray[ikeyL].value;
                }
            }

            f32 GetHermiteCurveValue(f32 frame, const res::HermiteKey* keyArray, u32 keySize) {
                if (keySize == 1 || frame <= keyArray[0].frame) {
                    return keyArray[0].value;
                } else if (frame >= keyArray[keySize - 1].frame) {
                    return keyArray[keySize - 1].value;
                }

                int ikeyL = 0;
                int ikeyR = (int)keySize - 1;
                while (ikeyL != ikeyR - 1 && ikeyL != ikeyR) {
                    int ikeyCenter = (ikeyL + ikeyR) / 2;
                    if (frame <= keyArray[ikeyCenter].frame) {
                        ikeyR = ikeyCenter;
                    } else {
                        ikeyL = ikeyCenter;
                    }
                }

                const res::HermiteKey& key0 = keyArray[ikeyL];
                const res::HermiteKey& key1 = keyArray[ikeyR];
                if (RIsSame(frame, key1.frame, R_FRAME_TOLERANCE)) {
                    if (ikeyR < keySize - 1 && key1.frame == keyArray[ikeyR + 1].frame) {
                        return keyArray[ikeyR + 1].value;
                    } else {
                        return key1.value;
                    }
                }
                f32 t1 = frame - key0.frame;
                f32 t2 = 1.0F / (key1.frame - key0.frame);
                f32 v0 = key0.value;
                f32 v1 = key1.value;
                f32 s0 = key0.slope;
                f32 s1 = key1.slope;

                f32 t1t1t2 = t1 * t1 * t2;
                f32 t1t1t2t2 = t1t1t2 * t2;
                f32 t1t1t1t2t2 = t1 * t1t1t2t2;
                f32 t1t1t1t2t2t2 = t1t1t1t2t2 * t2;

                return v0 * (2.0F * t1t1t1t2t2t2 - 3.0F * t1t1t2t2 + 1.0F) + v1 * (-2.0F * t1t1t1t2t2t2 + 3.0F * t1t1t2t2) +
                       s0 * (t1t1t1t2t2 - 2.0F * t1t1t2 + t1) + s1 * (t1t1t1t2t2 - t1t1t2);
            }

}
