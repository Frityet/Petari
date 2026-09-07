# Gateway compatibility audit — 2026-09-07

This is a source and retained-evidence audit of the flattened checkout, conducted
while the parent task synchronizes upstream. No application was built or run by
this audit. The PC port is the repository root; the original reference is `decomp/`.

## Current state and corrections to older notes

- `README.md` and `MACOS.md` explicitly record the current clean showcase failure:
  two pointer-to-`u32` casts in `src/Game/Player/MarioTeresa.cpp`. The strict main
  executable also has unresolved Game-method link dependencies. The Sept 3 native
  title/Gateway results are historical; do not reuse them as current success.
- `src/scene/nameobj/NameObjFactory.cpp` now enables actual `DemoRabbit` actors,
  ordinary catalog-selected planets, sky, air, brightness and gravity objects.
  The old Aug 7 NPC note that DemoRabbit is disabled is obsolete. The source
  tests expect three real DemoRabbit instances.
- NPCActor's actual original body is compiled through
  `src/compat/NPCActorSource.inl`; the direct source is excluded only to avoid
  duplicate definitions. Shared actor transforms, model ownership, gravity,
  animation, talk-flow parsing and stage switch/camera helpers are substantial.
  The older missing-JGeometry/missing-NPCActor prerequisite list is obsolete.
- Chase actors `RunawayRabbit`, `RunawayRabbitCollect`, `RunawayTico`, `Tico`, and
  `Rosetta` are not imported under port `src/Game/NPC` or enabled by its factory.
  `ActorStateBase` and `FootPrint` exist in the port; the two walker states and
  SpotMarkLight remain additional import leaves.
- The selected decomp branch has a partial 41-line `RunawayRabbitCollect.cpp`
  containing declaration-only gameplay methods and no `RunawayRabbit.cpp`, even
  though older notes describe their completed implementation. The retained parent
  revision `1347a481f^` and cleanup backup retain those completed sources. Restore
  proven previous work before attempting decompilation again. RunawayTico does
  contain its reconstructed guide/comment body now; compare exact provenance.

## Scope of existing native routes

`GatewayDemoScene` loads the actual scenario, placement tables, KCL, gravity,
resources and ordinary actors through common providers, then permits an external
Mario owner and explicitly reports blocked placements. This is useful subsystem
validation but it is an incomplete development scene.

`GatewaySpinCheckpoint` is a bounded host-authored checkpoint at story progress
10. It constructs `GatewaySpinRouteTico` and `GatewaySpinRosettaTrigger` as
simple LiveActors and reproduces selected route triggers around a retained
DemoSheet. It does not instantiate original Rosetta or execute the rabbit chase.
Do not expand those stage-specific trigger classes to claim the requested demo.
The requested chase begins from selected-file progress 5; completion should come
from original actor nerves, catches, grouped child completion and authored demo
switches, then original Rosetta appearance.

The strict factory retains an explicit FileSelector unavailable reason. The
showcase title/File Select route therefore also differs from running an entirely
original GameScene initialization sequence.

## Best next general closures

1. Restore a clean current application build, with pointer-sized identity through
   the host boundary and only necessary architecture fixes in Game source. Keep
   full player closure missing methods explicit rather than adding silent stubs.
2. Complete programmable demo ownership, request arbitration, movement blocking,
   Mario puppetable state, cinema frame and end/cancel semantics. Current
   `src/compat/DemoUtilCompat.cpp` throws for programmable starts and requests;
   `TalkRuntime.cpp` similarly rejects normal programmable talk-demo ownership.
   A real DemoSheet executor already exists, but is not this missing subsystem.
   This directly blocks RunawayTico comments and ordinary NPC conversation.
3. Restore/import the original NPC utility orchestration and NPC item table.
   `NPCActorRuntimeCompat.cpp` still throws for turn/reaction/talk utilities,
   NPC item parameters, joint controller creation/callbacks and joint-bound
   StarPointer targets. Existing shadow CSV and random-BCK functions now have
   real providers, so do not reimplement them from the old note.
4. Close ordinary moving-NPC leaves (walker states, SpotMarkLight, footprint
   helper), followed by Tico and then RunawayTico/RunawayRabbit/collector. Use
   source-first restoration and source-mirror checks. Factory registration comes
   after construction, child initialization, nerves, sensors, messages and
   teardown succeed. Implement general APIs, never stage-name/switch-ID branches.
5. Original Rosetta/RosettaDemoHeavensDoor integration can follow the NPC/demo
   substrate. The user only requires appearance for this milestone, not the
   post-appearance spin-unlock sequence.

## Bounded independent implementation lanes

- Decomp-only restoration of prior runaway NPC sources/headers and verification.
- General programmable DemoDirector plus TalkRuntime owner integration, with
  synthetic actors and existing real DemoSheet tests.
- NPCUtil/item-table/joint-controller provider closure using exact original
  methods and ordinary NPC tests, then a Tico import (separate files from demo).
- Moving-NPC leaf import and gravity/collision/sensor verification before enabling
  a RunawayRabbit placement. Read real stage data only in integration tests.

## Build and validation route

Root launcher: `./script/build_and_run.sh --build-only`; strict build:
`./script/build_and_run.sh --strict --build-only`. Launcher chooses Homebrew
LLVM 23, arm64, shared libc++, macOS 26 target. Do not parallelize Xmake builds.

The minimum post-sync gates are:

```sh
xmake build -y smg-pc-game
xmake build -y smg-pc-debug-path-tests
xmake run smg-pc-debug-path-tests
xmake build -y smg-pc-scene-scheduler-heap-tests
xmake run smg-pc-scene-scheduler-heap-tests
xmake source-closeness-audit --output=notes/source-closeness-current
```

Existing focused target families include `smg-pc-game-math-rotation-tests`,
`smg-pc-npc-actor-real-or-absent-tests`, `smg-pc-demo-sheet-runtime-tests`,
`smg-pc-demo-scene-runtime-tests`, `smg-pc-talk-real-or-absent-tests`, and
`smg-pc-stage-start-camera-tests`. Match tests to the subsystem changed.

After current binaries link, set `SMGPC_REAL_DISC` to the supplied root RVZ and
run `smg-pc-gateway-demo-scene-tests`, `smg-pc-mario-gateway-walk-tests`, and the
bounded `./script/build_and_run.sh gateway --verify`. Real-disc tests require
the fixture explicitly; launcher's disc discovery does not export it for them.
The spin-checkpoint test validates only its stated checkpoint.

The final gameplay gate must start from progress 5, observe the original Tico
intro and rabbit-group progression, catch the three completion groups from
four authored rabbit placements, and observe the original Rosetta appearance.
The historical grouping evidence is in prior notes, not freshly read from disc
in this audit; verify it again in the actual run. Record scene/frame traces,
errors, placement reports and visible evidence under a new notes directory.

Source parity is a separate axis: the Sept 7 flattened audit reports 737 Game
files, 469 exact and 231 with behavioral/API differences against the selected
reference. It can overstate gaps when that reference lost previous decomp work.
Re-run after restoration; do not equate a passing host test with original parity.

## Follow-up restoration completed

The decomp-only follow-up restored the collector and rabbit sources and their
matching destructor declarations from parent revision `e1985ac3a`; RunawayTico
already matched and remains unchanged. Fresh GC/3.0a3 compilation passes all
three actors. Newly split RMGK01 target objects give 97.40519%, 95.95944%, and
98.82346% text fuzzy matches respectively. See
`decomp/notes/runaway-restoration-20260907/README.md` and local `restoration/`
artifacts for provenance and limits. No port Game source was changed.
