# Original J3D transform resources in the PC renderer

The PC resource boundary now loads BCK/ANK1 and BCA/ANF1 files into actual
`J3DAnmTransformKey`, `J3DAnmTransformFull` or
`J3DAnmTransformFullWithLerp` objects. Native derived owners retain the decoded
axis tables and float/signed-16 value arrays. The original public pointers
refer to that owned storage; the input archive may be destroyed after loading.
No serialized Wii object or raw archive entry is cast to a native class.

`resource::load_j3d_transform_animation` follows the original file-type and
block dispatch, skips unknown blocks and uses the last matching transform
block. It preserves attributes, signed frame maximum, key rotation shift and
joint count. Full interpolation selects the same original class as the
original loader flag. It bounds each block and array to the declared file and
block sizes, validates referenced channel extents, and rejects decreasing or
nonfinite active key times. Repeated key times remain valid under the original
upper-bound search. Every nonzero tangent type uses four-element records;
zero-key channels ignore their offsets and one-key channels read one value.
Full channels must contain a sample because the original sampler always reads
their first or last entry. ANF1 pool extents come from the channel descriptors:
the original Full loader ignores header bytes `0x0e..0x13`, so their contents
do not restrict a fully bounded Full animation. Invalid resources raise an error rather than
creating a partially initialized object.

`render::inspect_j3d_animation` retains a shared immutable original Key object
for every BCK resource. `j3d_evaluate_bck_joint_transform` now delegates to its
`calcTransform` instead of the renderer's independent Hermite implementation.
The actor's existing playback frame is still normalized through the existing
frame-controller boundary before this call. The original virtual
`getTransform` and `calcTransform` themselves use their supplied frame directly,
including before zero and at or beyond the end; they do not add a second loop.
Separate actors can share the same object without mutating its stored frame.
The summary's value arrays remain available to the resource inspection tools.

This changes skeletal animation for every BCK-backed live actor, including
Mario's current Wait/Run path. Material animation and the complete original
Xanime player/core lifecycle are separate remaining work. No Game code was
modified for this runtime integration. Root Xanime lifecycle recovery is
recorded in `../xanime-core-lifecycle-restoration-20260903/`; it is not activated
in the PC animator by this checkpoint.

## Validation

The macOS arm64 game archive, live walking target, spin checkpoint and showcase
build passed. The dedicated transform test target passed all seven groups,
including the ANF1 metadata regression added after review. With the real RMGK01
disc it loaded Wait (30 joints, 180 frames), Run (30 joints, 60 frames) and Jump
(30 joints, 24 frames), checked every native value and key descriptor, retired
the source archive, and sampled every joint at fractional and endpoint frames.
Renderer summaries retained their shared owners and exactly matched original
sampling after the existing playback normalization. No authored BCA exists in
the inspected MarioAnime archive; BCA coverage is from the explicit byte
fixtures, not a claim to have located or played an authored BCA.

The real-disc Gateway walking proof passed with 14,521 KCL triangles, initial
floor prism 4642, Wait -> Run -> Wait, movement distance 325.684 and release
frame 297. Its lifetime/recreation checks also passed. The new 1280x960 walking
capture was inspected: Mario, the curved Gateway surface and the existing
original camera pipeline render correctly for the captured frame. This is a
walking frame, not evidence of gameplay jumping. The authored spin-unlock
checkpoint also passed. Logs and screenshot remain ignored in this directory.

The original PowerPC compiler comparison verifies all three sampler bodies and
the recovered Full destructor at 100%. Independent retail operation-graph
checks verify the signed-16 and float Hermite arithmetic, including explicit
fused versus separate operations; see README.md, evidence.json and verify.py.
The existing original resource-table source verifier passed after being updated
to permit the expanded sampler provider while still checking its unchanged
constructor. The Xanime root lifecycle checkpoint is separately committed as
`85a73fd53`; it does not activate the full PC animator.

Remaining movement work includes the typed original model/joint traversal,
Game ResourceHolder ownership migration, original Xanime pose blending, and
the original Mario jump/landing/action dependencies. The complete Gateway
bunny chase through Rosalina is not yet verified.

## Reproduce native validation

From `pc-port`, with the supplied RMGK01 disc at the repository root:

```sh
PATH=/opt/homebrew/opt/llvm/bin:$PATH xmake build -j8 smg-pc-original-j3d-transform-animation-tests smg-pc-mario-gateway-walk-tests smg-pc-gateway-spin-checkpoint-tests smg-pc-showcase
SMGPC_REAL_DISC='../Super Mario Wii - Galaxy Adventure (Korea).rvz' build/macosx/arm64/debug/smg-pc-original-j3d-transform-animation-tests
SMGPC_REAL_DISC='../Super Mario Wii - Galaxy Adventure (Korea).rvz' build/macosx/arm64/debug/smg-pc-mario-gateway-walk-tests
SMGPC_REAL_DISC='../Super Mario Wii - Galaxy Adventure (Korea).rvz' build/macosx/arm64/debug/smg-pc-gateway-spin-checkpoint-tests
```
