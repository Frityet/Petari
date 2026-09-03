# Original XanimeCore system tests

`pc-port/tests/OriginalXanimeCoreTests.cpp` and target
`smg-pc-original-xanime-core-tests` exercise the original source methods through
real constructed objects. The fixtures use actual `J3DModelData`, its embedded
joint tree, actual `J3DJoint` and `J3DMtxBuffer` objects, and actual
`J3DAnmTransformKey` / `J3DAnmTransformFull` objects with owned typed descriptor
and sample arrays. There are no virtual sampler substitutes or fabricated
model objects. Constant channels, a two-key linear Hermite channel, and a
three-frame Full channel provide independently known inputs.

The nine groups cover:

- Genuine static `j3dSys` construction and nontrivial final table entries.
- Core/track defaults, bind-pose initialization, transform-parent discovery,
  reconfiguration, shared construction, and recursive freeze copying.
- Original normalized frame publication, Key interpolation, Full nearest-frame
  sampling at the half-frame boundary, single-track weight/smoothing behavior,
  freeze request/copy/progress, and missing-resource pose retention.
- Two real animation types blended together, the unnormalized cached
  quaternion versus its normalized matrix result, per-track timing,
  multi-track temporal smoothing, and signed weights whose sum is zero.
- The two exact `MR::PSvecBlend` rounding witnesses supplied by the independent
  retail audit, including output aliasing either input. Expected results are
  fixed bit patterns, not a duplicated interpolation formula.
- Separate special pose-cache and matrix phases, per-joint progress clamp,
  and forced Maya matrix behavior when normal convention is SI.
- All original Core scale conventions with nonuniform base/local scale,
  quarter-turn rotation, translation, and accumulated-scale cancellation.
- Recursive traversal through Core, child parent-scale compensation, root
  sibling restoration, and the original retained global pointer identities.
- All three optional adjustment matrix pointers, separate local/world/output
  offsets, mode-0 fallthrough with transforms, and parent matrix/scale removal.

The SI expected matrices deliberately differ from ordinary J3D NoAnm SI.
For base scale `(5,7,11)` and translation `(10,20,30)`, local Rz90, local scale
`(2,3,4)` and translation `(1,2,3)`, the hand-calculated CoreSI result is
`[0,-315,0,35; 140,0,0,118; 0,0,1936,393]`. The corresponding Basic/Maya result is
`[0,-15,0,15; 14,0,0,34; 0,0,44,63]`. These exercise the recovered original
initializer and both SI scale applications rather than assuming the SDK
NoAnm calculator and Core are interchangeable.

The original Core destructor is empty. Fixtures explicitly own the allocations
made by each original constructor and `enableJointTransform`; a shared Core
owns only its separately allocated tracks. Borrowed pose/transform arrays are
freed exactly once by their original fixture owner. Global traversal state is
restored by a test scope after the original calls, without altering production
Core semantics.

`_6 == 3` / `fixT` is outside this fixture's runtime coverage because it needs
the complete original `J3DModel` lifecycle. No replacement model was fabricated.
These tests also do not claim Mario's full XanimePlayer or animator lifecycle
is active. The parent owns the native build and run; when written, this test
file had only source/diff checks and had not yet been executed.

## Native result

The parent built and ran the target on macOS arm64: all nine groups pass.
`native-evidence.json` retains the final build and run hashes. The first build
needed only the existing Metrowerks compatibility include in the test and
consistent float arguments to a test vector setter; no production fix was
needed to pass these groups.
