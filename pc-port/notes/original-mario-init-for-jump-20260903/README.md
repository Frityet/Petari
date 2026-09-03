# Existing initForJump and complete sensor-source frontier

`MarioActor::initForJump` is already complete in root and its identical PC mirror, both `Game/Player/MarioActorSensor.cpp:457`. It clears the two `u8` fields `_988` and `_989`; it neither reads input nor computes a jump velocity. No source recovery or production edit was needed in this tranche.

The original GC3.0a3 compiler, real root headers and configured Game flags reproduce all four retail instructions at `0x802BFB74`, size `0x10`, without relocation or normalization: `38000000 98030988 98030989 4e800020`. An original-compiler layout probe confirms the two byte offsets `0x988` and `0x989`. The verified RMGK01 DOL SHA1 is `25c5959534b3c21246c6c7e42021b916b41fb578`. The function has no external references or allocation/lifecycle dependencies beyond a genuine constructed MarioActor.

## Original caller and meaning

Root `MarioActor.cpp:939–986`, `updateBehavior`, calculates gravity and applies pressing/stun/rush/throw guards. It calls `initForJump` when `getMovementStates()._1` is grounded, before swing/throw/DPD updates and `Mario::update`. It is a grounded-phase counter reset, not the launch routine.

`MarioActorSensor.cpp` uses `_988` to cycle trample animations and apply the existing second-trample multipliers. Its `doTrampleJump` uses `_989` for authored enemy combo sounds, CollectCounter display and original one-up thresholds. Actual trample launch is the separate `trampleJump` → `Mario::tryForceFreeJump` path. Ordinary button launch remains original `mainMove` → `tryJump` → `procJump`; adding velocity behavior to `initForJump` would change the game.

## Staged complete translation unit

The full original Sensor source is staged under `build/original-mario-init-for-jump-20260903/`, with no native source activation. Actual `smg-pc-game` compile-database flags initially fail because native `CollectCounter.hpp` is absent. With that exact root header, the next errors are:

- Native MarioActor suppresses its original `updateHitSensor` and `attackSensor` override declarations in the PC branch. The staged probe uses the complete unchanged root MarioActor header, including the real nerve declarations, rather than masking these methods.
- Three comparisons use `_468 != nullptr` although `_468` is the existing `u32` carried-sensor count. Staged-only comparisons use `!= 0`. `MarioActorTakeMsg.cpp:11–16` indexes the sensor array by this field and increments it; it is not a host pointer requiring widening.
- `startPadVib(0ul)` is ambiguous on LP64 between the original integer and pointer overloads. Staged-only `static_cast<u32>(0)` selects the original integer call.

Those adaptations allow the complete Sensor source to compile. Original-compiler comparison confirms all eleven MarioActor methods retain identical instruction bytes and relocations; only generated constant symbol labels are normalized by their unchanged data. This is an equivalence check against existing source, not a fresh claim that every inherited Sensor reconstruction matches retail.

An isolated link retains the entire original TU, including its genuine nerve code, against current showcase libraries with dead stripping disabled. `initForJump` resolves. **26 direct unresolved symbols remain**, including seven real game-over/time-wait handlers emitted by the root header; there are 223 unresolved symbols including the older walking archive's transitive whole-object gaps. The exact symbols and calling methods are recorded in `evidence.json`. No fake actor was constructed and the diagnostic link result was never executed.

The direct frontier includes animation/audio/effects, sensor attack/rush/tornado methods, `CollectCounter::setCount`, one-up/life service calls, `addHitSensorCallback`, horizontal-angle math, `tryForceFreeJump`, spin rotation and trample-combo sound. Most have existing root source. Two relevant root bodies are still missing: `attackOrPushSensor` (`MarioActorOffensiveMsg.cpp`, retail `0x802C55E0/0x6E0`) and `tryTornadoPull` (`MarioActorTakeMsg.cpp`, retail `0x802CA408/0x2F8`). They were not replaced or recovered as part of this bounded audit.

## Current native activation boundary

- `pc-port/src/Game/xmake.lua:44` excludes the Sensor TU; `src/showcase/xmake.lua` does not add it to the walking slice. The existing native body is consequently not linked.
- The active PC `MarioActor::control` (`MarioActor.cpp:1010`) supplies its current walking gravity/basis updates and calls Mario update directly. It bypasses the original `control2`/`controlMain`/`updateBehavior` phase and therefore the original `initForJump` call.
- The active PC `Mario::update` (`Mario.cpp:2018`) remains the grounded walking replacement. It does not call original `actionMain`. `MarioMove.cpp:18` uses the native walking branch, never consumes original `isRequestJump`, and explicitly rejects the jumping state.
- The current PC Mario constructor still leaves actual Wall/Hang/Swim and other state owners null. Actor init still constructs the incomplete native MarioAnimator. Their full source/owner closure, original ground/collision phase and effect/audio dependencies remain necessary before restoring original update/mainMove. See `../original-mario-jump-20260903/README.md` and `../original-mario-animator-native-20260903/README.md`; subsequent root special-mode/chest recovery is in `../original-mario-special-animation-20260903/README.md`.

The counter method can be imported verbatim as a source-backed provider if a complete owner group needs the symbol before whole Sensor activation. That would close one link edge only. It would not enable jumping, restore sensor behavior, or justify calling original methods on incomplete owners. No such provider was added here.

## Reproduce

Run `python3 pc-port/notes/original-mario-init-for-jump-20260903/verify.py` from the root. This compiles root and staged source, checks original layout/bytes and adaptation equivalence, and captures the expected native link frontier. It requires the existing configured compile database and compiled showcase libraries; no xmake command is invoked. Commands, objects and full diagnostic logs remain in the ignored build directory. Only this note, verifier and `evidence.json` are intended for the checkpoint.
