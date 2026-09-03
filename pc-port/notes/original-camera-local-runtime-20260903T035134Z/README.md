# Original camera utility runtime

`compat/CameraLocalUtilRuntime.hpp/.cpp` supplies the scene boundary needed by
the original `CameraParallel` and `CameraHeightArrange` implementations.
Camera calculation and height/offset policy stay in original game code.

`ScopedCameraTargetBinding(Camera&, CameraTargetObj&, OriginalCameraMode)`
binds a real camera and target for a reset/calculation call. Its required mode
argument identifies the original manager lane (`Game` or `Subjective`); there
is no default. Thread-local state restores the outer camera, target, and mode
after a nested scope. `CameraLocalUtil::getTarget` verifies the actual camera
or manager identity. Missing ownership raises an explicit error. The camera
must already have its real `CameraMan` and both `CameraPoseParam` objects;
no partial or fabricated `CameraDirector` is allocated.

`MR::isFirstPersonCamera` reads that bound mode and reports missing ownership.
The reset helpers use this query, which is the existing root CameraUtil
wrapper for `CameraDirector::isSubjectiveCamera`. They retain the original
sub-pad C held/trigger checks. Round-left/right bodies retain the exact
`MR::isDemoActive`, first-person, and core-pad trigger gates. Input therefore
uses the current WPAD service, and demo ownership remains required by the
existing demo provider.

## Source correspondence

The following blocks from `src/Game/Camera/CameraLocalUtil.cpp` are copied
verbatim, including their function bodies and arithmetic:

- Lines 21–92: camera-manager pose getters and setters.
- Lines 125–257: camera pose getters/setters, `recalcUpVec`, interpolated
  and immediate local/global watch-offset and watch-point calculations.
- Lines 294–317: round-left and round-right pad triggers.
- Lines 318–351: `slerpCamera`.
- Lines 411–464: `keepAwayWatchPos`, `calcSafeUpVec`, and `calcSafePose`.

`makeWatchOffset` consequently reads the actual
`CameraMan::mRequestLOfsReset`; it preserves the original target-motion
interpolation factor and zone-transformed global offset.

`CameraTargetObj`'s constructor is copied verbatim from
`src/Game/Camera/CameraTargetObj.cpp`; its unneeded actor/player subclasses
are not pulled into the compatibility translation unit.

`smgpc::compat::calcCameraViewMtxFromPoseParam` has exactly the body of root
`CameraDirector::calcViewMtxFromPoseParam` (lines 744–761), with only its
qualified function name and indentation changed. It preserves original
front/side/up construction, signs, position, roll rotation, and matrix
concatenation.

A source comparison verified all five copied blocks as exact substrings.
The renamed view helper matched after removing whitespace. The final Apple
Silicon debug build and original camera runtime test passed on 2026-09-03.
The source-identical controller also passed the real Gateway walking fixture.

The `smg-pc-original-camera-runtime-tests` target
(`CameraLocalUtilRuntimeTests.cpp`) checks matching camera/manager ownership,
missing-owner errors, failed-constructor preservation, nested target and mode
restoration during exception unwinding, and all four original subjective pad
suppression helpers with held/triggered input present. The pad tests install a
real empty scene demo runtime and use Aurora's actual `WpadService`; the
global SDK input service is the same one the runtime uses and needs no
renderer. An original `CameraParallel` runs the right-trigger trajectory,
advances its retained angle by 0.08 radians per frame to pi/4, does not repeat
while held, and returns to zero after sub-pad C reset. The per-frame rate and
round interval were checked in the verified RMGK01 DOL:
`CameraParallel::calcRound` at `0x800a94c8` loads the 0.08 float from
`r2-25816` (`0x806b9748`) in both directions; the interval at `r2-25804`
(`0x806b9754`) is pi/4. It really uses the constant rate even after assigning
`mRoundAddition`; the test preserves that original behavior.

The controller-wrapper tests cover actual original offset interpolation,
roll, and controller state separately.

## Deliberately unprovided services

Director register lookups, forced camera changes, target replacement,
`tryCameraReset`'s director-level availability decision, and unrelated camera
utilities remain absent. This boundary supplies the real imported
controller's dependencies; it does not claim the entire CameraDirector,
all camera types, or complete player camera-state support.
