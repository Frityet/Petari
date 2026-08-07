# Demo runtime: real scene ownership or explicit absence

Date: 2026-08-07 UTC

## Boundary audited

- `src/compat/DemoCompat.cpp`
- `src/compat/DemoUtilCompat.cpp`
- `src/compat/DemoUtilCompat.hpp`
- `src/compat/DemoSceneRuntime.cpp`
- `src/compat/DemoSceneRuntime.hpp`
- `tests/AuroraNativeTests.cpp`
- `tests/DemoSceneRuntimeTests.cpp`

No file under `src/Game` was changed for this slice.

## Decompiled behavior used as authority

The decisions were checked against:

- `../src/Game/Util/DemoUtil.cpp`
- `../src/Game/Demo/DemoDirector.cpp`
- `../src/Game/Demo/DemoFunction.cpp`
- `../src/Game/Demo/DemoExecutor.cpp`
- `../src/Game/Demo/DemoExecutorFunction.cpp`
- `../src/Game/Demo/DemoTimeKeeper.cpp`
- `../src/Game/Demo/DemoSubPartKeeper.cpp`
- `../src/Game/Demo/DemoActionKeeper.cpp`
- `../src/Game/Demo/PrologueDirector.cpp`

Key source facts:

1. `DemoDirector::mIsActive`, its current executor, starter, and current name are scene-director state. They are not process globals.
2. `tryStartDemoWithoutCinemaFrame` and `tryStartDemoMarioPuppetable` are programmable-demo starts. They do **not** select or start a DemoSheet executor.
3. `PrologueDirector` uses those programmable starts for `プロローグデモ` and `主人公ピーチ城に到着`.
4. Time-keep starts use a real `DemoExecutor` and its Time/SubPart keepers.
5. Missing `DemoDirector` ownership is not one of retail's ordinary `false` refusal paths. Retail dereferences the scene director.
6. Required `void` registration/pause/resume entry points do not define a successful missing-owner/no-op fallback.
7. Direct step/total queries assume a real active keeper and known part; the previous `-1`/`0` normalization was PC-only.

## Changes

- Removed the process-global `sIsDemoActive`, active name/owner, and puppetable-control globals.
- `DemoSceneRuntime::Impl` now owns the active definition, starter, and puppetable player-control lease.
- Natural end, explicit end, owner release, and scene destruction clear only that runtime's state.
- Added a required-runtime accessor. Missing scene ownership now raises an explicit error instead of returning `false`, `0`, `-1`, `nullptr`, or silently doing nothing.
- Kept `false` for real lookup/capability/refusal cases: missing DemoGroup, missing actor membership, missing Action capability, active-director refusal, and missing/empty Time data.
- Required registration, end, pause, and resume operations now raise when their real target is absent.
- `DemoSceneRuntime` binds a real scene `WipeService` at construction (explicit injection or the installed `RuntimeContext` owner). A dispatchable Wipe row now raises if that owner is absent rather than disappearing.
- Programmable demo starts are explicitly unavailable until the real scene-movement/cinema-frame/simple-cast director closure exists. They are no longer routed through an unrelated DemoSheet with the same name.
- Real Time/SubPart sheet starts, clocks, Action dispatch, Wipe dispatch, registered-cast lookup, and puppetable ownership remain intact.

## Prologue/picturebook consequence

The exact `PrologueDirector` call path remains in Game source. It now stops honestly at the missing programmable `DemoDirector` closure. This does not claim that a DemoSheet start advanced the picturebook. Once the generalized programmable director services exist, the unchanged Game call path can proceed.

## Focused regression coverage

The 18-case `DemoSceneRuntimeTests` target now distinguishes:

- no installed director (explicit missing owner),
- installed director plus genuine try-operation refusal (`false`),
- required operation with a missing target (exception),
- unsupported programmable demo versus a working real Time sheet,
- independent active state in nested installed scene runtimes,
- no invented step/total sentinels,
- unrelated-actor pause/resume rejection,
- scene teardown with no process-global state leak.

The Aurora-native demo-cast boundary test also expects a missing scene-owned
DemoDirector runtime to raise explicitly. It no longer treats that absent owner
as a normal `false` result.

## Verification

The four changed translation units passed standalone C++23 syntax checks with the project's PC defines and compatibility preinclude:

```text
src/compat/DemoSceneRuntime.cpp: exit 0
src/compat/DemoUtilCompat.cpp:   exit 0
src/compat/DemoCompat.cpp:       exit 0
tests/DemoSceneRuntimeTests.cpp: exit 0
```

The first aggregate focused build was temporarily blocked by the concurrent exact Layout core migration (`LayoutCoreUtil.hpp` / `nw4r/lyt/drawInfo.h` not yet present). After that shared boundary landed, the focused target linked and all 18 cases passed:

```text
$ xmake build smg-pc-demo-scene-runtime-tests
build ok, spent 6.722s

$ xmake run smg-pc-demo-scene-runtime-tests
18/18 tests passed
```

The aggregate Aurora-native test binary passed after updating its old fallback
expectation:

```text
$ xmake build smg-pc-aurora-native-tests
build ok, spent 0.184s

$ xmake run smg-pc-aurora-native-tests
26 Aurora-native test(s) passed
```

The complete application target also linked:

```text
$ xmake build smg-pc
build ok, spent 12.527s
```

The title-path smoke run used the real RMGK01 disc and an X server:

```text
$ env SMGPC_EXIT_AFTER_FRAME=30 SMGPC_FRAME_PACING=0 \
    xvfb-run -a build/linux/x86_64/debug/smg-pc \
    --disc /workspaces/pcport/RMGK01.iso
```

It opened the disc, initialized Aurora and `RuntimeContext`, loaded the Korean message/particle resources, and resolved the title layouts. The first explicit missing owner was then:

```text
[src/app/main.cpp:56][APP - FATAL] Uncaught exception Effect deletion requires a registered effect keeper.
```

No DemoSceneRuntime ownership/default error occurred before that boundary. No fallback was added for the unrelated missing effect keeper.
