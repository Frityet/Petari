# Original XanimeCore matrix calculation on PC

Checkpoint based on `d403870037e46e26d764cb9f31ab459548803305`, on macOS arm64.
Root recovery is committed as `734a1bdd7`; shared Aurora math as `deab54e`.
The native renderer now uses the actual game's `XanimeCore` for sampled BCK
joint animation. Its original single-track calculation, Euler/quaternion
conversion, scale-mode dispatch, and recursive J3D traversal produce the
matrices used by rendered actors and named-joint queries. This is a general
animation change; no actor, stage, joint-name, or pose-specific exception was
added.

## Original source and SDK surface

Seven missing matrix methods were recovered in root `src/Game/Animation` before
importing the entire `XanimeCore.cpp` and header byte-for-byte into the PC Game
tree. `_68` in `XjointTransform` is an actual matrix pointer, confirmed by its
retail uses as a concat/inverse operand. Its corrected type preserves the Wii
ABI while retaining a complete pointer on the native ABI.

The original compiler proves exact instructions after verified relocations for
Maya, Softimage, special, Maya without transforms, `calc`, and `init`. Basic
has a lower raw match (84.14%) because of address formation/register scheduling;
its arithmetic operands, helper calls, and branch topology agree. See
`entrypoints-README.md` and `scale-README.md` for the addresses, source hashes,
compiler evidence and limits. Neither the low percentage nor native numeric
limits are hidden behind a claim of perfect matching.

The required native SDK services now include the genuine J3DSys global and
constructor with its four original GPU table builders; full typed ModelData,
MaterialTable, ShapeTable and VertexData construction; original Euler-to-quat
and shortest-hemisphere quaternion lerp; and original matrix translation/vector
blend helpers. VertexData construction and the missing vector translation
wrapper were recovered root-first. The two existing paired-single assembly
helpers received guarded portable branches; their Metrowerks code is unchanged.

Aurora now exposes a real `PSMTXQuat` entry point, with the console operation
order and explicit fused arithmetic. `C_MTXQuat` remains the distinct C SDK
function. The measured Gekko reciprocal estimate moved from a Game-side JMath
provider into shared C-compatible `ppc_fres`; JMath calls that shared service.
This avoids making Aurora depend on Game/JMath. See `math-README.md` for direct
production-provider comparisons with the retail instructions and exceptional
NaN/FPSCR limits.

## Native ownership and integration

`OriginalJ3dJointTree` retains an actual non-sharing XanimeCore plus explicit
owners for the joint and track arrays allocated by its original constructor.
The original destructor stays empty, preserving the game's shared-core
contract. The native owner never enables or borrows a transform array.

Each animated calculation makes a real SDK Key animation object with its own
frame, borrowing immutable decoded tables only for the synchronous call. The
original sampler's virtual `getTransform` is invoked by the core. The caller's
resource/frame remain unchanged; the borrowed track is cleared before the
local playback object retires. No cast to a fabricated model or resource holder
is used. The owner restores the new `j3dSys.mCurrentMtxCalc` field along with the
existing traversal globals on normal completion, nested calls and exceptions.

Unanimated bind-pose calculation retains the original J3D NoAnm calculator.
The old custom sampling calculator was removed. In particular, real XanimeCore
Softimage calculation differs from J3D NoAnm Softimage: it uses the Maya
initializer, scales the local matrix and translation before concatenation, and
then scales the stored output by accumulated scale. The integration regression
now has independent expected results for these actual original paths; neither
implementation is altered to make them equal.

The recovered Game core is built with scalar contraction disabled to match the
original compiler; explicitly paired fused operations remain in the shared SDK
providers. No PC-specific behavior was inserted into its Game source.

## Validation and remaining boundary

`native-evidence.json` records the macOS build/runs and the real-disc walk
result. All nine new XanimeCore system groups pass: actual typed construction,
frame/freeze/cache behavior, both Key and Full samplers, multi-track blending,
all scale modes, recursive compensation, optional matrices and exact vector
blend rounding witnesses. See `native-tests-README.md` for their boundaries.
Existing checks cover original traversal and callback ownership, shared
input, all five camera groups (59 cases, including both real-disc probes), the
original camera runtime, FixedPosition, NPC, live actor/joint/GPU integration,
Gateway stand/walk/release and spin entitlement. The walk still travels 325.684
units across the 14,521-triangle collision setup and renders Wait/Run/Wait with
12 Run packets. Its authored camera follows the player; its raw orbit/FOV are
retained. The captured Run pose was visually inspected.

One first-run compound SDL swing assertion failed after earlier movement,
release and camera checks passed. The rerun with test-only failure diagnostics
passed without a production change. The first failure did not report which
component failed, so its exact cause is not established. The failed log hash
and successful rerun remain in evidence rather than being replaced by a
clean-only record.

This activates the original Core's ordinary single-track rendering path. It
does not yet activate the full XanimePlayer action/transition lifecycle, mode-3
bind-translation correction through a real J3DModel, or a complete typed BMD
loader. ModelData is genuinely constructible, but the renderer does not publish
its partial decoded data as a complete typed ModelData. `model-closure.md`
records the actual missing Model virtual/material/shape/deformation providers.
Original Mario jump/action sequencing and the full bunny/Rosalina demo remain
unfinished. The currently active camera is still the recovered game camera;
this checkpoint does not replace it or claim the complete CameraDirector.

Source import checks: `python3 pc-port/notes/xanime-core-matrix-calculation-20260903/verify-integration.py`.
Compiler/source and numerical reproductions are described in the adjacent
entrypoint, scale, model, sys, helper and math notes. Logs, screenshots, tool
objects and disc data are local evidence and are not committed.
