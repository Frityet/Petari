# Original actor and matrix camera targets

`pc-port/src/Game/Camera/CameraTargetMtx.cpp` and `.hpp` are exact root
source/header copies, replacing the previous native approximation. The class
now derives from the actual `CameraTargetObj` and uses its original `TPos3f`
matrix, virtual target interface, CubeCamera area, and gravity state. Root
`configure.py:1124` marks this translation unit Matching; no new binary
matching was performed for this import.

`pc-port/src/compat/OriginalCameraTargetActor.cpp` contains all eleven original
`CameraTargetActor` method definitions from
`src/Game/Camera/CameraTargetObj.cpp:26-105`, unchanged. It excludes the base
constructor and player methods, which are already supplied by
`CameraLocalUtilRuntime.cpp` and `OriginalCameraTargetPlayer.cpp` respectively.
The existing PC `CameraTargetObj.hpp` already has the original actor target
declarations. Root marks the combined target-object translation unit
NonMatching; extracting its existing bodies does not change that status.

## Original behavior retained

The matrix target copies raw matrix Y/Z/X columns to up/front/side on each
movement, computes translation delta, and consumes `invalidateLastMove` for
one zero-delta movement. It looks up CubeCamera at the resulting position and
queries the real scene gravity manager. The original constructor's side is
`(0,0,1)` until the first movement, even though identity-matrix side then
becomes `(1,0,0)`. This source constant was preserved. Ground position is the
matrix target position. It no longer manufactures world-down gravity every
frame.

The actor target returns early when its actor is dead or clipped, preserving
cached basis and CubeCamera area. Otherwise it reads the actual actor's base
matrix axes when present, or uses the original Euler rotation matrix helper
when the actor exposes no base matrix. Its position and ground position
getters return the actor's current position; its last-move getter returns
current actor velocity. `getGravityVector` returns virtual `getGravityInfo`'s
vector when available, or cached up when absent. It does not negate cached
up or substitute `LiveActor::mGravity`. `getGroundTriangle` returns null as
in the original body.

The three `MR::calcFrontVec`, `calcUpVec`, and `calcSideVec` definitions from
root `ActorMovementUtil.cpp:179-192` were copied unchanged into the new compat
translation unit. The PC Game file contains those bodies but is excluded by
`Game/xmake.lua`; no compiled definitions existed. These helpers call the
actor's virtual `getBaseMtx` and copy the raw column without normalization or
fallback. Target lifecycle, ownership, direct event-camera binding, and
regression results are described in `integration.md`.

## Root rotation correction backed by retail instructions

Inspection of `MR::makeMtxRotate(MtxPtr,s16,s16,s16)` found one wrong root
source expression. At `mtx[0][2]`, the old root body added
`cosX*(sinZ*sinY) + sinX*cosZ`. The verified RMGK01 DOL instead computes
`cosX*(cosZ*sinY) + sinX*sinZ`. Existing native MtxCompat already used the
correct expression. The root was corrected first by adding the two named
`f32` intermediates and fixing this single output; the corrected signed-angle
function was then copied verbatim into MtxCompat. Other matrix providers
were left unchanged. This does not change the native rotation formula.

Evidence is the retail function at `0x803EB2C4`, size `0xBC`, from the DOL
whose SHA1 is `25c5959534b3c21246c6c7e42021b916b41fb578`:

| Instruction | Contribution to `mtx[0][2]` |
| --- | --- |
| `0x803EB2F8: fmuls f2,f12,f0` | `cosZ*sinY` |
| `0x803EB30C: fmuls f1,f8,f9` | `sinZ*sinX` |
| `0x803EB32C: fmuls f2,f10,f2` | `cosX*(cosZ*sinY)` |
| `0x803EB33C: fadds f4,f2,f1` | Adds these terms with single-precision rounding. |
| `0x803EB360: stfs f4,8(r3)` | Writes row 0, column 2. |

The disassembly uses separate scalar multiply/add instructions; no new
inline assembly was added. The shared signed-angle implementation continues
to use the original `JMASSin`/`JMASCos` table and explicit `f32` intermediates.
`AGENT_DECOMP_GUIDE.md` was read before the root correction. Original-compiler
validation is recorded below; no new object-diff match percentage is claimed.
Raw disassembly and slices remain in ignored
`build/original-camera-targets-20260903/`.

The floating overload at `0x803EB380`, size `0x60`, was also inspected. It
multiplies each degree value by the original `DEGREE_TO_S16`, uses `fctiwz`
to produce a signed 32-bit integer, then sign-extends the low 16 bits before
calling the signed overload. The existing direct native floating-to-`s16`
cast has an out-of-range C++ conversion boundary for larger degree values;
this remains unchanged and was reported separately from the source imports.

## Original compiler validation

After the import was frozen, the parent requested an original-compiler check.
The entire root `src/Game/Util/MtxUtil.cpp` compiled successfully with no
diagnostics using `build/compilers/GC/3.0a3/mwcceppc.exe` through the existing
macOS wibo. The reproducible verifier reads the original `cflags_game` list
from `configure.py`, with `RMGK01` and `VERSION=0`, and records the exact
compiler command in the ignored build directory. No native xmake build was
run by this agent.

The corrected signed-angle function emits `0xD4` bytes / 53 instructions,
compared with retail's `0xBC` bytes / 47 instructions. It is not byte-identical.
The compiler keeps `f31` live, adding six instructions for stack allocation,
double/paired-single preservation, and restoration. Register allocation and
instruction scheduling also differ. No source changes were made to chase
these code-generation differences.

`verify-rotation-object.py` applies all three relocations in this function:
the sine/cosine table's high-adjusted and low halves resolve to `0x8060FC80`;
the compiler's local positive-zero constant resolves to retail `0x806C17E4`.
The latter is checked against both actual object bytes and DOL bytes, using
the `r2 = 0x806BFC20` base verified from retail startup instructions at
`0x80004224`. It then decodes each straight-line instruction in both versions,
tracks table indices and loaded sin/cos inputs, and compares all twelve
matrix output expression trees. All twelve agree, including the corrected
row-0/column-2 value. Each single-precision arithmetic operation remains a
separate tree node; only the two operands of an individual finite multiply
or add are allowed to commute, never the operation grouping. The operation
counts also agree after excluding the six exact stack/FPR preservation
instructions. This is relocation-aware functional compiler evidence, not
an instruction-match percentage or a native runtime test.

```sh
python3 pc-port/notes/original-camera-targets-20260903/verify-rotation-object.py --compile
```

`rotation-compiler-evidence.json` retains the compiler and object hashes,
sizes, relocation targets, instruction counts, and the twelve output trees.
The full original object, compiler output, and relocated disassembly are
under ignored `build/original-camera-targets-20260903/`.

## Evidence and validation

`source-correspondence.json` records two whole-file hashes, fifteen exact
function-body hashes, the prior root rotation body hash, and the retail
rotation instruction-range hashes. `verify-source.py` checks all source
correspondence and, when the ignored DOL exists, rechecks its identity and
both recorded instruction slices. It does not depend on a compiler.

The source/hash, whitespace, and original-compiler checks pass. Native build
and runtime results are recorded in `integration.md`.
Actual matrix target movement requires a scene gravity owner even when no
gravity field covers the position; absence of that service is visible
through the existing provider rather than silently creating a gravity value.
