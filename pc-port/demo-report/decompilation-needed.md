# Decompilation And Game-Code Work Needed

## Current State

`pc-port/src/Game` is still a title/file-select/prologue-focused subset. It contains no `RunawayRabbit`, `RunawayRabbitCollect`, `RunawayTico`, `DemoRabbit`, `Tico`, `Rosetta`, `HeavensDoorDemoObj`, `PeachCastleGardenPlanet`, or `MarioActor` gameplay files.

Evidence:

- Current factory supports only `PrologueDirector` and `FileSelector`: `src/Game/NameObj/NameObjFactory.cpp:15`.
- Stage placement skips unsupported factory objects instead of failing or importing them: `src/scene/StageHostScene.cpp:70`.
- Current port tree search found only file-select "Mario/Rabbit/Tico/HeavensDoor" name matches under `src/Game`, while the sibling decomp contains the expected gameplay files.
- Archive preload data already knows many HeavensDoor/rabbit archives, such as `Rabbit`, `TrickRabbit`, `RunawayRabbitCollect`, HeavensDoor map objects, `SpotMarkLight`, `Tico`, `TicoBaby`, and `TrickRabbitBaby`: `src/scene/nameobj/NameObjArchiveTable.inc:11`, `src/scene/nameobj/NameObjArchiveTable.inc:609`, `src/scene/nameobj/NameObjArchiveTable.inc:1243`.

## Route Actors Already Present In The Sibling Decomp

These should be copied/imported first, then compiled against real support systems.

| Area | Files | Why needed |
| --- | --- | --- |
| Rabbit collector | `../src/Game/NPC/RunawayRabbitCollect.cpp/.hpp` | Counts child rabbits/Ticos, links message controllers, tracks caught count, triggers Tico comments, drives the bunny-chase BGM states. |
| Rabbit actor | `../src/Game/NPC/RunawayRabbit.cpp/.hpp` | The catchable moving bunny. Depends on sensors, binders, Walker states, player/Mario demo control, talk, star pointer, shadow, light, and sound. |
| Tico actor | `../src/Game/NPC/RunawayTico.cpp/.hpp`, `Tico.cpp/.hpp` | Starts the chase, owns the post-catch comments, and hands off to the high-tower demo after the final dialogue. |
| Intro/demo bunny | `../src/Game/NPC/DemoRabbit.cpp/.hpp` | Handles the pre-chase guide/talk/fade route used by the HeavensDoor opening sequence. |
| Map objects | `../src/Game/MapObj/HeavensDoorDemoObj.cpp/.hpp`, `PeachCastleGardenPlanet.cpp/.hpp` | HeavensDoor tower/cage/planet objects, demo actions, projected-map setup, stage effects, and indirect map object behavior. |
| Rosalina boundary | `../src/Game/NPC/Rosetta.cpp/.hpp`, `RosettaDemoHeavensDoor.cpp/.hpp` | Needed if placement creates `Rosetta`; visible Rosalina demo is after the requested stop boundary. |
| Player/catch support | `../src/Game/Player/Mario*.cpp`, especially `MarioActor*`, `MarioRabbit.cpp`, `MarioAccess.cpp`, `MarioHolder.cpp`, `MarioMove*`, `MarioCollision.cpp`, `MarioActorSensor.cpp` | The demo is playable, so Mario must move, collide, trigger bunny catch sensors, and run the toss/catch animation states. |

Factory entries already exist in the sibling decomp:

- `Rabbit`, `TrickRabbit`, `RunawayRabbitCollect`: `../src/Game/NameObj/NameObjFactory.cpp:646`.
- `DemoRabbit`: `../src/Game/NameObj/NameObjFactory.cpp:786`.
- `Rosetta`: `../src/Game/NameObj/NameObjFactory.cpp:816`.
- HeavensDoor map objects: `../src/Game/NameObj/NameObjFactory.cpp:3827`.

## Systems That Must Come With The Actors

The route cannot be made correct by only adding the actor `.cpp` files. A scan of ten route files found 248 unique `MR::` calls, with 197 not present by name in current `pc-port/src`. The missing surface clusters are the actual gameplay runtime.

### Scene, Factory, And Placement

Needed:

- Replace the current tiny `NameObjFactory` with generated/original factory coverage for this route.
- Preserve and pass real `JMapInfoIter` state through actor construction.
- Support child object APIs: `getChildObjNum`, `getChildObjName`, `initChildObj`.
- Support placement links for `Path_ID`, `CommonPath_ID`, `CameraSetId`, `DemoGroupId`, `MessageId`, `CastId`, `GroupId`, and switches.
- Fail loudly on unsupported placement objects during the HeavensDoor demo path.

Current `StagePlacementResolver` reads many IDs, but the runtime still only turns flat placement rows into root objects. `RunawayRabbitCollect` directly requires child object enumeration and initialization in its `init()`.

### NPC And Map Object Bases

Needed:

- `NPCActor`, `NPCActorCaps`, `TalkMessageCtrl`, `TalkMessageFunc`, `Tico`, `RunawayTico`, `DemoRabbit`.
- `MapObjActor`, `MapObjActorInitInfo`, `MapObjActorUtil`, `SimpleMapObj`, `RotateMoveObj`, `SimpleMapObjNoSilhouetted`.
- `SpotMarkLight`, `FootPrint`, `IndirectPlanetModel`, stage effects, shadows, clipping groups, LOD controls.

This is necessary because `RunawayRabbit` uses NPC scene connection, light control, gravity, base-matrix follow target, hit sensors, star pointer targeting, binder, shadows, footprint, effects, sound, and stage-switch appear listeners.

### Player And Catch Mechanics

Needed:

- A usable `MarioActor`/player runtime with position, rotation, front vector, base matrix, animation state, control enable/disable, and collision.
- Player hit sensors and enemy/player message dispatch.
- Catch/toss demo animation hooks: `startBckPlayer`, `getPlayerPos`, `getPlayerRotate`, `getPlayerFrontVec`, `tryTalkForceWithoutDemoMarioPuppetableAtEnd`, and `requestStartDemoMarioPuppetable`.
- Basic nunchuk/analog control and jump/spin/camera input for manual bunny catching.

Current `PlayerUtil.cpp` returns an identity player matrix by default and has empty player animation hooks. That cannot support the bunny catch/toss route.

### Sensors, Physics, Collision, And Rails

Needed:

- Live hit sensor registration and pair testing.
- Player/enemy/body/catch/star-pointer sensor categories.
- `WalkerStateRunaway`, `WalkerStateBlowDamage`, and supporting actor-state helpers.
- Actor velocity, gravity, binder, ground/wall/water checks, map collision line tests, move-limit collision, and collision rebound.
- Rail/path movement for `DemoRabbit` and any route actors placed on rails.

Current `LiveActor` has position/rotation/scale, a spine, and model animation glue, but no velocity, binder, gravity vector, hit sensor holder, original model manager, rail rider, or sensor collision integration.

### Demo, Talk, Camera, And Wipe

Needed:

- `DemoDirector`/demo action registration and execution, not just "start demo returns true".
- Time-keep demo parts, demo cast IDs, demo action nerves/functors, demo part first/last-step checks.
- Talk message graph and branch navigation: `forwardNode`, `forwardNodeNextBranchLeft/Right`, `offRootNodeAutomatic`, `tryTalkNearPlayer`, force-talk APIs.
- Actor/talk/start-position/animated camera support.
- Wipes and cinema frame timing.

Current `DemoUtil.cpp` emits debug traces and returns success immediately. That will skip or collapse the route because `RunawayTico`, `DemoRabbit`, and `RunawayRabbit` use demo timing to advance states.

### Story, Flags, Switches, And Sequence

Needed:

- Direct debug boot to `HeavensDoorGalaxy` scenario 1 for the first demo.
- Later, original story transition from file select/prologue into HeavensDoor.
- Game event flags used by this route, such as `EndTicoGuideDemo` and Rosalina/Tico talk flags.
- Stage switches A/B, appear/dead/sleep switches, switch listeners, and switch writes.

Current `StorySequenceExecutor` has only file-select initial boot and a prologue request to `PeachCastleGardenGalaxy` with `PrologueDirector`. It has no HeavensDoor path.

### Sound, Effects, Shadows, And Lights

Needed:

- At minimum, deterministic stateful wrappers for BGM state calls, level sound calls, limited sounds, effect emit/delete, and rumble requests.
- For quality, actual audio playback and particle/effect rendering can come later.

The route uses BGM state and effect calls heavily. Most can be state-only for a first proof, but they should not be inert if actor state branches query them.

## What Can Be Deferred

These are after the requested stop line:

- The visible `高楼出現[デモ]` tower/Rosalina appearance sequence.
- Spin-get/PowerStar/Grand Star reward logic.
- Later Rosalina red-star/nostalgia variants.
- Full hub progression beyond the starting planet.
- Real audio playback, if stateful BGM/demo calls are enough to keep route logic deterministic.

Do not necessarily omit `Rosetta.cpp` from compilation if placement requires it. Instead, import enough for placement/init and stop before the visible Rosetta path.

## Recommended Decomp Strategy

Do not add dozens of one-off MR stubs until the route compiles. Import original-shaped modules in dependency slices and let compile/link failures reveal the next support layer:

1. Factory/placement fail-loud path for HeavensDoor.
2. Route actor source files and headers.
3. NPC/MapObj base classes.
4. Demo/talk/camera services.
5. LiveActor/sensor/collision/rail/player runtime.
6. Story/flag/switch services.
7. Visual/sound/effect polish.

Because API/ABI stability is not required, prefer changing the PC runtime to match original game expectations instead of changing imported `Game/` code to match temporary PC APIs.

## Priority-Sorted Object Decomp Plan For The Bunny Demo MVP

This section treats an "object" as either a placed HeavensDoor `NameObj`/area row or a child/runtime object family that the placed actors instantiate directly. It is intentionally sorted for an imperfect MVP first: get the bunny route to boot, talk, chase, catch, and stop at the requested boundary, then replace visual and optional gameplay fallbacks. The repo-wide file inventory remains below this section.

Important current-state constraint: after the upstream rebase, `pc-port/src/Game/NameObj/NameObjFactory.cpp` only creates `PrologueDirector` and `FileSelector`. Therefore any HeavensDoor `created` status in the captured placement report should be read as evidence of what the original factory/source can support, not proof that the current `pc-port/src/Game` has imported that object. The replacement path is still to import/decompile source-shaped `Game/` code and expand general compat services, not add route-specific constructors or hacks.

Priority legend:

- **P0:** required to make the route logic compile and advance at all.
- **P1:** required shared systems for the P0 actors to behave without bespoke route code.
- **P2:** physical stage objects needed for plausible movement, collision, switches, rails, or hazards.
- **P3:** enemies, pickups, and optional interactions that can follow the first playable proof.
- **P4:** visual, audio, guidance, and post-boundary polish.

| Priority | Object count | Goal |
|---|---:|---|
| P0 | 10 | boot and advance the bunny route logic |
| P1 | 27 | make route systems original-shaped enough to avoid hacks |
| P2 | 30 | make the stage physically plausible for play |
| P3 | 17 | restore optional enemies/items/interactions |
| P4 | 9 | restore polish after MVP behavior works |

Total object sections in this priority plan: **93** (84 placement objects plus 9 child/runtime object families).

### P0 - Import First: Demo Logic Must Run

#### Mario / MarioActor runtime

- **Priority:** P0.
- **Placement/dependency evidence:** not a root placement in the captured HeavensDoor report; required as child/runtime support for placed route actors.
- **Current decomp progress:** sibling-decomp factory/source maps to MarioActor using Mario. Source evidence: `../src/Game/Player/MarioActor.cpp`, `../src/Game/Player/MarioActorInit.cpp`, `../src/Game/Player/MarioActorSensor.cpp`, `../src/Game/Player/MarioAccess.cpp`, `../src/Game/Player/MarioHolder.cpp`, `../src/Game/Player/MarioRabbit.cpp`, `../src/Game/Player/MarioMove.cpp`, `../src/Game/Player/MarioCollision.cpp`; plus 2 more related files
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Player object family. The bunny demo becomes real only when Mario can move, collide, face, talk, and participate in the rabbit catch/toss demo.
- **Imperfect-MVP replacement path:** MVP action: import or decompile this before chasing polish. It should compile as source-shaped `Game/` code, with missing Wii services filled below it in compat/Aurora or shared runtime services.

#### NPCActor / TalkMessageCtrl

- **Priority:** P0.
- **Placement/dependency evidence:** not a root placement in the captured HeavensDoor report; required as child/runtime support for placed route actors.
- **Current decomp progress:** sibling-decomp factory/source maps to NPCActor using NPCActor; TalkMessageCtrl using TalkMessageCtrl. Source evidence: `../src/Game/NPC/NPCActor.cpp`, `../include/Game/NPC/NPCActor.hpp`, `../src/Game/NPC/TalkMessageCtrl.cpp`, `../src/Game/NPC/TalkMessageInfo.cpp`, `../include/Game/NPC/TalkMessageCtrl.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Shared NPC and message base layer. Importing route actors without this turns into one-off stubbing; this should become the general NPC/talk compatibility surface.
- **Imperfect-MVP replacement path:** MVP action: import or decompile this before chasing polish. It should compile as source-shaped `Game/` code, with missing Wii services filled below it in compat/Aurora or shared runtime services.

#### DemoDirector / demo services

- **Priority:** P0.
- **Placement/dependency evidence:** not a root placement in the captured HeavensDoor report; required as child/runtime support for placed route actors.
- **Current decomp progress:** sibling-decomp factory/source maps to DemoDirector using DemoDirector; DemoActionKeeper using DemoActionKeeper; DemoTimeKeeper using DemoTimeKeeper. Source evidence: `../src/Game/Demo/DemoDirector.cpp`, `../src/Game/Demo/DemoActionKeeper.cpp`, `../src/Game/Demo/DemoExecutor.cpp`, `../src/Game/Demo/DemoFunction.cpp`, `../src/Game/Demo/DemoTimeKeeper.cpp`, `../src/Game/Demo/DemoCastGroup.cpp`, `../src/Game/Demo/DemoCastSubGroup.cpp`, `../include/Game/Demo/DemoDirector.hpp`; plus 3 more related files
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Demo scheduler and cast grouping. It must preserve time-keep/demo action semantics so route code advances for the same reasons as the game.
- **Imperfect-MVP replacement path:** MVP action: import or decompile this before chasing polish. It should compile as source-shaped `Game/` code, with missing Wii services filled below it in compat/Aurora or shared runtime services.

#### DemoGroup

- **Priority:** P0.
- **Placement/dependency evidence:** 3 placement(s); status: ignored x3; zones: HeavensDoorMysteriousZone x2, HeavensDoorInsideZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to DemoExecutor using no fixed archive. Source evidence: `../src/Game/Demo/DemoExecutor.cpp`, `../include/Game/Demo/DemoExecutor.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Demo scheduler and cast grouping. It must preserve time-keep/demo action semantics so route code advances for the same reasons as the game.
- **Imperfect-MVP replacement path:** MVP action: import or decompile this before chasing polish. It should compile as source-shaped `Game/` code, with missing Wii services filled below it in compat/Aurora or shared runtime services.

#### RunawayRabbitCollect

- **Priority:** P0.
- **Placement/dependency evidence:** 1 placement(s); status: created x1; zones: HeavensDoorMysteriousZone x1; child placements: 7; model/archive aliases: RunawayRabbitCollect x1
- **Current decomp progress:** sibling-decomp factory/source maps to RunawayRabbitCollect using TrickRabbit; extra archive deps: SpotMarkLight, Tico, TicoBaby, TrickRabbitBaby. Source evidence: `../src/Game/NPC/RunawayRabbitCollect.cpp`, `../include/Game/NPC/RunawayRabbitCollect.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Core bunny-route NPC logic. This is where child placement, talk, demo timing, catch state, BGM state, and actor messaging converge.
- **Imperfect-MVP replacement path:** MVP action: import or decompile this before chasing polish. It should compile as source-shaped `Game/` code, with missing Wii services filled below it in compat/Aurora or shared runtime services.

#### RunawayRabbit

- **Priority:** P0.
- **Placement/dependency evidence:** not a root placement in the captured HeavensDoor report; required as child/runtime support for placed route actors.
- **Current decomp progress:** sibling-decomp factory/source maps to RunawayRabbit using TrickRabbit/TrickRabbitBaby. Source evidence: `../src/Game/NPC/RunawayRabbit.cpp`, `../include/Game/NPC/RunawayRabbit.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Core bunny-route NPC logic. This is where child placement, talk, demo timing, catch state, BGM state, and actor messaging converge.
- **Imperfect-MVP replacement path:** MVP action: import or decompile this before chasing polish. It should compile as source-shaped `Game/` code, with missing Wii services filled below it in compat/Aurora or shared runtime services.

#### RunawayTico

- **Priority:** P0.
- **Placement/dependency evidence:** not a root placement in the captured HeavensDoor report; required as child/runtime support for placed route actors.
- **Current decomp progress:** sibling-decomp factory/source maps to RunawayTico using Tico. Source evidence: `../src/Game/NPC/RunawayTico.cpp`, `../include/Game/NPC/RunawayTico.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Core bunny-route NPC logic. This is where child placement, talk, demo timing, catch state, BGM state, and actor messaging converge.
- **Imperfect-MVP replacement path:** MVP action: import or decompile this before chasing polish. It should compile as source-shaped `Game/` code, with missing Wii services filled below it in compat/Aurora or shared runtime services.

#### DemoRabbit

- **Priority:** P0.
- **Placement/dependency evidence:** 3 placement(s); status: created x3; zones: HeavensDoorMysteriousZone x3; model/archive aliases: DemoRabbit x3
- **Current decomp progress:** sibling-decomp factory/source maps to DemoRabbit using no fixed archive. Source evidence: `../src/Game/NPC/DemoRabbit.cpp`, `../include/Game/NPC/DemoRabbit.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Core bunny-route NPC logic. This is where child placement, talk, demo timing, catch state, BGM state, and actor messaging converge.
- **Imperfect-MVP replacement path:** MVP action: import or decompile this before chasing polish. It should compile as source-shaped `Game/` code, with missing Wii services filled below it in compat/Aurora or shared runtime services.

#### Tico

- **Priority:** P0.
- **Placement/dependency evidence:** 7 placement(s); status: created x7; zones: HeavensDoorMiddleZone x2, HeavensDoorInsideZone x2, HeavensDoorMysteriousZone x1; model/archive aliases: Tico x7
- **Current decomp progress:** sibling-decomp factory/source maps to Tico using no fixed archive. Source evidence: `../src/Game/NPC/Tico.cpp`, `../include/Game/NPC/Tico.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Core bunny-route NPC logic. This is where child placement, talk, demo timing, catch state, BGM state, and actor messaging converge.
- **Imperfect-MVP replacement path:** MVP action: import or decompile this before chasing polish. It should compile as source-shaped `Game/` code, with missing Wii services filled below it in compat/Aurora or shared runtime services.

#### TicoBaby

- **Priority:** P0.
- **Placement/dependency evidence:** 1 placement(s); status: created x1; zones: HeavensDoorMysteriousZone x1; model/archive aliases: TicoBaby x1
- **Current decomp progress:** sibling-decomp factory/source maps to Tico using no fixed archive. Source evidence: `../src/Game/NPC/Tico.cpp`, `../include/Game/NPC/Tico.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Core bunny-route NPC logic. This is where child placement, talk, demo timing, catch state, BGM state, and actor messaging converge.
- **Imperfect-MVP replacement path:** MVP action: import or decompile this before chasing polish. It should compile as source-shaped `Game/` code, with missing Wii services filled below it in compat/Aurora or shared runtime services.

### P1 - Route Systems: Make The P0 Actors Behave

#### Rosetta

- **Priority:** P1.
- **Placement/dependency evidence:** 1 placement(s); status: created x1; zones: HeavensDoorMysteriousZone x1; model/archive aliases: Rosetta x1
- **Current decomp progress:** sibling-decomp factory/source maps to Rosetta using no fixed archive. Source evidence: `../src/Game/NPC/Rosetta.cpp`, `../include/Game/NPC/Rosetta.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Boundary object for the high-tower/Rosalina sequence. Needed for placement and clean stop conditions, even if visible Rosalina behavior remains after MVP.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### RosettaDemoHeavensDoor

- **Priority:** P1.
- **Placement/dependency evidence:** not a root placement in the captured HeavensDoor report; required as child/runtime support for placed route actors.
- **Current decomp progress:** sibling-decomp factory/source maps to RosettaDemoHeavensDoor using Rosetta. Source evidence: `../src/Game/NPC/RosettaDemoHeavensDoor.cpp`, `../include/Game/NPC/RosettaDemoHeavensDoor.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Boundary object for the high-tower/Rosalina sequence. Needed for placement and clean stop conditions, even if visible Rosalina behavior remains after MVP.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### TrickRabbit / Rabbit support

- **Priority:** P1.
- **Placement/dependency evidence:** not a root placement in the captured HeavensDoor report; required as child/runtime support for placed route actors.
- **Current decomp progress:** sibling-decomp factory/source maps to TrickRabbit using TrickRabbit; RabbitStateCaught using TrickRabbit. Source evidence: `../src/Game/NPC/TrickRabbit.cpp`, `../include/Game/NPC/TrickRabbit.hpp`, `../src/Game/NPC/Rabbit.cpp`, `../include/Game/NPC/Rabbit.hpp`, `../src/Game/NPC/RabbitStateCaught.cpp`, `../include/Game/NPC/RabbitStateCaught.hpp`, `../src/Game/NPC/RabbitStateWaitStart.cpp`, `../include/Game/NPC/RabbitStateWaitStart.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Visual, effect, audio, guidance, or background placement. This should stay behind general render/effect/audio compatibility and not become route-specific code.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### SpotMarkLight

- **Priority:** P1.
- **Placement/dependency evidence:** not a root placement in the captured HeavensDoor report; required as child/runtime support for placed route actors.
- **Current decomp progress:** sibling-decomp factory/source maps to SpotMarkLight using SpotMarkLight. Source evidence: `../src/Game/LiveActor/SpotMarkLight.cpp`, `../include/Game/LiveActor/SpotMarkLight.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Visual, effect, audio, guidance, or background placement. This should stay behind general render/effect/audio compatibility and not become route-specific code.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### ChangeBgmCube

- **Priority:** P1.
- **Placement/dependency evidence:** 3 placement(s); status: ignored x3; zones: HeavensDoorGalaxy x3
- **Current decomp progress:** sibling-decomp factory/source maps to ChangeBgmCube using no fixed archive. Source evidence: `../src/Game/AreaObj/ChangeBgmCube.cpp`, `../include/Game/AreaObj/ChangeBgmCube.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Area/helper placement. These should feed generic area, message, respawn, and collision services; they are not decorative models.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### CollisionArea

- **Priority:** P1.
- **Placement/dependency evidence:** 2 placement(s); status: ignored x2; zones: HeavensDoorMysteriousZone x2
- **Current decomp progress:** sibling-decomp factory/source maps to CollisionArea using no fixed archive. Source evidence: `../src/Game/AreaObj/CollisionArea.cpp`, `../include/Game/AreaObj/CollisionArea.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Area/helper placement. These should feed generic area, message, respawn, and collision services; they are not decorative models.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### CollisionBlocker

- **Priority:** P1.
- **Placement/dependency evidence:** 8 placement(s); status: created_deferred_stub x8; zones: HeavensDoorMysteriousZone x7, HeavensDoorGalaxy x1
- **Current decomp progress:** sibling-decomp factory/source maps to CollisionBlocker using no fixed archive. Source evidence: `../src/Game/MapObj/CollisionBlocker.cpp`, `../include/Game/MapObj/CollisionBlocker.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Visual, effect, audio, guidance, or background placement. This should stay behind general render/effect/audio compatibility and not become route-specific code.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### CubeCameraBox

- **Priority:** P1.
- **Placement/dependency evidence:** 2 placement(s); status: ignored x2; zones: HeavensDoorMysteriousZone x1, HeavensDoorSmallZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to CubeCameraArea using no fixed archive. Source evidence: `../src/Game/AreaObj/CubeCamera.cpp`, `../include/Game/AreaObj/CubeCamera.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Scene service placement for gravity, camera, light, or view groups. MVP can be approximate, but the data flow should be original-shaped.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### CubeCameraCylinder

- **Priority:** P1.
- **Placement/dependency evidence:** 8 placement(s); status: ignored x8; zones: HeavensDoorMysteriousZone x6, HeavensDoorSmallZone x2
- **Current decomp progress:** sibling-decomp factory/source maps to CubeCameraArea using no fixed archive. Source evidence: `../src/Game/AreaObj/CubeCamera.cpp`, `../include/Game/AreaObj/CubeCamera.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Scene service placement for gravity, camera, light, or view groups. MVP can be approximate, but the data flow should be original-shaped.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### CubeCameraSphere

- **Priority:** P1.
- **Placement/dependency evidence:** 6 placement(s); status: ignored x6; zones: HeavensDoorMiddleZone x2, HeavensDoorMysteriousZone x1, HeavensDoorSmallZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to CubeCameraArea using no fixed archive. Source evidence: `../src/Game/AreaObj/CubeCamera.cpp`, `../include/Game/AreaObj/CubeCamera.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Scene service placement for gravity, camera, light, or view groups. MVP can be approximate, but the data flow should be original-shaped.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### GlobalPlaneGravity

- **Priority:** P1.
- **Placement/dependency evidence:** 1 placement(s); status: ignored x1; zones: HeavensDoorMysteriousZone x1; child placements: 7
- **Current decomp progress:** sibling-decomp factory/source maps to MR::createGlobalPlaneGravityObj using no fixed archive. Source evidence: No source file mapped yet; decompile/import decision should start from factory entry, archive table, and placement data.
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Scene service placement for gravity, camera, light, or view groups. MVP can be approximate, but the data flow should be original-shaped.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### GlobalPlaneGravityInBox

- **Priority:** P1.
- **Placement/dependency evidence:** 1 placement(s); status: ignored x1; zones: HeavensDoorSmallZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to MR::createGlobalPlaneInBoxGravityObj using no fixed archive. Source evidence: No source file mapped yet; decompile/import decision should start from factory entry, archive table, and placement data.
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Scene service placement for gravity, camera, light, or view groups. MVP can be approximate, but the data flow should be original-shaped.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### GlobalPointGravity

- **Priority:** P1.
- **Placement/dependency evidence:** 5 placement(s); status: ignored x5; zones: HeavensDoorMysteriousZone x1, HeavensDoorSmallZone x1, HeavensDoorMiddleZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to MR::createGlobalPointGravityObj using no fixed archive. Source evidence: No source file mapped yet; decompile/import decision should start from factory entry, archive table, and placement data.
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Scene service placement for gravity, camera, light, or view groups. MVP can be approximate, but the data flow should be original-shaped.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### GroupSwitchWatcher

- **Priority:** P1.
- **Placement/dependency evidence:** 1 placement(s); status: created x1; zones: HeavensDoorSmallZone x1; model/archive aliases: GroupSwitchWatcher x1
- **Current decomp progress:** sibling-decomp factory/source maps to GroupSwitchWatcher using no fixed archive. Source evidence: `../src/Game/Map/GroupSwitchWatcher.cpp`, `../include/Game/Map/GroupSwitchWatcher.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage switch infrastructure. It generalizes route state instead of hardcoding which placed objects appear or unlock.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### HeavensBlackHoleZone

- **Priority:** P1.
- **Placement/dependency evidence:** 1 placement(s); status: ignored x1; zones: HeavensDoorGalaxy x1
- **Current decomp progress:** sibling-decomp factory/source maps to stage zone placement using no fixed archive. Source evidence: No source file mapped yet; decompile/import decision should start from factory entry, archive table, and placement data.
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Visual, effect, audio, guidance, or background placement. This should stay behind general render/effect/audio compatibility and not become route-specific code.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### HeavensDoorInsideZone

- **Priority:** P1.
- **Placement/dependency evidence:** 1 placement(s); status: ignored x1; zones: HeavensDoorGalaxy x1
- **Current decomp progress:** sibling-decomp factory/source maps to stage zone placement using no fixed archive. Source evidence: No source file mapped yet; decompile/import decision should start from factory entry, archive table, and placement data.
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### HeavensDoorMiddleZone

- **Priority:** P1.
- **Placement/dependency evidence:** 1 placement(s); status: ignored x1; zones: HeavensDoorGalaxy x1
- **Current decomp progress:** sibling-decomp factory/source maps to stage zone placement using no fixed archive. Source evidence: No source file mapped yet; decompile/import decision should start from factory entry, archive table, and placement data.
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### HeavensDoorMysteriousZone

- **Priority:** P1.
- **Placement/dependency evidence:** 1 placement(s); status: ignored x1; zones: HeavensDoorGalaxy x1
- **Current decomp progress:** sibling-decomp factory/source maps to stage zone placement using no fixed archive. Source evidence: No source file mapped yet; decompile/import decision should start from factory entry, archive table, and placement data.
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### HeavensDoorSmallZone

- **Priority:** P1.
- **Placement/dependency evidence:** 1 placement(s); status: ignored x1; zones: HeavensDoorGalaxy x1
- **Current decomp progress:** sibling-decomp factory/source maps to stage zone placement using no fixed archive. Source evidence: No source file mapped yet; decompile/import decision should start from factory entry, archive table, and placement data.
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### LightCtrlCube

- **Priority:** P1.
- **Placement/dependency evidence:** 8 placement(s); status: ignored x8; zones: HeavensDoorGalaxy x7, HeavensDoorMysteriousZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to LightArea using no fixed archive. Source evidence: `../src/Game/AreaObj/LightArea.cpp`, `../include/Game/AreaObj/LightArea.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Scene service placement for gravity, camera, light, or view groups. MVP can be approximate, but the data flow should be original-shaped.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### MessageAreaCube

- **Priority:** P1.
- **Placement/dependency evidence:** 1 placement(s); status: ignored x1; zones: HeavensDoorSmallZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to MessageArea using no fixed archive. Source evidence: `../src/Game/AreaObj/MessageArea.cpp`, `../include/Game/AreaObj/MessageArea.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Area/helper placement. These should feed generic area, message, respawn, and collision services; they are not decorative models.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### MessageAreaCylinder

- **Priority:** P1.
- **Placement/dependency evidence:** 1 placement(s); status: ignored x1; zones: HeavensDoorMysteriousZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to MessageArea using no fixed archive. Source evidence: `../src/Game/AreaObj/MessageArea.cpp`, `../include/Game/AreaObj/MessageArea.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Area/helper placement. These should feed generic area, message, respawn, and collision services; they are not decorative models.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### PullBackCylinder

- **Priority:** P1.
- **Placement/dependency evidence:** 1 placement(s); status: ignored x1; zones: HeavensDoorMysteriousZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to AreaObj using no fixed archive. Source evidence: `../src/Game/AreaObj/AreaObj.cpp`, `../include/Game/AreaObj/AreaObj.hpp`, `../include/Game/AreaObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Area/helper placement. These should feed generic area, message, respawn, and collision services; they are not decorative models.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### RestartCube

- **Priority:** P1.
- **Placement/dependency evidence:** 4 placement(s); status: ignored x4; zones: HeavensDoorGalaxy x4
- **Current decomp progress:** sibling-decomp factory/source maps to RestartCube using no fixed archive. Source evidence: `../src/Game/AreaObj/RestartCube.cpp`, `../include/Game/AreaObj/RestartCube.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Area/helper placement. These should feed generic area, message, respawn, and collision services; they are not decorative models.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### SwitchCube

- **Priority:** P1.
- **Placement/dependency evidence:** 4 placement(s); status: created x4; zones: HeavensDoorMysteriousZone x2, HeavensBlackHoleZone x2; model/archive aliases: SwitchCube x4
- **Current decomp progress:** sibling-decomp factory/source maps to SwitchArea using no fixed archive. Source evidence: `../src/Game/AreaObj/SwitchArea.cpp`, `../include/Game/AreaObj/SwitchArea.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage switch infrastructure. It generalizes route state instead of hardcoding which placed objects appear or unlock.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### SwitchSynchronizerReverse

- **Priority:** P1.
- **Placement/dependency evidence:** 2 placement(s); status: created x2; zones: HeavensDoorGalaxy x1, HeavensDoorInsideZone x1; model/archive aliases: SwitchSynchronizerReverse x2
- **Current decomp progress:** sibling-decomp factory/source maps to SwitchSynchronizer using no fixed archive. Source evidence: `../src/Game/Map/SwitchSynchronizer.cpp`, `../include/Game/Map/SwitchSynchronizer.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage switch infrastructure. It generalizes route state instead of hardcoding which placed objects appear or unlock.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

#### ViewGroupCtrlCube

- **Priority:** P1.
- **Placement/dependency evidence:** 8 placement(s); status: ignored x8; zones: HeavensDoorGalaxy x8
- **Current decomp progress:** sibling-decomp factory/source maps to AreaObj using no fixed archive. Source evidence: `../src/Game/AreaObj/AreaObj.cpp`, `../include/Game/AreaObj/AreaObj.hpp`, `../include/Game/AreaObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Scene service placement for gravity, camera, light, or view groups. MVP can be approximate, but the data flow should be original-shaped.
- **Imperfect-MVP replacement path:** MVP action: implement enough original-shaped service behavior for the P0 actors to progress. Avoid path-specific shortcuts; state-only behavior is acceptable when the original result is preserved.

### P2 - Physical Stage Objects: Make Movement And Collision Plausible

#### BlackHole

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensBlackHoleZone x1; model/archive aliases: BlackHole x1
- **Current decomp progress:** sibling-decomp factory/source maps to BlackHole using BlackHole; extra archive deps: BlackHoleRange. Source evidence: `../src/Game/MapObj/BlackHole.cpp`, `../include/Game/MapObj/BlackHole.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### CapsuleCage

- **Priority:** P2.
- **Placement/dependency evidence:** 2 placement(s); status: created_model_fallback x2; zones: HeavensDoorSmallZone x1, HeavensDoorMiddleZone x1; model/archive aliases: CapsuleCage x2
- **Current decomp progress:** sibling-decomp factory/source maps to CapsuleCage using CapsuleCage. Source evidence: `../src/Game/MapObj/CapsuleCage.cpp`, `../include/Game/MapObj/CapsuleCage.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### CrystalCageM

- **Priority:** P2.
- **Placement/dependency evidence:** 3 placement(s); status: created_model_fallback x3; zones: HeavensDoorMysteriousZone x3; model/archive aliases: CrystalCageM x3
- **Current decomp progress:** sibling-decomp factory/source maps to CrystalCage using CrystalCageM; extra archive deps: CrystalCageSBreak. Source evidence: `../src/Game/MapObj/CrystalCage.cpp`, `../include/Game/MapObj/CrystalCage.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### CrystalCageS

- **Priority:** P2.
- **Placement/dependency evidence:** 7 placement(s); status: created_model_fallback x7; zones: HeavensDoorSmallZone x4, HeavensBlackHoleZone x3; model/archive aliases: CrystalCageS x7
- **Current decomp progress:** sibling-decomp factory/source maps to CrystalCage using CrystalCageS; extra archive deps: CrystalCageSBreak. Source evidence: `../src/Game/MapObj/CrystalCage.cpp`, `../include/Game/MapObj/CrystalCage.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### EarthenPipe

- **Priority:** P2.
- **Placement/dependency evidence:** 4 placement(s); status: created_model_fallback x4; zones: HeavensDoorGalaxy x2, HeavensDoorMysteriousZone x2; model/archive aliases: EarthenPipe x4
- **Current decomp progress:** sibling-decomp factory/source maps to EarthenPipe using EarthenPipe. Source evidence: `../src/Game/MapObj/EarthenPipe.cpp`, `../include/Game/MapObj/EarthenPipe.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### FlipPanelObserver

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_deferred_stub x1; zones: HeavensDoorInsideZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to FlipPanelObserver using no fixed archive. Source evidence: `../src/Game/MapObj/FlipPanel.cpp`, `../include/Game/MapObj/FlipPanel.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### FlipPanelReverse

- **Priority:** P2.
- **Placement/dependency evidence:** 16 placement(s); status: created_model_fallback x16; zones: HeavensDoorInsideZone x16; model/archive aliases: FlipPanelReverse x16
- **Current decomp progress:** sibling-decomp factory/source maps to FlipPanel using FlipPanelReverse; extra archive deps: FlipPanelReverseBloom. Source evidence: `../src/Game/MapObj/FlipPanel.cpp`, `../include/Game/MapObj/FlipPanel.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorAppearStepA

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorMysteriousZone x1; model/archive aliases: HeavensDoorAppearStepA x1
- **Current decomp progress:** sibling-decomp factory/source maps to HeavensDoorDemoObj using HeavensDoorAppearStepA. Source evidence: `../src/Game/MapObj/HeavensDoorDemoObj.cpp`, `../include/Game/MapObj/HeavensDoorDemoObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorAppearStepAAfter

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorMysteriousZone x1; model/archive aliases: HeavensDoorAppearStepAAfter x1
- **Current decomp progress:** sibling-decomp factory/source maps to SimpleMapObj using HeavensDoorAppearStepAAfter. Source evidence: `../src/Game/MapObj/SimpleMapObj.cpp`, `../include/Game/MapObj/SimpleMapObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorBlackHolePlanet

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensBlackHoleZone x1; model/archive aliases: HeavensDoorBlackHolePlanet x1
- **Current decomp progress:** sibling-decomp factory/source maps to static model/mapparts using HeavensDoorBlackHolePlanet. Source evidence: No source file mapped yet; decompile/import decision should start from factory entry, archive table, and placement data.
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorFlowerA

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorMysteriousZone x1; model/archive aliases: HeavensDoorFlowerA x1
- **Current decomp progress:** sibling-decomp factory/source maps to SimpleMapObjNoSilhouetted using HeavensDoorFlowerA. Source evidence: `../src/Game/MapObj/SimpleMapObj.cpp`, `../include/Game/MapObj/SimpleMapObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorHouseDoor

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorMysteriousZone x1; model/archive aliases: HeavensDoorHouseDoor x1
- **Current decomp progress:** sibling-decomp factory/source maps to SimpleMapObj using HeavensDoorHouseDoor. Source evidence: `../src/Game/MapObj/SimpleMapObj.cpp`, `../include/Game/MapObj/SimpleMapObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorInsideCage

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorInsideZone x1; model/archive aliases: HeavensDoorInsideCage x1
- **Current decomp progress:** sibling-decomp factory/source maps to HeavensDoorDemoObj using HeavensDoorInsideCage. Source evidence: `../src/Game/MapObj/HeavensDoorDemoObj.cpp`, `../include/Game/MapObj/HeavensDoorDemoObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorInsidePlanet

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorInsideZone x1; model/archive aliases: HeavensDoorInsidePlanet x1
- **Current decomp progress:** sibling-decomp factory/source maps to static model/mapparts using HeavensDoorInsidePlanet. Source evidence: No source file mapped yet; decompile/import decision should start from factory entry, archive table, and placement data.
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorInsidePlanetPartsA

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorInsideZone x1; model/archive aliases: HeavensDoorInsidePlanetPartsA x1
- **Current decomp progress:** sibling-decomp factory/source maps to HeavensDoorDemoObj using HeavensDoorInsidePlanetPartsA; extra archive deps: HeavensDoorInsidePlanetPartsABloom. Source evidence: `../src/Game/MapObj/HeavensDoorDemoObj.cpp`, `../include/Game/MapObj/HeavensDoorDemoObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorInsideRotatePartsA

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorInsideZone x1; model/archive aliases: HeavensDoorInsideRotatePartsA x1
- **Current decomp progress:** sibling-decomp factory/source maps to RotateMoveObj using HeavensDoorInsideRotatePartsA. Source evidence: `../src/Game/MapObj/RotateMoveObj.cpp`, `../include/Game/MapObj/RotateMoveObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorInsideRotatePartsB

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorInsideZone x1; model/archive aliases: HeavensDoorInsideRotatePartsB x1
- **Current decomp progress:** sibling-decomp factory/source maps to RotateMoveObj using HeavensDoorInsideRotatePartsB. Source evidence: `../src/Game/MapObj/RotateMoveObj.cpp`, `../include/Game/MapObj/RotateMoveObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorInsideRotatePartsC

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorInsideZone x1; model/archive aliases: HeavensDoorInsideRotatePartsC x1
- **Current decomp progress:** sibling-decomp factory/source maps to RotateMoveObj using HeavensDoorInsideRotatePartsC. Source evidence: `../src/Game/MapObj/RotateMoveObj.cpp`, `../include/Game/MapObj/RotateMoveObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorMiddlePlanet

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorMiddleZone x1; model/archive aliases: HeavensDoorMiddlePlanet x1
- **Current decomp progress:** sibling-decomp factory/source maps to static model/mapparts using HeavensDoorMiddlePlanet. Source evidence: No source file mapped yet; decompile/import decision should start from factory entry, archive table, and placement data.
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorMiddleRotatePartsA

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorMiddleZone x1; model/archive aliases: HeavensDoorMiddleRotatePartsA x1
- **Current decomp progress:** sibling-decomp factory/source maps to RotateMoveObj using HeavensDoorMiddleRotatePartsA. Source evidence: `../src/Game/MapObj/RotateMoveObj.cpp`, `../include/Game/MapObj/RotateMoveObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorMiddleRotatePartsB

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorMiddleZone x1; model/archive aliases: HeavensDoorMiddleRotatePartsB x1
- **Current decomp progress:** sibling-decomp factory/source maps to RotateMoveObj using HeavensDoorMiddleRotatePartsB. Source evidence: `../src/Game/MapObj/RotateMoveObj.cpp`, `../include/Game/MapObj/RotateMoveObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorMysteriousPlanet

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorMysteriousZone x1; model/archive aliases: HeavensDoorMysteriousPlanet x1
- **Current decomp progress:** sibling-decomp factory/source maps to static model/mapparts using HeavensDoorMysteriousPlanet. Source evidence: No source file mapped yet; decompile/import decision should start from factory entry, archive table, and placement data.
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### HeavensDoorSmallPlanet

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorSmallZone x1; model/archive aliases: HeavensDoorSmallPlanet x1
- **Current decomp progress:** sibling-decomp factory/source maps to static model/mapparts using HeavensDoorSmallPlanet. Source evidence: No source file mapped yet; decompile/import decision should start from factory entry, archive table, and placement data.
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### KoopaJrNormalShipA

- **Priority:** P2.
- **Placement/dependency evidence:** 3 placement(s); status: created_model_fallback x3; zones: HeavensDoorMiddleZone x3; model/archive aliases: KoopaJrNormalShipA x3
- **Current decomp progress:** sibling-decomp factory/source maps to SimpleMapObj using KoopaJrNormalShipA; extra archive deps: KoopaJrNormalShipALow. Source evidence: `../src/Game/MapObj/SimpleMapObj.cpp`, `../include/Game/MapObj/SimpleMapObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### PeachCastleGardenPlanet

- **Priority:** P2.
- **Placement/dependency evidence:** not a root placement in the captured HeavensDoor report; required as child/runtime support for placed route actors.
- **Current decomp progress:** sibling-decomp factory/source maps to PeachCastleGardenPlanet using PeachCastleGardenPlanet. Source evidence: `../src/Game/MapObj/PeachCastleGardenPlanet.cpp`, `../include/Game/MapObj/PeachCastleGardenPlanet.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### PunchingKinoko

- **Priority:** P2.
- **Placement/dependency evidence:** 14 placement(s); status: created_model_fallback x14; zones: HeavensDoorMysteriousZone x14; model/archive aliases: PunchingKinoko x14
- **Current decomp progress:** sibling-decomp factory/source maps to PunchingKinoko using PunchingKinoko. Source evidence: `../src/Game/MapObj/PunchingKinoko.cpp`, `../include/Game/MapObj/PunchingKinoko.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### SphereAir

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorMysteriousZone x1; model/archive aliases: SphereAir x1
- **Current decomp progress:** sibling-decomp factory/source maps to PriorDrawAir using SphereAir. Source evidence: `../src/Game/Map/Air.cpp`, `../include/Game/Map/Air.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### SpinDriver

- **Priority:** P2.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorGalaxy x1; model/archive aliases: SpinDriver x1
- **Current decomp progress:** sibling-decomp factory/source maps to SpinDriver using SpinDriver; extra archive deps: SpinDriverShadow. Source evidence: `../src/Game/MapObj/SpinDriver.cpp`, `../include/Game/MapObj/SpinDriver.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### SuperSpinDriver

- **Priority:** P2.
- **Placement/dependency evidence:** 2 placement(s); status: created_model_fallback x2; zones: HeavensDoorGalaxy x2; model/archive aliases: SuperSpinDriver x2
- **Current decomp progress:** sibling-decomp factory/source maps to MR::createSuperSpinDriverYellow using SuperSpinDriver; extra archive deps: SpinDriverPath, SuperSpinDriverShadow. Source evidence: `../src/Game/MapObj/SuperSpinDriver.cpp`, `../include/Game/MapObj/SuperSpinDriver.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

#### WarpPod

- **Priority:** P2.
- **Placement/dependency evidence:** 2 placement(s); status: created_model_fallback x2; zones: HeavensDoorMysteriousZone x2; model/archive aliases: WarpPod x2
- **Current decomp progress:** sibling-decomp factory/source maps to WarpPod using WarpPod. Source evidence: `../src/Game/MapObj/WarpPod.cpp`, `../include/Game/MapObj/WarpPod.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Stage geometry or route obstacle. Model fallback can make it visible, but MVP gameplay needs original-shaped collision, switches, rails, demos, or hazards where the actor owns them.
- **Imperfect-MVP replacement path:** MVP action: import once the route actors boot, prioritizing collision, switches, rails, and demo actions over perfect rendering. Temporary model fallback is acceptable only as visibility, not behavior.

### P3 - Gameplay Fill: Enemies, Pickups, And Optional Interactions

#### BenefitItemOneUp

- **Priority:** P3.
- **Placement/dependency evidence:** 1 placement(s); status: created_alias_model_fallback x1; zones: HeavensBlackHoleZone x1; model/archive aliases: KinokoOneUp x1
- **Current decomp progress:** sibling-decomp factory/source maps to BenefitItemOneUp using KinokoOneUp. Source evidence: `../src/Game/MapObj/BenefitItemObj.cpp`, `../include/Game/MapObj/BenefitItemObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Collectible/reward layer. First MVP can skip most rewards, but import should use the real item holder and pickup systems when gameplay polish starts.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### Butterfly

- **Priority:** P3.
- **Placement/dependency evidence:** 3 placement(s); status: created_model_fallback x3; zones: HeavensDoorMysteriousZone x3; model/archive aliases: Butterfly x3
- **Current decomp progress:** sibling-decomp factory/source maps to Butterfly using Butterfly. Source evidence: `../src/Game/Map/Butterfly.cpp`, `../include/Game/Map/Butterfly.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Environmental interaction fill. These are useful after the route actor loop works because they exercise sensors, effects, spawning, and archive aliases.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### Coin

- **Priority:** P3.
- **Placement/dependency evidence:** 10 placement(s); status: created_model_fallback x10; zones: HeavensDoorMysteriousZone x10; model/archive aliases: Coin x10
- **Current decomp progress:** sibling-decomp factory/source maps to MR::createDirectSetCoin using Coin. Source evidence: `../src/Game/MapObj/Coin.cpp`, `../include/Game/MapObj/Coin.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Collectible/reward layer. First MVP can skip most rewards, but import should use the real item holder and pickup systems when gameplay polish starts.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### CutBushGroup

- **Priority:** P3.
- **Placement/dependency evidence:** 1 placement(s); status: created_alias_model_fallback x1; zones: HeavensDoorMysteriousZone x1; model/archive aliases: CutBush x1
- **Current decomp progress:** sibling-decomp factory/source maps to PlantGroup using CutBush. Source evidence: `../src/Game/MapObj/PlantGroup.cpp`, `../include/Game/MapObj/PlantGroup.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Environmental interaction fill. These are useful after the route actor loop works because they exercise sensors, effects, spawning, and archive aliases.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### ExterminationKuriboKeySwitch

- **Priority:** P3.
- **Placement/dependency evidence:** 1 placement(s); status: created_alias_model_fallback x1; zones: HeavensDoorSmallZone x1; child placements: 1; model/archive aliases: Kuribo x1
- **Current decomp progress:** sibling-decomp factory/source maps to MR::createExterminationKeySwitch using Kuribo/KeySwitch; extra archive deps: KeySwitch. Source evidence: `../src/Game/MapObj/ExterminationChecker.cpp`, `../include/Game/MapObj/ExterminationChecker.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Enemy/key-switch fill. These are not first-frame bunny logic, but they validate the general enemy/sensor/switch pipeline needed by real stages.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### FlowerBlueGroup

- **Priority:** P3.
- **Placement/dependency evidence:** 1 placement(s); status: created_alias_model_fallback x1; zones: HeavensDoorMysteriousZone x1; model/archive aliases: FlowerBlue x1
- **Current decomp progress:** sibling-decomp factory/source maps to PlantGroup using FlowerBlue. Source evidence: `../src/Game/MapObj/PlantGroup.cpp`, `../include/Game/MapObj/PlantGroup.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Environmental interaction fill. These are useful after the route actor loop works because they exercise sensors, effects, spawning, and archive aliases.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### FlowerGroup

- **Priority:** P3.
- **Placement/dependency evidence:** 1 placement(s); status: created_alias_model_fallback x1; zones: HeavensDoorMysteriousZone x1; model/archive aliases: Flower x1
- **Current decomp progress:** sibling-decomp factory/source maps to PlantGroup using Flower. Source evidence: `../src/Game/MapObj/PlantGroup.cpp`, `../include/Game/MapObj/PlantGroup.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Environmental interaction fill. These are useful after the route actor loop works because they exercise sensors, effects, spawning, and archive aliases.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### GrandStar

- **Priority:** P3.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorInsideZone x1; model/archive aliases: GrandStar x1
- **Current decomp progress:** sibling-decomp factory/source maps to PowerStar using GrandStar. Source evidence: `../src/Game/MapObj/PowerStar.cpp`, `../include/Game/MapObj/PowerStar.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Collectible/reward layer. First MVP can skip most rewards, but import should use the real item holder and pickup systems when gameplay polish starts.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### Kuribo

- **Priority:** P3.
- **Placement/dependency evidence:** 8 placement(s); status: created_model_fallback x8; zones: HeavensDoorMiddleZone x5, HeavensDoorInsideZone x3; model/archive aliases: Kuribo x8
- **Current decomp progress:** sibling-decomp factory/source maps to Kuribo using Kuribo. Source evidence: `../src/Game/Enemy/Kuribo.cpp`, `../include/Game/Enemy/Kuribo.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Enemy/key-switch fill. These are not first-frame bunny logic, but they validate the general enemy/sensor/switch pipeline needed by real stages.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### KuriboChief

- **Priority:** P3.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorMiddleZone x1; model/archive aliases: KuriboChief x1
- **Current decomp progress:** sibling-decomp factory/source maps to KuriboChief using KuriboChief. Source evidence: `../src/Game/Enemy/KuriboChief.cpp`, `../include/Game/Enemy/KuriboChief.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Enemy/key-switch fill. These are not first-frame bunny logic, but they validate the general enemy/sensor/switch pipeline needed by real stages.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### KuriboMini

- **Priority:** P3.
- **Placement/dependency evidence:** 8 placement(s); status: created_model_fallback x8; zones: HeavensDoorSmallZone x4, HeavensDoorMiddleZone x2, HeavensBlackHoleZone x2; model/archive aliases: KuriboMini x8
- **Current decomp progress:** sibling-decomp factory/source maps to KuriboMini using KuriboMini. Source evidence: `../src/Game/Enemy/KuriboMini.cpp`, `../include/Game/Enemy/KuriboMini.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Enemy/key-switch fill. These are not first-frame bunny logic, but they validate the general enemy/sensor/switch pipeline needed by real stages.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### MeteorCannon

- **Priority:** P3.
- **Placement/dependency evidence:** 2 placement(s); status: created_alias_model_fallback x2; zones: HeavensBlackHoleZone x2; model/archive aliases: MeteorStrike x2
- **Current decomp progress:** sibling-decomp factory/source maps to MeteorStrikeLauncher using MeteorStrike; extra archive deps: MeteorCannonBreak. Source evidence: `../src/Game/MapObj/MeteorStrikeLauncher.cpp`, `../include/Game/MapObj/MeteorStrikeLauncher.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Environmental interaction fill. These are useful after the route actor loop works because they exercise sensors, effects, spawning, and archive aliases.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### RailCoin

- **Priority:** P3.
- **Placement/dependency evidence:** 2 placement(s); status: created_deferred_stub x2; zones: HeavensDoorMiddleZone x1, HeavensDoorInsideZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to MR::createRailCoin using no fixed archive. Source evidence: `../src/Game/MapObj/RailCoin.cpp`, `../include/Game/MapObj/RailCoin.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Collectible/reward layer. First MVP can skip most rewards, but import should use the real item holder and pickup systems when gameplay polish starts.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### StarPiece

- **Priority:** P3.
- **Placement/dependency evidence:** 7 placement(s); status: created_model_fallback x7; zones: HeavensDoorMysteriousZone x7; model/archive aliases: StarPiece x7
- **Current decomp progress:** sibling-decomp factory/source maps to StarPiece using StarPiece. Source evidence: `../src/Game/MapObj/StarPiece.cpp`, `../include/Game/MapObj/StarPiece.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Collectible/reward layer. First MVP can skip most rewards, but import should use the real item holder and pickup systems when gameplay polish starts.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### StarPieceGroup

- **Priority:** P3.
- **Placement/dependency evidence:** 1 placement(s); status: created_deferred_stub x1; zones: HeavensDoorMiddleZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to StarPieceGroup using no fixed archive. Source evidence: `../src/Game/MapObj/StarPieceGroup.cpp`, `../include/Game/MapObj/StarPieceGroup.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Collectible/reward layer. First MVP can skip most rewards, but import should use the real item holder and pickup systems when gameplay polish starts.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### YellowChip

- **Priority:** P3.
- **Placement/dependency evidence:** 5 placement(s); status: created_model_fallback x5; zones: HeavensBlackHoleZone x5; model/archive aliases: YellowChip x5
- **Current decomp progress:** sibling-decomp factory/source maps to YellowChip using YellowChip. Source evidence: `../src/Game/MapObj/YellowChip.cpp`, `../include/Game/MapObj/YellowChip.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Collectible/reward layer. First MVP can skip most rewards, but import should use the real item holder and pickup systems when gameplay polish starts.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

#### YellowChipGroup

- **Priority:** P3.
- **Placement/dependency evidence:** 1 placement(s); status: created_deferred_stub x1; zones: HeavensBlackHoleZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to YellowChipGroup using no fixed archive. Source evidence: `../src/Game/MapObj/ChipGroup.cpp`, `../include/Game/MapObj/ChipGroup.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Collectible/reward layer. First MVP can skip most rewards, but import should use the real item holder and pickup systems when gameplay polish starts.
- **Imperfect-MVP replacement path:** MVP action: defer until the bunny loop is playable unless this blocks placement or a route switch. When imported, use the real enemy/item holder systems rather than isolated stubs.

### P4 - Visual, Audio, Guidance, And Post-MVP Polish

#### AudioEffectSphere

- **Priority:** P4.
- **Placement/dependency evidence:** 1 placement(s); status: ignored x1; zones: HeavensDoorInsideZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to AudioEffectArea using no fixed archive. Source evidence: `../src/Game/AreaObj/AudioEffectArea.cpp`, `../include/Game/AreaObj/AudioEffectArea.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Visual, effect, audio, guidance, or background placement. This should stay behind general render/effect/audio compatibility and not become route-specific code.
- **Imperfect-MVP replacement path:** MVP action: defer until after the imperfect demo works. Revisit for Aurora/render/effect/audio fidelity and remove any generic fallback that hides real game behavior.

#### BloomCube

- **Priority:** P4.
- **Placement/dependency evidence:** 1 placement(s); status: ignored x1; zones: HeavensDoorInsideZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to BloomArea using no fixed archive. Source evidence: `../src/Game/AreaObj/BloomArea.cpp`, `../include/Game/AreaObj/BloomArea.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Visual, effect, audio, guidance, or background placement. This should stay behind general render/effect/audio compatibility and not become route-specific code.
- **Imperfect-MVP replacement path:** MVP action: defer until after the imperfect demo works. Revisit for Aurora/render/effect/audio fidelity and remove any generic fallback that hides real game behavior.

#### BlueStarGuidanceCube

- **Priority:** P4.
- **Placement/dependency evidence:** 1 placement(s); status: ignored x1; zones: HeavensDoorSmallZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to AreaObj using no fixed archive. Source evidence: `../src/Game/AreaObj/AreaObj.cpp`, `../include/Game/AreaObj/AreaObj.hpp`, `../include/Game/AreaObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Visual, effect, audio, guidance, or background placement. This should stay behind general render/effect/audio compatibility and not become route-specific code.
- **Imperfect-MVP replacement path:** MVP action: defer until after the imperfect demo works. Revisit for Aurora/render/effect/audio fidelity and remove any generic fallback that hides real game behavior.

#### BrightSun

- **Priority:** P4.
- **Placement/dependency evidence:** 1 placement(s); status: created_alias_model_fallback x1; zones: HeavensDoorGalaxy x1; model/archive aliases: LensFlare x1
- **Current decomp progress:** sibling-decomp factory/source maps to BrightSun using LensFlare; extra archive deps: GlareGlow, GlareLine, Sun. Source evidence: `../src/Game/MapObj/BrightObj.cpp`, `../include/Game/MapObj/BrightObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Visual, effect, audio, guidance, or background placement. This should stay behind general render/effect/audio compatibility and not become route-specific code.
- **Imperfect-MVP replacement path:** MVP action: defer until after the imperfect demo works. Revisit for Aurora/render/effect/audio fidelity and remove any generic fallback that hides real game behavior.

#### LensFlareArea

- **Priority:** P4.
- **Placement/dependency evidence:** 1 placement(s); status: ignored x1; zones: HeavensDoorGalaxy x1
- **Current decomp progress:** sibling-decomp factory/source maps to AreaObj using no fixed archive. Source evidence: `../src/Game/AreaObj/AreaObj.cpp`, `../include/Game/AreaObj/AreaObj.hpp`, `../include/Game/AreaObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Visual, effect, audio, guidance, or background placement. This should stay behind general render/effect/audio compatibility and not become route-specific code.
- **Imperfect-MVP replacement path:** MVP action: defer until after the imperfect demo works. Revisit for Aurora/render/effect/audio fidelity and remove any generic fallback that hides real game behavior.

#### ShockWaveGenerator

- **Priority:** P4.
- **Placement/dependency evidence:** 9 placement(s); status: created_model_fallback x9; zones: HeavensDoorMiddleZone x8, HeavensDoorSmallZone x1; model/archive aliases: ShockWaveGenerator x9
- **Current decomp progress:** sibling-decomp factory/source maps to ShockWaveGenerator using ShockWaveGenerator. Source evidence: `../src/Game/MapObj/ShockWaveGenerator.cpp`, `../include/Game/MapObj/ShockWaveGenerator.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Visual, effect, audio, guidance, or background placement. This should stay behind general render/effect/audio compatibility and not become route-specific code.
- **Imperfect-MVP replacement path:** MVP action: defer until after the imperfect demo works. Revisit for Aurora/render/effect/audio fidelity and remove any generic fallback that hides real game behavior.

#### SpinGuidanceCube

- **Priority:** P4.
- **Placement/dependency evidence:** 5 placement(s); status: ignored x5; zones: HeavensBlackHoleZone x3, HeavensDoorMysteriousZone x1, HeavensDoorSmallZone x1
- **Current decomp progress:** sibling-decomp factory/source maps to SpinGuidanceArea using SpinGuidance. Source evidence: `../src/Game/AreaObj/SpinGuidanceArea.cpp`, `../include/Game/AreaObj/SpinGuidanceArea.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Visual, effect, audio, guidance, or background placement. This should stay behind general render/effect/audio compatibility and not become route-specific code.
- **Imperfect-MVP replacement path:** MVP action: defer until after the imperfect demo works. Revisit for Aurora/render/effect/audio fidelity and remove any generic fallback that hides real game behavior.

#### Steam

- **Priority:** P4.
- **Placement/dependency evidence:** 8 placement(s); status: created x8; zones: HeavensDoorInsideZone x8; model/archive aliases: Steam x8
- **Current decomp progress:** sibling-decomp factory/source maps to SimpleEffectObj using no fixed archive. Source evidence: `../src/Game/Effect/SimpleEffectObj.cpp`, `../include/Game/Effect/SimpleEffectObj.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Visual, effect, audio, guidance, or background placement. This should stay behind general render/effect/audio compatibility and not become route-specific code.
- **Imperfect-MVP replacement path:** MVP action: defer until after the imperfect demo works. Revisit for Aurora/render/effect/audio fidelity and remove any generic fallback that hides real game behavior.

#### VROrbit

- **Priority:** P4.
- **Placement/dependency evidence:** 1 placement(s); status: created_model_fallback x1; zones: HeavensDoorGalaxy x1; model/archive aliases: VROrbit x1
- **Current decomp progress:** sibling-decomp factory/source maps to ProjectionMapSky using VROrbit. Source evidence: `../src/Game/Map/Sky.cpp`, `../include/Game/Map/Sky.hpp`
- **pc-port status:** not imported as original `Game/` code in `pc-port`; current tree still relies on the tiny factory and/or generic placement/model compatibility.
- **Where it fits:** Visual, effect, audio, guidance, or background placement. This should stay behind general render/effect/audio compatibility and not become route-specific code.
- **Imperfect-MVP replacement path:** MVP action: defer until after the imperfect demo works. Revisit for Aurora/render/effect/audio fidelity and remove any generic fallback that hides real game behavior.

## Maximalist Repo-Wide Decomp Inventory

The route-focused list above is the immediate bunny-demo path. The inventory below is merged from `../DECOMP_NEEDED.md` and is intentionally maximal: every `../src/Game` source file that is not yet present in `src/Game` is treated as import/decomp backlog, and every pc-port-only `Game/` declaration or implementation is treated as either decomp/header-recovery work or code that should move out to compat/Aurora if it is not original game logic.

This report records what must be decompiled, imported, or moved out of `Game/` to keep core game code close to `../src/Game`. Rendering and layout backend behavior is allowed to deviate when it belongs in Aurora or the compatibility layer, but those deviations should be periodically revisited now that Aurora is the renderer/windowing direction.

Generated from the current tree on 2026-05-30.

### Snapshot

| Metric | Count |
|---|---:|
| `src/Game` files in pc-port | 192 |
| `../src/Game` files in decomp source | 1345 |
| `src/Game` files missing from `../src/Game` | 105 |
| `../src/Game` files not yet present in `src/Game` | 1258 |
| Common `Game/` files that differ | 86 |

### How To Read This

- **Already decompiled, not imported** means the file exists in `../src/Game` and should generally be copied/ported into `src/Game` once its dependencies are satisfied.
- **Needs decomp/header recovery** means the pc-port currently has local `Game/` declarations or implementation with no matching decomp file. These should be reconciled with real decomp output or moved to compat if they are not original game code.
- **Compat/Aurora, not decomp** means renderer, layout backend, host input/windowing, filesystem, NAND, RFL, and debug harness work that should live outside core `Game/` unless the decomp source actually owns it.

### Priority Buckets

1. **Core sequence and scene flow:** replace local story/sequence routing with source-close `StorySequenceExecutor`, scene objects, stage requests, and original scene transition behavior. Keep only the debug bunny-after-picturebook exception until the broader route no longer needs it.
2. **NameObj and actor coverage:** import or decompile the original factory table and actor classes so stage placement creates real game objects instead of compat-side model fallbacks.
3. **File-select and save state:** reconcile file-select, save, NAND, RFL/Mii, and user-file classes against decomp source; move host storage and synthetic data into compat.
4. **MR utility surface:** replace PC-native `MR::*` implementations in `Game/Util` with source-close game logic backed by lower-level compatibility services.
5. **Camera, sensors, events, and player systems:** recover enough original systems that actors/scenes do not call directly into host runtime services.
6. **Rendering/layout exception:** `Screen/`, layout parsing, J3D/GX draw details, light upload, texture copy, and Aurora backend behavior may intentionally differ, but core scene/actor decisions should not be hidden in those layers.

### Needs Decomp Or Header Recovery In `Game/`

These files exist in `src/Game` but not in `../src/Game`. Implementation files here are the highest concern. Headers may be local declarations created because the decomp tree does not carry matching headers; each should be checked against real decomp declarations before being treated as stable API. Layout/render implementation files should be moved toward compat/Aurora rather than forced to match decomp if the original game delegated that behavior to platform libraries.

#### PC-only Implementation Files

```text
Screen/LayoutManager.cpp
Screen/ReplaceTagProcessor.cpp
Util/PlayerUtil.cpp
Util/SequenceUtil.cpp
```

#### PC-only Header Files

```text
Camera/CameraTargetArg.hpp
Camera/CameraTargetMtx.hpp
Demo/PrologueDirector.hpp
LiveActor/ActorCameraInfo.hpp
LiveActor/ActorLightCtrl.hpp
LiveActor/ActorStateKeeper.hpp
LiveActor/HitSensor.hpp
LiveActor/LiveActor.hpp
LiveActor/ModelObj.hpp
LiveActor/Nerve.hpp
LiveActor/PartsModel.hpp
LiveActor/Spine.hpp
Map/FileSelectCameraController.hpp
Map/FileSelectEffect.hpp
Map/FileSelectFunc.hpp
Map/FileSelectIconID.hpp
Map/FileSelectItem.hpp
Map/FileSelectItemDelegator.hpp
Map/FileSelectModel.hpp
Map/FileSelectSky.hpp
Map/FileSelector.hpp
Map/LightDataHolder.hpp
Map/LightFunction.hpp
Map/LightPointCtrl.hpp
Map/LightZoneDataHolder.hpp
NPC/MiiFacePartsHolder.hpp
NameObj/NameObj.hpp
NameObj/NameObjArchiveListCollector.hpp
NameObj/NameObjFactory.hpp
Scene/Scene.hpp
Scene/SceneFunction.hpp
Scene/SceneObjHolder.hpp
Screen/BackButton.hpp
Screen/BrosButton.hpp
Screen/ButtonPaneController.hpp
Screen/CaptureScreenDirector.hpp
Screen/EncouragePal60Window.hpp
Screen/FileSelectButton.hpp
Screen/FileSelectInfo.hpp
Screen/FileSelectNumber.hpp
Screen/GalaxyMapGalaxyPlain.hpp
Screen/IconAButton.hpp
Screen/InformationMessage.hpp
Screen/LayoutActor.hpp
Screen/LayoutActorFlag.hpp
Screen/LayoutManager.hpp
Screen/LayoutPaneCtrl.hpp
Screen/Manual2P.hpp
Screen/MiiConfirmIcon.hpp
Screen/MiiSelect.hpp
Screen/PictureBookCloseButton.hpp
Screen/PictureBookLayout.hpp
Screen/PrologueLetter.hpp
Screen/ProloguePictureBook.hpp
Screen/ReplaceTagProcessor.hpp
Screen/SaveIcon.hpp
Screen/ScreenAlphaCapture.hpp
Screen/SimpleLayout.hpp
Screen/SysInfoWindow.hpp
Screen/TitleSequenceProduct.hpp
Screen/YesNoController.hpp
System/ConfigDataHolder.hpp
System/GameDataFunction.hpp
System/GameDataHolder.hpp
System/GameSequenceFunction.hpp
System/NANDManager.hpp
System/NerveExecutor.hpp
System/SaveDataBannerCreator.hpp
System/SaveDataHandleSequence.hpp
System/SaveDataHandler.hpp
System/StorySequenceExecutor.hpp
System/SysConfigFile.hpp
System/UserFile.hpp
Util/ActorCameraUtil.hpp
Util/ActorSensorUtil.hpp
Util/CameraUtil.hpp
Util/DemoUtil.hpp
Util/DrawUtil.hpp
Util/EventUtil.hpp
Util/FileUtil.hpp
Util/Functor.hpp
Util/GamePadUtil.hpp
Util/JMapInfo.hpp
Util/JMapUtil.hpp
Util/LayoutUtil.hpp
Util/LightUtil.hpp
Util/LiveActorUtil.hpp
Util/MathUtil.hpp
Util/MessageUtil.hpp
Util/NerveUtil.hpp
Util/ObjUtil.hpp
Util/PlayerUtil.hpp
Util/ScreenUtil.hpp
Util/SequenceUtil.hpp
Util/SoundUtil.hpp
Util/StarPointerUtil.hpp
Util/StringUtil.hpp
Util/SystemUtil.hpp
Util/TriggerChecker.hpp
```

#### PC-only Other Files

```text
.clang-format
xmake.lua
```

### Already Decompiled But Not Imported Into `src/Game`

These files already exist in `../src/Game` but are absent from `src/Game`. They are not "needs decomp" in the strict sense; they are import/port backlog unless their dependencies require additional decomp work.

```text
Animation/AnmPlayer.cpp
Animation/BckCtrl.cpp
Animation/BpkPlayer.cpp
Animation/BrkPlayer.cpp
Animation/BtkPlayer.cpp
Animation/BtpPlayer.cpp
Animation/BvaPlayer.cpp
Animation/LayoutAnmPlayer.cpp
Animation/XanimeCore.cpp
Animation/XanimePlayer.cpp
Animation/XanimeResource.cpp
AreaObj/AreaForm.cpp
AreaObj/AreaFormDrawer.cpp
AreaObj/AreaObj.cpp
AreaObj/AreaObjContainer.cpp
AreaObj/AreaObjFollower.cpp
AreaObj/AstroChangeStageCube.cpp
AreaObj/AudioEffectArea.cpp
AreaObj/BgmProhibitArea.cpp
AreaObj/BigBubbleCameraArea.cpp
AreaObj/BigBubbleGoalArea.cpp
AreaObj/BloomArea.cpp
AreaObj/CameraRepulsiveArea.cpp
AreaObj/ChangeBgmCube.cpp
AreaObj/CollisionArea.cpp
AreaObj/CubeCamera.cpp
AreaObj/DeathArea.cpp
AreaObj/DepthOfFieldArea.cpp
AreaObj/GlaringLightArea.cpp
AreaObj/HazeCube.cpp
AreaObj/ImageEffectArea.cpp
AreaObj/LightArea.cpp
AreaObj/LightAreaHolder.cpp
AreaObj/MercatorTransformCube.cpp
AreaObj/MessageArea.cpp
AreaObj/PlayerSeArea.cpp
AreaObj/QuakeEffectArea.cpp
AreaObj/RestartCube.cpp
AreaObj/ScreenBlurArea.cpp
AreaObj/SimpleBloomArea.cpp
AreaObj/SoundEmitterCube.cpp
AreaObj/SoundEmitterSphere.cpp
AreaObj/SpinGuidanceArea.cpp
AreaObj/SunLightArea.cpp
AreaObj/SwitchArea.cpp
AreaObj/WarpCube.cpp
AreaObj/WaterArea.cpp
AudioLib/AudFader.cpp
AudioLib/AudRemixMgr.cpp
AudioLib/AudRemixSequencer.cpp
AudioLib/AudSoundObjHolder.cpp
AudioLib/AudWrap.cpp
Boss/BossAccessor.cpp
Boss/BossBegoman.cpp
Boss/BossBegomanHead.cpp
Boss/BossKameck.cpp
Boss/BossKameckAction.cpp
Boss/BossKameckBarrier.cpp
Boss/BossKameckBattleDemo.cpp
Boss/BossKameckBattlePattarn.cpp
Boss/BossKameckMoveRail.cpp
Boss/BossKameckSequencer.cpp
Boss/BossKameckStateBattle.cpp
Boss/BossKameckVs1.cpp
Boss/BossKameckVs2.cpp
Boss/BossStinkBug.cpp
Boss/BossStinkBugActionBase.cpp
Boss/BossStinkBugActionFlyHigh.cpp
Boss/BossStinkBugActionFlyLow.cpp
Boss/BossStinkBugActionGround.cpp
Boss/BossStinkBugActionSequencer.cpp
Boss/BossStinkBugAngryDemo.cpp
Boss/BossStinkBugBomb.cpp
Boss/BossStinkBugBombHolder.cpp
Boss/BossStinkBugFinishDemo.cpp
Boss/BossStinkBugFlyDemo.cpp
Boss/BossStinkBugFunction.cpp
Boss/BossStinkBugOpeningDemo.cpp
Boss/DinoPackun.cpp
Boss/DinoPackunAction.cpp
Boss/DinoPackunBall.cpp
Boss/DinoPackunBattleEgg.cpp
Boss/DinoPackunBattleEggVs2.cpp
Boss/DinoPackunBattleVs1Lv1.cpp
Boss/DinoPackunBattleVs1Lv2.cpp
Boss/DinoPackunBattleVs2Lv1.cpp
Boss/DinoPackunDemo.cpp
Boss/DinoPackunDemoPosition.cpp
Boss/DinoPackunEggShell.cpp
Boss/DinoPackunFire.cpp
Boss/DinoPackunSequencer.cpp
Boss/DinoPackunStateAwake.cpp
Boss/DinoPackunStateDamage.cpp
Boss/DinoPackunStateFire.cpp
Boss/DinoPackunTail.cpp
Boss/DinoPackunTailNode.cpp
Boss/DinoPackunTailPart.cpp
Boss/DinoPackunTailRoot.cpp
Boss/DinoPackunTrackFire.cpp
Boss/DinoPackunVs1.cpp
Boss/DinoPackunVs2.cpp
Boss/Dodoryu.cpp
Boss/DodoryuStateLv2.cpp
Boss/KoopaDemoPowerUp.cpp
Boss/KoopaFireStairs.cpp
Boss/KoopaPlanetShadow.cpp
Boss/KoopaPowerUpSwitch.cpp
Boss/KoopaRestarterVs3.cpp
Boss/KoopaSensorCtrl.cpp
Boss/KoopaSwitchKeeper.cpp
Boss/KoopaViewSwitchKeeper.cpp
Boss/OtaKing.cpp
Boss/Polta.cpp
Boss/PoltaActionBase.cpp
Boss/PoltaActionSequencer.cpp
Boss/PoltaArm.cpp
Boss/PoltaBattleLv1.cpp
Boss/PoltaBattleLv2.cpp
Boss/PoltaDemo.cpp
Boss/PoltaFunction.cpp
Boss/PoltaGroundRock.cpp
Boss/PoltaGroundRockHolder.cpp
Boss/PoltaRock.cpp
Boss/PoltaRockHolder.cpp
Boss/PoltaSensorCtrl.cpp
Boss/PoltaStateAttackGround.cpp
Boss/PoltaStateGenerateRock.cpp
Boss/PoltaStateGroundRockAttack.cpp
Boss/PoltaStatePunch.cpp
Boss/PoltaStateStagger.cpp
Boss/PoltaWaitStart.cpp
Boss/SkeletalFishBaby.cpp
Boss/SkeletalFishBabyRail.cpp
Boss/SkeletalFishBabyRailHolder.cpp
Boss/SkeletalFishBoss.cpp
Boss/SkeletalFishBossBattleDirector.cpp
Boss/SkeletalFishBossFunc.cpp
Boss/SkeletalFishBossInfo.cpp
Boss/SkeletalFishBossRail.cpp
Boss/SkeletalFishBossRailHolder.cpp
Boss/SkeletalFishGuard.cpp
Boss/SkeletalFishGuardHolder.cpp
Boss/SkeletalFishJointCalc.cpp
Boss/SkeletalFishRailControl.cpp
Boss/TombSpider.cpp
Boss/TombSpiderAcid.cpp
Boss/TombSpiderAction1st.cpp
Boss/TombSpiderAction2nd.cpp
Boss/TombSpiderActionBase.cpp
Boss/TombSpiderActionCocoon.cpp
Boss/TombSpiderDemo.cpp
Boss/TombSpiderEnvironment.cpp
Boss/TombSpiderFunction.cpp
Boss/TombSpiderGland.cpp
Boss/TombSpiderParts.cpp
Boss/TombSpiderSensorCtrl.cpp
Boss/TombSpiderStateSwoon.cpp
Boss/TombSpiderThreadAttacher.cpp
Boss/TombSpiderVitalSpot.cpp
Boss/TripodBoss.cpp
Boss/TripodBossAccesser.cpp
Boss/TripodBossBaseJointPosition.cpp
Boss/TripodBossBreakMovement.cpp
Boss/TripodBossCoin.cpp
Boss/TripodBossCore.cpp
Boss/TripodBossFixParts.cpp
Boss/TripodBossFixPartsBase.cpp
Boss/TripodBossGuardWall.cpp
Boss/TripodBossGuardWallPart.cpp
Boss/TripodBossKillerGenerater.cpp
Boss/TripodBossKillerGeneraterCircle.cpp
Boss/TripodBossKinokoOneUp.cpp
Boss/TripodBossLeg.cpp
Boss/TripodBossMovableArea.cpp
Boss/TripodBossRailMoveParts.cpp
Boss/TripodBossRotateParts.cpp
Boss/TripodBossShell.cpp
Boss/TripodBossStepPoint.cpp
Boss/TripodBossStepSequence.cpp
Boss/TripodBossStepStartArea.cpp
Camera/CamHeliEffector.cpp
Camera/CamKarikariEffector.cpp
Camera/CamTranslatorAnim.cpp
Camera/CamTranslatorBehind.cpp
Camera/CamTranslatorBlackHole.cpp
Camera/CamTranslatorCharmedFix.cpp
Camera/CamTranslatorCharmedTripodBoss.cpp
Camera/CamTranslatorCharmedVecReg.cpp
Camera/CamTranslatorCharmedVecRegTower.cpp
Camera/CamTranslatorCubePlanet.cpp
Camera/CamTranslatorDPD.cpp
Camera/CamTranslatorDead.cpp
Camera/CamTranslatorFix.cpp
Camera/CamTranslatorFixedPoint.cpp
Camera/CamTranslatorFixedThere.cpp
Camera/CamTranslatorFollow.cpp
Camera/CamTranslatorFooFighter.cpp
Camera/CamTranslatorFooFighterPlanet.cpp
Camera/CamTranslatorFrontAndBack.cpp
Camera/CamTranslatorGround.cpp
Camera/CamTranslatorInnerCylinder.cpp
Camera/CamTranslatorInwardSphere.cpp
Camera/CamTranslatorInwardTower.cpp
Camera/CamTranslatorMedianPlanet.cpp
Camera/CamTranslatorMedianTower.cpp
Camera/CamTranslatorMtxRegParallel.cpp
Camera/CamTranslatorObjParallel.cpp
Camera/CamTranslatorParallel.cpp
Camera/CamTranslatorRaceFollow.cpp
Camera/CamTranslatorRailDemo.cpp
Camera/CamTranslatorRailFollow.cpp
Camera/CamTranslatorRailWatch.cpp
Camera/CamTranslatorSlide.cpp
Camera/CamTranslatorSpiral.cpp
Camera/CamTranslatorTalk.cpp
Camera/CamTranslatorTower.cpp
Camera/CamTranslatorTowerPos.cpp
Camera/CamTranslatorTripodBoss.cpp
Camera/CamTranslatorTripodBossJoint.cpp
Camera/CamTranslatorTripodPlanet.cpp
Camera/CamTranslatorTrundle.cpp
Camera/CamTranslatorTwistedPassage.cpp
Camera/CamTranslatorWaterFollow.cpp
Camera/CamTranslatorWaterPlanet.cpp
Camera/CamTranslatorWaterPlanetBoss.cpp
Camera/CamTranslatorWonderPlanet.cpp
Camera/Camera.cpp
Camera/CameraAnim.cpp
Camera/CameraBehind.cpp
Camera/CameraBlackHole.cpp
Camera/CameraCalc.cpp
Camera/CameraCharmedFix.cpp
Camera/CameraCharmedTripodBoss.cpp
Camera/CameraCharmedVecReg.cpp
Camera/CameraCharmedVecRegTower.cpp
Camera/CameraContext.cpp
Camera/CameraCover.cpp
Camera/CameraCubePlanet.cpp
Camera/CameraDPD.cpp
Camera/CameraDead.cpp
Camera/CameraDirector.cpp
Camera/CameraFix.cpp
Camera/CameraFixedPoint.cpp
Camera/CameraFixedThere.cpp
Camera/CameraFollow.cpp
Camera/CameraFooFighter.cpp
Camera/CameraFooFighterPlanet.cpp
Camera/CameraFrontAndBack.cpp
Camera/CameraGround.cpp
Camera/CameraHeightArrange.cpp
Camera/CameraHolder.cpp
Camera/CameraInnerCylinder.cpp
Camera/CameraInwardSphere.cpp
Camera/CameraInwardTower.cpp
Camera/CameraLocalUtil.cpp
Camera/CameraMan.cpp
Camera/CameraManEvent.cpp
Camera/CameraManGame.cpp
Camera/CameraManPause.cpp
Camera/CameraManSubjective.cpp
Camera/CameraMedianPlanet.cpp
Camera/CameraMedianTower.cpp
Camera/CameraMtxRegParallel.cpp
Camera/CameraObjParallel.cpp
Camera/CameraParallel.cpp
Camera/CameraParamChunk.cpp
Camera/CameraParamChunkHolder.cpp
Camera/CameraParamChunkID.cpp
Camera/CameraParamString.cpp
Camera/CameraPoseParam.cpp
Camera/CameraRaceFollow.cpp
Camera/CameraRailDemo.cpp
Camera/CameraRailFollow.cpp
Camera/CameraRailHolder.cpp
Camera/CameraRailWatch.cpp
Camera/CameraRegisterHolder.cpp
Camera/CameraRotChecker.cpp
Camera/CameraShakePatternImpl.cpp
Camera/CameraShakeTask.cpp
Camera/CameraShaker.cpp
Camera/CameraSlide.cpp
Camera/CameraSpiral.cpp
Camera/CameraSubjective.cpp
Camera/CameraTalk.cpp
Camera/CameraTargetHolder.cpp
Camera/CameraTargetObj.cpp
Camera/CameraTower.cpp
Camera/CameraTowerBase.cpp
Camera/CameraTowerPos.cpp
Camera/CameraTripodBoss.cpp
Camera/CameraTripodBossJoint.cpp
Camera/CameraTripodPlanet.cpp
Camera/CameraTrundle.cpp
Camera/CameraTwistedPassage.cpp
Camera/CameraViewInterpolator.cpp
Camera/CameraWaterFollow.cpp
Camera/CameraWaterPlanet.cpp
Camera/CameraWaterPlanetBoss.cpp
Camera/CameraWonderPlanet.cpp
Camera/DotCamParams.cpp
Camera/GameCameraCreator.cpp
Camera/OnlyCamera.cpp
Demo/AstroDemoFunction.cpp
Demo/AstroDomeDemoStarter.cpp
Demo/DemoActionKeeper.cpp
Demo/DemoCameraFunction.cpp
Demo/DemoCastGroup.cpp
Demo/DemoCastGroupHolder.cpp
Demo/DemoCastSubGroup.cpp
Demo/DemoCtrlBase.cpp
Demo/DemoDirector.cpp
Demo/DemoExecutor.cpp
Demo/DemoExecutorFunction.cpp
Demo/DemoFunction.cpp
Demo/DemoKoopaJrShip.cpp
Demo/DemoPadRumbler.cpp
Demo/DemoParamCommonDataTable.cpp
Demo/DemoPlayerKeeper.cpp
Demo/DemoPositionController.cpp
Demo/DemoSimpleCastHolder.cpp
Demo/DemoStartRequestHolder.cpp
Demo/DemoSubPartKeeper.cpp
Demo/DemoTalkAnimCtrl.cpp
Demo/DemoTimeKeeper.cpp
Demo/GrandStarReturnDemoStarter.cpp
Demo/ReturnDemoRailMove.cpp
Demo/ScenarioStarter.cpp
Demo/StarReturnDemoStarter.cpp
Effect/AstroEffectObj.cpp
Effect/EffectObjGravityDust.cpp
Effect/EffectSystem.cpp
Effect/MultiEmitterParticleCallBack.cpp
Effect/ParticleDrawExecutor.cpp
Effect/ParticleEmitter.cpp
Effect/ParticleEmitterHolder.cpp
Effect/RandomEffectObj.cpp
Effect/SimpleEffectObj.cpp
Effect/SingleEmitter.cpp
Effect/SpinPullParticleCallBack.cpp
Enemy/AnimScaleController.cpp
Enemy/BallBeamer.cpp
Enemy/Balloonfish.cpp
Enemy/BasaBasa.cpp
Enemy/BegomanBaby.cpp
Enemy/BegomanBase.cpp
Enemy/BegomanLauncher.cpp
Enemy/BegomanSpike.cpp
Enemy/BegomanSpring.cpp
Enemy/BegomanSpringHead.cpp
Enemy/Birikyu.cpp
Enemy/BombBird.cpp
Enemy/BombHei.cpp
Enemy/BombHeiLauncher.cpp
Enemy/BombTeresa.cpp
Enemy/CannonShellBase.cpp
Enemy/CocoNutBall.cpp
Enemy/CocoSambo.cpp
Enemy/DharmaSambo.cpp
Enemy/Dossun.cpp
Enemy/ElectricPressureBullet.cpp
Enemy/EyeBeamer.cpp
Enemy/FireBall.cpp
Enemy/FireBubble.cpp
Enemy/Gesso.cpp
Enemy/HammerHeadPackun.cpp
Enemy/Hanachan.cpp
Enemy/HomingKiller.cpp
Enemy/IceMerameraKing.cpp
Enemy/ItemGenerator.cpp
Enemy/Jellyfish.cpp
Enemy/JellyfishElectric.cpp
Enemy/Jiraira.cpp
Enemy/JumpBeamer.cpp
Enemy/JumpGuarder.cpp
Enemy/JumpSpider.cpp
Enemy/Kabokuri.cpp
Enemy/KameckBeamHolder.cpp
Enemy/KameckFireBall.cpp
Enemy/Kanina.cpp
Enemy/Karikari.cpp
Enemy/KarikariDirector.cpp
Enemy/Karon.cpp
Enemy/Kiraira.cpp
Enemy/KoopaJrShip.cpp
Enemy/KoopaJrShipCannonMainShell.cpp
Enemy/KoopaJrShipCannonShell.cpp
Enemy/KoteBug.cpp
Enemy/Kuribo.cpp
Enemy/KuriboChief.cpp
Enemy/KuriboMini.cpp
Enemy/MechanicKoopaMini.cpp
Enemy/Metbo.cpp
Enemy/Mogu.cpp
Enemy/MoguStone.cpp
Enemy/Mogucchi.cpp
Enemy/MogucchiHill.cpp
Enemy/MogucchiRefuseTerritory.cpp
Enemy/MogucchiShooter.cpp
Enemy/NokonokoLand.cpp
Enemy/OnimasuJump.cpp
Enemy/OnimasuPivot.cpp
Enemy/OtaRock.cpp
Enemy/PackunPetit.cpp
Enemy/Petari.cpp
Enemy/Poihana.cpp
Enemy/Pukupuku.cpp
Enemy/RingBeam.cpp
Enemy/RingBeamer.cpp
Enemy/SamboHead.cpp
Enemy/SearchBeamer.cpp
Enemy/Snakehead.cpp
Enemy/SpinHitController.cpp
Enemy/StinkBugBase.cpp
Enemy/StinkBugParent.cpp
Enemy/StinkBugSmall.cpp
Enemy/StringSpider.cpp
Enemy/TakoHei.cpp
Enemy/Takobo.cpp
Enemy/Teresa.cpp
Enemy/TeresaWater.cpp
Enemy/TerritoryMover.cpp
Enemy/Unizo.cpp
Enemy/UnizoLauncher.cpp
Enemy/WalkerStateBindStarPointer.cpp
Enemy/WalkerStateBlowDamage.cpp
Enemy/WalkerStateChase.cpp
Enemy/WalkerStateFindPlayer.cpp
Enemy/WalkerStateFunction.cpp
Enemy/WalkerStateParam.cpp
Enemy/WalkerStateRunaway.cpp
Enemy/WalkerStateStagger.cpp
Enemy/WalkerStateWander.cpp
Enemy/WaterBazooka.cpp
Enemy/WaterBazookaCapsule.cpp
GameAudio/AudCameraWatcher.cpp
GameAudio/AudEffectDirector.cpp
Gravity/ConeGravity.cpp
Gravity/CubeGravity.cpp
Gravity/DiskGravity.cpp
Gravity/DiskTorusGravity.cpp
Gravity/GlobalGravityObj.cpp
Gravity/GraviryFollower.cpp
Gravity/GravityCreator.cpp
Gravity/GravityInfo.cpp
Gravity/ParallelGravity.cpp
Gravity/PlanetGravity.cpp
Gravity/PlanetGravityManager.cpp
Gravity/PointGravity.cpp
Gravity/SegmentGravity.cpp
Gravity/WireGravity.cpp
LiveActor/ActiveActorList.cpp
LiveActor/ActorAnimKeeper.cpp
LiveActor/ActorJointCtrl.cpp
LiveActor/ActorStateBase.cpp
LiveActor/ActorStateKeeper.cpp
LiveActor/AllLiveActorGroup.cpp
LiveActor/AnimationRandomPlayer.cpp
LiveActor/Binder.cpp
LiveActor/ClippingActorHolder.cpp
LiveActor/ClippingActorInfo.cpp
LiveActor/ClippingDirector.cpp
LiveActor/ClippingGroupHolder.cpp
LiveActor/ClippingJudge.cpp
LiveActor/DynamicJointCtrl.cpp
LiveActor/FaceJointCtrl.cpp
LiveActor/FlashingCtrl.cpp
LiveActor/HitSensorInfo.cpp
LiveActor/HitSensorKeeper.cpp
LiveActor/IKJointCtrl.cpp
LiveActor/LiveActorFlag.cpp
LiveActor/LiveActorGroup.cpp
LiveActor/LiveActorGroupArray.cpp
LiveActor/LodCtrl.cpp
LiveActor/MaterialCtrl.cpp
LiveActor/MessageSensorHolder.cpp
LiveActor/MirrorActor.cpp
LiveActor/MirrorCamera.cpp
LiveActor/MirrorReflectionModel.cpp
LiveActor/ModelManager.cpp
LiveActor/RailRider.cpp
LiveActor/SensorHitChecker.cpp
LiveActor/ShadowController.cpp
LiveActor/ShadowDrawer.cpp
LiveActor/ShadowSurfaceDrawer.cpp
LiveActor/ShadowVolumeBox.cpp
LiveActor/ShadowVolumeCylinder.cpp
LiveActor/ShadowVolumeDrawer.cpp
LiveActor/ShadowVolumeLine.cpp
LiveActor/ShadowVolumeModel.cpp
LiveActor/ShadowVolumeOval.cpp
LiveActor/ShadowVolumeOvalPole.cpp
LiveActor/ShadowVolumeSphere.cpp
LiveActor/SpotMarkLight.cpp
LiveActor/ViewGroupCtrl.cpp
LiveActor/VolumeModelDrawer.cpp
Map/ActorAppearSwitchListener.cpp
Map/Air.cpp
Map/BezierRail.cpp
Map/Butterfly.cpp
Map/CollisionCategorizedKeeper.cpp
Map/CollisionCode.cpp
Map/CollisionDirector.cpp
Map/CollisionParts.cpp
Map/FishGroup.cpp
Map/Flag.cpp
Map/GravityDust.cpp
Map/GroundChecker.cpp
Map/GroupSwitchWatcher.cpp
Map/Halo.cpp
Map/HitInfo.cpp
Map/KCollision.cpp
Map/KCollisionPlus.cpp
Map/KoopaBattleMapCoinPlate.cpp
Map/KoopaBattleMapPlate.cpp
Map/KoopaBattleMapStair.cpp
Map/LavaShellTower.cpp
Map/LavaSunPlanet.cpp
Map/LightDataHolder.cpp
Map/LightDirector.cpp
Map/LightPointCtrl.cpp
Map/LightZoneDataHolder.cpp
Map/NamePosHolder.cpp
Map/OceanBowl.cpp
Map/OceanBowlBloomDrawer.cpp
Map/OceanBowlPoint.cpp
Map/OceanHomeMapCtrl.cpp
Map/OceanRing.cpp
Map/OceanRingBloomDrawer.cpp
Map/OceanRingDrawer.cpp
Map/OceanRingPipe.cpp
Map/OceanRingPipeInside.cpp
Map/OceanRingPipeOutside.cpp
Map/OceanSphere.cpp
Map/OceanSpherePoint.cpp
Map/PlanetMap.cpp
Map/PlanetMapCreator.cpp
Map/PlanetMapWithoutHighModel.cpp
Map/QuakeEffectGenerator.cpp
Map/RaceManager.cpp
Map/RaceRail.cpp
Map/RailGraph.cpp
Map/RailGraphEdge.cpp
Map/RailGraphIter.cpp
Map/RailGraphNode.cpp
Map/RailPart.cpp
Map/ScenarioSelectStar.cpp
Map/SeaGull.cpp
Map/Sky.cpp
Map/SleepController.cpp
Map/SleepControllerHolder.cpp
Map/SpaceInner.cpp
Map/SphereSelector.cpp
Map/SphereSelectorHandle.cpp
Map/StageSwitch.cpp
Map/SunshadeMapHolder.cpp
Map/SunshadeMapParts.cpp
Map/SwitchSynchronizer.cpp
Map/SwitchWatcher.cpp
Map/SwitchWatcherHolder.cpp
Map/TimerSwitch.cpp
Map/WaterAreaHolder.cpp
Map/WaterInfo.cpp
Map/WaterPlant.cpp
Map/WaterPoint.cpp
Map/WaterRoad.cpp
Map/WhirlPoolAccelerator.cpp
MapObj/AirBubble.cpp
MapObj/AirBubbleGenerator.cpp
MapObj/AirBubbleHolder.cpp
MapObj/AnmModelObj.cpp
MapObj/ArrowSwitch.cpp
MapObj/ArrowSwitchMulti.cpp
MapObj/ArrowSwitchMultiHolder.cpp
MapObj/AssemblyBlock.cpp
MapObj/AstroCore.cpp
MapObj/AstroCountDownPlate.cpp
MapObj/AstroDome.cpp
MapObj/AstroDomeAsteroid.cpp
MapObj/AstroDomeBlueStar.cpp
MapObj/AstroDomeCameraController.cpp
MapObj/AstroDomeComet.cpp
MapObj/AstroDomeDemoAstroGalaxy.cpp
MapObj/AstroDomeGalaxySelector.cpp
MapObj/AstroDomeOrbit.cpp
MapObj/AstroDomeSky.cpp
MapObj/AstroMapBoard.cpp
MapObj/AstroMapObj.cpp
MapObj/AstroMapObjFunction.cpp
MapObj/AstroOverlookObj.cpp
MapObj/BallOpener.cpp
MapObj/BallRail.cpp
MapObj/Banekiti.cpp
MapObj/BattleShipElevator.cpp
MapObj/BeamGoRoundPlanet.cpp
MapObj/BeeFlowerHover.cpp
MapObj/BenefitItemInvincible.cpp
MapObj/BenefitItemLifeUp.cpp
MapObj/BenefitItemObj.cpp
MapObj/BigBubbleDrawer.cpp
MapObj/BigBubbleGenerator.cpp
MapObj/BigBubbleHolder.cpp
MapObj/BigBubbleMoveLimitter.cpp
MapObj/BigFan.cpp
MapObj/BigFanHolder.cpp
MapObj/BlackHole.cpp
MapObj/BlueChip.cpp
MapObj/BlueStarCupsulePlanet.cpp
MapObj/BreakableCage.cpp
MapObj/BrightObj.cpp
MapObj/BumpAppearPlanet.cpp
MapObj/Candlestand.cpp
MapObj/CannonFortressBreakStep.cpp
MapObj/CapsuleCage.cpp
MapObj/ChipBase.cpp
MapObj/ChipCounter.cpp
MapObj/ChipGroup.cpp
MapObj/ChipHolder.cpp
MapObj/ChooChooTrain.cpp
MapObj/CircleCoinGroup.cpp
MapObj/ClipArea.cpp
MapObj/ClipAreaDrop.cpp
MapObj/ClipAreaDropHolder.cpp
MapObj/ClipAreaDropLaser.cpp
MapObj/ClipAreaHolder.cpp
MapObj/ClipAreaMovable.cpp
MapObj/ClipAreaShape.cpp
MapObj/ClipFieldMapParts.cpp
MapObj/ClipFieldSwitch.cpp
MapObj/CocoNut.cpp
MapObj/CoconutTree.cpp
MapObj/CoconutTreeLeaf.cpp
MapObj/Coin.cpp
MapObj/CoinBox.cpp
MapObj/CoinGroup.cpp
MapObj/CoinHolder.cpp
MapObj/CoinReplica.cpp
MapObj/CoinRotater.cpp
MapObj/CoinSpot.cpp
MapObj/CollapsePlane.cpp
MapObj/CollectCounter.cpp
MapObj/CollisionBlocker.cpp
MapObj/CrystalCage.cpp
MapObj/CrystalCageMoving.cpp
MapObj/CrystalSwitch.cpp
MapObj/CutBushModelObj.cpp
MapObj/DashRing.cpp
MapObj/DeadLeaves.cpp
MapObj/DesertLandMoveSwitch.cpp
MapObj/DesertMovingLand.cpp
MapObj/DragonHeadFlower.cpp
MapObj/DriftWood.cpp
MapObj/DynamicCollisionObj.cpp
MapObj/EarthenPipe.cpp
MapObj/ElectricBall.cpp
MapObj/ElectricRail.cpp
MapObj/ElectricRailMoving.cpp
MapObj/ExterminationChecker.cpp
MapObj/FallDownBridge.cpp
MapObj/FallOutFieldDraw.cpp
MapObj/FallingSmallRock.cpp
MapObj/FireBar.cpp
MapObj/FirePressure.cpp
MapObj/FirePressureBullet.cpp
MapObj/FirePressureBulletHolder.cpp
MapObj/FirePressureRadiate.cpp
MapObj/FlameGun.cpp
MapObj/FlexibleSphere.cpp
MapObj/FlipPanel.cpp
MapObj/FloaterFloatingForce.cpp
MapObj/FloaterFloatingForceTypeNormal.cpp
MapObj/FloaterFunction.cpp
MapObj/Fountain.cpp
MapObj/FountainBig.cpp
MapObj/GCapture.cpp
MapObj/GCaptureTarget.cpp
MapObj/GeneralMapParts.cpp
MapObj/GravityLight.cpp
MapObj/GravityLightRoad.cpp
MapObj/GreenCaterpillarBig.cpp
MapObj/HatchWaterPlanet.cpp
MapObj/HeavensDoorDemoObj.cpp
MapObj/HipDropMoveObj.cpp
MapObj/HipDropRock.cpp
MapObj/HipDropSwitch.cpp
MapObj/HipDropTimerSwitch.cpp
MapObj/HitWallTimerSwitch.cpp
MapObj/IceStep.cpp
MapObj/IceVolcanoUpDownPlane.cpp
MapObj/InvisiblePolygonObj.cpp
MapObj/InvisiblePolygonObjGCapture.cpp
MapObj/IronCannonShell.cpp
MapObj/ItemAppearStone.cpp
MapObj/ItemBlock.cpp
MapObj/ItemBubble.cpp
MapObj/JetTurtle.cpp
MapObj/JumpHole.cpp
MapObj/JumpStand.cpp
MapObj/KeySwitch.cpp
MapObj/KillerGunnerSingle.cpp
MapObj/LargeChain.cpp
MapObj/LargeChainParts.cpp
MapObj/LavaBallRisingPlanetLava.cpp
MapObj/LavaBreakColumn.cpp
MapObj/LavaFloater.cpp
MapObj/LavaGalaxyParts.cpp
MapObj/LavaGeyser.cpp
MapObj/LavaHomeSeesaw.cpp
MapObj/LavaHomeVolcanoFlow.cpp
MapObj/LavaJamboSunPlanet.cpp
MapObj/LavaProminence.cpp
MapObj/LavaProminenceTriple.cpp
MapObj/LavaSteam.cpp
MapObj/LavaStrangeRock.cpp
MapObj/LotusLeaf.cpp
MapObj/MagicBell.cpp
MapObj/ManholeCover.cpp
MapObj/MapObjActor.cpp
MapObj/MapObjActorInitInfo.cpp
MapObj/MapObjConnector.cpp
MapObj/MapParts.cpp
MapObj/MapPartsAppearController.cpp
MapObj/MapPartsBreaker.cpp
MapObj/MapPartsFloatingForce.cpp
MapObj/MapPartsFunction.cpp
MapObj/MapPartsRailGuideDrawer.cpp
MapObj/MapPartsRailGuideHolder.cpp
MapObj/MapPartsRailGuidePoint.cpp
MapObj/MapPartsRailPointPassChecker.cpp
MapObj/MapPartsRotator.cpp
MapObj/MarblePlanet.cpp
MapObj/MarioLauncher.cpp
MapObj/MarioLauncherAttractor.cpp
MapObj/MechaKoopaPartsArm.cpp
MapObj/MechaKoopaPartsHead.cpp
MapObj/MercatorFixParts.cpp
MapObj/MercatorRailMoveParts.cpp
MapObj/MercatorRotateParts.cpp
MapObj/MeteoContainer.cpp
MapObj/MeteorStrikeLauncher.cpp
MapObj/MiniatureGalaxy.cpp
MapObj/MiniatureGalaxyHolder.cpp
MapObj/MorphItemObjNeo.cpp
MapObj/NeedlePlant.cpp
MapObj/NormalMapBase.cpp
MapObj/NormalMapTestObj.cpp
MapObj/Note.cpp
MapObj/NoteFairy.cpp
MapObj/OceanFloaterLandParts.cpp
MapObj/OceanSmallTurtle.cpp
MapObj/OceanWaveFloater.cpp
MapObj/PTimerSwitch.cpp
MapObj/PalmIsland.cpp
MapObj/PeachCastleGardenPlanet.cpp
MapObj/PhantomShipBoxFloater.cpp
MapObj/PhantomShipBridge.cpp
MapObj/PhantomShipHandle.cpp
MapObj/PhantomTorch.cpp
MapObj/PicketSwitch.cpp
MapObj/PlantGroup.cpp
MapObj/PlantPoint.cpp
MapObj/PlantRailInfo.cpp
MapObj/PomponPlant.cpp
MapObj/PowerStar.cpp
MapObj/PowerStarAppearPoint.cpp
MapObj/PowerStarHolder.cpp
MapObj/PressureBase.cpp
MapObj/PrizeRing.cpp
MapObj/PunchBox.cpp
MapObj/PunchingKinoko.cpp
MapObj/PurpleCoinHolder.cpp
MapObj/PurpleCoinStarter.cpp
MapObj/QuarterRollGravityRoomArrow.cpp
MapObj/QuestionBoxGalleryObj.cpp
MapObj/QuestionCoin.cpp
MapObj/RailBlock.cpp
MapObj/RailCoin.cpp
MapObj/RailMoveObj.cpp
MapObj/RainCloud.cpp
MapObj/ReverseGravityRoomPlanet.cpp
MapObj/RevolvingWay.cpp
MapObj/Rock.cpp
MapObj/RockCreator.cpp
MapObj/RosettaChair.cpp
MapObj/RosettaPictureBook.cpp
MapObj/RotateMoveObj.cpp
MapObj/SandCapsuleInsidePlanet.cpp
MapObj/SandCapsulePressGround.cpp
MapObj/SandUpDownEffectObj.cpp
MapObj/SandUpDownTriRock.cpp
MapObj/Sandstorm.cpp
MapObj/ScrewSwitch.cpp
MapObj/ScrewSwitchReverse.cpp
MapObj/SeaBottomTriplePropeller.cpp
MapObj/SeesawMoveNut.cpp
MapObj/Shellfish.cpp
MapObj/ShockWaveGenerator.cpp
MapObj/ShootingStar.cpp
MapObj/SideSpikeMoveStep.cpp
MapObj/SimpleBreakableObj.cpp
MapObj/SimpleClipPartsObj.cpp
MapObj/SimpleFloaterObj.cpp
MapObj/SimpleMapObj.cpp
MapObj/SimpleNormalMapObj.cpp
MapObj/SimpleTimerObj.cpp
MapObj/SmallStone.cpp
MapObj/SnowCapsulePlanet.cpp
MapObj/SnowFloor.cpp
MapObj/SnowFloorTile.cpp
MapObj/SnowMan.cpp
MapObj/SnowplowSwitch.cpp
MapObj/SoundEmitter.cpp
MapObj/SpaceMine.cpp
MapObj/SpaceShipStep.cpp
MapObj/SpiderCoin.cpp
MapObj/SpiderMapBlock.cpp
MapObj/SpiderThread.cpp
MapObj/SpiderThreadHangInfo.cpp
MapObj/SpiderThreadMainPoint.cpp
MapObj/SpiderThreadPart.cpp
MapObj/SpiderThreadPoint.cpp
MapObj/SpiderThreadRadialLine.cpp
MapObj/SpiderThreadWindCtrl.cpp
MapObj/SpinDriver.cpp
MapObj/SpinDriverCamera.cpp
MapObj/SpinDriverOperateRing.cpp
MapObj/SpinDriverPathDrawer.cpp
MapObj/SpinDriverShootPath.cpp
MapObj/SpinLeverSwitch.cpp
MapObj/SpringJetWater.cpp
MapObj/SpringWaterFloaterSpot.cpp
MapObj/StageEffectDataTable.cpp
MapObj/StarPiece.cpp
MapObj/StarPieceDirector.cpp
MapObj/StarPieceFollowGroup.cpp
MapObj/StarPieceGroup.cpp
MapObj/StarPieceMother.cpp
MapObj/StarPieceSpot.cpp
MapObj/SubmarineSteam.cpp
MapObj/SubmarineVolcanoBigColumn.cpp
MapObj/Sun.cpp
MapObj/SuperSpinDriver.cpp
MapObj/SurprisedGalaxy.cpp
MapObj/SwingLight.cpp
MapObj/Swinger.cpp
MapObj/SwitchBox.cpp
MapObj/TimeAppearObj.cpp
MapObj/TimerMoveWall.cpp
MapObj/TrampleStar.cpp
MapObj/TransparentWall.cpp
MapObj/TreasureBoxCracked.cpp
MapObj/TreasureSpot.cpp
MapObj/Tsukidashikun.cpp
MapObj/TypicalDoor.cpp
MapObj/UFOBase.cpp
MapObj/UFOKinoko.cpp
MapObj/ValveSwitch.cpp
MapObj/WarpPod.cpp
MapObj/WatchTowerRotateStep.cpp
MapObj/WaterLeakPipe.cpp
MapObj/WaterPressure.cpp
MapObj/WaterPressureBullet.cpp
MapObj/WaterPressureBulletHolder.cpp
MapObj/WaterfallCaveCover.cpp
MapObj/WaveFloatingForce.cpp
MapObj/WhirlPool.cpp
MapObj/WoodBox.cpp
MapObj/WormEatenPlanet.cpp
MapObj/YellowChip.cpp
NPC/Butler.cpp
NPC/ButlerExplain.cpp
NPC/ButlerMap.cpp
NPC/ButlerStateStarPieceReaction.cpp
NPC/CareTaker.cpp
NPC/CollectTico.cpp
NPC/CometEventExecutorTimeLimit.cpp
NPC/CometEventKeeper.cpp
NPC/DemoRabbit.cpp
NPC/EventDirector.cpp
NPC/HoneyBee.cpp
NPC/HoneyQueen.cpp
NPC/Kinopio.cpp
NPC/KinopioAstro.cpp
NPC/KoopaJr.cpp
NPC/LuigiNPC.cpp
NPC/MiiDatabase.cpp
NPC/MiiFaceIcon.cpp
NPC/MiiFaceIconHolder.cpp
NPC/MiiFaceParts.cpp
NPC/MiiFaceRecipe.cpp
NPC/NPCActor.cpp
NPC/NPCParameter.cpp
NPC/NPCSupportRail.cpp
NPC/Peach.cpp
NPC/Penguin.cpp
NPC/PenguinCoach.cpp
NPC/PenguinMaster.cpp
NPC/PenguinRacer.cpp
NPC/PenguinRacerLeader.cpp
NPC/PenguinSkater.cpp
NPC/PenguinStudent.cpp
NPC/PowerStarEventKeeper.cpp
NPC/Rabbit.cpp
NPC/RabbitStateCaught.cpp
NPC/RabbitStateWaitStart.cpp
NPC/Rosetta.cpp
NPC/RosettaDemoAstroDome.cpp
NPC/RosettaDemoEpilogue.cpp
NPC/RosettaDemoHeavensDoor.cpp
NPC/RosettaReading.cpp
NPC/RunawayRabbit.cpp
NPC/RunawayRabbitCollect.cpp
NPC/RunawayTico.cpp
NPC/SignBoard.cpp
NPC/StageStateKeeper.cpp
NPC/Syati.cpp
NPC/TalkBalloon.cpp
NPC/TalkDirector.cpp
NPC/TalkMessageCtrl.cpp
NPC/TalkMessageInfo.cpp
NPC/TalkNodeCtrl.cpp
NPC/TalkState.cpp
NPC/TalkSupportPlayerWatcher.cpp
NPC/TalkTextFormer.cpp
NPC/TeresaRacer.cpp
NPC/Tico.cpp
NPC/TicoAstro.cpp
NPC/TicoComet.cpp
NPC/TicoDemoGetPower.cpp
NPC/TicoDomeLecture.cpp
NPC/TicoFat.cpp
NPC/TicoGalaxy.cpp
NPC/TicoRail.cpp
NPC/TicoReading.cpp
NPC/TicoShop.cpp
NPC/TicoStarRing.cpp
NPC/TimeAttackEventKeeper.cpp
NPC/TrickRabbit.cpp
NPC/TrickRabbitFreeRun.cpp
NPC/TrickRabbitSnowCollect.cpp
NWC24/LuigiMailDirector.cpp
NWC24/NWC24Function.cpp
NWC24/NWC24Messenger.cpp
NWC24/NWC24SendThread.cpp
NWC24/NWC24System.cpp
NWC24/ReceiverTagMail.cpp
NWC24/UTF16Util.cpp
NameObj/ModelChangableObjFactory.cpp
NameObj/MovementOnOffGroupHolder.cpp
NameObj/NameObjAdaptor.cpp
NameObj/NameObjCategoryList.cpp
NameObj/NameObjExecuteHolder.cpp
NameObj/NameObjFinder.cpp
NameObj/NameObjGroup.cpp
NameObj/NameObjHolder.cpp
NameObj/NameObjListExecutor.cpp
NameObj/NameObjRegister.cpp
Player/FireMarioBall.cpp
Player/GhostPacket.cpp
Player/GhostPlayer.cpp
Player/GroupChecker.cpp
Player/Mario.cpp
Player/MarioAccess.cpp
Player/MarioActor.cpp
Player/MarioActorBlackHole.cpp
Player/MarioActorClap.cpp
Player/MarioActorDraw.cpp
Player/MarioActorEye.cpp
Player/MarioActorInit.cpp
Player/MarioActorSensor.cpp
Player/MarioAnimator.cpp
Player/MarioBump.cpp
Player/MarioCollision.cpp
Player/MarioConst.cpp
Player/MarioDamageStun.cpp
Player/MarioEffect.cpp
Player/MarioEnforce.cpp
Player/MarioHolder.cpp
Player/MarioInit.cpp
Player/MarioJump.cpp
Player/MarioMagic.cpp
Player/MarioMapCode.cpp
Player/MarioMessenger.cpp
Player/MarioModule.cpp
Player/MarioMove.cpp
Player/MarioMove2D.cpp
Player/MarioNullBck.cpp
Player/MarioRabbit.cpp
Player/MarioSlider.cpp
Player/MarioSlip.cpp
Player/MarioSound.cpp
Player/MarioState.cpp
Player/MarioStep.cpp
Player/MarioStick.cpp
Player/MarioSukekiyo.cpp
Player/MarioSwim.cpp
Player/MarioSwimDamage.cpp
Player/MarioTask.cpp
Player/ModelHolder.cpp
RhythmLib/AudBgmTempoAdjuster.cpp
RhythmLib/AudChordInfo.cpp
Ride/BigBubble.cpp
Ride/Creeper.cpp
Ride/Fluff.cpp
Ride/FluffWind.cpp
Ride/JumpBranch.cpp
Ride/Plant.cpp
Ride/PlantLeaf.cpp
Ride/PlantStalk.cpp
Ride/Pole.cpp
Ride/SledRopePoint.cpp
Ride/SlingShooter.cpp
Ride/SpaceCocoon.cpp
Ride/SphereAccelSensorController.cpp
Ride/SphereController.cpp
Ride/SpherePadController.cpp
Ride/SurfRay.cpp
Ride/SurfRayTutorial.cpp
Ride/SwingRope.cpp
Ride/SwingRopePoint.cpp
Ride/Tamakoro.cpp
Ride/TamakoroTutorial.cpp
Ride/Trapeze.cpp
Scene/GameScene.cpp
Scene/GameSceneFunction.cpp
Scene/GameScenePauseControl.cpp
Scene/GameSceneScenarioOpeningCameraState.cpp
Scene/IntermissionScene.cpp
Scene/LogoScene.cpp
Scene/MultiSceneActor.cpp
Scene/PlacementInfoOrdered.cpp
Scene/PlacementStateChecker.cpp
Scene/PlayTimerScene.cpp
Scene/ScenarioSelectScene.cpp
Scene/SceneDataInitializer.cpp
Scene/SceneExecutor.cpp
Scene/SceneFactory.cpp
Scene/SceneNameObjListExecutor.cpp
Scene/SceneNameObjMovementController.cpp
Scene/ScenePlayingResult.cpp
Scene/StageDataHolder.cpp
Scene/StageFileLoader.cpp
Scene/StageResourceLoader.cpp
Scene/StopSceneController.cpp
Screen/BatteryInfo.cpp
Screen/BombTimerLayout.cpp
Screen/CameraInfo.cpp
Screen/CenterScreenBlur.cpp
Screen/CinemaFrame.cpp
Screen/CoinCounter.cpp
Screen/CometRetryButton.cpp
Screen/CopyFilterNegater.cpp
Screen/CountUpPaneRumbler.cpp
Screen/CounterLayoutAppearer.cpp
Screen/CounterLayoutController.cpp
Screen/DepthOfFieldBlur.cpp
Screen/ErrorMessageWindow.cpp
Screen/FullScreenBlur.cpp
Screen/FullnessMeter.cpp
Screen/GalaxyCometScreenFilter.cpp
Screen/GalaxyConfirmLayout.cpp
Screen/GalaxyMapBackground.cpp
Screen/GalaxyMapCometIcon.cpp
Screen/GalaxyMapDomeIcon.cpp
Screen/GalaxyMapGalaxyDetail.cpp
Screen/GalaxyMapIcon.cpp
Screen/GalaxyMapMarioIcon.cpp
Screen/GalaxyMapSelectButton.cpp
Screen/GalaxyMapTicoIcon.cpp
Screen/GalaxyMapTitle.cpp
Screen/GalaxyNamePlate.cpp
Screen/GalaxySelectBackButton.cpp
Screen/GalaxySelectInfo.cpp
Screen/GamePauseSequence.cpp
Screen/GameSceneLayoutHolder.cpp
Screen/GameStageClearSequence.cpp
Screen/HeatHazeEffect.cpp
Screen/IconComet.cpp
Screen/ImageEffectBase.cpp
Screen/ImageEffectDirector.cpp
Screen/ImageEffectResource.cpp
Screen/ImageEffectState.cpp
Screen/ImageEffectSystemHolder.cpp
Screen/InformationObserver.cpp
Screen/IsbnManager.cpp
Screen/LayoutActorFlag.cpp
Screen/LayoutCoreUtil.cpp
Screen/LensFlare.cpp
Screen/LogoFader.cpp
Screen/LuigiLetter.cpp
Screen/MarioMeter.cpp
Screen/MarioSubMeter.cpp
Screen/MessageTagSkipTagProcessor.cpp
Screen/MiiSelectIcon.cpp
Screen/MissLayout.cpp
Screen/MoviePlayerSimple.cpp
Screen/MoviePlayingSequence.cpp
Screen/MovieStarter.cpp
Screen/MovieSubtitles.cpp
Screen/MovieSubtitlesDataTable.cpp
Screen/NoteCounter.cpp
Screen/OdhConverter.cpp
Screen/OneUpBoard.cpp
Screen/PauseMenu.cpp
Screen/PeachLetter.cpp
Screen/PlayerActionGuidance.cpp
Screen/PlayerLeft.cpp
Screen/PlayerMissLeft.cpp
Screen/PowerStarList.cpp
Screen/PurpleCoinCounter.cpp
Screen/ScenarioSelectLayout.cpp
Screen/ScenarioTitle.cpp
Screen/SceneWipeHolder.cpp
Screen/ScreenPreserver.cpp
Screen/StaffRoll.cpp
Screen/StageResultInformer.cpp
Screen/StarCounter.cpp
Screen/StarPieceCounter.cpp
Screen/StarPointerBlur.cpp
Screen/StarPointerCommandStream.cpp
Screen/StarPointerController.cpp
Screen/StarPointerDirector.cpp
Screen/StarPointerGuidance.cpp
Screen/StarPointerLayout.cpp
Screen/StarPointerTarget.cpp
Screen/SubMeterLayout.cpp
Screen/SuddenDeathMeter.cpp
Screen/SurfingGuidance.cpp
Screen/THPDraw.c
Screen/THPSimplePlayerWrapper.cpp
Screen/TimeLimitLayout.cpp
Screen/WaterCameraFilter.cpp
Screen/WipeFade.cpp
Screen/WipeGameOver.cpp
Screen/WipeHolderBase.cpp
Screen/WipeKoopa.cpp
Screen/YesNoLayout.cpp
Speaker/SpkData.cpp
Speaker/SpkMixingBuffer.cpp
Speaker/SpkSound.cpp
Speaker/SpkSpeakerCtrl.cpp
Speaker/SpkSystem.cpp
Speaker/SpkTable.cpp
Speaker/SpkWave.cpp
System/AlreadyDoneFlagInGalaxy.cpp
System/ArchiveHolder.cpp
System/AudSystemWrapper.cpp
System/BinaryDataChunkHolder.cpp
System/BinaryDataContentAccessor.cpp
System/ConfigDataMii.cpp
System/ConfigDataMisc.cpp
System/DrawBuffer.cpp
System/DrawBufferExecuter.cpp
System/DrawBufferGroup.cpp
System/DrawBufferHolder.cpp
System/DrawSyncManager.cpp
System/FileHolder.cpp
System/FileLoader.cpp
System/FileLoaderThread.cpp
System/FileRipper.cpp
System/FindingLuigiEventScheduler.cpp
System/FunctionAsyncExecutor.cpp
System/GalaxyCometScheduler.cpp
System/GalaxyCometState.cpp
System/GalaxyMoveArgument.cpp
System/GalaxyNameSortTable.cpp
System/GalaxyStatusAccessor.cpp
System/GameDataConst.cpp
System/GameDataGalaxyStorage.cpp
System/GameDataPlayerStatus.cpp
System/GameDataTemporaryInGalaxy.cpp
System/GameEventFlag.cpp
System/GameEventFlagChecker.cpp
System/GameEventFlagStorage.cpp
System/GameEventFlagTable.cpp
System/GameEventValueChecker.cpp
System/GameSequenceDirector.cpp
System/GameSequenceProgress.cpp
System/GameSystem.cpp
System/GameSystemDimmingWatcher.cpp
System/GameSystemErrorWatcher.cpp
System/GameSystemException.cpp
System/GameSystemFontHolder.cpp
System/GameSystemFrameControl.cpp
System/GameSystemFunction.cpp
System/GameSystemObjHolder.cpp
System/GameSystemResetAndPowerProcess.cpp
System/GameSystemSceneController.cpp
System/GameSystemStationedArchiveLoader.cpp
System/HeapMemoryWatcher.cpp
System/HomeButtonMenuWrapper.cpp
System/HomeButtonStateNotifier.cpp
System/Language.cpp
System/LayoutHolder.cpp
System/LuigiLeftSupplier.cpp
System/MainLoopFramework.cpp
System/MessageHolder.cpp
System/NANDErrorSequence.cpp
System/NANDManagerThread.cpp
System/OSThreadWrapper.cpp
System/PauseButtonCheckerInGame.cpp
System/PlacedHiddenStarScenarioTable.cpp
System/RenderMode.cpp
System/ResourceHolder.cpp
System/ResourceHolderManager.cpp
System/ResourceInfo.cpp
System/ScenarioDataParser.cpp
System/ScenarioProgressTestRun.cpp
System/SpinDriverPathStorage.cpp
System/StageResultSequenceChecker.cpp
System/StarPieceAlmsStorage.cpp
System/StarPointerOnOffController.cpp
System/StationedArchiveLoader.cpp
System/StationedFileInfo.cpp
System/WPad.cpp
System/WPadButton.cpp
System/WPadHolder.cpp
System/WPadInfoChecker.cpp
System/WPadLeaveWatcher.cpp
System/WPadPointer.cpp
System/WPadRumble.cpp
System/WPadRumbleData.cpp
System/WPadStick.cpp
Util/ActorMovementUtil.cpp
Util/ActorShadowLocalUtil.cpp
Util/ActorShadowUtil.cpp
Util/ActorStateUtil.cpp
Util/ActorSwitchUtil.cpp
Util/AreaObjUtil.cpp
Util/BaseMatrixFollowTargetHolder.cpp
Util/BezierSurface.cpp
Util/BitArray.cpp
Util/BothDirList.cpp
Util/CollisionPartsFilter.cpp
Util/Color.cpp
Util/DirectDrawUtil.cpp
Util/EffectUtil.cpp
Util/FixedPosition.cpp
Util/GravityUtil.cpp
Util/HashUtil.cpp
Util/IKJoint.cpp
Util/JointController.cpp
Util/JointUtil.cpp
Util/MapUtil.cpp
Util/MemoryUtil.cpp
Util/ModelUtil.cpp
Util/MtxUtil.cpp
Util/NPCUtil.cpp
Util/OctahedronBezierSurface.cpp
Util/ParabolicPath.cpp
Util/PostureHolder.cpp
Util/RailGraphUtil.cpp
Util/RailUtil.cpp
Util/RumbleCalculator.cpp
Util/SceneUtil.cpp
Util/SchedulerUtil.cpp
Util/SpringValue.cpp
Util/SwitchEventFunctorListener.cpp
Util/ValueControl.cpp
```

### Regeneration Commands

```sh
find src/Game -type f | sort | sed 's#^src/Game/##' > /tmp/pc_game_rel.txt
find ../src/Game -type f | sort | sed 's#^../src/Game/##' > /tmp/orig_game_rel.txt
comm -23 /tmp/pc_game_rel.txt /tmp/orig_game_rel.txt
comm -13 /tmp/pc_game_rel.txt /tmp/orig_game_rel.txt
comm -12 /tmp/pc_game_rel.txt /tmp/orig_game_rel.txt | while read f; do cmp -s "src/Game/$f" "../src/Game/$f" || echo "$f"; done
```
