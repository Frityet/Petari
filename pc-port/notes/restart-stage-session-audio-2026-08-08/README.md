# RestartCube stage-session and audio evidence

This evidence covers the PC compatibility services used by the byte-identical
`Game/AreaObj/RestartCube.cpp`. Host-owned state lives in `src/compat` and
`src/runtime`; the retail actor is not patched and its factory remains disabled.

## Real-disc metadata proof

- Disc revision used by the strict probe: RMGK01.
- Retail archive: `/StageData/HeavensDoorGalaxy/HeavensDoorGalaxyScenario.arc`.
- Resource: `/ScenarioData.bcsv`.
- The row whose `ScenarioNo` field hash is `0xed08b591` has value `1`.
- Its `Comet` field hash is `0x03e441d0`, and the decoded field is an explicit
  empty string. The resolver therefore proves `StageCometType::None`; it does
  not substitute a default for missing metadata.
- `RestartStageSessionTests.cpp` opens the real disc image through Aurora DVD,
  parses this archive/BCSV through the production resolver, and asserts the
  explicit no-comet result.
- The exact retail `AudStageBgmWrap` maps `Game` + `HeavensDoorGalaxy` to
  scenario group 2. Its scenario-1 entry is `-1`, so the initial audio state is
  proven to have no current stage BGM. It remains distinct from an unknown ID.

Unknown archives, rows, fields, comet strings, unbound sessions, unresolved
player-death state, and unresolved BGM identity all raise explicit errors.

## Lifetime and facade checks

The focused test covers:

- immutable initial versus mutable restart `JMapIdInfo`;
- all five retail comet predicates, including resolved Purple metadata;
- explicit Power Star get-demo and player nerve/death state;
- exact stage-table initialization and raw sound-ID identity;
- a genuinely constructed retail-shaped `AudBgmMgr`/keeper/BGM chain;
- exact RestartCube selection of `MBGM_GALAXY_25` for slot 1;
- current ID, track/cube state, and facade synchronization;
- reset-before-bind teardown and a clean second stage lifetime;
- rollback of a failed nested audio binding without a dangling thread-local
  service or stale retail manager state;
- endian-correct `JAISoundID` composite construction and round trips.

## Verification

Run from `pc-port/` on 2026-08-08:

```text
$ xmake -j 1 smg-pc-restart-stage-session-tests
build ok
$ ./build/linux/x86_64/debug/smg-pc-restart-stage-session-tests
Restart/stage-session tests passed: 7/7

$ xmake -j 1 smg-pc
build ok

$ git diff --check
(no output)
```

The AreaObj-focused suite was independently rerun by the AreaObj owner and
reported 6/6 passing after the same providers linked.

See `exact-game-sha256.txt` for byte-identity evidence and
`service-api.txt` for the host integration surface.
