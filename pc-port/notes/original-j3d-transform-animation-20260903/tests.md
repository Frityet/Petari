# Native loader and sampler regression coverage

`pc-port/tests/OriginalJ3DTransformAnimationTests.cpp` is registered as
`smg-pc-original-j3d-transform-animation-tests`. The parent task owns native
builds and execution; the test author did not run a separate concurrent build.
Execution results are recorded with the checkpoint's integration evidence.

The seven groups exercise the actual owned `J3DAnmTransformKey`,
`J3DAnmTransformFull`, and `J3DAnmTransformFullWithLerp` objects. Fixtures contain
big-endian J3D files and pass through the production decoder. They do not create
partial Game resource holders or substitute a test sampler.

## Independent expected values

- Keyed metadata, joint indexing, native alignment, and input ownership: an
  unaligned input span is parsed, overwritten, and destroyed before sampling.
  Two joints retain different scale/translation/rotation data. Zero-count
  channels ignore invalid offsets and use original defaults; constant channels
  ignore tangent type.
- Hermite and signed rotation: type-zero scale keys give `2 + frame`. A
  type-one translation has values 10/30, outgoing tangent 2, incoming tangent
  -1, and duration 4; expected frame-one and frame-two values are 14.4375 and
  21.5. Deliberately different unused tangents expose wrong argument ordering.
  Signed rotation Hermite values +1.625/-1.625 truncate toward zero before a
  two-bit shift, producing +4/-4. Raw negative and past-end frames retain the
  original endpoint behavior without wrapping by the resource attribute.
- Key search/type semantics: type 7 follows the original nonzero four-value
  predicate. Initial and interior duplicate times select the last equal key;
  an all-equal track exercises both endpoint guards. Shift bytes 31, 32, 64,
  and 255 exercise the PowerPC word shift and low-halfword storage boundary.
- Full sampling: independent channel lengths prove per-channel endpoint
  selection. Frame 0.5 selects sample one in nearest mode. The three unnamed
  ANF1 header fields at +0xE/+0x10/+0x12 are cleared and then understated as
  one; both Full classes still read the actual tracks. The original full
  loader never reads these fields, so they cannot constrain sample storage.
- FullWithLerp: fractional values differ from nearest sampling, including a
  later segment and short channels. Rotation crosses the signed boundary and
  zero in both directions. Exact half-turn deltas prove the original strict
  `> 0x8000` wrap threshold; the expected midpoint is +16384 in both tested
  directions.
- Renderer ownership/delegation: copies share the typed transform owner after
  inspection and source bytes retire. Every joint is compared to independently
  loaded original sampling at `j3d_animation_frame(...)`. A raw frame-four
  sample returns the final translation 30, while the existing renderer loop
  policy normalizes frame four to zero and returns 10. The last summary copy
  controls destruction of the typed owner.
- Bounds: truncated headers, invalid magic/length/block sizes, missing matching
  blocks, table extents, single/multiple value extents, decreasing/nonfinite key
  times, zero-sample full tracks, and out-of-block full reads are rejected.
  An unknown block before the transform block remains accepted.

Expected sampler behavior is derived from root
`src/JSystem/J3DGraphAnimator/J3DAnimation.cpp`; loader field usage is derived
from `src/JSystem/J3DGraphLoader/J3DAnmLoader.cpp`. The separate `README.md` and
`verify.py` in this directory document original-compiler and retail instruction
evidence, including the signed-16 arithmetic path and explicit native integer
conversions. Comparing renderer output with the original sampler establishes
delegation and frame policy; the independent numeric fixtures establish sampler
behavior rather than using that comparison as its own oracle.

## Optional authored resources

With `SMGPC_REAL_DISC` set to the supplied game image, the same executable opens
the actual `MarioAnime` archive through Aurora DVD and the shared RARC service.
`MarioActorDraw::initDraw` names this archive, and the authored Mario animator
tables reference `wait.bck`, `run.bck`, and `jump.bck`.

For each resource the test checks the stored header, every native float and
signed rotation value, and every key descriptor against the original bytes.
It destroys the DVD service and its cached archive before sampling all joints
at negative, fractional, interior, endpoint, and past-end frames. The copied
renderer summaries must still match original sampling after renderer frame
normalization. Each authored resource must produce a changing finite pose.

The already-loaded Mario archive is also inspected for actual `bca1` files. If
present, both Full owners are sampled after archive retirement and must agree
at integer frames. If none is present, the executable explicitly reports that
authored BCA coverage was skipped. The byte fixtures still exercise the real
BCA decoder/samplers, but do not establish the presence of BCA in this archive.
This bounded scan does not claim to inventory all game archives.

These tests cover the transform resource, storage, and sampling boundary.
They do not claim that the original Xanime player/core, typed model loader,
Game resource-holder lifecycle, or Mario animation state machine is active.
