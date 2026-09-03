# Original camera trigonometry dependencies

The original `CameraParallel::calcIdealPose` calls `MR::crossToPolar`, which
uses `JMAATan2`. The old PC table class delegated to host `atan2` and therefore
skipped the original quantized angle lookup. The same primitive is used by
Mario's stick direction and quaternion direction rotation.

The following functions were recovered from the supplied RMGK01 DOL with
SHA-1 `25c5959534b3c21246c6c7e42021b916b41fb578`:

| Function | Address | Size |
| --- | --- | --- |
| `TAtanTable<1024,f32>::atan2_` | `0x804428fc` | `0x110` |
| `TAtanTable<1024,f32>::get_` | `0x80442a4c` | `0x48` |
| `TSinCosTable<14,f32>` constructor | `0x80442a94` | `0x10c` |
| `TAtanTable<1024,f32>` constructor | `0x80442ba0` | `0xb4` |
| `TQuat4<f32>::setRotate(from,to)` | `0x80016314` | `0x114` |

New decompilation lives first in root
`src/JSystem/JMath/JMATrigonometricTable.cpp` and
`src/JSystem/JGeometry/TQuat.cpp`. Both files are copied byte-for-byte into
the PC source tree. No Game algorithm was changed.

## Binary evidence

The atan quadrant dispatcher preserves its eight comparison branches, including
signed-zero behavior different from host `atan2`. The bounded-ratio lookup
returns zero for a zero denominator. Otherwise it truncates
`0.5f + (1024.0f * numerator) / denominator` and loads that table cell. The
multiplication occurs before division in the binary at `0x80442a6c` and
`0x80442a70`. Index 1024 loads the separate `_1000` float, which is pi/4.
The reconstructed source explicitly selects that field to avoid C++ array
bounds undefined behavior while preserving the original memory value.

The atan constructor calls the double `atan` routine at `0x80527aa0` with
`double(index) * (1.0 / 1024.0)`, then rounds to float. The double reciprocal
at `0x8055c3a8` is exactly `0.0009765625`.

The sin/cos constructor calls double `sin` at `0x80528258` and double `cos`
at `0x80527e4c`, then rounds each sample to float. It evaluates
`(double(index) * 6.2831854820251465) / 16384.0` in double arithmetic. The
constant at `0x8055c390` is `0x401921fb60000000`: the double representation of
float-rounded two-pi, not full double-precision mathematical two-pi. The old
PC constructor rounded the angle itself to float before calling libm.

The root recovered JMath header already specifies direct degree indexing with
`45.511112f` and lap indexing with `16384`. The PC methods now use those
operations instead of converting to radians first. A concrete boundary is
float bits `0x3de0ffff` degrees: the original selects cell 5 while the old
conversion selected cell 4.

The two-vector quaternion rotation computes the cross product and its length,
returns identity when that length is at most `32 * FLT_EPSILON`, and otherwise
uses half the table `atan2(crossLength,dot)`, double sine/cosine, and scaled
cross-product components. This includes the original antiparallel identity
result. The existing three-argument root implementation uses the same JMath
angle primitive; its PC version now does too.

## Additional source-backed interface closure

Original `CameraLocalUtil::calcSafeUpVec` needs quaternion `transform(source,
destination)`, including aliasing, and the one-vector overload. Those now
follow the Hamilton-product body in root
`libs/JSystem/include/JSystem/JGeometry/TVec.hpp`. `rotate` delegates to that
same transform. There is no added quaternion normalization.

The general `MR::isNan(TVec3f)` provider implements root MathUtil's component
NaN classification with host `std::isnan`; infinities remain distinct from
NaN. The column setters `TRotation3::setXDir/setYDir/setZDir` follow the root
TMatrix header and expose the original Director view-conversion calls.

## Validation scope

The native Apple Silicon debug build and `smg-pc-game-math-rotation-tests`
passed on 2026-09-03. The original camera local-runtime checks also passed
with these shared math implementations; full real-disc walking verification
is recorded separately in the movement/camera note.

The existing GameMathRotationTests now checks all eight atan octants, axes and
signed zero, endpoint cells, nearest-cell threshold, known sin/cos samples,
direct degree/lap lookup, quaternion direction rotation and input/output
aliasing, nonunit transform scaling, and pose NaN classification. The parent
task owns serialized build and test execution; this note does not claim a
test run for this new tranche before its result is recorded.

The recovered methods are a functional reconstruction backed by the listed
instructions and constants, not an object-match percentage. Host libm and
square-root instruction rounding can still differ from PowerPC. The atan
lookup expects the original finite bounded-ratio inputs; it does not add a
new nonfinite-angle policy.

Local extraction and disassembly outputs stay ignored under
`build/compat-math-oracle/`, including the `atan2`, `atan_get`, `atan_ctor`,
`sincos_ctor`, and `quat_setrotate` files. No binary game data is staged.
