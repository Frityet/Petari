# Original CameraAnim import

The four PC Game files `Camera/CameraAnim.cpp`, `CameraAnim.hpp`,
`CamTranslatorAnim.cpp`, and `CamTranslatorAnim.hpp` are byte-identical copies
of their root source/header counterparts. They supply the original CANM and
CKAN accessors, controller reset/calc/frame progression, and translator
virtual contract. No Game algorithm or constant changed. Existing source
FIXME comments remain; this import makes no new binary-matching claim.

The shared native JMath header lacked `JMAHermiteInterpolation`. Root
`libs/JSystem/include/JSystem/JMath/JMath.hpp` contained only its Metrowerks
assembly implementation. A non-Metrowerks scalar fallback was added there
first and copied into `pc-port/src/JSystem/JMath/JMath.hpp`. It follows the
original scalar instruction order, preserving all intermediate `f32` values
and explicit fused operations through `std::fma`. The final
`-std::fma(ff31, ff25, -ff26)` retains the negation of the original `fnmsubs`,
including its result-sign behavior. The original Metrowerks branch is
unchanged. This is an architecture fallback from existing source instructions,
not a replacement Hermite formula or a new clamping policy.

`source-correspondence.json` records the four whole-file hashes and the shared
fallback-body hash. `verify-source.py` checks them. No build or runtime test
was run by the source-import agent; integration and testing are owned by the
root task. Native CameraAnimation parsing, EventCamera, OriginalGameCamera,
and tests were not edited by this import.

Integration must supply validated, aligned native-endian file storage to
`CameraAnim::setParam(u8*, f32)` and retain it for the controller's lifetime.
The original controller reads native numeric fields directly and does not
perform byte swapping or allocation-size validation. `loadBin` can reject a
header, while `setParam` does not return that result; validate before invoking
the typed method. CKAN interpolation retains the original key search and
tangent division by 30. Its accessors do not add endpoint clamps or guards
for empty tracks.

`reset` initializes the original current frame and copies the real manager's
pose. `calc` intentionally returns null; successful CANM calculation must not
be interpreted as a target-identity failure. While the current frame is below
the authored frame count, it evaluates position/watch/up/roll/FOV and advances
by speed unless animation-paused. At or beyond the end it updates only final
roll/FOV, preserving the last evaluated position/watch. A director pause
continues to skip the camera phase entirely.

CameraAnim allocates two accessor objects in its constructor. The native
controller owner must retain and retire those children along with its actual
camera and manager allocations. The translator remains compiled unchanged,
but its `mNum1` field carries a retail 32-bit pointer. It is not a valid host
pointer transport on 64-bit PC. Use the typed `CameraAnim::setParam` entrypoint
until a general pointer-argument binding exists; do not fabricate or truncate
an address to exercise the translator.
