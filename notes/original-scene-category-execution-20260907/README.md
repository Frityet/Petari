# Original scene category execution boundary — 2026-09-07

The native scheduler now exposes one-category movement, animation and draw-buffer view entry. `CategoryList` uses these methods from `src/compat/SceneMovementCompat.cpp`; complete original SceneExecutor can therefore choose its own order without invoking the native aggregate frame loop repeatedly. No Game source bodies changed.

## Boundary and ownership

- `begin_frame()` clears only the diagnostic execution trace. The scene owner calls it before the original scene update; it does not select a nerve or dispatch objects.
- `execute_movement_category(category)` dispatches only matching current registrations. The current native clipping evaluator runs at ClippingDirector, after the Camera category can publish its updated pose. The current native contact computation runs only at SensorHitChecker; player movement consumes those contacts through its existing real keeper.
- `execute_calc_anim_category(category)` dispatches one animation category independently of movement. The original SceneExecutor can run the four collision-object animation categories before CollisionDirector. Animation continues to use the original direct-calcAnim rule rather than the movement-off flag.
- `entry_draw_buffer(camera_type)` enters the existing original DrawBufferHolder using the caller-selected view matrix. CategoryList opaque/translucent methods call the same actual holder, without a fabricated camera argument.
- Existing host scenes retain their aggregate entry point. Movement now uses the same category dispatch, including camera-before-clipping and one sensor phase. That host aggregate still has its own aggregate animation phase; the full original collision animation interleave belongs to original SceneExecutor activation.

Per-entry helpers retain the existing object-kind dispatch, selected scene Game allocation domain, J3D context guard, callback heap lease and monotonic registration identity. A callback may delete another object, reallocate registration storage, or remove/re-register the same address; the old snapshot is revalidated before access. New registrations join the next category call, not an already-running batch. Category snapshots, traces and other native metadata use host storage, and nested/throwing original callbacks restore their caller's heap and J3D state.

The current native ClippingDirector factory is active, but its compatibility `movement()` explicitly has no second clipping evaluator. The original SensorHitChecker TU and factory are not active. Thus these saved category hooks do not duplicate an existing original judge/contact pass. They do not claim full original clipping groups or sensor-group pairing: the existing native sphere/frustum and contact algorithms are preserved, with their full original ownership still a later closure. When those actual owners are activated, transfer execution to their registered movement callbacks and retire the corresponding native category hooks together.

## Proof

`native-syntax-results.json` records exact commands, hashes and four successful LLVM 23 native syntax probes: SceneScheduler, SceneMovementCompat, ClippingDirectorCompat and extended SceneSchedulerHeapTests. `git diff --check` passes for this cohort. No root Xmake was run by this agent.

The existing `smg-pc-scene-scheduler-heap-tests` target now also checks:

- caller-selected movement/animation interleave and no unrelated animation or aggregate replay;
- movement-off versus animation behavior;
- deletion and same-address re-registration during a category batch;
- nested category J3D and heap restoration after an exception;
- no implicit contact recomputation in player, animation or collision categories;
- all callback allocations reclaimed with the scene domain.

The independent camera agent is adding a real CameraContext/frustum regression to OriginalCameraDirectorTests: a test Camera-category NameObj updates the actual owned camera context before the ClippingDirector category. That specifically validates scheduler publication order, not a full original CameraDirector movement without its Mario dependency. Its source and evidence belong to the camera agent's manifest.

Runtime results are pending the parent's coordinated build. The new test source compiling is not a gameplay or full SceneExecutor runtime claim.

## Next owner: original execution requirements (read-only audit)

`decomp/src/Game/NameObj/NameObjExecuteHolder.cpp` is complete. The original SceneObj factory creates capacity 4096. Each NameObjExecuteInfo owns separate movement and draw states: 1/2 are pre-initialization connection choices, 3 is connected, 4 pending connection, 5 disconnected, 6 pending disconnection and 7 delayed disconnection. `initConnectting()` publishes initial memberships after fixed buffers are allocated; the five requirement methods apply the requested transitions at the explicit original SceneExecutor boundaries. Immediate temporary draw toggles in current NameObjExecuteCompat do not provide these queue semantics.

The original `SceneNameObjListExecutor` is also complete. It constructs movement/calcAnim/draw NameObjCategoryLists and the DrawBufferHolder from the original capacity tables. The existing native NameObjListExecutor/NameObjCategoryList TUs provide the actual data operations, but Scene's list pointer remains null and SceneDrawBufferService privately owns only a draw-list/holder executor. Importing the requirement holder alone would dereference the missing GameSystem/controller executor and would not control native movement registration.

A coherent next activation should:

1. Own one genuine SceneNameObjListExecutor and the genuine NameObjExecuteHolder; expose typed executor access through the active scene boundary until the real GameSystem controller exists.
2. Transfer/share the current actual draw holder and retained model metadata with that executor. Do not allocate a second holder whose registrations diverge.
3. Route original registration, appear/death and temporary connect/disconnect calls through the original requirement holder. The original `connectToScene(LiveActor*)` only registers; the NameObj overload also requests initial movement/draw connection. Preserve that distinction.
4. Dispatch the real lists with host callback lifetime/domain guards around their execution. Original category deletion uses swap-with-last and therefore changes encounter order; retaining only the native insertion-order vector would miss that behavior.
5. Retire the parallel native immediate flags only with the corresponding entrypoint transfer. Test request-connect/disconnect reversal, delayed draw removal, initial dead actors, category order after removal, failed construction rollback, and scene retirement before enabling whole original SceneFunction.

These queue-owner changes are an audit, not implemented in this checkpoint. Full SceneExecutor still requires them plus original StopSceneController/SceneNameObjMovementController and the remaining scene providers listed in the preceding GameScene activation audit.
