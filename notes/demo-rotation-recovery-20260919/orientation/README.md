# Shared orientation audit, 2026-09-19

The user narrowed the stationary-facing observation to the starting guide
`DemoRabbit`; Mario's unusual orientations were also seen while walking on
curved ground. This is a source and existing-object audit, not a new gameplay
verification.

## Confirmed defect: native vector equality silently compares addresses

`src/JSystem/JGeometry/TVec.hpp:204` specializes `TVec3<f32>` and supplies
implicit `Vec*`/`const Vec*` conversions at lines 226/230, but omits both
equality operators. A search of native source and the SDK vector headers found
no replacement free equality overload. The built-in pointer comparison is
therefore viable when comparing two vectors.

The canonical `decomp/libs/JSystem/include/JSystem/JGeometry/TVec.hpp:576`
defines `operator==` as three component comparisons, each using inclusive
`[-32*FLT_EPSILON, +32*FLT_EPSILON]` bounds. Its `operator!=` negates this
result. Native `TUtil` already implements those same bounds and epsilon.

The existing retail assembly independently confirms this donor behavior:
`npc-base-matrix-retail.s` shows the three `epsilonEquals` calls at
`0x8029B530`, `0x8029B548` and `0x8029B560`. Once all three succeed,
`0x8029B574` branches over the Euler replacement and cache copy. The constant
at `0x806BF2A8` is `32*FLT_EPSILON`. `retail-equality-proof.json` records the
full assembly source hash and instruction references. The canonical header
already contains the correct implementation; no new decompilation is needed.

The real consumer is `NPCActor::calcAndSetBaseMtx`
(`src/Game/NPC/NPCActor.cpp:470`): it normalizes `_A0`, replaces it with the
Euler-derived quaternion only if cached `_CC` differs from `mRotation`, then
writes `_A0` to the model. These are distinct vector members, so the missing
equality makes that replacement unconditional.

This is proven in the already-built ARM64 object, without compiling or running
anything: `npc-base-matrix-before.txt` contains the complete function. After
normalizing `_A0`, offset `+0x6c` unconditionally calls
`MR::makeQuatRotateDegree`, followed by unconditional `_CC = mRotation`.
There are no component loads/comparisons for the intended cache decision.

`DemoRabbit` inherits that callback. Its unchanged `control()` at line 93
uses `MR::blendQuatUpFront(&_A0, -mGravity, mFrontVec, 0.1f, 0.2f)`; its
nerve handlers update `mFrontVec` toward the player/route. The unconditional
cache replacement discards this gravity/facing work before the model matrix
is written. Restoring the general vector operators preserves the original NPC
behavior without modifying an actor, story state, or stage.

## Bounded checks with no new defect found

The native single-argument quaternion multiply already applies the canonical
world rotation (`rotation * this`). The two-argument multiply stages all
components before writing and supports aliasing. Up/front blending, its
antiparallel perturbations, `makeMtxUpFront` cross-product order, vector turn
signs, and the quaternion-to-base-matrix convention match the inspected donor
operations. Existing historical matrix recovery reports and focused rotation
tests are useful prior evidence, not fresh test results from this audit.

The equality defect also affects `MarioActor::updateForCamera`
(`src/Game/Player/MarioActorCamera.cpp:136`). Its `_300` head-vector cache
compares against `mMario->mHeadVec` before restarting the `_330` interpolation
timer. Address comparison therefore restarts this timer every update even
when the head direction has stopped changing. The same header restoration
allows the original camera-up interpolation to finish. Retail instructions
independently confirm the three epsilon comparisons and branch over the timer
reset; see `mario-camera-update-retail.s` and
`retail-mario-camera-equality-proof.json`. The latter records the bounded
caller-search scope and the exact constant/address evidence.

No corresponding vector comparison was found in the inspected core Mario
walking/basis functions. Do not claim that the camera-cache fix alone explains
every reported Mario mesh rotation. The parent is comparing original Mario
orientation with the final J3D model/joint bases.

## Change and validation scope

The canonical two operators have been restored in the native shared vector
header, with no Game changes. The new `smg-pc-original-npc-orientation-tests`
target has `--vectors-only`, `--owner-only`, and combined default modes. Checks cover
separate equal objects, equal aliases, const combinations, each component,
positive/negative epsilon boundaries, just-outside values, signed zero and
unordered NaNs/infinities. The owner mode exercises the actual NPC base-matrix
cache with unchanged and changed Euler rotation while a non-Euler control
quaternion is present, restores all injected fields, and requires subsequent
ordinary frames and actual actor retirement. A clean
original-process replay must then show the guide bunny's rendered basis
following its original quaternion/front direction; compilation or a pure
vector test alone is insufficient for that visual claim.

The parent captured RED against the unchanged header. The pure-vector mode
exited 1 at the component equality assertion. The first owner RED aborted
without an assertion message; it is retained as an inconclusive failed run.
After adding a flushed failure diagnostic (test only), the second owner RED
printed `original NPC callback retains the control quaternion's facing axis`
and aborted with command exit 134 during exception unwind. This confirms the
expected wrong-axis assertion, but is not a cleanly handled test failure or
successful process completion. The initial logs and parent JSON records remain
under the enclosing rotation-recovery notes directory.

The first GREEN built successfully and passed vector boundaries, all three
actual DemoRabbit cache/model probes at frame 38, 120 completed original
frames, and actor retirement. It then aborted during test destruction: the
probe's retained pointer vector had been allocated inside the scene heap and
outlived that heap. The fixture now collects retained observations under the
host allocation scope before entering the original guest scope, and its
exceptions use the host-owned exception helper. This corrects test lifetime
ownership; it does not suppress an abort or change production behavior.
Final GREEN passes: NPC vector/owner checks completed 120 frames and normal
retirement with exit 0 (3.120 seconds). Its actual three-NPC cache/model checks
ran at frame 38. The separately owned player target also passed the original
stable-head camera timer expiry and changed-head reset (retail table value 16),
360 completed frames and normal retirement with exit 0 (7.027 seconds).
`validation.json` records the parent results and exact binary/source hashes.
The same initial batch's existing math-rotation,
Xanime-core and J3D-joint-traversal suites passed with exit 0.

`npc-base-matrix-after.txt` records the parent's newly built production object:
it now loads and compares all three cache components, then skips the Euler
replacement when equal. `npc-object-after.json` records its hash.

## Original guide replay observations

The parent ran the ordinary application with authored placements and external
controller input. `analyze_npc_trace.py` reads its actor trace without changing
the game. Each analysis records the exact complete-line byte prefix read and
its SHA-256; these prefixes remain verifiable after the live file grows.

In the baseline, guide NPC 874 retained exactly one quaternion over 70 visible
samples (frames 2000–2690) even though its original nerve advanced through
Guide, Goal, Talk1, Runaway and Change. Its model-up dot normalized negative
gravity ranged from -0.922874 to -0.471934. Both companion DemoRabbits also
retained one quaternion each. See `baseline-npc-trace.json`.

After restoration, guide 874 retains one placement Euler rotation but has 233
different control quaternions over 282 visible samples (frames 2000–4810).
Its model-up alignment stays between 0.998227 and 1.0 throughout 81 Guide,
21 Goal, 101 Talk1 and the subsequent Runaway/Change samples, as well as the
earlier Talk0/Wait states. The two companions also rotate, with minimum
up alignment 0.998215. All three reach their original Change/dead state after
this observed interval. See `after-guide-complete-npc-trace.json`. The lower
instantaneous front alignment during turns is consistent with the original
finite-rate front smoothing; it is recorded, not hidden or clamped.

The after frame-1800 screenshot was visually inspected: the guide faces Mario
upright in the initial conversation. It is a different conversation and frame
from the baseline frame-2300 screenshot, so these images are not a pixel-parity
comparison. The live run was still in progress when the bounded guide analysis
was recorded; its eventual completion and Mario's separate visual assessment
belong to the parent's final run record.
