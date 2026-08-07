# NameObj factory: retail source or explicit absence

## Outcome

`pc-port/src/Game/NameObj/NameObjFactory.cpp` and `.hpp` are byte-identical to
the root decompilation again. The 8,355-line retail translation unit is kept as
source evidence but excluded from the PC target because it directly references
the complete retail actor closure.

The host-supported subset and the implementations of the retail
`NameObjFactory` API now live in
`pc-port/src/scene/nameobj/NameObjFactory.cpp`. There is no generic model actor,
same-name archive inference, or route/stage-specific creator.

## Exactness evidence

```text
aeb4a134477cd797213213e7aa23820f94875531d65cdc2cdfee872fb2200249  src/Game/NameObj/NameObjFactory.cpp
aeb4a134477cd797213213e7aa23820f94875531d65cdc2cdfee872fb2200249  pc-port/src/Game/NameObj/NameObjFactory.cpp
9615f963b6f7bd1a2d15a9e5d15d7b7e1da26e9a518ded0732e0bb57c618617c  include/Game/NameObj/NameObjFactory.hpp
9615f963b6f7bd1a2d15a9e5d15d7b7e1da26e9a518ded0732e0bb57c618617c  pc-port/src/Game/NameObj/NameObjFactory.hpp
```

## Advertised creator audit

The compiled table contains only these retail entries:

- `PrologueDirector`: the normal initialization path is source-present and is
  retained for the picture-book sequence.
- `FileSelector`: the source-present file-select/title product construction is
  retained. Its archives are taken from the retail tables.
- `GroupSwitchWatcher`: its stage-switch controller and scene-owned switch
  container are present.
- `SwitchSynchronizerReverse`: its stage-switch controller and scene-owned
  switch container are present.
- `CollisionBlocker`: its sensor, clipping, and stage-switch initialization is
  present and has a focused native lifecycle test.

These formerly advertised entries are now explicitly unavailable:

| Placement name | Mandatory unavailable initialization dependency |
| --- | --- |
| `Steam` | `SimpleEffectObj::init` always calls group clipping; the real `ClippingGroupHolder` is absent. |
| `Coin`, `PurpleCoin` | `Coin::init` always initializes a shadow; real shadow projection/controller ownership is absent. |
| `RailCoin`, `PurpleRailCoin` | Child Coin initialization requires shadows, and rail placement can require the absent AreaObj/Mercator runtime. |
| `PurpleCoinStarter` | Creating `PurpleCoinHolder` immediately requires EventPowerStar declaration and scene-layout counter ownership. |
| `DemoRabbit` | The required NPC joint-controller and behavior closure is absent. |
| `StarPieceFlow`, `StarPieceGroup` | The required real `StarPieceDirector` closure is absent. |

Each of these names has no creator and produces no archive preload list. The
diagnostic support query reports the concrete reason, and construction throws
an explicit unsupported-factory error.

## Archive behavior

- Archive names come only from the supported retail creator entry and the
  generated retail `Name2Archive` mapping.
- A same-name `/ObjectData/<name>.arc` is never inferred.
- An unsupported creator never advertises retail archive metadata.
- Preloading a mapped archive that is missing rejects explicitly instead of
  silently proceeding.
- Retail case-insensitive creator lookup and the player-archive-loader name
  table are preserved.

## Placement classification

Only tables with a proven separate consumer are ignored by the actor factory:

- `StageObjInfo` is consumed by recursive zone composition in
  `StagePlacementResolver`.
- `DemoObjInfo` is consumed by `DemoSceneRuntime` as DemoGroup/DemoSubGroup
  definitions.

`AreaObjInfo`, `CameraCubeInfo`, and `PlanetObjInfo` are actor-bearing and are
not blanket-hidden. Unsupported rows from those tables remain blocked.

## Verification

- `cmp` and SHA-256 checks above prove the two `Game/NameObjFactory` files are
  exact.
- The focused target compiled both the updated
  `NameObjFactoryPlacementTests.cpp` object and the new non-Game
  `scene/nameobj/NameObjFactory.cpp` object successfully.
- The focused target could not finish its aggregate static library/link during
  this task because concurrent work left `GameDataHolderCompat.cpp` failing on
  a `map<string>::find(string_view)` call. That translation unit is unrelated
  to this boundary. The same shared build had first failed in `UserFile.cpp`
  while that GameData migration was in an earlier intermediate state.
- `git diff --check` passes.
