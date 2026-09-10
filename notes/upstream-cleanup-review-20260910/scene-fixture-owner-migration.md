# Existing fixture scene-owner repair — 2026-09-10

Root's first linked runs showed stale setup rather than the intended assertions: NameObjGroupLifetimeTests failed at `Name lookup requires the actual scene NameObjHolder`, and three SceneObjHolderRealOrAbsentTests groups failed at `Execution registration needs an active scheduler`.

Original name lookup now resolves through the scene's actual NameObjHolder. Original constructors such as SwitchWatcherHolder and CollisionDirector register with the original execution owner. A bare SceneObjHolderBinding, or a standalone group, no longer supplies that lifecycle. The repaired tests use existing SceneExecutionFixture with an actual JKR allocation domain and explicit SceneSchedulerBinding, following OriginalSceneWipeOwnerTests and OriginalSceneExecutionOwnerTests.

`tests/NameObjGroupLifetimeTests.cpp` retains 32 complete ownership cycles, direct/MR duplicate membership removal, stable survivor ordering, cleared slots, derived-group retirement, and failed factory rollback followed by successful retry. Every cycle now owns the original executor, NameObjHolder and SceneObjHolder. Its old immediate-resume expectation was also stale: original pauseOffAll requests resume, then SceneNameObjMovementController applies deferred flags. The fixture checks both phases through the actual controller.

`tests/SceneObjHolderRealOrAbsentTests.cpp` preserves unbound absence, explicit singleton creation, parallel-binding rejection, scene isolation, exact watcher child counts and complete teardown. Counts are relative to the initialized scene's baseline rather than assuming no mandatory scene controllers exist. CollisionDirector is now an actual supported singleton; its former unsupported assertion was replaced by real identity checks, while EventDirector and MiiFacePartsHolder retain unsupported coverage.

The failed raw-child adoption case first constructs its externally owned SwitchWatcherHolder under a real scene, then retires that scene and checks its execution slot is detached. Adding a new raw SwitchWatcher now reaches the intended missing SceneObjHolder owner error; the test verifies that exact error, unchanged empty membership, and unchanged registration count. This avoids accidentally accepting constructor-registration failure as proof of adoption cleanup.

Only the two test files were changed. Scoped `git diff --check` passed. No production change, global build, runtime, decomp/index mutation, or commit was performed by this agent. Root owns rebuild and execution.

Frozen SHA-256:

- `tests/NameObjGroupLifetimeTests.cpp`: `4f0b1c5572501820fb62876bc8d48091a9618ae2f6c872da3eefeca8cc85aa3b`
- `tests/SceneObjHolderRealOrAbsentTests.cpp`: `ec4492b3449255908bc6fd5e2d9da8d44ca3e0ad25146a6a38537efeda7d5362`
