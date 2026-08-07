# Game actor physics: real or absent

## Source boundary

The following PC-port files are byte-identical to the decompiled source and
headers; host behavior is supplied outside `src/Game`:

- `src/Game/Util/ActorMovementUtil.cpp` and
  `include/Game/Util/ActorMovementUtil.hpp`;
- `src/Game/Util/ActorShadowUtil.cpp` and
  `include/Game/Util/ActorShadowUtil.hpp`;
- `src/Game/Util/MapUtil.cpp` and `include/Game/Util/MapUtil.hpp`.

Their PC copies are excluded from the host build where the compatibility unit
provides the unresolved boundary. There are no actor-, route-, stage-, or
object-name exceptions in that boundary.

## Real host behavior

- Binder state is a snapshot of contacts produced by the registered KCL
  collision service. Ground, wall, roof, normals, and the accumulated fix
  reaction are recorded in the centralized `ActorRuntimeRegistry` and released
  with the actor. A raw KCL attribute is retained only as an attribute-table
  index; it is not reinterpreted as a floor code.
- Movement and rebound helpers consume those real contacts and match the
  decompiled normal/tangent calculations. Invalid gravity or actor bases are
  rejected instead of being replaced by a world-axis default.
- Actor clipping has a real sphere/frustum evaluator. The scene scheduler
  consumes it for movement, animation, view calculation, sensor checks, and
  draw-buffer collection. An actor with no far-level override uses the active
  camera's far plane; there is no fabricated 100 m default. Invalid sphere
  radii are rejected rather than clamped.
- `resetPosition` clears real sensor and Binder contacts, refreshes enabled
  gravity, and performs direct animation calculation while preserving the
  actor's no-calc flag.

Binder and clipping data live only in the existing centralized actor runtime
registry; this work does not add a second physics map. Lifecycle cleanup removes
both states. No shadow configuration registry or substitute shadow state was
added.

## Explicitly unavailable behavior

These operations now throw until their real retail owner and consumer exist:

- shadow creation/configuration/validation and shadow-aware clipping, because
  projection, collision, and drawing are not implemented;
- MirrorActor creation, because parsed MirrorArea ownership and mirror drawing
  are absent;
- clip-area Binder filtering, because CollisionParts sensor ownership and the
  ClipArea holder are absent;
- group clipping, because ClippingGroupHolder is absent;
- DeathArea membership, moving roof/ground pressure, and collision floor-code
  interpretation where their source ownership tables are absent;
- scene result coin/Purple Coin writes, Dark Comet state, the 100-Coin Power
  Star declaration, and Purple Coin counter operations.

Consequently, `DemoRabbit` factory/archive selection remains available but its
initialization rejects missing shadows. `StarPieceGroup` factory/archive
selection remains available but its initialization rejects missing group
clipping. Broad tests assert those capability boundaries rather than accepting
partial actor initialization as success.

## Evidence

Run from the repository root:

```text
$ xmake b -P pc-port -y smg-pc-game-actor-physics-real-or-absent-tests
build ok
$ xmake r -P pc-port smg-pc-game-actor-physics-real-or-absent-tests
Game actor physics real-or-absent tests passed: 7/7

$ xmake r -P pc-port smg-pc-stage-collision-registration-tests
2 stage collision registration test(s) passed

$ xmake r -P pc-port smg-pc-actor-sensor-real-or-absent-tests
6 ActorSensor real-or-absent tests passed

$ xmake r -P pc-port smg-pc-aurora-native-tests
27 Aurora-native test(s) passed
```

The focused source is `tests/GameActorPhysicsRealOrAbsentTests.cpp`. It covers
strict scene-owned operations, reset behavior, unscaled actor axes, real Binder
movement/rebound, camera-consumed clipping, and the unavailable capability
boundaries above.
