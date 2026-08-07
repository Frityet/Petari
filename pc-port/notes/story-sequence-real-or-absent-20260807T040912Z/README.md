# Story sequence real-or-absent closure

## Outcome

The two port-owned sequence shortcuts were removed.

- `SequenceBootService` no longer broadcasts `ACTMES_AUTORUSH_BEGIN` to every live actor. The title remains present, but activation is explicitly unavailable until the real `MarioActor` auto-rush/binder event is installed.
- `SceneTransitionRequestService` no longer contains a Mario-only check or a hardcoded `PeachCastleGardenGalaxy` / `PrologueDirector` request.
- The initial move is retail `StorySequenceExecutor` move type 7.
- The after-save-load move is retail `StorySequenceExecutor` move type 6.
- The service translates the resulting `GalaxyMoveArgument` generically and notifies the retail executor only after the selected stage is active.
- The existing environment transition remains an explicit, generic external test request. It is not implicit story routing.

`pc-port/src/Game/System/StorySequenceExecutor.cpp` remains byte-identical to the decompiled source. Its canonical Game TU remains excluded by `Game/xmake.lua`; `compat/StorySequenceExecutorSource.cpp` compiles that exact file through the compatibility glob. The lexical `nullptr` compatibility is scoped to the exact source include because two decompiler expressions compare a `bool` with the Metrowerks-era integer null token.

## Compatibility boundary

The compatibility layer supplies only platform state and dependencies:

- an explicit scoped scene-state binding for the retail scene/stage/scenario queries;
- the real simplified user-file state for Mario/Luigi, story flags, Grand Star flags, and aggregate star count;
- the retail `GameEventFlagTable` data and dependency evaluation used by the new-file route;
- explicit start IDs through the byte-exact `GalaxyMoveArgument` implementation;
- comet-scheduler activation state requested by the retail executor.

Dependencies whose real backing data is absent throw an explicit unavailable error. Examples include GalaxyID-backed event predicates, aggregate-only saves that cannot answer per-star ownership, stage-result sequences, staff roll, ending movies, and dynamic story demo construction. They never return a fabricated success or a convenient false value.

The one retail flag-table function that requires embedded `GalaxyIDBCSV` is fenced at its call boundary and throws. The retail static flag table itself is compiled unchanged.

## Preserved route

The executor-backed tests cover:

1. move type 7 -> `FileSelect`, scenario 1;
2. new Mario file -> `PeachCastleGardenGalaxy`, scenario 1, with the retail Prologue demo scheduled;
3. Luigi or passed `ピーチ城浮上後` -> `HeavensDoorGalaxy`;
4. passed `クッパ襲来後` -> Peach Castle Garden start ID 1;
5. incomplete positive aggregate star data -> explicit unavailable instead of invented per-star state.

The real Prologue path still calls `MR::startPrologue()` from the exact `PrologueDirector`. The title-to-file-select binder activation is intentionally not simulated. The picturebook-to-HeavensDoor smoke transition remains an explicitly configured external transition until its retail upstream event is present.

See `verification.log` for the build and test results and `source-parity.sha256` for exact-source evidence.
