# OriginalProcess CollisionArea integration — 2026-09-19

This file records the completed execution gate separately from the earlier read-only activation audit. Original CollisionArea placement support is now enabled.

The debug-only `OriginalGameDebugObserver` is an explicit optional argument to `run_original_game`. Its callback runs after a completed original frame, within the actual process's guest execution and allocation scopes. There is no actor-specific environment switch or production actor selection logic. Ordinary callers supply no observer; release builds omit this API.

`tests/OriginalProcessCollisionAreaTests.cpp` boots the same OriginalProcess through its normal GameSystem/scene sequence with a fresh temporary native console and real disc. Once the requested original GameScene finishes initialization, it scans the actual StageDataHolder placement records and uses each authored CollisionArea cube form. Each temporary AreaPolygon uses the real scene allocation domain, placement zone, Mario mode, sensor group and CollisionDirector; construction and actor retirement happen synchronously before another gameplay frame.

The test exercises all six original `setSurfaceAndSync` writes for both forms. It checks authored dimensions/axes, stable actual part/server/prism/surface identities, original KCL traversal against the native spatial index, original invalidate/validate membership and actor retirement. After `run_original_game` returns, it verifies normal scene-cache teardown released the generated typed-resource identities. This is an API integration test under actual process owners, not evidence of natural switch activation or rabbit/Rosetta progression.

Initial `--probe-only` mode expects no advertised CollisionArea factory support; the normal mode will require exactly two authored original placements and their unchanged off appearance switch. The descriptor will be added only after the initial probe executes successfully.

Initial execution: `process-probe-build1.log` stopped on the independently added trace's unavailable `dolphin/kpad.h` include. After its owner corrected the include, `process-probe-build2.log` passed. `process-probe-only.json`/`.log` records an actual process exit 1 after 4.69 seconds: full AreaPolygon/sensor/CollisionParts creation and initial face geometry checks succeeded, then the first original arrow-query assertion failed. No factory descriptor was enabled. The process retired normally on that test exception.

`process-probe-build3.log` compiles additional query diagnostics. Despite Xmake's help text, its `--shallow` option still refreshed changed dependency sources here, including Aurora's command processor and layout files being edited by another agent. This binary is diagnostic only, not the same source snapshot as the parent's independently running baseline. Future builds use explicit coherent source freezes and normal dependency builds.

`process-probe-build4.log` rebuilt normally under explicit coherent Aurora/layout freezes. `process-query-diagnostic.json`/`.log` then resolved the initial failure as a test expectation error: the original generated octree visits the same leaf four times, yielding four encounters of local prism 0 at fraction 0.5. Direct original server traversal reports the same sequence, direct prism 0 intersects while prism 1 misses, and the native nearest-hit query agrees on identity/fraction. The test now preserves and compares the original encounter sequence; collision code is unchanged.

The authored-placement check also distinguishes diagnostic cached geometry (`surface(parts, prism)`) from enabled query membership (`surface(global_id)`). Disabled real placements must retain the former, exclude the latter, and be absent from both original/native filtered line queries.

`process-probe-build5.log` rebuilt under the next explicit coherent Aurora/layout freeze. `process-probe-corrected.json`/`.log` records **PASS**, exit 0, 360 completed original frames in 23.77 seconds. At frame 56, both real placement forms completed all six mutations, preserving each actual part/server/prism identity and matching original/direct/service encounter sequences with native query geometry. Actual actor retirement passed before gameplay resumed; after normal process teardown, the scene service and all retained generated typed-resource identities were gone. Encounter multiplicities ranged from one to four and were preserved.

After that gate passed, `src/scene/AreaObjRuntime.cpp` gained only the CollisionArea include and the exact center-origin cube placement descriptor (manager order 61, capacity 0x40).

The first activation build stopped in an independent new pointer utility because native TVec2 lacked the donor's subInline method. Its owner added the general value-returning geometry method; no CollisionArea workaround was added. `process-activation-build2.log` then built both the diagnostic target and main application successfully in 65.06 seconds under coherent source freezes. `process-activation.json`/`.log` records **PASS**, exit 0, 360 completed frames in 23.55 seconds. At frame 54 the diagnostic verified exactly two original factory-created CollisionAreas, their shared forms, actual sensor groups, two-prism parts in authored zone 5, unchanged NoInit defaults, and SW_APPEAR 1015 still off. Both original and native collision queries excluded these disabled placements. All twelve temporary face writes and actor-retirement checks passed again; normal scene teardown retired the actual two placement resources and both temporary resources.

Test binary SHA-256: `d59e421b793a765d1ce2b1aee9264b4db0ce9b437ae6037e8a5bb448931b089b`. Main binary built in the same snapshot: `05652f4f8634c25074d03c4eb5ff9c5e602b28c3010419b7fe32f2698f7fe1bd`.

Natural story switch activation and rabbit/Rosetta progression remain outside this diagnostic's claims. The real placed actors' switch and geometry were not changed by the probe: all twelve API mutations belonged to separately owned temporary polygons, constructed and retired before another gameplay frame. The old standalone RuntimeOwned probe remains historical blocked evidence; it was not repaired with substitute FileLoader or player owners.

## Change and publication scope

- `src/scene/AreaObjRuntime.cpp`: exact original one-row placement registration and include.
- `tests/OriginalProcessCollisionAreaTests.cpp`: actual process/six-face/retirement and authored switch-off integration coverage.
- only the new `smg-pc-original-process-collision-area-tests` target at EOF of `tests/xmake.lua`; preserve its unrelated existing debug-path hunk.
- this integration note, the updated directory README, and `process-*` build/runtime logs and JSON evidence.

The generic explicit observer in `src/app/OriginalGameApplication.hpp/.cpp` was already published by the parent in checkpoint `7f40292d7`. This activation changes no Game source. Independent in-progress layout, Aurora and pointer work was held coherent during builds and is not part of the CollisionArea source diff. No index, commit or push was performed by this subtask.
