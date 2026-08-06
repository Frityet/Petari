# Source-close CollisionBlocker PC port

Updated: 2026-08-06T19:30:33Z

## Outcome

The PC placement factory now constructs the original `CollisionBlocker` actor. All eight HeavensDoor scenario 1 placements moved from `blocked` to `created`, reducing the current blocker frontier from 24 to 16 without an object-position, zone, stage, or route exception.

`pc-port/src/Game/MapObj/CollisionBlocker.{hpp,cpp}` are byte-identical to `include/Game/MapObj/CollisionBlocker.hpp` and `src/Game/MapObj/CollisionBlocker.cpp`.

## General compatibility boundary

The original actor required three utility providers that the earlier PC subset did not expose:

- the original `ATYPE_EYE` sensor type and `addHitSensorEye` constructor boundary;
- generic `sendArbitraryMsg` delivery through `HitSensor::receiveMessage`; and
- the `setClippingFar50m` policy boundary.

Their host implementations live in `src/compat/GameRuntimeCompat.cpp`; no behavior was added to the actor. The current host scheduler does not cull registered actors, so the far-clip setter intentionally records no actor-specific state yet. This is a general clipping-policy gap, not a CollisionBlocker or HeavensDoor special case.

`Game/Util.hpp` is a narrow PC compatibility umbrella for original translation units that use the root aggregate include. `MR::Functor_Inline` now forwards to the existing generic host functor adapter, preserving the original call site.

## Verification

```text
xmake build smg-pc
[100%]: build ok

xmake test
[ok] CollisionBlocker sensor lifecycle
11 Aurora-native test(s) passed
100% tests passed
```

The native test verifies the original eye sensor type, group size, and radius, plus the actor's appear/force-break lifecycle.

Real-disc artifacts: `/tmp/smgpc-collision-blocker.8xM4vp/`

```text
input_sent=1
app_result=0
wrapper_result=0
FileSelect scheduler cleanup: 78 -> 2 (76 removed)
HeavensDoor placement: objects=242;created=154;ignored=72;blocked=16
CollisionBlocker: 8, status=created, support_reason=original_factory
fatal/segmentation/crash/abort matches: 0
```

The run exercises the existing FileSelect-to-picturebook-handoff debug boundary rather than a direct stage boot. It does not yet prove the full title, interactive file-selection, and rendered picturebook sequence; those remain required for the final demo route.

## Remaining limitation

The actor now participates in the general host hit-sensor pass and sends its original push-force message to player sensors. The full gameplay effect still depends on the original player actor/message handling, which is not yet ported; the placement is no longer represented by an inert model alias.
