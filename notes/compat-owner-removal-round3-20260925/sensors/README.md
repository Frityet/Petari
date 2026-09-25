# Original sensor owner consolidation

Baseline: root `8490a01df`; current decomp donor is recorded in `static-verification.json`. All owned source/test files were snapshotted under `before/` before editing. Source/test edits were clean at task start; unrelated dirty source, tests, notes and index entries were preserved. No builds, xmake edits, index changes or commits were made by this agent.

## Canonical source and removals

`src/Game/Util/ActorSensorUtil.cpp` is already byte-identical to current `decomp/src/Game/Util/ActorSensorUtil.cpp` (1,033 lines). Remove its exclusion from `src/Game/xmake.lua` to compile the complete original owner, including the original all-actor broadcast loop. No donor algorithm or signature was changed.

Deleted `GameActorSensorCompat.cpp/.hpp`, `OriginalActorBroadcast.cpp`, and `SensorHitCheckerOwnership.cpp`. The compat utility had many null/no-op branches and vector-backed indexed access that differed from original preconditions and keeper access. Canonical source restores those original behaviors, including optional null-matrix handling by HitSensorInfo and the original single-sensor name shortcut. `actor_message_name` had no callers; its sole external header include in ParityTrace was unused and removed. No replacement debug mapping is necessary.

## Native ownership and borrow retirement

Actual LiveActor now owns its mSensorKeeper. It releases other runtime resources (notably collision parts that borrow sensors) before deleting the keeper. Reinitialization constructs the new keeper before retiring the previous one. Its getSensor method is the original donor body; movement calls the original tryUpdateHitSensorsAll.

HitSensorKeeper destroys its infos; HitSensorInfo destroys its sensor; HitSensor unregisters its system group membership and frees its contact array. SensorHitChecker destroys its six actual groups, each of which frees its pointer array and detaches any remaining active sensor members. These are native reclamation adaptations in the actual owners, not alternate sensor implementations. The prior negative-capacity exception moved with the keeper.

ActorRuntimeRegistry no longer owns sensors or exposes sensor initialization, registration, getter, name, collection, count, validity, or update helpers. One agreed temporary dependency remains: `retire_hit_sensor_borrows(const HitSensorKeeper*) noexcept`. It uses existing NameObj/actor identity registries without allocating new state. It cancels pending MsgSharedGroup senders, clears taking/taken borrowed links, removes all retiring contacts from other actual keepers, and clears vacated contact slots before storage deletion. This hook must move to canonical NameObj lifetime traversal when the remaining actor registry is removed; this batch does not claim complete compat deletion.

MsgSharedGroup::movement now stops its loop if native retirement cancels mMsg during an earlier recipient's callback. Otherwise the original loop is unchanged. Without this guard, later recipients would receive message -1 and null endpoints after cancellation.

Known separate gap: original MarioMessenger's queued endpoints are not covered by the existing shared-group retirement hook; its private NameObj inheritance prevents using the current NameObj dynamic_cast traversal directly. This is unchanged and is a separate owner batch, not claimed fixed here. As with the earlier implementation, deleting the currently executing actor/keeper itself from its own sensor callback is not claimed supported.

## Focused validation changes and exact semantics

- SceneSchedulerHeapTests used two ATYPE_PLAYER sensors, which the original checker never pairs, and its earlier scheduler.clear calls had disconnected the original checker. The sensor cases now reconnect the actual checker and use player/enemy sensors. They retain contact phase ordering, callback heap routing, and actor retirement assertions.
- ActorSensorRealOrAbsentTests now creates a real original GameSystem/scene/executor/clipping owner per test, checks actual keeper/info fields, and verifies every scene retires registrations and Game arenas. Contact retirement asserts both contact arrays and taking/taken links are cleared. It no longer asserts fabricated null receiver success or an absent message-sensor fallback. Optional null matrix follows the original HitSensorInfo actor-relative path. Initial group identity follows original SensorHitChecker classification.
- OriginalActorBroadcastTests retains its two lightweight scene generations with actual original process and message sensor owners. It additionally boots the original process through OriginalStageResourceProcessFixture to construct MsgSharedGroup using a real retail placement row. First recipient destroys the queued sender; remaining delivery must stop and borrowed endpoints must clear. A second case reinitializes the sender's keeper before dispatch and verifies the message is canceled. This target now requires SMGPC_REAL_DISC and adds a 120-frame process run.
- Registry and actor physics tests use actual keeper fields/MR::addHitSensor instead of removed sensor helper APIs. ActorRuntimeRegistryTests has unrelated old model/name/clipping assumptions and is not claimed fully migrated by these sensor-only edits.

Recommended root checks: `smg-pc`, `smg-pc-actor-sensor-real-or-absent-tests`, `smg-pc-scene-scheduler-heap-tests`, and `smg-pc-original-actor-broadcast-tests`. Set SMGPC_REAL_DISC for the broadcast process regression. Root runs all builds and runtime validation. Static `git diff --check` passed; removed sensor helper/provider references are absent from src/tests. See `static-verification.json` and `changes.patch` for exact scope.

## Integrated validation and NPC donor correction

Root compiled the restored sensor owner successfully. `smg-pc-actor-sensor-real-or-absent-tests` passed all 10 existing migrated cases. `smg-pc-scene-scheduler-heap-tests` passed all categories and native callback/contact retirement after correcting its expected trace to seven callbacks: the six explicit probes plus the original SensorHitChecker.

Root's first fresh game run crashed during Rosetta placement; `gateway-lldb-stack.log` records NPCActor::initialize calling HitSensorKeeper::add with a null keeper. The port had closed the mSensor-enabled branch before the Body sensor registration; current decomp correctly nests both body registration variants inside that branch. Rosetta intentionally disables this default sensor because it creates its Head and Body sensors afterward. `src/Game/NPC/NPCActor.cpp` now restores that exact donor block; no generic autoinitialization or null-result workaround was added. Its clean baseline was snapshotted before editing. An 11th focused ActorSensor case verifies disabled caps leave no keeper and enabled caps create exactly one Body sensor with the requested capacity. Root rerun is pending as this note is written.

The broadcast process regression links app helpers and therefore additionally needs `add_deps("smg-pc-app")` for only `smg-pc-original-actor-broadcast-tests`; root owns this initially-dirty tests/xmake.lua edit. Root reported that exact linker gap and is wiring it. Neither broadcast regression nor the game rerun is claimed passed yet.

Separate pre-existing legacy fixture blockers are recorded in static-verification.json: ActorRuntimeRegistryTests references removed model/clipping/shadow APIs and expects copied NameObj names/pending flags; GameActorPhysicsRealOrAbsentTests includes deleted ActorPhysicsRuntime.hpp and calls removed clipping helpers. Sensor helper calls in those files were migrated, but this task does not recreate their obsolete production APIs or claim they compile.
