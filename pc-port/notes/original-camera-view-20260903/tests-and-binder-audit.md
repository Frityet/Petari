# Original view interpolation tests and camera Binder audit

This work adds tests around the actual `CameraViewInterpolator`, owned through
`camera/OriginalCameraView`, and reviews the existing generalized collision
boundary. It adds no camera algorithm, Binder fallback, or gameplay change.

## Source-defined expectations

`src/Game/Camera/CameraViewInterpolator.cpp` supplies the state machine and
numeric expectations:

- `setInterpolation` preserves its timer/time when `mIsInterpolationOff` is
  set, and a zero-time request otherwise forces a camera cut.
- `interpolateCameraSwitching` uses the squared current timer fraction and
  feeds each result back into the following calculation. For a four-frame
  transition from distance 600 to 1200 with an unchanged orientation, the
  distances are 600, 637.5, 778.125, 1015.4296875, and 1200. The timer reports
  interpolation finished on the fourth update; the following update uses
  rate one and reaches the exact endpoint.
- `reduceOscillation` retains 70 percent of the preceding position but
  90 percent of the preceding FOV. The tests distinguish these two rates and
  verify that a forced cut bypasses both.
- `updateCalcState` uses target object identity, waits for switching to finish
  before becoming ready, and then applies the original target's `getLastMove`.
  Disabling camera-position correction for one frame still moves the watched
  position; the method restores its correction flag afterward.
- `checkNearlyEnd` accepts distance exactly one and rejects greater distance.
  Its angle threshold is one radian: the root matrix helper delegates to
  quaternion `getRotate`, which returns twice `JMAAcosRadian(w)`. Thirty-degree
  and sixty-degree matrices independently bracket this threshold.

`CameraViewInterpolatorTests.cpp` also verifies the native service boundary:
after a target translation of 100 with zero last movement, original
`CameraParallel` publishes raw eye X = 100, while the persistent original view
interpolator publishes render X = 30, then 51. Repeated camera-phase indices do
not apply damping twice. Raw manager state remains unchanged by render damping.
The fixture begins with target X = 600, because `CameraParallel::calcIdealPose`
subtracts 90 degrees from the zero azimuth and places its distance-600 eye at
X = 0. Moving that target to X = 700 then gives the independent raw X = 100
expectation.

The owner tests preserve projection metadata and inspect both output view
matrices. Algebra tests use the original interpolation/collision/repulsion
flags to isolate one source stage. They do not replace those stages with new
algorithms.

## Camera caller compatibility audit

The original interpolator constructs `Binder(nullptr, &mPosition, &mGravity,
100, 0, 64)` and installs `TriangleFilterFunc(MR::isCameraCodeThrough)`.

The current `compat/BinderCompat.cpp` supports this caller without an actor
owner. A null matrix bypasses the actor/base-matrix up-vector path, the camera
supplies valid live position and gravity references, and zero offset leaves
the center at the camera position. The 64-plane storage remains available to
`calcBinder`'s `mPlaneNum` check.

`compat/HitInfoCompat.cpp::make_collision_triangle_filter` negates
`isInvalidTriangle`, preserving the original rejection convention. The query
invokes this filter before capacity and contact selection.
`compat/GameMapCollisionCompat.cpp::isCameraCodeThrough` reads the real source
prism's `Camera_through` attribute and rejects the `Through` code. No actor
name, resource name, zone, or camera identity enters this decision.

The focused collision test registers a wide, one-sided KCL wall with its own
BCSV attribute row, exercises the actual constructor-installed filter, and
checks that `Through` permits the entire movement while `NoThrough` stops the
sphere and retains contacts. It also verifies the FOV-derived radius at
90 degrees, the original quarter-step camera response, and the exact
3400-unit early-return boundary. That boundary clears prior contacts and
leaves the caller's output vector untouched.

Static caller inspection found no missing Binder shape/filter bridge, but
execution exposed a shared material-attribute cache lifetime bug. A destroyed
`StageCollisionService` and its replacement could share both an address and
resource revision. `Triangle::getAttributes` then reused the preceding
service's decoded BCSV rows. The second wall was authored as `Through` but
still produced two collision planes from the cached `NoThrough` row.

`StageCollisionService` now assigns a unique immutable generation to each
instance. The attribute cache checks this lifetime generation in addition to
service address and resource revision. Generation allocation is atomic and
exhaustion cannot wrap into a reused identity. This fixes the shared material
boundary for all collision attributes, without a camera-specific exception.
The test explicitly reconstructs the services in the same storage and verifies
equal resource revisions with distinct generations before checking the second
wall's real filter result.

This is not a claim of complete collision parity. Existing general limitations
remain:

- Contact capacity follows deterministic source/prism order rather than
  retained original octree encounter lists.
- The native movement query uses fixed 35-unit subdivisions and one projected
  retry. Earlier `stage-physics-parity-20260806` notes describe source/binary
  evidence for this behavior; this audit did not freshly decompile the full
  Binder body, which is still incomplete in the current root source.
- This test covers one static wide wall. It does not establish moving geometry,
  every edge/corner contact, or arbitrary multi-plane parity.

## Fixture changes and validation

The target is `smg-pc-camera-view-interpolator-tests`, registered in
`tests/xmake.lua` with the shared Game/common/Aurora dependencies. It contains
seven focused groups.

Existing actor-event and stage-start service tests now install actual
`SceneObj_AreaObjContainer` fixtures because the original view phase performs
the repulsive-area query. Target tests that already own their scene retain
that ownership; the runners skip their outer fixture to avoid nested
`SceneObjHolderBinding` instances. The real-disc stage fixture creates its
area/gravity scene before initial view publication and does not preseed matrix
movement values.

The root agent authorized a serialized build and run of only the new target
to diagnose the two initial failures. After the source-derived fixture
correction and shared cache lifetime fix:

- `xmake build smg-pc-camera-view-interpolator-tests`: passed.
- `xmake run smg-pc-camera-view-interpolator-tests`: 7/7 passed.
- The existing missing-`override` warnings remain in original Game headers.
- `git diff --check`: passed.

The run output is retained in
`camera-view-interpolator-verified.log`. The root agent owns broader
integration verification; no other targets were built or run by this author.
