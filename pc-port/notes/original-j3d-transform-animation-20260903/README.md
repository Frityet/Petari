# Original J3D transform-animation sampling

This tranche supplies the original `J3DAnmTransformKey`, `J3DAnmTransformFull`,
and `J3DAnmTransformFullWithLerp` classes for native BCK/BCA resources. It does
not alter Game animation algorithms or apply playback wrapping inside samplers.
The parent owns the typed endian loader, resource lifetime, renderer integration,
and native build; the gateway agent owns focused loader/sampler tests.

## Source changes

- `libs/JSystem/include/JSystem/J3DGraphAnimator/J3DAnimation.hpp` supplies the
  imported Key/Full table definitions and all three class declarations. Native
  fields, virtual methods, constructors, kind values, and data pointers preserve
  the original surface. `<cstddef>` makes the original `NULL` spelling portable.
- `src/JSystem/J3DGraphAnimator/J3DAnimation.cpp` gains the previously missing
  empty `J3DAnmTransformFull` destructor and the scalar `#else` for its signed-16
  Hermite helper. The original compiler branch is untouched. The portable math
  was added here first, then copied into the PC provider.
- `pc-port/src/compat/J3DTransformAnimationCompat.cpp` copies the original
  transform constructor, destructor, three sampler bodies, binary key search,
  and Hermite dispatch. Native integer operations are the only mechanical
  substitutions inside the sampler bodies. The source verifier applies those
  substitutions explicitly and compares the resulting token sequences.

The original f32 Hermite overload already delegates to the shared
`JMAHermiteInterpolation`. The signed-16 overload has a different floating-point
operation sequence, so it retains its own scalar implementation. Both use
explicit `std::fma` exactly where the retail code fuses operations; the negated
multiply/subtract preserves its final negation.

## Behavior retained

Key animation reads `mFrame` directly through `getTransform`, or the explicit
frame passed to `calcTransform`. Zero-count tracks give scale one and rotation /
translation zero. Single-count tracks read one value. Multi-key tracks clamp to
the first/last key and use the original binary search and Hermite interpolation
inside the interval. Type zero has one shared tangent; any nonzero type has
separate incoming/outgoing tangents. No new clamp is applied to `mDecShift`.

Full animation selects the first sample for negative frames; otherwise it
truncates `mFrame + 0.5f` and clamps each component to that component's last
sample. FullWithLerp retains the separate integer-frame and fractional-frame
branches. Each component clamps independently. Fractional rotation operates on
unsigned 16-bit angles and only adjusts deltas strictly larger than `0x8000`,
then stores the low halfword. Neither sampler uses the resource's frame maximum
or loop attribute to wrap its input.

## Retail arithmetic evidence

All addresses below refer to supplied RMGK01 `main.dol`, SHA-1
`25c5959534b3c21246c6c7e42021b916b41fb578`.

| Method | Address | Bytes | Original compiler objdiff |
| --- | --- | ---: | ---: |
| Full `getTransform` | `0x8043436C` | `0x360` | 100% |
| FullWithLerp `getTransform` | `0x804346CC` | `0x824` | 100% |
| Key `calcTransform` | `0x80434EF0` | `0x428` | 100% |
| Full destructor | `0x80437524` | `0x40` | 100% |
| Signed-16 key search | `0x80435318` | `0x1D0` | 99.086205% |
| Signed-16 Hermite | `0x804354E8` | `0x54` | 89.285710% |
| f32 key search | `0x8043553C` | `0x120` | 86.166664% |

The three sampler bodies and the recovered destructor compile to matching
instructions with configured GC/3.0a3 JSystem flags. This is a comparison of the
root PowerPC code, not a claim that native machine code matches PowerPC code.
The existing key helpers remain nonmatching: the signed-16 search is four bytes
shorter; the f32 search is eight bytes shorter and inlines Hermite differently.
The signed-16 Hermite allocation/scheduling differs without changing its
arithmetic dataflow. No matching-only changes were made to those helpers.

`verify.py` independently decodes the actual retail scalar floating-point and
PSQ instructions and compares the resulting operation graph with the portable
signed-16 body. All 20 load/arithmetic operations produce the same graph. It
also compares the retail f32 Hermite at `0x8043565C` with the already shared
portable JMA implementation; that graph matches too. This checks fused versus
separate operations and negations, rather than only a rearranged polynomial.

The same verifier checks these native adaptation boundaries at exact addresses:

- `0x8043509C` / `0x804350A0` / `0x804350A4`: signed load, `slw`, halfword store
  for the constant X rotation. `0x804350C8` / `0x804350D8` / `0x804350DC` add the
  truncating `fctiwz` before shifting an interpolated value. Y/Z repeat these
  sequences. Unsigned native word shifts avoid negative-signed-shift undefined
  behavior. As on PowerPC, shift bit 5 gives zero and bits 0–4 select the shift;
  therefore 32 gives zero and 64 acts as zero. `std::bit_cast<s16>` preserves
  the stored low 16 bits without implementation-defined signed narrowing.
- `0x80434454` / `0x80434458`: separate add of the actual SDA2 constant `0.5f`,
  then `fctiwz`, for Full sampling. FullWithLerp starts with `fctiwz` at
  `0x804347D8` and word addition of one at `0x80434A70`. Native conversion
  preserves truncation, positive/negative saturation, and the NaN integer result;
  it does not emulate FPSCR exception flags. The local Dolphin implementation
  in `Interpreter_FloatingPoint.cpp::ConvertToInteger` documents the same
  PowerPC conversion behavior, and `Interpreter_Integer.cpp::slwx` implements
  the same shift-count semantics.
- `0x80434ACC` / `0x80434AD0` / `0x80434AD4`: separate subtract, multiply, add
  for BCA scale interpolation. Rotation likewise uses separate multiply/add
  at `0x80434B84` / `0x80434B88`. Provider-local compiler pragmas disable implicit
  contraction, matching the original JSystem `-fp_contract off` flag.
- `J3DSys::drawInit` at `0x80422F30` writes `0x00070007` to GQR5. The signed-16
  Hermite PSQ loads select that register, one element, and offset zero. Plain
  signed-16-to-float conversion therefore preserves the unscaled input exactly.
- The Full destructor only conditionally calls `__dl__FPv` for a deleting
  destructor invocation. It does not own or free the animation data tables;
  the native typed resource owner retains that backing storage.

## Reproduce

From the repository root, with the supplied DOL already extracted to ignored
`build/compat-math-oracle/main.dol` and the pinned original compiler/tool setup:

```sh
python3 pc-port/notes/original-j3d-transform-animation-20260903/verify.py
```

The script compiles only the original PowerPC translation unit. It uses DTK to
split the verified DOL into ignored build files when necessary, runs objdiff,
checks source correspondence and arithmetic graphs, and writes
`build/original-j3d-transform-animation-20260903/evidence.json`. An existing DTK
split can be reused with `--target-object path/to/J3DAnimation.o`.

The checked-in `evidence.json` captures that successful verification. Extracted
binary code, original compiler objects, and the complete objdiff are kept only
under ignored `build/`. Native regression/build results belong to the parent
checkpoint; this note does not claim renderer or gameplay validation by itself.
