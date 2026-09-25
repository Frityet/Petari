# Demo facade deletion — 2026-09-25

All nine assigned compat files have been deleted. Their full reviewed inventory, hashes, ownership evidence and dispositions are in `audit.json`.

## Production ownership

There were no production constructors of DemoSceneRuntime, DemoSheetRuntime or DemoStartRequestOwner. Actual DemoGroup/DemoSubGroup placement already uses original DemoExecutor/DemoCastSubGroup, and actual DemoDirector uses DemoTimeKeeper/SubPartKeeper and the original typed start-request holder.

DemoSheetRuntime duplicated all seven BCSV sheet schemas and the original main/subpart clock. DemoSceneRuntime retained a duplicate placement/diagnostic model and forwarded runtime actions to original owners. Its only indirect production reference was actor retirement removing facade diagnostic identities. The real actor/request/action unlink already occurs in `ActorRuntimeRegistry::release_name_obj_runtime_state` through `DemoDirectorOwnership::release_name_obj`; that path remains intact. The four redundant DemoCompat declarations and its one retirement call were removed. DemoUtilCompat's ignored-bool endRemoteDemo forwarder had no callers.

## Necessary owner adaptation

`Game/Demo/DemoStartRequestHolder.cpp/.hpp` now owns its sixteen request records and proxy NameObj. It has a native destructor, exception cleanup during construction, noncopyable ownership, and claims the proxy so scene construction cannot also adopt it. Allocations remain in the caller's selected Game or host domain. The now-deleted wrapper previously forced host allocation for all of its children.

The coordinated narrow change in `DemoDirectorOwnership.cpp` removes its manual request-record deletion; deleting the actual holder invokes the new cleanup. The proxy was previously an unclaimed scene child; it is now claimed by the holder and deleted there. No other lifetime-manager behavior changed.

## Test changes and build wiring

Delete these two complete target blocks from `tests/xmake.lua` (parent owns xmake):

- `smg-pc-demo-sheet-runtime-tests` / `demo_sheet_runtime`
- `smg-pc-demo-scene-runtime-tests` / `demo_scene_runtime`

Their source files `DemoSheetRuntimeTests.cpp` and `DemoSceneRuntimeTests.cpp` were deleted because they exercised the removed facade APIs/clock rather than the actual original production entry graph.

Retained test changes:

- `DemoStartRequestHolderTests.cpp`: uses the actual holder directly; keeps typed identity, FIFO/ring capacity, slot reuse and pointer-width checks. Ownership checks now verify caller-selected Game allocation versus explicitly requested host lifetime, proxy claim and no orphan proxy after deletion.
- `CameraLocalUtilRuntimeTests.cpp`, `CameraViewInterpolatorTests.cpp`, `CameraViewServiceTests.cpp`, `StageStartCameraTests.cpp`: replace empty DemoSceneRuntime fixtures with original SceneExecutionFixture plus actual DemoDirector creation.
- `SphereSelectorRealOrAbsentTests.cpp`: creates actual DemoDirector directly and observes its existing simple-cast lifetime record.
- `AuroraNativeTests.cpp`: missing-scene registration checks observe actual SceneObjHolder absence instead of an unused facade registry.

The old camera fixtures constructed DemoSceneRuntime before any SceneObjHolder, although the facade already required an active scene and resource owner. These tests were stale before this batch. Their retained entry points now request real owners, but **standalone camera tests still require a full GameResourceRuntime/DemoSheet archive fixture for runtime validation**. This batch does not claim they pass. No fake inactive director or alternate resource path was introduced.

The four initially dirty test files were copied before editing into `dirty-test-baseline/`; `before.json` records their starting status/hashes. `dirty-tests-demo-only.patch` contains only this lane's additional changes to those files, preserving prior edits. SphereSelector and CameraLocalUtil were clean at entry. ActorEventCameraTests was not touched.

Source validation passed eight checks in `source-validation.json`: all nine deleted; queue methods and ring-buffer templates unchanged; holder/proxy ownership explicit; no duplicated director cleanup; real NameObj retirement preserved; no retired facade references in any src/tests CPP/header. `git diff --check` passed for modified files. No xmake changes, build, runtime execution, staging or commits were performed.

Batch is frozen for parent build/validation.
