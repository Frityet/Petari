# Group and shadow ownership providers

## Scope

This tranche supplies the generalized runtime ownership needed by exact NPC
initialization without enabling any actor or scene by name:

- `GroupCheckManager` owns the two retail attribute groups through native
  lifetime state while retaining the exact Game object layout.
- SearchTurtle and ReflectSpinningBox membership is keyed by an owned copy of
  `NameObj::mName`, matching the RMGK02 `HashSortTable` add/search assembly.
  Membership is idempotent by name, isolated by group, and lives until the
  scene-owned checker is destroyed; it is not tied to an individual actor
  identity.
- `SceneObjHolder` can own the real `SceneObj_GroupCheckManager`. The MR utility
  boundary requires that the scene created it during normal pre-placement and
  explicitly rejects a missing manager instead of fabricating one on lookup.
- `LiveActor::initShadowControllerList` creates a fixed-capacity external list
  instead of placing a native pointer in the 32-bit retail field.
- Each shadow controller retains its authored name, shape kind, radius, drop
  pointers/length, validity, collision calculation mode, and private-gravity
  mode. The retail single-controller name shortcut and multi-controller named
  lookup are preserved.

Projection, collision sampling, and shadow drawing are deliberately not
fabricated here. Shadow-aware clipping remains explicitly unavailable until
those providers exist.

## Source boundary

`pc-port/src/Game/Player/GroupChecker.hpp` mirrors the root decomp declaration.
The class method bodies and native ownership live in
`src/compat/GroupCheckManagerCompat.*`, keeping STL and host lifetime state out
of the Game layout. The existing PC-only body of
`LiveActor::initShadowControllerList` delegates capacity to the generalized
actor registry.

No changes were made to DemoRabbit, NameObjFactory, Gateway scene hosts, talk
compatibility, rabbit routing, SaveIcon, TriggerChecker, or RFL.

## Focused contracts

- `NPCActorRealOrAbsentTests` covers duplicate insertion, two actors in one
  group, distinct actors with the same name, cross-group isolation,
  checker-owned membership lifetime, explicit pre-placement ownership,
  scene-manager cleanup, and null rejection.
- `GameActorPhysicsRealOrAbsentTests` covers exact sphere metadata, null-name
  lookup for a single controller, named lookup for multiple controllers,
  calculation/gravity/validity mutations, drop setters, missing-controller
  rejection, and destruction cleanup.

## Verification

Run serially from `/workspaces/pcport/pc-port` with the existing PC package
configuration:

- `xmake -b -j2 smg-pc-npc-actor-real-or-absent-tests` — build passed;
  `NPCActor real-or-absent tests passed: 5/5`.
- `xmake -b -j2 smg-pc-game-actor-physics-real-or-absent-tests` — build
  passed; `Game actor physics real-or-absent tests passed: 8/8`.
- `xmake -b -j2 smg-pc-actor-runtime-registry-tests` — build passed; all five
  registry cases passed.
- `xmake -b -j2 smg-pc-sceneobj-holder-real-or-absent-tests` — build passed;
  all seven SceneObjHolder cases passed.

`git diff --check` is clean for the scoped source and test files. No package
configuration was changed.
