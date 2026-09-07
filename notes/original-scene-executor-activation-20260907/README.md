# Original frame sequencing — 2026-09-07

The complete reference SceneExecutor.cpp is imported byte-for-byte into Game. All 19 functions freshly compile with the Wii compiler and match 100% against the retail SceneExecutor object. The complete native source and original declarations compile without changing any Game function bodies. The original SpinDriver path, SpiderThread and ParabolicPath headers satisfy its original includes; they do not activate those actors.

SceneExecutionService now calls the actual original stop-controller update and movement list, followed by the actual animation list, the GameScene animation-ignore-pause category and actual view/entry list. The host supplies one Game allocation scope and J3D restoration scope around each phase. This preserves camera/mirror-before-clipping, the original connection requirement phases, all four moving-collision movement and animation passes before CollisionDirector/player, and ShadowController movement in the later animation phase. The full original GameScene still needs activation; its movie, pause, time-up and drawing decisions are not replaced by this service.

The camera fixture now adds named probes to the real original scene categories. It asserts that collision animation executes before collision/player movement and is not repeated during later animation. It also exercises the actual StopSceneController decrement/resume boundary. Its synthetic camera publisher still deliberately suspends CameraDirector because full Director movement needs the Mario graph; the test now synchronizes that pending movement request through the original movement controller before directly exercising camera categories. These are future runtime assertions, not a claim that full gameplay has run.

The small scheduler follow-up removes the duplicate post-movement clipping evaluation and makes NameObjFinder use the active scene's original NameObjHolder/cache, excluding process identities. Two original lazy draw helpers use their actual typed scene objects and retain the original absence guards. They do not construct placeholder owners.

## Validation

Fresh coordinated full Game archive build passed. All compilation for the main showcase, camera fixture and original execution-owner fixture passed. All three executable links stop on the same unresolved MR::requestEndMissDemo, MR::requestEndGameOverDemo and MR::requestStartGameOverDemo; no new unresolved symbols were introduced. The fixtures were not run. Commands, source hashes, reference scores and final build outcomes are in the adjacent JSON files.

Previous scene-lifetime, registry and layout-group runtime passes belong to root checkpoint 192bbe4df. They are not relabeled as runtime verification of this subsequent frame-sequencing change. The full original camera, jumping and bunny chase remain unverified; the installed walking demo has not changed.

## Next integration

Provide actual GameScene ownership and its original mandatory sequence/control descendants. Expose the already split stage-loading/placement services at the original SceneFunction boundaries and close actual audio owners needed by pause controls. Original clearZBuffer requires generalized GX depth-texture support, currently under separate implementation. Deliver an updated demo once either jumping or the full original camera is independently playable.
