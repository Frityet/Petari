# Exact PC actor ABI tranche

Captured 2026-08-09 against the frozen Player compile frontier in
`../pc-player-compile-probe-20260809T035937Z/`.

## Result

- `pc-port/src/Game/NameObj/NameObj.hpp` is byte-identical to the root decomp
  header (`cmp` exits 0).
- `pc-port/src/Game/LiveActor/LiveActor.hpp` differs from the root decomp header
  only by an explicit virtual destructor declaration. The declaration reuses
  the inherited destructor slot, adds no object storage, and is required to
  release native external state while the dynamic type is still `LiveActor`.
- On the supported m64 PC ABI, `sizeof(NameObj) == 24` and
  `sizeof(LiveActor) == 208`. Focused tests also verify the pointer-width-adjusted
  field sequence (`mPosition` 20, `mModelManager` 80, `mFlag` 144, and
  `mCameraCtrl` 200).
- Host name ownership and all host model, animation, sensor, binder, clipping,
  and shadow bookkeeping now live in a generalized address-keyed compatibility
  registry. The retail fields remain their exact pointer/flag surface. Exact
  helper objects such as `Spine`, `RailRider`, `StageSwitchCtrl`, and
  `ActorLightCtrl` are owned by that registry and exposed through their retail
  raw-pointer slots only while alive.
- `LiveActor` destruction removes model/effect/star-pointer/scheduler-adjacent
  registrations, sensor bindings and owned sensors, collision/material/talk/demo
  compatibility state, and finally the generalized actor record. `NameObj`
  destruction removes the stable external name record.

No Player-specific compatibility branch or Mario/MarioActor creator was added.
Protected SaveIcon/TriggerChecker work and unrelated RFL work were not touched
by this tranche.

## Frozen compile-frontier proof

Run:

```sh
python3 pc-port/notes/exact-actor-abi-pc-20260809T042842Z/verify_frozen_frontier.py
```

The frozen closure contains 96 translation units represented by 93 source
compile attempts. The baseline passed 52; the live exact actor headers pass 56.
The four new passes are exactly:

- `src/Game/Player/MarioCollision.cpp`
- `src/Game/Player/MarioEnforce.cpp`
- `src/Game/Player/MarioSpecial.cpp`
- `src/Game/Player/MarioWalk.cpp`

There are zero regressions among the 52 frozen baseline passes. Machine-readable
results are in `verification.json`; per-source results and first errors are in
`live-frontier.tsv`. The verbose compiler output remains in the local ignored
`compile-logs/` work directory rather than being duplicated in the repository.

## Production and lifecycle gates

The release `smg-pc-game` archive builds successfully. A debug build and run of
the focused `smg-pc-actor-runtime-registry-tests` target passes 3/3 checks:

- exact native Game layout;
- external NameObj name lifetime;
- external LiveActor state lifetime and stale-identity cleanup.

The five existing targets mechanically migrated off the removed host-only member
surface also build and pass:

- `smg-pc-aurora-native-tests`: 27/27
- `smg-pc-demo-scene-runtime-tests`: 19/19
- `smg-pc-live-actor-util-real-or-absent-tests`: 5/5
- `smg-pc-game-actor-physics-real-or-absent-tests`: 7/7
- `smg-pc-actor-sensor-real-or-absent-tests`: 6/6

## Explicitly unresolved providers

Audio was deferred for this tranche. `LiveActor::initSound` and
`LiveActor::addToSoundObjHolder` therefore remain explicit no-ops, and the exact
`mSoundObject` slot remains null; this work does not claim a sound-object
provider. Likewise, host model/sensor/binder/shadow state is not disguised as a
retail `ModelManager`, `HitSensorKeeper`, `Binder`, or `ShadowControllerList`.
Those exact provider slots remain null while the generalized PC registry retains
the production host implementation.
