# Original camera and clipping ownership

Removed seven more files from `src/compat` (98 → 91). The complete donor CameraLocalUtil restores all52 executable bodies unchanged. The deleted compatibility target/view bindings supported an obsolete standalone camera branch: OriginalGameCamera, OriginalAnimationCamera, OriginalCameraView, EventCamera, StageStartCamera, PublishedCameraTarget and StageEventCameraBinding are removed too. CameraSystemService and its duplicate shake/programmable/controller state are deleted; RuntimeContext and parity traces now consume the actual CameraDirectorRuntime and CameraContext. Original Game camera managers continue controlling gameplay. Manual presentation and debug freecam remain.

ClippingDirectorOwnership is replaced by native destructors in actual Game owners, which reclaim their own arrays and IDs and unregister borrowed group identities. Scene capture/reclaim hooks and the extra LiveActorUtil capture call are gone. Constructor-local guards retain failure cleanup; original clipping algorithms are unchanged.

## Validation

`xmake build smg-pc` passed. The final fresh-save Metal run of HeavensDoorGalaxy scenario1 completed120 frames, exited0, and left no process. Binary SHA256: `50a278b7ff2bc7832f833f796672707349371edc91a69f274890d13d30c01b65`. The unchanged placement report has187 supported,51 known-unlinked and0 unknown objects. No focused tests were built/run and no screenshot was captured, following the requested reduced testing policy. This opening smoke does not establish the full Gateway demo or Rosalina sequence.

## Scope preservation

Before-edit snapshots and manifests isolate source ownership. Unrelated preexisting changes and six staged route-note files remain outside this commit. Camera fixture files whose entire API is retired necessarily leave the build; existing direct camera math cases are retained or migrated. The retained HEAD-only legacy draw entry receives only the two-line direct CameraDirector presentation change documented in runtime-camera/staged-only-legacy-draw.patch; its unrelated working-tree deletion is not absorbed. Build evidence describes the working tree, not a separate clean checkout.

An initial120-frame pass preceded a source-review fix for LOD initialization failure: actual LodCtrl destruction now unregisters its borrowed ViewGroupCtrl entry, and view destruction restores living LOD flag pointers to their original false defaults. The app was rebuilt and the same120-frame smoke repeated after this concrete change. Both passed. The only final post-build source edit trims a trailing blank line from CameraLocalUtil.

Scoped staging intentionally retires the dirty ActorEventCamera, StageStartCamera and CameraViewService fixtures together with their deleted APIs, and adopts the actual-context CameraViewInterpolator migration. All other initially dirty owned files stage only this batch's deltas; tests/xmake retains unrelated target edits unstaged. Exact initial versions remain in local before snapshots.
