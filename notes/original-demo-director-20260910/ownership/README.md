# Native lifetime owner for original DemoDirector

`src/compat/DemoDirectorOwnership.hpp/.cpp` adds storage ownership around the complete original Demo graph. It does not execute demo clocks, dispatch actions, replace membership tables, or own/delete NameObjs. Parent wires it into actual SceneObj construction, placement initialization, NameObj retirement and scene teardown.

## Required ordering

- Construct the native service on the host. Call `capture(DemoDirector&)`, `capture_cast_group(DemoCastGroup&)` and `capture_executor(DemoExecutor&)` under the corresponding real JkrAllocationScope. Capture is repeatable, including constructor-before-init followed by successful initialization. Metadata is host allocated and retains that actual allocation domain.
- `prepare_retirement()` runs while original Director/Executor objects remain alive, before their typed NameObj destructors. It refreshes published child pointers and copies the actual fixed-capacity talk-controller array without allocating. `prepare_rollback(marker)` prepares only captured roots in that original registration suffix.
- The scene binding destroys its NameObjs in its normal order. `reclaim()` then deletes the prepared non-NameObj graph before scene domains are released. Prepared records are removed; repeated reclaim is harmless. No original gameplay callback is invoked to perform teardown.

The scene owns Director, both DemoCastGroupHolders, request proxy, Executors/SubGroups and their LiveActorGroups. ResourceHolderService retains the Demo archive/ResourceHolder. Native JMapInfo already derives NativeJkrDisposer, so its actual JKR heap disposer list destroys parser C++ caches/shared ownership at domain retirement. The new service retains that domain; it does not add duplicate parser or archive ownership. Unpublished partial-constructor raw storage remains governed by original arena disposal; nontrivial JMap state has its existing disposer registration.

## Reclaimed graph

- DemoSimpleCastHolder uses its actual generated destructor, including all three AssignableArray buffers.
- DemoStartRequestHolder owns its sixteen current DemoStartInfos; its registered proxy remains scene-owned.
- Cast groups' JMapIdInfo values are reclaimed separately from their scene-owned LiveActorGroups.
- Time/SubPart/Player keepers and their actual row arrays are deleted.
- Camera keeper generated executor[part] names, ActorCameraInfo values, row array and keeper are deleted. Borrowed actors and parser-owned names are not deleted.
- Action keeper deletes current cloned functors through the actual virtual FunctorBase destructor, parallel cast/functor/nerve arrays, ActionInfo rows, pointer array and keeper. Nerves and cast actors are borrowed.
- Wipe/Sound keepers use their actual generated destructors, preserving their multiple-inheritance layout and AssignableArray ownership.
- Talk controllers own their ActorCameraInfo and run the actual NerveExecutor destructor for Spine.
- Executor StageSwitchCtrl owns the four optional SwitchIdInfos and each ID value.

Original allocations that were overwritten and intentionally unreachable under the retail arena model remain arena-reclaimed. This service snapshots the actual currently published graph and does not intercept or rewrite Game allocation expressions.

## Early borrowed-actor retirement

Parent calls `release_name_obj` before the runtime NameObj identity is removed. The service compacts the actual three SimpleCast vectors, Executor clipping-restoration list, talk-message/controller arrays, and parallel action arrays. It clears camera target and starter pointers, destroys removed owned talk controllers/functor clones, and removes queued start records borrowing the retired identity while preserving surviving request order. Generic NameObjGroup membership is already compacted by ActorRuntimeRegistry and is not duplicated here.

`simple_cast_registration_count` reads those original vectors directly (one identity, or total with null). The only Game header difference is a TARGET_PC-gated friendship declaration in DemoSimpleCastHolder.hpp for this owner. It adds no field, virtual method or runtime behavior; the original private arrays remain authoritative.

## Validation and boundary

One isolated LLVM23 native full-TU compile succeeded: exact command, source hash and output are in `native-compile.json` / `native-compile.log`. No root Xmake, extra test matrix, or commit was performed. Parent is building the coherent integration and owns linked/runtime validation. This note does not claim an executed original DemoDirector scene yet.
