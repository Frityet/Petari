# Original scene execution integration — 2026-09-07

This checkpoint shares the actual original SceneNameObjListExecutor, NameObjExecuteHolder, category arrays and draw-buffer owner between each real scene and the native host. Scene lifetime callbacks keep the same Game allocation domain and services alive through derived scene destruction, then retire registrations, SceneObj children and the executor in dependency order. A scene-wide original NameObjHolder now includes unscheduled objects and provides the recovered 16-entry lookup cache. The original StopSceneController and SceneNameObjMovementController own delayed stop and movement synchronization. Four LiveActor appearance/clipping lifecycle bodies are restored from the reference; native clipping membership follows those calls.

The new GeneralPos binding retains actual authored records and publishes the original NamePosHolder. Original CinemaFrame, ScenarioTitle, GameStageClearSequence and PauseButtonCheckerInGame are imported with original bodies. Gateway and Title host setups use the same explicit scene executor ownership, including title-to-stage handoff. Aurora supplies original OSMessage queue algorithms backed by native synchronization. No GameSystem/GameScene object is fabricated and no Gateway-specific gameplay workaround is added.

## Current validation

The coordinated `xmake build -v smg-pc-game` passed. All application compilation passed; `smg-pc-showcase` linking failed with exactly three existing unresolved functions: `MR::requestEndMissDemo`, `MR::requestEndGameOverDemo`, and `MR::requestStartGameOverDemo`. They require genuine original GameScene ownership and transitions.

Fresh Xmake builds and direct runs passed for scene lifetime binding (32 actual scene Game domains), scene NameObj registry (16 domains), and the original layout-group regression. The new NamePos owner and six additional fixtures all compiled, then failed linking on exactly the same three functions: original scene execution owner, scheduler heap, CameraDirector, scene wipes, image-effect ownership and stage resource initialization. Those seven fixtures were not run. `build-results.json`, `fixture-results.json` and `fixture-followup-results.json` record exact outcomes. `link-frontier.json` captures the current symbols without the enormous linker command lines.

The camera/wipe/image-effect fixtures now own the original executor and complete original connection initialization after constructing their test objects. Image effects also explicitly bind the active scheduler. These migrations compiled but their runtime assertions remain unverified at this checkpoint.

## Remaining behavior and delivery

The native StageHostScene remains active. The complete original SceneExecutor call order, genuine GameScene and mandatory pause/opening children still need activation. Audio output is optional, but the original scene pause controls must access actual typed owners. Shared clipping currently uses native frustum services rather than the complete original clipping hierarchy. New model registrations after fixed draw-list allocation remain explicitly unsupported.

The walking demo app is unchanged. Jumping, the complete original game camera and Gateway bunny chase are not verified playable. The user's delivery request is to replace the demo as soon as either jumping or the full camera is independently playable; do not wait for both. Preserve the old app until a verified replacement exists.

Submodule sources are committed and published as codex before recording their parent links. User-staged deletions of DISCREPENCY_REPORT.md and MACOS.md and the untracked script/package_walking_demo.py are excluded from this checkpoint. Detailed source proofs live in the adjacent original-scene-execution-owner, original-scene-lifetime, original-scene-movement-owners, original-game-scene-child-owners, original-scene-executor and original-os-message notes directories.
