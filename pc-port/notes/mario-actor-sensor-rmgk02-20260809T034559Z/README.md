# MarioActor sensor reconstruction

Snapshot: `2026-08-09T03:45:59Z`

This tranche restores the complete retail collision-sensor and scouting behavior in
`main/Game/Player/MarioActorSensor` while preserving the already exact auto-rush
message path.

## Recovered behavior

- per-frame body, eye, and scouter sensor placement/radius updates;
- all retail spin/swim/Foo attack-animation gates;
- normal and special trample jumps, combo rewards, animation, effect, and rumble;
- enemy/map/ride scouter acquisition, carry-over lifetime, range, and cone growth;
- retail MarioActor nerve singleton initialization;
- the assembly-proven `MarioActor::_9D4` type correction from `u32` to
  `HitSensor*` (size and offset unchanged).

No PC factory, fallback, route-specific compatibility, or runtime activation was
added. This is an exact root-source prerequisite for the later atomic Mario
runtime closure.

## RMGK02 objdiff

| Section/function | Retail bytes | Fuzzy match |
|---|---:|---:|
| `.text` | 4,312 | 96.38497% |
| `.data` | 584 | 91.449814% |
| `.ctors` | 4 | 100% |
| `.sdata` | 8 | 100% |
| `.sdata2` | 72 | 100% |
| `setupSensors` | 332 | 100% |
| `updateHitSensor` | 1,120 | 97.02857% |
| `doTrampleJump` | 548 | 98.27007% |
| `trampleJump` | 704 | 88.57386% |
| static initializer | 108 | 100% |
| `attackSensor` | 380 | 100% |
| `sendMsgToSensor` | 80 | 100% |
| `resetSensorCount` | 20 | 100% |
| `recordScoutingObject` | 232 | 92.67242% |
| `updateScouter` | 652 | 97.59509% |
| `initScouter` | 120 | 100% |
| `initForJump` | 16 | 100% |

The remaining fuzzy differences are ordinary register scheduling, temporary
layout, and pooled-relocation identity. Every target function is present and the
full retail control flow is represented in C++.

## Verification

See `verification.log` for the exact commands and hashes. The focused object
SHA-256 is:

```text
67c8e312ac7580f5389c629d653e416e37b2cc54d63f9df85733eaa02069e935
```
