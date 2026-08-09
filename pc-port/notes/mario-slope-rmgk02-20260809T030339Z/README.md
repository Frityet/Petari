# MarioSlope RMGK02 recovery

Date: 2026-08-09 UTC

## Outcome

`src/Game/Player/MarioSlope.cpp` now reconstructs all four retail functions in
the RMGK02 `MarioSlope.cpp` unit:

- `Mario::lockGroundCheck(void*, bool)`
- `Mario::unlockGroundCheck(void*)`
- `Mario::isUseSimpleGroundCheck() const`
- `Mario::checkGroundOnSlope()`

The implementation restores owner-scoped ground-check locking, the simple
ground-check selection rules, steep-floor probing, floor-normal and triangle
commit behavior, rising-player rejection, forced translation, and the
slip-floor velocity correction used while walking on a slope.

No PC activation, compatibility, stage, factory, audio, SaveIcon, or
TriggerChecker files were changed.

## Authoritative evidence

- Retail assembly: `build/RMGK02/asm/Game/Player/MarioSlope.s`
- Retail text range: `0x8030635C..0x80306B68` (`0x80C`, 2060 bytes)
- Focused source object: `build/RMGK02/src/Game/Player/MarioSlope.o`
- Objdiff unit: `main/Game/Player/MarioSlope`
- Full-build checksum manifest: `config/RMGK02/build.sha1`

Retail instructions established every branch, call, member offset, constant,
probe distance, animation string, translation tag, and return path. A local
m2c discovery pass was used only to label stack values and broad control flow;
the retail assembly and MWCC/objdiff output remained authoritative.

## ABI correction

`Mario::_574` is now declared as `void*` rather than `u32`. This is a
size-neutral target-ABI correction: retail `lockGroundCheck(void*, bool)` stores
its opaque owner argument directly at offset `0x574`, while
`unlockGroundCheck(void*)` compares the supplied pointer directly against that
same field before releasing the lock. The previous integer declaration made
the exact typed source illegal under MWCC.

## Objdiff fidelity

The final focused object has the same 2060-byte `.text` size as retail and is
99.3767% matched overall. Function results are:

| Function | Retail bytes | Match |
| --- | ---: | ---: |
| `lockGroundCheck` | 36 | 95.888885% |
| `unlockGroundCheck` | 44 | 100% |
| `isUseSimpleGroundCheck` | 396 | 99.71717% |
| `checkGroundOnSlope` | 1476 | 99.30624% |
| generated static initializer | 108 | 100% |

The constructor section matches at 100%. The retail `.sdata2` payload measures
100%; the source object omits only the retail split's unreferenced four-byte
trailing gap. Remaining text differences are register/stack allocation and
constant-symbol identity, not intentionally omitted behavior.

## Integrity

- The focused RMGK02 source object compiles successfully with the configured
  `GC/3.0a3` compiler.
- The focused objdiff completed successfully.
- `git diff --check` is clean for the owned paths.
- The full RMGK02 DOL checksum passes, and the rebuilt/original DOL SHA-256
  values are identical.
