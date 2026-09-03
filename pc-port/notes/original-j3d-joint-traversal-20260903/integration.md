# Native joint pipeline and separate model scale

The renderer now evaluates poses through actual `J3DJointTree`, `J3DJoint`,
`J3DMtxBuffer`, and the original Basic, Softimage, and Maya calculators. The
previous host hierarchy recursion and trigonometric joint transform builder
were removed. Authored INF1 matrix mode and hierarchy commands survive geometry
extraction; JNT1 fields populate actual native joint objects.

`OriginalJ3dJointTree` owns native joints, pointers, hierarchy commands, scale
flags, animation matrices, and a default calculator. The renderer adapter lends
the immutable owned BCK object to the original `calcTransform` sampler. Existing
playback chooses the sample frame before traversal; the owner itself adds no
looping or clamping policy. A model/resource lifetime test and raw-frame test
cover these distinctions.

The native storage owner is deliberately not a partial or type-punned
`J3DModelData`/`J3DModel`. Material and shape objects still belong to the native
renderer. Joint hierarchy links follow the original `makeHierarchy` parent and
current-joint scopes, including scopes containing material and shape commands.
Malformed or incomplete joint input fails at the loader boundary.

Original traversal uses process-wide globals. A scoped recursive mutex saves
and restores those globals around native entry, including exceptional exits.
Different owners can nest through callbacks. Reentering the same owner's matrix
buffer is rejected before its borrowed animation adapter is modified. Actual
`recursiveCalc` callback order and its unusual calculator restoration behavior
remain unchanged inside this boundary.

## Base transform and scale

The initial integration calculated unit/identity poses and concatenated the
actor matrix afterward. Review identified that this inherited renderer shortcut
cannot express the original ordinary-model scale contract. In particular,
Softimage applies cumulative scale after rotation; Basic can skip local scale
when cumulative scale cancels to one.

The final integration passes base transform and base scale separately into
`J3DJointTree::calc`, as `J3DModel::calcAnmMtx` does in ordinary mode. The returned
joint matrices already include the base. Drawing and joint queries do not
concatenate the actor matrix again. Envelope evaluation consumes those matrices
times unchanged inverse-bind matrices and sums authored weights without the
previous host renormalization. The second, now redundant model-matrix cache was
removed.

`LiveActorModel` retains base scale independently from the actor's TR matrix.
Model initialization snapshots `mScale`; generic `calcAnmMtx` snapshots it before
`calcAndSetBaseMtx`, matching the root sequence. `MR::setBaseScale` can override
model scale without changing `LiveActor::mScale` or the base matrix.
`MR::setBaseTRMtx` writes the supplied matrix unchanged, without implicit scale.
Draw options and joint queries receive the same retained scale.

Game changes restore original behavior in existing native replacement code:

- `LiveActor::calcAndSetBaseMtx` is copied from root, including the taken-host
  branch and original Y-only/general rotation helpers. Its old native TRS
  trigonometry helper is deleted.
- The missing `MR::getTaken` dependency is imported unchanged into the sensor
  compat provider. Its required `HitSensorKeeper` header is an original copy;
  no substitute keeper is constructed.
- Generic `LiveActor::calcAnmMtx` regains the root base-scale snapshot.
- The existing PC `MarioActor::calcAndSetBaseMtx` branch snapshots `mScale`
  immediately after writing TR. Root `MarioActor.cpp` writes
  `mBaseTransformMtx`, then `mBaseScale` at lines 2334–2337. No movement rule or
  camera algorithm is changed by this integration call.

Binder and FixedPosition consume raw `getBaseMtx` in root. Projmap inverts the
base transform alone. The former FixedPosition test expectation containing
implicit model scale is therefore corrected to the original raw-TR result.
CollisionParts' explicit `makeMtxTRS` fallback remains separate.

The broad native regression caught an additional obsolete Binder adaptation:
it still divided the matrix Y axis by actor scale and reconstructed collapsed
axes using actor rotation. With raw TR that inverted negative-scale actors'
Binder offsets. The owner map, registration API/header, division, and rotation
reconstruction are deleted. Binder now uses its existing direct supplied-Y
normalization path for every caller. Negative/zero model-scale fixtures retain
the original raw-TR direction; explicit matrix bases remain explicit.

The same run exposed a pre-existing stale GamePad test. Production
`GamePadUtilCompat.cpp` was already reading global Aurora WPAD and is unchanged
in this checkpoint. The old test expected every input query to throw without
RuntimeContext. It now verifies actual Aurora-fed holds, triggers, releases,
and stick transitions without that context, while still rejecting camera-relative
stick projection when the camera is absent.

## Validation and limits

`verify.py` recompiles the root SDK sources and checks the retail recovery,
imported bodies, default transform, and reciprocal evidence. The additional
`verify-integration.py` checks seven tree/buffer/data lifecycle bodies against
their original source and the byte-identical JointTree header.

Nine native traversal groups pass, including all three modes, callback timing,
override inheritance, native ownership, exceptions, and reentrancy. The added
base-transform group checks literal independent expectations for rotated and
translated nonuniform base scale, both bind and BCK paths, scale cancellation,
descendants, and scale flags. See `tests.md` for the complete oracle.

Final macOS arm64 build and all 13 test processes passed. `native-evidence.json`
records their outcomes, log hashes, source hashes, and rendered-frame hash:

- Nine joint-traversal groups and 28 Aurora-native cases.
- 57 camera cases across OnlyCamera, ViewInterpolator, ViewService, stage-start,
  and actor-event suites, plus the original camera runtime fixture. The suites
  report 59 groups; two optional real-disc camera probes were skipped because
  their processes did not receive `SMGPC_REAL_DISC`.
- Five FixedPosition and six NPC cases.
- Six LiveActorUtil groups, including actual Tico animation, retained joint
  pointers, explicit model-scale overrides, pre-base scale snapshot ordering,
  and paused 3D/2D GPU rendering. This wiring test uses an independently created
  tree with the same original calculator; the nine traversal groups separately
  provide literal matrix oracles.
- Real-disc Gateway stand/walk/release: 14,521 KCL triangles, 325.684 distance,
  `Wait -> Run -> Wait`, release frame 297, and 12 Run packets. Lifetime/recreation
  assertions pass. The final walking screenshot is byte-identical to the
  visually inspected frame.
- Real-disc Gateway spin-unlock checkpoint. This is not a completed bunny-chase
  or Rosalina-spawn claim.

The showcase executable also builds. Test executables run from `pc-port` under
`build/macosx/arm64/debug`; set `SMGPC_REAL_DISC` to the user's local image for
the live walking, spinning, and actor tests. Build using the configured LLVM
toolchain, for example:

```sh
PATH=/opt/homebrew/opt/llvm/bin:$PATH xmake build -j8 \
  smg-pc-original-j3d-joint-traversal-tests smg-pc-aurora-native-tests \
  smg-pc-live-actor-util-real-or-absent-tests smg-pc-mario-gateway-walk-tests \
  smg-pc-gateway-spin-checkpoint-tests smg-pc-showcase
```

This is the ordinary J3D model path. The full `J3DModel` flag pipeline is still
absent. Retail `UseDefaultJ3D` deliberately evaluates joints with unit/identity
and applies base TR/scale later in draw-matrix mode 2; mode 1 also has distinct
view handling. Those paths must be implemented with the actual model flags,
not inferred from a stage or object name.

Envelope paired-single FMA ordering and original envelope scale flags/normal
matrix selection are still pending. Existing invalid inverse-bind/influence
fallbacks are also not a complete original loader contract. The current CPU
normal path always inverse-transposes/normalizes and can differ from the
original scale-flag fast path. None of these are claimed as bit-exact here.

Effects have a separate unresolved host-SRT versus host-matrix binding boundary.
The native effect host now sees raw actor TR rather than accidentally inheriting
model scale. Root auto-effect registration bodies are absent; no universal scale
reapplication was guessed. Full Xanime/Mario animation sequencing remains
unfinished; root pose blending recovery is checkpointed separately in
`e75789ce4` and is not activated by this renderer change.
