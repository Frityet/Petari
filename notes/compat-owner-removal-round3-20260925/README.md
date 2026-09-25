# Compat removal: sensors, JKernel, J3D geometry, and save serialization

This batch removes another 25 files from the initial `src/compat/` inventory. 98 of the original 330 are now removed; 232 remain. Total removal remains the top objective. This is an incremental checkpoint, not completion of that objective or the Gateway gameplay goal.

## Changes and ownership

- Enable the unchanged original `Game/Util/ActorSensorUtil.cpp`; delete four sensor/broadcast compat files. `LiveActor`, `HitSensorKeeper`, `HitSensorInfo`, `HitSensor`, and `SensorHitChecker` own their storage. The remaining registry hook only retires borrowed contact/queued-message references. `MsgSharedGroup` stops dispatch after a callback cancels its queued sender. MarioMessenger's separate borrowed-endpoint lifetime gap remains documented in the sensor report.
- The first real run exposed a null sensor keeper in `NPCActor::initialize` reached through Rosetta. Restoring the current upstream block (add the Body sensor only inside `if (rCaps.mSensor)`) fixes the real cause. No automatic sensor allocation was reintroduced. A regression covers both capability states.
- Consolidate four heap implementations, allocation provenance, and finalizers in canonical JKernel owners; remove ten compat files. Replace diagnostic forwarding wrappers with existing OSReport/OSPanic. SDK owns the current-heap mutex and restoration scope; original MR/J3D callers share that same mutex. Game budgets do not move into the SDK.
- Restore six complete J3D geometry/shape/matrix/vertex owners, including displaced original lifecycle and copy methods. Preserve full native pointers, packed data byte order, heap allocation, and explicit matrix arithmetic. Six compat files removed.
- Restore `BinaryDataChunkHolder`, `ConfigDataMisc`, and `SysConfigFile`; delete those three replacement providers and both SaveChunkEncoding files. Save chunk serializers now emit/consume Wii bytes directly through explicit packed-scalar JSU helpers or Aurora endian operations. Raw JSU reads/writes retain native-byte semantics. PLAY's legacy partial-read/default behavior is preserved.
- Replace signature-switch payload conversion with virtual chunk validation and the actual binary-content accessor's schema validation. The container preflights every chunk before changing save state. The original SYSC header serializer runs again; its null-pointer/manual-table replacement is gone. FLG1, PCE1, SPN1, VLE1, GALA, MISC and SYSC own their format checks.

## Evidence

`validation.json` records 15 passing focused targets. They cover real heap retirement and exceptions, overlapping normal-matrix copy, J3D resources from the actual disc, all sensor bindings, queued sender deletion, scheduler categories, and save serialization. The full save-owner test runs in the actual GameSystem, tests all six game-data chunks, 188 event flags and 14 story thresholds, checks GALA schemas/golden bytes, rejects malformed later chunks before earlier state changes, then restores the original current/backup files. Configuration tests retain the Dolphin byte oracle and test reordered, unknown, duplicate, missing, overlapping and unaligned fields plus signed timestamps. PLAY checks every short prefix, null/oversized buffers, high scalar bits and life resupply.

`gateway-fixed-600.json` verifies a fresh-save real-disc Metal run: 600 completed original GameSystem frames, exit 0, no timeout, retired process, unchanged binary. `gateway-fixed-600-frame480.png` was visually inspected and shows Mario/Luma in the wakeup cutscene. This does not prove Rosalina's encounter or Grand Star completion. The initial failed run and LLDB stack are retained to document the NPC correction.

The actual archive-provider audit reports zero duplicate strong providers, zero stale source providers, and zero unavailable owner inputs. Its broad approval gate remains red: the existing reviewed-provider mapping is incomplete (7,862 unreviewed symbols), and older anchored checks include 11 unresolved and 2 differing entries. These are not reported as passing.

Two older fixtures remain blocked by pre-existing removed APIs: ActorRuntimeRegistryTests references obsolete model/clipping/shadow helpers; GameActorPhysicsRealOrAbsentTests includes the already-deleted ActorPhysicsRuntime.hpp. Their sensor-specific references were updated without reconstructing obsolete compatibility APIs. All affected production code builds; the direct sensor, full scheduler, broadcast and real-process tests pass.

## Publication boundaries

The working tree began with unrelated staged notes, source changes, deleted scene services, and dirty tests. Code was staged in a separate temporary Git index. Only the two required target dependencies were applied to the committed `tests/xmake.lua`; its other existing edits are preserved. The original index patch is checked before and after publication. Submodule pointers and unrelated dirty files are excluded. Local baseline snapshots and NAND/cache data are not published.
