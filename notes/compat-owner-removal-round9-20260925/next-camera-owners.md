# Next camera owner removal (read-only source audit)

**Recommendation:** restore complete donor `Game/Camera/CameraLocalUtil.cpp`, delete the five requested compat files, and remove the obsolete standalone camera execution branch instead of moving its bindings. The actual Game `CameraDirector`, `CameraContext`, `CameraManGame/Event/Subjective`, `OnlyCamera` and `CameraViewInterpolator` owners are already compiled. No decompilation is required for CameraLocalUtil.

## Why the donor is sufficient

- `CameraLocalUtilRuntime.cpp` and `OriginalCameraOwnerUtil.cpp` supply the donor's pose/vector/quaternion/reset/register helpers. Donor target lookup is simply `camera->mCameraMan->mDirector->getTarget()` (or the equivalent CameraMan overload). Actual managers attach to the director through `CameraMan::owned`. Restore these complete original bodies; do not carry the null-director target fallback.
- `sBoundMode` in CameraLocalUtilRuntime is only saved/set/restored, never read. Delete it and the scope API. `OriginalCameraMode` also controls duplicated manager behavior in `camera/OriginalGameCamera.cpp`; delete that obsolete wrapper branch rather than relocating the enum.
- `calcCameraViewMtxFromPoseParam` is already implemented identically by `CameraDirector::calcViewMtxFromPoseParam` (line745). The compat copy exists solely for the standalone wrappers/runtime fallback.
- `CameraViewRuntime` has **no production reader of bound_camera_view_output()**. Canonical `MR::setCameraViewMtx/setFovy/setNearZ` already write directly to the actual scene `CameraContext` (`Game/Util/CameraUtil.cpp:175-200`). Consequently the wrapper's output view/inverse/FOV are not being updated by those setters. Remove this stale state, not just its TLS accessor.

## Concrete dependency closure

Delete `compat/CameraLocalUtilRuntime.cpp/.hpp`, `compat/OriginalCameraOwnerUtil.cpp`, `compat/CameraViewRuntime.cpp/.hpp` after replacing the public Game helpers with the donor. Also retire `camera/OriginalGameCamera.cpp/.hpp`, `OriginalAnimationCamera.cpp/.hpp`, `OriginalCameraView.cpp/.hpp`; they construct managers with no director and depend on the fallback target scope. Their production-source references are `EventCameraRuntime` in `camera/EventCamera.cpp/.hpp`, `camera/StageStartCamera.cpp/.hpp`, and CameraSystemService's duplicated authored/event/view state in `runtime/RuntimeServices.cpp/.hpp`.

These paths are not the active Game camera path: `CameraSystemService::begin_frame` returns immediately when CameraDirectorRuntime exists (RuntimeServices.cpp:1442), and `effective_camera_pose` reads that real context (line2088). No production caller installs the authored/start camera wrappers. `StageEventCameraBinding` has only a fixture caller (ActorEventCameraTests); remove its pair with the obsolete event runtime. Keep the actual `CameraDirectorRuntime` renderer adapter and its CameraAnimation/NativeCameraAnimationData retention: they serve the real scene, unlike the standalone controller wrappers.

RuntimeContext and ParityTrace still consume CameraSystemService presentation/query APIs. Remove only obsolete controller/state branches, then point any retained presentation read directly at the actual context. Preserve legitimate manual rendering input separately; do not replace the retired execution branch with another fake Game owner.

## Fixtures / blockers / integration

`CameraLocalUtilRuntimeTests.cpp` directly tests invented scope nesting/missing-owner exceptions; retire that group/target (`smg-pc-original-camera-runtime-tests`) or retain only actual-owner math cases. `OnlyCameraTests.cpp` and `CameraViewInterpolatorTests.cpp` use the stale output wrapper; migrate any retained assertions to real CameraContext. `StageStartCameraTests.cpp`, `ActorEventCameraTests.cpp` and `CameraViewServiceTests.cpp` exercise standalone service APIs; prune obsolete branches instead of recreating them. No new fixture framework is needed.

Initially dirty overlap observed: RuntimeContext.cpp, ActorEventCameraTests.cpp, CameraViewInterpolatorTests.cpp, StageStartCameraTests.cpp. Preserve exact existing edits. Restoring CameraLocalUtil alone would break still-compiled null-director wrappers, so the wrapper/service closure must be coordinated in the same batch (more than ten files). Add the new donor source via the existing Game wildcard, retaining unfused camera math flags where needed. This audit performed no source edits, builds or tests; only this note was written.
