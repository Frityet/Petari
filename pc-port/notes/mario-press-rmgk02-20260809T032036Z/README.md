# MarioPress RMGK02 recovery

Date: 2026-08-09 UTC

## Outcome

`src/Game/Player/MarioPress.cpp` now reconstructs the complete retail split:

- `Mario::checkPressDamage()`
- `Mario::checkVerticalPress(bool)`
- `Mario::checkSidePressPre()`
- `Mario::checkSidePress()`
- the generated `NrvMarioActor` static initializer

The implementation restores vertical and side squeeze detection, moving-map
velocity checks, press-sensor filtering, hang release, floor/wall press state
selection, corrective velocity and push behavior, steep wedge handling, and
the high-speed ground-press escape path.

No PC activation, fallback, compatibility, stage, factory, audio, SaveIcon,
TriggerChecker, or protected configuration file was changed. No shared header
change was required; the existing Mario, MarioActor, Triangle, HitSensor, and
FloorCode declarations already express all assembly-proven accesses.

## Authoritative evidence

- Retail assembly: `build/RMGK02/asm/Game/Player/MarioPress.s`
- Retail text range: `0x802EE148..0x802EF560` (`0x1418`, 5144 bytes)
- Focused source object: `build/RMGK02/src/Game/Player/MarioPress.o`
- Objdiff unit: `main/Game/Player/MarioPress`
- Full-build checksum manifest: `config/RMGK02/build.sha1`

The retail instructions establish every branch, call, field offset, sensor
comparison, movement-state bit, collision probe, constant, and return path. A
local m2c discovery pass was used to label broad control flow; retail assembly
and focused MWCC/objdiff output remained authoritative.

The local JSystem header currently declares `JMAAcosRadian` inline, whereas
this retail translation unit calls the external entry point. The source uses a
translation-unit-local preprocessor rename while reading the headers, then
declares and calls the external function. This preserves the retail ABI and
does not alter the shared SDK header.

## Objdiff fidelity

The focused source and retail objects both have a 5144-byte `.text` section.
Overall text match is 99.72006%; `.ctors` and `.sdata2` are 100%.

| Function | Retail bytes | Match |
| --- | ---: | ---: |
| `checkPressDamage` | 712 | 99.77528% |
| `checkVerticalPress` | 1072 | 99.83209% |
| `checkSidePressPre` | 560 | 99.89286% |
| `checkSidePress` | 2692 | 99.61367% |
| generated static initializer | 108 | 100% |

Remaining differences are local constant-symbol identity and a small number
of register/evaluation-order choices; no behavior is intentionally omitted.

## Integrity

- The focused RMGK02 object compiles with the configured `GC/3.0a3` compiler.
- Focused objdiff completes successfully.
- `git diff --check` is clean for the owned source.
- The full RMGK02 DOL checksum passes.
- Rebuilt and original DOL SHA-256 values are identical.
