# OnlyCamera and director lifecycle correspondence

This review followed the original `CameraDirector.cpp`, `OnlyCamera.cpp`, `CameraManGame.cpp`, `CameraManEvent.cpp`, and `CameraLocalUtil.cpp`. The native changes in this subtask are limited to matrix seed ownership and actual used-target propagation in the existing original controller wrappers and event runtime. The root agent owns OnlyCamera and service integration. This note belongs to the OnlyCamera checkpoint; the earlier `original-camera-view-20260903` notes describe the preceding interpolator checkpoint.

## Original phase ordering

`CameraDirector::movement` restores a preceding subjective view if necessary, moves the selected target once, processes manager reset/local-offset requests and calls the active manager's movement, calculates OnlyCamera's pose, creates the interpolated view, writes `MR::getCameraInvViewMtx()` into the current manager's `mMatrix`, and copies that manager's raw pose into `mPoseParam2`. Subjective camera processing and shaker movement follow those writes.

The manager's raw pose, OnlyCamera's adjusted pose (`mPoseParam1`), and final rendered inverse view matrix are separate state. Neither the adjusted pose nor the rendered view may overwrite the raw pose during ordinary movement. OnlyCamera is one persistent director owner across camera/chunk changes and game/event manager transitions.

OnlyCamera's start and safe paths copy position, up, watch, watch-up, global/local offsets, and roll. They do not copy FOV or front/upper offsets. `CameraDirector::createViewMtx` deliberately supplies `CameraLocalUtil::getFovy(currentMan)` separately to `CameraViewInterpolator::updateCameraMtx`; using OnlyCamera's default FOV40 there would be wrong. `CameraPoseParam::copyFrom` does copy all fields, including FOV, when explicitly requested elsewhere.

## Reset and transition conditions

- A director reset request is processed after target movement and before manager movement. `resetCameraMan` requests zero interpolation, seeds the current manager's eye to `target.position - target.front * 800 + target.up * 300`, assigns watch/up/watch-up from that target, deactivates and activates the same manager, and sets `OnlyCamera::mIsResetting`.
- On its next calculation, OnlyCamera clears `mCalcIdeal`, `_24`, and `mIsResetting`, sets `mStartPose`, and takes its start-pose path. This reset does not clear `mSpeed`. Ordinary controller resets or event push/pop do not reset OnlyCamera.
- Director copies an active camera's `isZeroFrameMoveOff()` result into OnlyCamera's flag before calculation. The retail safe path clears this flag after `moveToIdealPosition`. The inspected retail OnlyCamera functions contain no read of this flag; no extra host branch should be invented for it.
- `mCalcIdeal` is initialized false and cleared by reset/completion. No source setter enabling it was found in the current root camera sources. Its original acceleration/deceleration algorithm should remain intact without inventing a trigger.
- Entering an event from the game manager copies the raw game pose and current rendered inverse-view matrix into the event manager before activation/reset. Changing chunks while already in the event manager retains that manager's raw pose and matrix.
- Event finish first applies its interpolation request and then pops the event manager if empty. Only if `resetView`, `!isForceCameraChange()`, and the newly active manager is the game manager does the director copy its latest OnlyCamera-adjusted `mPoseParam1` into the raw game pose and the current inverse-view matrix into the game manager. The force-change check occurs after the finish request, so a zero-frame finish suppresses this copy. The interpolation-off gate is then set to protect the finish request from the newly activated game's chunk request.

The existing native authored-game-only reset request remains narrower than the original director: a reset during an event must ultimately address the active event manager too. This review reported that boundary; it did not add unrelated reset behavior to the event runtime.

## Native matrix seed and used-target changes

Both `OriginalGameCamera` and `OriginalAnimationCamera` append an optional `const TPos3f* manager_matrix_seed` argument. They copy that matrix into their actual `CameraMan::mMatrix` before original reset/calc. `EventCameraRuntime::start` accepts the same final optional argument and keeps an owned matrix snapshot alongside its raw pose seed. An uncalculated request replacement preserves the original pending seed; an event change after calculation copies the current event manager's actual matrix. The root service supplies its existing original view output on initial game-to-event entry.

The input target and used target are also distinct. Original managers call `CameraLocalUtil::setUsedTarget` with the return of `Camera::calc`, and the director passes that used target to view interpolation. `CameraAnim::calc` returns null even while reading its selected input target's live matrix. Both wrappers now retain their actual return pointer, and `EventCameraRuntime::view_target` delegates to the calculated wrapper. Input movement and selection are unchanged. Before first calc, no used target is fabricated.

`CameraViewServiceTests.cpp` adds three regression groups: matrix seeds survive original static/animation reset; pending request copies retain matrix ownership and calculated event transitions inherit the event manager's rendered matrix; and a moving CANM input still yields a null used target while its live transform affects the authored pose. The currently supported original reset bodies do not read `mMatrix`, so the tests establish retention across construction/reset; the source ordering establishes assignment before reset without a production fixture hook. Future matrix-consuming controllers include `CameraFixedThere`, `CameraRaceFollow`, and `CameraWonderPlanet`.

## Retail evidence

The verified RMGK01 rev0 DOL was disassembled for `OnlyCamera::calcPose` through `moveToIdealPosition`:

```
python3 build/compat-math-oracle/disassemble_dol.py 0x800B7170 0x6dc build/compat-camera-only-review/only_calc
```

The resulting `build/compat-camera-only-review/only_calc.asm` confirms resetting writes at `0x800B7198` (clear +0x11), `0x800B719C` (clear +0x24), `0x800B71A0` (clear +0x3D), and `0x800B71A4` (set +0x10). Safe calculation clears +0x3C at `0x800B7558`. The start/safe output writes do not write pose +0x30/FOV. Root configuration already marks `OnlyCamera.cpp` matching. This subtask did not recompile that original TU or run native builds.

## Further original-source closure

Small complete original owners suitable for a later import include `CameraTargetHolder` (its exact selected-target lifetime and movement), `CameraRegisterHolder` (actual register arrays and original dummy entries), and `CameraRotChecker` (uses already supplied view matrices and `TPos3f::getRotate`). `CameraManPause` is source-complete but needs a real manager-target binding over activation/calc. `CamHeliEffector` is similarly bounded with the actual target camera-state getter. `CamKarikariEffector` additionally requires real Karikari cling-count providers; substituting zero would discard gameplay behavior.

Full `CameraDirector` construction is not a small import: `CameraHolder::createCameras` eagerly constructs every camera/translator in its original table; `CameraParamChunkHolder` requires the real `CameraParamChunk`, `CameraParamChunkID`, and `DotCamReaderInBin` pipeline; `GameCameraCreator` needs CubeCamera areas and start-position enumeration; `CameraRailHolder` requires authored per-zone rail enumeration; cover and subjective owners require their actual rendering/player systems. Those dependencies should be closed as original source/runtime support, not removed from the original factory.

Three still-missing root scene helpers are bounded decomp candidates directly required by those startup owners:

- `MR::getStartPosNum`: RMGK01 `0x803F757C`, size `0x24`.
- `MR::getPlacedRailNum`: RMGK01 `0x803F7AF0`, size `0x54`.
- `MR::getCameraRailInfoFromRailDataIndex`: RMGK01 `0x803F7BA0`, size `0x74`.

The four earlier camera/start helpers, current start ID lookup, and current Mario start iterator were already recovered root-first in preceding checkpoints. No source restoration was needed for OnlyCamera itself.

## Review of the final native wiring

The final review read the actual integrated `OriginalCameraView` and `CameraSystemService` again after the OnlyCamera owner and event handoffs were written. It found no new correctness or allocation-lifetime defect in the reviewed paths:

- The owner calls original `OnlyCamera::calcPose` against the actual selected manager, passes the manager FOV separately, and writes the published inverse view back into that same manager. Raw pose data is not replaced during ordinary view processing.
- Event finish applies the original finish request before inspecting the force-change flag. Positive `resetView` return copies the adjusted OnlyCamera pose and rendered matrix into the game manager; a zero-frame finish suppresses that copy. Ordinary game reactivation requests a controller reset and retains OnlyCamera history.
- Director-style authored manager resets request both a view cut and OnlyCamera reset in the following active camera phase. The separately documented active-event reset gap remains outside this tranche.
- When a player target is retired, publication becomes unavailable and the view receives null instead of the controller's stale borrowed used-target pointer. The interpolator invalidates target correction on that null input without dereferencing its previous pointer. A resumed valid player calculation refreshes the controller's actual returned target before the view phase.
- Manual scene entry computes an original camera matrix seed before requesting the event, and initializes the persistent view from the existing manual pose. Matrix/pose snapshots are owned across pending request replacement; existing event changes take their matrix from the calculated event manager.
- Native child allocations have explicit typed owners. The original OnlyCamera and CameraMan destructors do not free their pose pointers; the interpolator does not free its Binder/filter. Each corresponding native unique owner releases its allocation once, after no further calculation can use it.

This is a bounded source/lifetime review, not a native execution result. The root agent runs the serialized final builds and regressions.
