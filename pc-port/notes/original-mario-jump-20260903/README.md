# Original Mario jump: compilation foundation and activation boundary

This 2026-09-03 tranche prepares existing original source to compile on the
native architecture. It does **not** enable jumping or replace the active
movement loop. The earlier `mario-jump-audit-20260903` note predates the recovered
Mario update/gravity, original Binder, state base, and XanimeCore checkpoints;
the findings below were checked against current source.

## Actual jump and landing sequence

Keyboard Space/Enter already reaches original A trigger/held helpers. Original
`MarioActor::isRequestJump` preserves its input guards and selects that trigger.
The current native `MarioMove.cpp` branch never consumes it and explicitly
rejects the jumping state. Native `Mario::update` never calls `actionMain`.

The original coherent phase is:

1. Actor `updateBehavior` calculates gravity/basis before calling Mario update.
2. `Mario::update` imports previous Binder results, computes map/shadow/ground
   and ceiling/wall information, then samples input and calls `actionMain`.
3. Grounded `mainMove` calls `tryJump`; that routine constructs the velocity
   from the authored jump table and calls `procJump(true)` **before** setting
   the jumping flag. `initJumpParam` initializes the related timers/state.
4. Following `actionMain` phases call `procJump(false)`. It preserves original
   A-release gravity, ceiling response, air steering and terminal speed. It
   calls `doLanding` only after the original ground phase says grounded.
5. Original physical writeback and timers finish the phase. LiveActor Binder
   runs afterward. The existing PC actor's post-Binder ground/shadow overwrite
   must retire with this lifecycle because it discards the original ground
   state independently calculated by floor rays.

These bodies already exist in root. No new impulse, gravity constant, airborne
flag workaround, animation-name predicate, or landing rule was introduced.

## Mechanical changes

- The retained camera area (`_568`), ground-check owner (`_574`), and tornado
  centering sensor (`_A38`) now have their actual pointer types in both Mario
  headers. The sensor setter and consumer no longer cast through `u32`.
  Current writers/readers only use these fields as addresses. Their Wii sizes
  remain four bytes, while host pointers retain their full width. `_7D0`
  remains its original unrelated `u16` damage timer.
- Root/PC return and argument declarations now agree with the existing Wall,
  Hang and Spin definitions. PC also receives the already corrected root
  Binder/damage query return types and `isUseFooSpecialGravity` spelling.
- Three ILP32 unsigned-long literal calls use explicit `u32` arguments, and
  bool-returning normalization is compared with bool values. MarineSnow's
  existing conversion to an 8-bit color is explicit. The Jump include uses
  the actual `KarikariDirector.hpp` capitalization for case-sensitive hosts.
- Two stale field references were corrected root-first using retail loads and
  stores: `entryWallWalkMode` uses the existing `mBeeWallWalk` byte at `0x9F1`;
  `MarioWall::start` uses `_20_HIGH_WORD` for the old movement word at `0x24`.
  Neither introduces a new field or changes the branch/mask.
- The old Wall translation-unit macro for renaming a trigonometric inline is
  disabled on TARGET_PC, where the forced compatibility header has already
  included that math header. The native call uses the same original table
  function; the original compiler retains its existing macro arrangement.
- The shared compiler boundary supplies the original RVL degree/radian macros
  with its exact `3.1415926f` literal. Existing EffectUtil/ObjUtil declarations
  were mirrored into native headers. Declarations do not supply fake effects
  or resource loading.
- The duplicate PC-only `XanimeCore::getJointTransform` was removed from Mario
  after the separate SDK task recovered and installed its exact original
  implementation in root/native Core. No other owner is replaced here.

## Verification

Run from the repository root:

```sh
python3 pc-port/notes/original-mario-jump-20260903/verify-adaptations.py
```

All thirteen units pass both original GC3.0a3 compilation and isolated native
syntax compilation: Mario, MarioMove, MarioJump, MarioCollision, MarioSlope,
MarioActorGravity, MarioWall, MarioHang, MarioSwim, MarioSpin, MarioBee,
MarioEnforce, and MarineSnow. Native parsing uses the current configured
MarioMove compile database arguments, with only compile/output/source arguments
replaced. It neither links nor executes these new paths. Generated objects,
commands, full diagnostics and objdiff output stay under
`build/original-mario-jump-20260903/`.

`source-evidence.json` records source/object hashes and exact instruction
witnesses from the supplied RMGK01 DOL, SHA1
`25c5959534b3c21246c6c7e42021b916b41fb578`. Fuzzy comparisons include:

| Original method | Match percent |
| --- | ---: |
| startTornadoCentering | 100 |
| entryWallWalkMode | 100 |
| tryJump | 99.588 |
| initJumpParam | 99.639 |
| procJump | 96.401 |
| doLanding | 99.601 |
| doAirWalk | 87.740 |
| updateCubeCode | 77.209 |
| updateBinderInfo | 65.609 |

The final three are historical reconstruction frontiers. Their low comparison
scores must not be represented as complete semantic parity. This tranche did
not alter their algorithms to chase a score. In particular, `updateBinderInfo`
needs a bounded retail control/field review before original landing activation.
The original empty `checkWallRising` has a four-byte retail symbol; it is not
evidence of a missing implementation.

## First link group and actual owners still needed

The first additional source group to link against the existing player slice is
`MarioJump`, `MarioCollision`, `MarioSlope`, `MarioActorGravity`, `MarioEnforce`,
`MarioWall`, `MarioHang`, `MarioSwim`, `MarioSpin`, `MarioBee`, and `MarineSnow`.
Retain original base state providers. When enabling full units, retire duplicate
extracts such as `Mario::isRising`, the collision/camera accessors, and the
gravity accessors; do not introduce multiple implementations. The recovered
root ActorGravity body still needs its full PC mirror at that activation point.
This list is an initial link group, **not** a claimed complete dependency list.
No xmake sources were enabled by this tranche.

Before production activation, these real owners are mandatory:

- Actual lower/upper XanimePlayers, authored tables, typed resources and the
  model whose calculated matrices reach rendering. Ordinary jump, air and
  landing branches query/change their state. Native MarioAnimator still has
  null original player/resource pointers; the separate renderer's one-track
  Core is not that owner. See `actual-mario-animation-owner-20260903/audit.md`.
- Real movement state objects. Grounded stride reset accesses Wall; every
  update timer accesses Hang; ordinary launch calls Swim's water search and
  falling reads its `_1B2`. Swim's constructor also creates real MarineSnow,
  which loads a texture through `loadTexFromArc`/JUTTexture. Full original
  Mario construction creates all its states; constructing pretend or partial
  state objects would not close their virtual behavior.
- Actual placed CollisionParts with current/previous/inverse matrices for
  relative ground tracking and Triangle matrix accessors. The current native
  static KCL registration explicitly has no CollisionParts owner, although
  the original Binder algorithm and geometry queries are already running.
- Original MarioEffect and remaining shared service providers reached by
  ordinary takeoff/landing. `MR::getKarikariClingNum` already has an original
  implementation returning zero only when its real scene object is absent;
  this is an example of a legitimate reusable provider, not a constant stub.

Once these owners and the verified floor group link, restore original Actor
gravity/control, Mario update/mainMove, and physical writeback together. The
live acceptance test must cover A-trigger rise, held-versus-released apex,
air steering, fall, original landing and a second jump, with actual animation
transitions and floor provenance. No such runtime result is claimed here.

## Parent integration gate

The complete native application and showcase built successfully with these
mechanical changes. The original Core, vertex buffer, Binder/KCL, Aurora native
and seven camera regression programs pass. These existing runtime gates do not
execute the not-yet-activated jump source. Xmake now forces its existing
`-Wno-register` option because its automatic option probe had silently dropped
it, rejecting original RailPart's `register` syntax under C++17.
