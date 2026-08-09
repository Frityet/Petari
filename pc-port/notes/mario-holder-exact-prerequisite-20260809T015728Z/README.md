# Exact MarioHolder production prerequisite

UTC: 2026-08-09T01:57:28Z

## Outcome

The PC stage lifecycle now creates the retail `SceneObj_MarioHolder` before it
preflights or constructs the active StartInfo row. `MarioHolder.hpp` and
`MarioHolder.cpp` are byte-identical to the root decompilation, and the PC Game
target compiles that exact source.

The holder is intentionally empty. Both `Mario` and `MarioActor` remain absent
from the compiled NameObj creator table and report `retail_creator_not_linked`.
There is no player proxy, synthetic actor, PlayerSystem attachment, direct
StartInfo construction, or fallback behavior in this change.

## Retail lifecycle evidence

Retail `GameScene::init()` creates `SceneObj_MarioHolder` immediately after
`SceneObj_PlanetGravityManager` and before stage placement. Retail
`SceneObjHolder::newEachObj()` maps scene object `0x14` to `new MarioHolder()`.
The exact holder constructor sets `mActor` to null; only
`MarioActor::init2()` registers the real actor with
`MR::getMarioHolder()->setMarioActor(this)`.

The existing PC StartInfo path remains the owner of player construction:

1. `StageStartInfo` owns its transformed `JMapInfo` and exact row.
2. StageHost preflights creator support.
3. `NameObjLifecycleService` establishes the placed-zone scope, invokes the
   retail creator with actor name `マリオアクター`, and calls `init(iter)`.
4. StageHost dispatches `initAfterPlacement()` after collision registration.

This mirrors `StageDataHolder::initPlacementMario()`; no second player-spawn
path was added.

## Production changes

- Added exact `Game/Player/MarioHolder.hpp` and `.cpp` mirrors.
- Added only `SceneObj_MarioHolder` to `SceneObjHolderCompat::newEachObj()`.
- Added `SceneObj_MarioHolder` to StageHost's required scene objects directly
  after the gravity manager, before the placement/start lifecycle.
- Added the two files to the permanent Game source mirror test.
- Extended the SceneObjHolder suite to freeze explicit creation, exact name,
  null actor state, singleton ownership, StageHost ordering, and continued
  absence of both player creators.

## Remaining atomic spawn gate

Enabling the player creator is still an all-or-nothing later boundary. The
measured constructor-seeded closure contains 96 Player translation units: 67
have source and 29 still require decompilation. That closure retains 830
non-Player symbol dependencies (665 Game, 53 JSystem, 82 RVL SDK, 30 native
runtime equivalents). `Mario::Mario` eagerly creates state modules, while
`MarioActor::init2()` immediately needs the exact LiveActor/Nerve/scheduler,
Binder/contact, sensor, stationed-resource, J3D/model/animation/effect/parts,
camera, and draw paths. The creator must remain absent until those real
semantics are complete.

Grounded keyboard walking is a separate gate after idle construction. The
Aurora WPad path already receives keyboard sub-stick input. A concurrent root
decompilation lane is adding `GamePadUtil`'s `getPlayerStickX()`,
`getPlayerStickY()`, and scalar `calcWorldStickDirectionXZ()`; those functions
still need their own RMGK02 evidence and PC promotion, outside this holder
lane. `MarioWalk.cpp`/`MarioWait.cpp` remain among the missing Player sources,
and walking also requires exact Binder/Triangle contact and planet-gravity
behavior.

## Verification

All commands and checksums are recorded in `verification.log`. The focused
mirror and lifecycle suites, StartInfo camera suite, NameObj placement suite,
Game library, and final `smg-pc` application link pass.
