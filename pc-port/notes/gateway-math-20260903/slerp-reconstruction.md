# RMGK01 quaternion interpolation binary evidence

The supplied `Super Mario Wii - Galaxy Adventure (Korea).rvz` was opened with
the already installed `encounter-nod` C API. Its header identifies `RMGK01`,
disc version 0. The extracted main DOL is 6,367,712 bytes and has SHA-1
`25c5959534b3c21246c6c7e42021b916b41fb578`, matching
`config/RMGK01/config.yml` exactly.

The local extraction helper, DOL, function byte slices, temporary ELF objects,
and disassembly are under ignored `build/compat-math-oracle/`. No game binary
or assembly is intended for staging. The extraction helper uses
`nod_disc_open`, `nod_disc_open_partition_kind(...DATA...)`, and
`nod_partition_meta(...).raw_dol`; no emulator or full disc conversion is
needed. The installed dependency is the `v2.0.0-alpha.10` static `libnod.a`
package already used by Aurora. Apple Clang linked the helper directly.

`llvm-objcopy -I binary -O elf32-powerpc` wraps a DOL function slice for
`llvm-objdump -D --triple=powerpc --adjust-vma=<address> --section=.data`.
LLVM mislabels some paired-single save/restore instructions as VSX instructions;
the scalar arithmetic and branches relevant below decode correctly.

## TQuat4<f32>::slerp(target, rate)

Symbol: `0x80016428`, size `0x1e8`; local DOL offset `0x11968`.

The recovered control flow is:

1. Normalize a local copy of `*this` and a local copy of `target` by calling
   `normalize(source)` at `0x80016610`.
2. Compute their quaternion dot product. If it is negative, negate the dot
   and remember to negate the target weight later.
3. If `1.0f - dot <= epsilon()`, set source weight to `1.0f - rate` and leave
   target weight equal to `rate`.
4. Otherwise, calculate `angle = JGeometry::TUtil<f32>::acos(dot)`,
   `sinAngle = float(sin(double(angle)))`, source weight
   `float(sin(double((1.0f-rate)*angle))) / sinAngle`, and target weight
   `float(sin(double(rate*angle))) / sinAngle`.
5. Negate the target weight if the original dot was negative.
6. Assign every component as `sourceWeight * normalizedSource.component +
   targetWeight * normalizedTarget.component`.

There is no clamp on `rate`, and no final normalization. The linear branch
threshold is `32 * FLT_EPSILON`, rather than the host's old `dot > 0.9995`.
Normalizing both local inputs also preserves aliasing and handles nonunit
callers. The three-argument root overload copies the source and invokes this
two-argument method.

Key instructions/call locations:

| Address | Meaning |
| --- | --- |
| `0x80016468`, `0x80016474` | Normalize both source inputs |
| `0x80016480` | `PSQUATDotProduct` |
| `0x80016488`–`0x800164a0` | Hemisphere/sign selection |
| `0x800164a8`–`0x800164b8` | `1-dot <= epsilon` linear branch |
| `0x800164c0`–`0x80016544` | Inlined table-based JGeometry acos |
| `0x8001654c`, `0x80016560`, `0x80016570` | Three scalar `sin` calls |
| `0x8001657c`–`0x80016584` | Restore target sign |
| `0x80016588`–`0x800165dc` | Component blend and four-component assignment |

`__init_registers` at `0x80004224`/`0x80004228` establishes the verified
`r2 = 0x806bfc20` small-data base. Relevant constants in this function are:

| Address | IEEE bits | Value |
| --- | --- | --- |
| `0x806b7d30` | `3f800000` | 1.0 |
| `0x806b7d34` | `00000000` | 0.0 |
| `0x806b7d38` | `36800000` | 32 * FLT_EPSILON |
| `0x806b7d48` | `bf800000` | -1.0 |
| `0x806b7d4c` | `40490fdb` | pi |
| `0x806b7d50` | `447fe000` | 1023.5 |
| `0x806b7d54` | `3fc90fdb` | pi / 2 |

## Required generalized dependencies

`normalize(source)` at `0x80016610`, size `0xd0`, computes the source squared
length, selects identity when the squared length is at most
`32 * FLT_EPSILON`, and otherwise uses one PowerPC reciprocal-square-root
refinement to scale the source. The current host `normalize()` cutoff of
`1e-12` is inaccurate. Its host reciprocal-square-root provider remains a
separate numerical implementation choice.

`JMath::TAsinAcosTable<1024, f32>` construction at `0x80442c54`, size `0xb4`,
computes each entry from double-precision `asin(double(index) * (1.0/1024.0))`
before rounding it to float. The double constant at `0x8055c3a8` is
`3f50000000000000`, exactly `1/1024`. The old host constructor uses
`float(index)/(Len-1)`, which gives different values. The recovered root
`JMATrigonometric.hpp:138` describes its lookup correctly: truncate
`abs(value) * 1023.5f`, then offset the selected asin entry from pi/2. The
constructor also writes table[0] to zero and its extra `_1000` slot to the
float at `0x806c1bc4` (`3f490fdb`, pi/4); ordinary acos indices never address
that slot.

## Implemented source boundary

The recovered slerp routine is now in root
`src/JSystem/JGeometry/TQuat.cpp`, copied verbatim to the corresponding PC
path. The recovered asin table constructor is in root
`src/JSystem/JMath/JMATrigonometricTable.cpp`, also copied verbatim to the PC
path. No percentage match is claimed: instruction/control-flow and literal
constant evidence establish the functional reconstruction, while a matching
Metrowerks compilation was not performed in this tranche.

The host header supplies the corresponding source-facing normalization, dot,
and squared-length methods. It corrects the table lookup and normalization
threshold and uses the existing host libm reciprocal-square-root replacement.
The original paired-single rounding and estimated reciprocal-square-root
instruction remain distinct from this portable arithmetic implementation.

## Comparison-shape review correction

The recovered `blendQuatUpFront` opposition check tests `!(dot >= 0)` before
the cross-direction test. The first source-equivalent patch used `dot < 0`;
these differ on NaN. The follow-up restores the original comparison shape.
This does not affect finite valid orientations.
