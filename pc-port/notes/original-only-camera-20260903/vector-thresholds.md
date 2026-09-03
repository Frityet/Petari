# Retail vector magnitude threshold regression

The change from the native C square-root path to original `TVec3::length -> PSVECMag` exposed two stale OnlyCamera fixture expectations. No camera or math production source was changed for this regression correction.

`PSVECMag` at `0x804B90D8` (`0x44` bytes) evaluates squared magnitude, the hardware `frsqrte` estimate, and one rounded Newton refinement. It returns the refined reciprocal multiplied by squared magnitude. It does not call an exact square root. `verify-vector-thresholds.py` checks all 17 retail instruction words and the actual `.5`/`3` constants against the verified RMGK01 DOL, then independently computes four axis witnesses using the hardware-derived estimate table in the checked-out Dolphin source. It compares those values to the real native SDK provider compiled separately with Homebrew LLVM.

| Input X bits | Retail and native magnitude bits | Consequence |
| --- | --- | --- |
| `3f800000` (1) | `3f7fffff` | Strict `<1` comparison is true. |
| `3f800001` (float immediately above 1) | `3f800000` | Strict `<1` comparison is false. |
| `3f800008` (1 + 2^-20) | `3f800007` | Above 1, also survives adding/subtracting the fixture eye X=10. |
| `40000000` (2) | `3fffffff` | The same one-step refinement effect scales with this power of two. |

The unchanged original `OnlyCamera::calcSafePose` compares this magnitude with 1 and reuses its previous watch displacement below that threshold. Thus a requested exact one-unit displacement retains the old `(0,0,-450)` vector, instead of extending the requested X direction to 300. `moveToIdealPosition` likewise treats unit distance as smaller than the newly accelerated speed 1 and stops immediately, storing speed 0. The float immediately above one yields an exact magnitude 1 and retains the original strict endpoint branch; the fixture now covers both sides explicitly.

`tests/OnlyCameraTests.cpp` now asserts the two exact magnitude bits and corrects those expected camera states. The translated-watch case also checks the next representable watch X above 11. All other camera cases are retained.

Validation: `xmake build smg-pc-only-camera-tests` passed, followed by **9/9** tests. Logs are `vector-threshold-build.log` and `vector-threshold-tests.log`. The executed binary SHA256 was `9c074affe5c80bee8016dc14cde9339314ec38ae17339eeb5c8f9581bc866ba5`. The isolated four-witness probe passed and records source/tool invocation evidence in `vector-threshold-evidence.json`. This bounded probe does not claim every PSVECMag exceptional input or every floating-point status behavior is emulated.
