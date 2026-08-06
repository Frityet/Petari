# DemoRabbit PC integration

## Result

This checkpoint supplies the construction and registration substrate for all three `DemoRabbit` placements in `HeavensDoorGalaxy` through the normal Game factory and placement lifecycle. The placement iterator controls both archive selection and actor initialization: cast 0 loads `TrickRabbitBaby`, registers cast 0, receives the real five-point `CommonPath`, and creates its talk controller; casts 1 and 2 each load `TrickRabbit`, register their own cast IDs, and do not synthesize rails or messages.

The title, file-select, and five-page prologue picturebook sequence remains the entry path. A real-disc `gateway_handoff` run followed that full route into `HeavensDoorGalaxy` scenario 1 and passed at frame 10350.

## Source closeness

These PC-port files are byte-identical to the regular decompiled tree:

- `Game/NPC/DemoRabbit.cpp`
- `Game/NPC/DemoRabbit.hpp`
- `Game/NPC/NPCActor.hpp`
- `Game/NPC/TalkMessageCtrl.hpp`
- `Game/Util/TalkUtil.hpp`

The SHA-256 pairs are recorded in [`artifacts/source-identity.tsv`](artifacts/source-identity.tsv). The independent whole-tree audit after this import reports 267 Game files: 59 exact-source, 8 compile-only, 198 compat-temporary, and 2 decomp-needed. The exact-source count rose from 54 to 59. No root decomp file was changed for this PC integration, and root `NPCActor.cpp` was deliberately not copied.

The root reconstruction evidence is in [`../demo-rabbit-decomp-20260806T200318Z/README.md`](../demo-rabbit-decomp-20260806T200318Z/README.md): RMGK02 `DemoRabbit` has all 48 code symbols mapped, 32 exact functions, and a 4,668-byte `.text` match of 99.46015%. The complete source-built RMGK02 DOL also passes its configured SHA-1 check.

## Compatibility boundary

General host behavior lives outside the exact actor source:

- `compat/NPCActorCompat.cpp` supplies the reusable NPC base surface needed by the exact header without importing the much larger root `NPCActor.cpp` dependency graph.
- `compat/DemoCompat.cpp` registers casts and named nerve/functor actions from placement metadata.
- `compat/TalkCompat.cpp` supplies the minimal generalized talk-controller runtime. `compat/ActorRuntimeRegistry.cpp` coordinates scene-owned talk/demo teardown keyed by the host actor, and demo re-registration replaces stale action maps.
- `compat/GameActorPhysicsCompat.cpp`, `compat/GameActorSensorCompat.cpp`, `compat/GameMathCompat.cpp`, and `compat/GameMapCollisionCompat.cpp` supply movement, the original NPC Body-sensor type, quaternion/math, gravity, and map-query boundaries used by the actor.
- The Game factory has a placement-aware archive callback table. The scene lifecycle passes the actor's actual `JMapInfoIter` into that table before construction, so it does not guess from stage, zone, row, route, or path constants.

There are no `HeavensDoorGalaxy`, zone-ID, placement-row, or rail-path special cases in the actor integration.

## Verification

Commands run from `pc-port/`:

```text
xmake build smg-pc-aurora-native-tests
xmake run smg-pc-aurora-native-tests
xmake build smg-pc
xmake aurora-route-smoke --disc=../RMGK01.iso --work-dir=notes/demo-rabbit-pc-20260806T210100Z/gateway --no-build gateway_handoff
```

Results:

- Native build and link passed with the exact `DemoRabbit.cpp` in `smg-pc-game`.
- 19/19 Aurora-native tests passed. The new tests construct three placement rows through the factory, check baby/adult archive selection, verify the cast-0 five-point rail and compat-owned talk controller, verify no rail/talk synthesis for casts 1 and 2, exercise teardown plus a fresh revisit registration, and ensure the generic stage host does not revive a placement actor that initialized dead. See [`artifacts/native-tests.log`](artifacts/native-tests.log).
- The complete `smg-pc` target built successfully.
- The rerun route manifest passed and records both new assertions: exactly three created `DemoRabbit` objects, and exactly one created `DemoRabbit` with an attached five-point rail. See [`gateway/manifest.json`](gateway/manifest.json) and the compact [`artifacts/gateway-summary.json`](artifacts/gateway-summary.json).
- Real-disc semantic events show `TrickRabbitBaby`, `TrickRabbit`, `TrickRabbit` loaded in placement order and demo cast IDs 0, 1, 2 registered. See [`artifacts/gateway-demo-rabbit-excerpt.log`](artifacts/gateway-demo-rabbit-excerpt.log).
- The full placement report records rows 8, 9, and 10 as `created`; row 8 has `CommonPath_ID=0`, five rail points, and first point `[15472.2, -12853.1, 6202.44]`. See [`gateway/gateway_handoff/gateway_handoff-placement-report.md`](gateway/gateway_handoff/gateway_handoff-placement-report.md).
- [`gateway/gateway_handoff/gateway_handoff-frame-10350.png`](gateway/gateway_handoff/gateway_handoff-frame-10350.png) is the 640x480 handoff capture (`nonblack_ratio=0.980612`, 563 render packets). It proves the full sequence reached a rendered gateway-stage frame; placement and semantic evidence, rather than this camera view, proves the rabbit instances and archives.

The full SQLite trace is retained locally at `gateway/gateway_handoff/gateway_handoff-frame-10350.trace.sqlite` for inspection. It is roughly 29 MB and should not be force-added when selecting the compact note artifacts.

## Current host limitations

- The host stage service does not yet expose a map-collision triangle world. The generalized map-query provider therefore reports no hit; `LiveActor::movement()` also does not yet apply velocity through an original-style binder. Rabbit registration, rails, talk state, demo actions, and movement calculations run, but the rabbits cannot yet physically run over terrain, bind to ground/walls, or reach their rail goals as in the original game.
- The compatibility demo registry supports placement-backed cast registration and dispatches the initiating named part, but it is not a reconstruction of the original data-driven demo timesheet director. Consequently it does not automatically advance to the later fade-in, talk, and runaway cast actions or end the demo; after `startTimeKeepDemoMarioPuppetable`, demo-active state remains set until an external end request. A route-specific frame timer was deliberately not invented here.
- Talk flow is a minimal host substrate: message/proximity state exists, but the original TalkDirector, message-area/front-facing rules, and node graph are not reconstructed (`inMessageArea()` currently accepts every position).
- BCK timing is provisional: `checkPassBckFrame()` uses a synthetic 30-step nerve cycle because the host animation player does not yet expose the original frame-crossing query. Run/change sound timing is therefore not parity-grade.
- `isOnGameEventFlagEndTicoGuideDemo()` currently returns the correct first-visit/unset state because the host save service does not yet expose that story-event bit.

These limitations are generalized subsystem gaps; none is implemented as a route-specific bypass in `DemoRabbit`.
