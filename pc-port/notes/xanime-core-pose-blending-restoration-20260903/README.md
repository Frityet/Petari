# Original XanimeCore pose blending recovery

Three missing methods were reconstructed directly from the verified RMGK01
retail executable into root `src/Game/Animation/XanimeCore.cpp`, with declarations
in root `include/Game/Animation/XanimeCore.hpp`. No fields or class layouts changed.
The starting lifecycle checkpoint is
`85a73fd53036aca5cabd8f538bb21de87ff7272e`. No historical blend body was assumed to
be correct or used as the binary oracle.

| Method | Retail address | Retail bytes | Compiled bytes | Live objdiff |
| --- | --- | ---: | ---: | ---: |
| `calcBlend(TVec3f*, TVec3f*)` | `0x800194AC` | 1044 | 1064 | 90.325670% |
| `calcSingle(TVec3f*, TVec3f*)` | `0x800198C0` | 460 | 460 | 99.434784% |
| `calcBlendSpecial()` | `0x80019A8C` | 1052 | 1080 | 94.882126% |

The original GC/3.0a3 compiler used the production `configure.py` `cflags_game`,
RMGK01 `VERSION=0`, and sjiswrap 1.2.2. Production include paths were used, with
no generated header overlays or fake class declarations. dtk 1.8.3 extracted the
retail object; objdiff-cli 3.6.1 compared relocations and instructions. The input
`build/compat-math-oracle/main.dol` SHA-1 is
`25c5959534b3c21246c6c7e42021b916b41fb578`.

## Recovered behavior

`calcSingle` obtains the current joint from `J3DMtxCalc::getJoint()` and its
animation matrix from `getMtxBuffer()->getAnmMtx(jointIndex)`. It samples track zero
through the actual virtual `J3DAnmTransform::getTransform` and converts signed
16-bit Euler components using `JMAEulerToQuat`. It does not inspect track weight,
`mTrackCount`, `_20`, or the per-joint `_5C` progress. A null track restores cached
scale/translation and converts the cached quaternion into the current matrix,
without updating freeze or translation-cache state (`0x800198DC–0x80019950`).

`calcBlend` sums weights of all tracks having nonnull animations, including zero
and negative weights. A zero total restores the same previous-pose fallback
(`0x8001953C–0x800195BC`). A nonzero total is inverted once. Each nonnull,
nonzero-weight track is sampled in index order; its scale and translation are
multiplied by `inverseTotal * trackWeight` and added to the running vectors. The
quaternion accumulator starts at identity; each iteration adds the normalized
weight to the cumulative weight before calling
`JMAQuatLerp(accumulator, sample, weight / cumulative, accumulator)`
(`0x800195C0–0x80019688`). There is no positive-weight filter, division guard,
epsilon substitution, sorting, or extra normalization in this method.

Both methods consume the `_29` freeze pulse by copying the previous computed
pose (`joint._28`) into the frozen pose (`joint._0`). They then store the sampled,
unblended translation in `joint._50` **before** applying crossfades. When `_1C < 1`,
the frozen pose is blended into the sample. Single uses the actual
`MR::vecBlend` calls; normal blend uses the original vector multiplication,
addition, and quaternion helper sequence. Normal blend additionally applies
`_20 != 1` against the previous computed pose, after the frozen-pose blend. Both
save the result in `joint._28` and call `PSMTXQuat` for the current matrix. These
methods write rotation there; full scaling and translation remain the caller's
responsibility.

`calcBlendSpecial` performs the same weighted sampling and cache updates, but
returns immediately on a zero total and never obtains or writes an animation
matrix. After caching the unblended translation, it starts with `_1C`, then checks
the per-joint `_5C`. If `_5C != 1`, it adds `_60`, clamps the result to `[0, 1]`
with the existing `MR::clamp`, stores it, and uses that updated progress as the
frozen-pose blend rate (`0x80019C58–0x80019CC0`). If progress reaches exactly one
on this invocation, this invocation uses one; only later invocations fall back
to `_1C`. The `_20` previous-pose blend then runs just as in normal blend. It saves
the result in `joint._28` for the subsequent joint pipeline to consume.

The original ordered/unordered comparisons are preserved. A NaN `_1C` does not
enter the `< 1` crossfade; a NaN `_20` enters the `!= 1` stage. The progress clamp
uses the original less-than/greater-than ordering and leaves a NaN progress NaN.
No input sanitization or fallback behavior was added to these Game methods.

## Arithmetic and compiler evidence

`verify-object.py` recompiles the actual root translation unit and a layout probe,
extracts the current verified DOL, and checks:

- Exact constant values and references to current joint/matrix-buffer globals.
- Original direct-call order, apart from the explicitly checked copy/zero
  adaptations below; one virtual sampler call per sampled-track loop remains.
- 171 identical normalized arithmetic/call/control events across the three
  methods (70, 25, and 76 respectively), retaining comparison opcodes and branch
  destinations. Only register names, addresses, and the checked adaptation calls
  are normalized away. This is a topology check, not a claim of formal equivalence.
- Original 32-bit sizes and accessed offsets, including `XanimeCore` 0x2C,
  `XanimeTrack` 0x10, `XjointInfo` 0x64, `XtransformInfo` 0x28,
  `J3DTransformInfo` 0x20, and `J3DMtxBuffer::mpAnmMtx` at 0xC.

Manual operand review confirms the sampled Euler components use signed `lha`,
the weight arithmetic uses `fdivs`/`fmuls`/`fadds` in the original order, and the
crossfades evaluate the new-pose product before the frozen/previous-pose product,
then add the products through the same `TVec3f` helpers. Each crossfade has separate
single-precision subtraction for `1 - rate`; the code does not replace this with
an algebraically rearranged interpolation. Freeze-source and destination pointers,
translation-cache timing, and final quaternion/matrix destinations were checked
against the retail instruction ranges above.

The SDK declares the sample's scale/translation as `Vec`, while retail calls
`TVec3f` members directly on the same field addresses. To avoid downcasting actual
base `Vec` objects into nonexistent derived objects, the reconstructed multi-track
methods construct typed `TVec3f` copies before applying the original multiplication
helpers. These are two additional copy calls per sampled-track loop. The emitted
constructor is exactly a paired x/y load/store plus a z load/store and return.
Single uses `TVec3f::set(const Vec&)` instead of the retail `TVec3f` assignment;
its emitted helper is three loads and three stores. This follows the same typed
boundary as the preceding `initT`/`fixT` recovery.

The compiler outlines `pScale->zero()` and `pTranslation->zero()` in normal blend;
retail inlines the same zero stores. The verifier checks the helper's zero constant
and stores at offsets 8, 4, and 0. Remaining fuzzy discrepancies are stack/register
allocation, member-address formation, those typed copies, and this outlining.
The only compile warning is the existing `J3DTransformInfo` union-member warning
from the preceding lifecycle header; there are no compile errors. The source also
passes the repository clang-format check.

## Reproduction and boundary

Run from the repository root:

```sh
python3 pc-port/notes/xanime-core-pose-blending-restoration-20260903/verify-object.py
python3 pc-port/notes/xanime-core-pose-blending-restoration-20260903/verify-source.py
```

The first command requires the verified extracted DOL and previously installed
original compiler/tools. It writes objects, assembly, the full comparison, and
logs under `build/xanime-core-pose-blending-restoration-20260903/`. The committed
`compiler-evidence.json` and `original-compiler.log` record this verification run;
`source-correspondence.json` records root source/body hashes and retail hashes.
The second command checks that this checkpoint's source still matches that record.

This tranche does not import these bodies into PC production, implement the
remaining `XanimeCore::calc`/`init` joint pipeline, construct native ownership, or
run native animation. The methods need an actual `J3DMtxCalc` current joint and,
for normal/single mode, a real matrix buffer. Tracks need retained, constructed
`J3DAnmTransform` instances with their frame set through the original lifecycle.
Math dependencies are the existing original `JMAEulerToQuat`/`JMAQuatLerp` in
`src/JSystem/JMath/JMath.cpp`, `MR::vecBlend` in
`src/Game/Util/MathUtil.cpp`, SDK `PSMTXQuat`, and existing `TVec3f` operations.
No synthetic sampler, pose, owner, or new animation state was added.

Once the original joint pipeline is active, useful runtime coverage is to sample
retail Mario BCKs through actual constructed animation resources and check ordered
two-track weights, cancellation to zero, `_29` freeze capture, unblended `_50`,
`_1C` followed by `_20`, and special progress reaching one. A single-track case
with zero weight should still sample its animation; a special zero-total case
should leave the stored pose and matrix untouched. These runtime checks were not
run in this root-only recovery.
