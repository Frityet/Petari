# Canonical CameraLocalUtil and obsolete branch removal

Restored complete current donor `Game/Camera/CameraLocalUtil.cpp`; every executable body is exact. Only two matching-only anchor functions are omitted. Original target lookup now always follows the actual camera manager/director relationship. The unused thread-local mode, null-director target override, and stale alternate view output are removed instead of being moved to another compatibility bucket.

Deleted19 files: five compat provider/header files, six obsolete camera wrapper/value-branch pairs (OriginalGameCamera, OriginalAnimationCamera, OriginalCameraView, EventCamera, StageStartCamera, PublishedCameraTarget), and StageEventCameraBinding pair. PublishedCameraTarget had no external production consumer beyond the removed wrappers; its StageCameraTargetState disappeared with that execution branch. Existing CameraParam/CameraPose decoders/value types remain, as do actual CameraDirectorRuntime presentation and CameraAnimation/NativeCameraAnimationData resource retention.

Applied required `src/Game/xmake.lua` addition with `-ffp-contract=off`. Existing source globs naturally remove retired files. RuntimeServices/RuntimeContext/ParityTrace migration is owned by decomp_validation; obsolete fixture cleanup/tests wiring by merge_audit. This lane changed no runtime or test files. All21 paths were clean or absent before edit; snapshots/manifests/scoped patch retained here.

Source-only validation: full donor comparison passed, single canonical CameraLocalUtil provider found, scoped diff check passed. No build, runtime, test, git index, commit or push was performed by this lane. Root will validate the integrated closure.
